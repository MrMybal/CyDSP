#include <algorithm>
#include <array>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <vector>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}
void ok(Status s) {
    require(s == Status::ok, "performance regression status");
}
// Slow sample-domain oracle: scan the complete look-ahead window, rather than
// sharing the production monotonic deque. Keep the original dB-to-gain equations.
template <class T> struct DynamicsOracle {
    DynamicsMode mode;
    DynamicsParameters p;
    std::array<double, 3> envelope{}, gate{};
    std::array<bool, 3> opened{};
    std::array<std::uint64_t, 3> hold{};
    std::array<std::vector<double>, 3> detector;
    std::array<std::vector<T>, 3> delay;
    std::size_t latency = 0, head = 0;
    double reduction = 0;
    explicit DynamicsOracle(DynamicsMode m) : mode(m) {
        clearDelay();
    }
    void clearDelay() {
        head = 0;
        for (auto& v : detector)
            v.assign(latency + 1, 0);
        for (auto& v : delay)
            v.assign(latency + 1, T{});
    }
    void set(const DynamicsParameters& next) {
        const auto frames = std::size_t(std::ceil(next.lookAheadMs * 48.));
        if (frames != latency) {
            latency = frames;
            clearDelay();
        }
        p = next;
    }
    void reset() {
        envelope = {};
        gate = {};
        opened = {};
        hold = {};
        reduction = 0;
        clearDelay();
    }
    std::array<T, 3> step(std::array<T, 3> x) {
        std::array<T, 3> out{};
        double linked = 0;
        for (T v : x)
            linked = std::max(linked, std::abs(double(v)));
        const double attack = p.attackMs <= 0 ? 0 : std::exp(-1000. / p.attackMs / 48000.);
        const double release = p.releaseMs <= 0 ? 0 : std::exp(-1000. / p.releaseMs / 48000.);
        const auto advance = [&](double value, double old) {
            return value + (value > old ? attack : release) * (old - value);
        };
        for (std::size_t ch = 0; ch < 3; ++ch) {
            const auto delayed = latency ? delay[ch][(head + 1) % (latency + 1)] : x[ch];
            delay[ch][head] = x[ch];
            const double magnitude = p.linkedChannels ? linked : std::abs(double(x[ch]));
            detector[ch][head] =
                p.detector == DetectorMode::rms ? magnitude * magnitude : magnitude;
            const double peak = *std::max_element(detector[ch].begin(), detector[ch].end());
            envelope[ch] = advance(peak, envelope[ch]);
            const double amplitude = p.detector == DetectorMode::rms
                                         ? std::sqrt(std::max(0., envelope[ch]))
                                         : envelope[ch];
            const double level = 20 * std::log10(std::max(1e-12, amplitude));
            const double over = level - p.thresholdDb;
            double db = 0;
            if (mode == DynamicsMode::compressor || mode == DynamicsMode::limiter) {
                const double slope = mode == DynamicsMode::limiter ? -1. : 1. / p.ratio - 1.;
                if (p.kneeDb > 0 && std::abs(over) < p.kneeDb * .5) {
                    const double t = over + p.kneeDb * .5;
                    db = slope * t * t / (2 * p.kneeDb);
                } else if (over >= p.kneeDb * .5)
                    db = slope * over;
            } else if (mode == DynamicsMode::noiseGate) {
                const auto frames = std::uint64_t(std::ceil(p.holdMs * 48000. / 1000.));
                if (level >= p.thresholdDb) {
                    opened[ch] = true;
                    hold[ch] = frames;
                } else if (opened[ch] && level < p.thresholdDb - p.hysteresisDb) {
                    if (hold[ch])
                        --hold[ch];
                    else
                        opened[ch] = false;
                } else if (opened[ch])
                    hold[ch] = frames;
                gate[ch] = advance(opened[ch] ? 1. : 0., gate[ch]);
                db = 20 * std::log10(std::max(1e-8, gate[ch]));
            } else if (over < 0)
                db = std::max(-160., (p.ratio - 1) * over);
            reduction = db;
            double value =
                double(delayed) * std::pow(10., db / 20) * std::pow(10., p.makeupDb / 20);
            if (mode == DynamicsMode::limiter) {
                const double ceiling = std::pow(10., p.thresholdDb / 20);
                value = std::clamp(value, -ceiling, ceiling);
            }
            out[ch] = T(value);
        }
        head = (head + 1) % (latency + 1);
        return out;
    }
};
template <class T> void dynamics() {
    double maximumError = 0, maximumMeterError = 0;
    for (auto mode : {DynamicsMode::compressor, DynamicsMode::expander, DynamicsMode::noiseGate,
                      DynamicsMode::limiter}) {
        Dynamics<T> processor;
        ok(processor.prepare({48000, 128, 3}, mode, 5));
        DynamicsOracle<T> reference(mode);
        AudioBuffer<T> input, output;
        ok(input.resize(3, 128));
        ok(output.resize(3, 128));
        std::size_t time = 0;
        for (unsigned phase = 0; phase < 10; ++phase) {
            DynamicsParameters p;
            p.thresholdDb = -24;
            p.ratio = 3.75;
            p.kneeDb = phase % 2 ? 0. : 7.;
            p.makeupDb = 3;
            p.attackMs = phase % 3 ? .15 : 0.;
            p.releaseMs = phase % 3 ? .7 : 0.;
            p.holdMs = .17;
            p.hysteresisDb = 4;
            p.detector = phase % 2 ? DetectorMode::peak : DetectorMode::rms;
            p.lookAheadMs = phase % 3 ? 3. : 0.;
            p.linkedChannels = phase % 4 >= 2;
            // Exercise the default prepared parameters before any setter too.
            if (phase) {
                ok(processor.setParameters(p));
                reference.set(p);
            }
            if (phase == 7) {
                processor.reset();
                reference.reset();
            }
            for (unsigned block = 0; block < 40; ++block) {
                const std::array<std::size_t, 6> splits{1, 7, 32, 3, 0, 128};
                const auto n = splits[block % splits.size()];
                auto in = input.block().subBlock(0, n), out = output.block().subBlock(0, n);
                std::array<std::array<T, 128>, 3> expected{};
                for (std::size_t i = 0; i < n; ++i, ++time) {
                    std::array<T, 3> x{};
                    for (std::size_t ch = 0; ch < 3; ++ch) {
                        const auto section = (time + ch * 31) % 401;
                        const double v =
                            section < 120   ? .63 * std::sin(double(time) * .137 + double(ch))
                            : section < 210 ? std::pow(10., (-24. - double(section % 9)) / 20.)
                                            : 0.;
                        x[ch] = T(v);
                        in.channel(ch)[i] = x[ch];
                    }
                    const auto y = reference.step(x);
                    for (std::size_t ch = 0; ch < 3; ++ch)
                        expected[ch][i] = y[ch];
                }
                const auto meter = processor.gainReductionDb();
                const bool inplace = block % 2 == 0;
                ok(processor.process(in, inplace ? in : out));
                if (!n)
                    require(processor.gainReductionDb() == meter,
                            "empty block changed dynamics meter");
                for (std::size_t ch = 0; ch < 3; ++ch)
                    for (std::size_t i = 0; i < n; ++i) {
                        const auto error = std::abs(double((inplace ? in : out).channel(ch)[i]) -
                                                    double(expected[ch][i]));
                        maximumError = std::max(maximumError, error);
                        require(error < (std::same_as<T, float> ? 2e-7 : 2e-14),
                                "dynamics sample oracle mismatch");
                    }
                const auto meterError = std::abs(processor.gainReductionDb() - reference.reduction);
                maximumMeterError = std::max(maximumMeterError, meterError);
                require(meterError < 2e-12, "dynamics meter oracle mismatch");
                require(processor.latencyFrames() == reference.latency, "dynamics latency changed");
            }
        }
        struct Context {
            Dynamics<T>* p;
            AudioBlock<T> b;
            bool passed = true;
        } context{&processor, input.block()};
        runRealtimeAudit(
            [](void* raw) noexcept {
                auto& c = *static_cast<Context*>(raw);
                for (unsigned i = 0; i < 10; ++i) {
                    c.passed &= c.p->process(c.b, c.b) == Status::ok;
                    c.p->reset();
                }
            },
            &context);
        require(context.passed, "dynamics realtime processing");
    }
    std::cout << "QUALITY dynamics " << (std::same_as<T, float> ? "float" : "double")
              << " maxAbsError=" << maximumError << " meterErrorDb=" << maximumMeterError << '\n';
}
template <class T> void wavetable() {
    Wavetable<T> table;
    std::vector<T> cycle(2048);
    for (std::size_t i = 0; i < cycle.size(); ++i)
        cycle[i] = T(2. * double(i) / double(cycle.size()) - 1.);
    ok(table.prepare(cycle, CpuBackend::scalar));
    AudioBuffer<T> output;
    ok(output.resize(3, 128));
    WavetableOscillator<T> block, one;
    ok(block.prepare({48000, 128, 3}, table));
    ok(one.prepare({48000, 128, 3}, table));
    for (double frequency : {0., .01, 55., 440., 379.123, 12000., 23999., 24000.}) {
        ok(block.setFrequency(frequency));
        ok(one.setFrequency(frequency));
        block.reset(-2.75);
        one.reset(-2.75);
        for (unsigned pass = 0; pass < 12; ++pass) {
            const std::array<std::size_t, 6> splits{0, 1, 7, 128, 31, 3};
            const auto n = splits[pass % 6];
            const bool add = pass % 2 != 0;
            auto out = output.block().subBlock(0, n);
            for (std::size_t ch = 0; ch < 3; ++ch)
                std::fill(out.channel(ch).begin(), out.channel(ch).end(), T(.125));
            ok(block.process(out, add));
            for (std::size_t i = 0; i < n; ++i) {
                const T sample = one.next();
                const T expected = add ? T(T(.125) + sample) : sample;
                for (std::size_t ch = 0; ch < 3; ++ch)
                    require(out.channel(ch)[i] == expected, "wavetable block/next mismatch");
            }
        }
        require(block.next() == one.next(), "wavetable phase progression changed");
    }
    // Refresh bank pointers after table replacement, move-out and move-back.
    auto moved = std::move(table);
    ok(block.process(output.block()));
    for (T sample : output.block().channel(0))
        require(sample == 0, "moved-from wavetable read");
    table = std::move(moved);
    cycle.resize(16);
    std::fill(cycle.begin(), cycle.end(), T(.25));
    ok(table.prepare(cycle, CpuBackend::scalar));
    ok(block.process(output.block()));
    for (T sample : output.block().channel(0))
        require(std::abs(double(sample) - .25) < 1e-6, "reprepared table read");
    // An invalid block must not advance phase.
    one = block;
    ok(one.process(output.block().subBlock(0, 0)));
    require(block.process({nullptr, 3, 1}) != Status::ok, "invalid oscillator output accepted");
    require(block.next() == one.next(), "invalid block advanced phase");
    struct Context {
        WavetableOscillator<T>* p;
        AudioBlock<T> b;
        bool passed = true;
    } context{&block, output.block()};
    runRealtimeAudit(
        [](void* raw) noexcept {
            auto& c = *static_cast<Context*>(raw);
            c.passed &= c.p->process(c.b) == Status::ok;
            c.p->reset();
        },
        &context);
    require(context.passed, "wavetable realtime processing");
}
template <class T> void vectorBounds() {
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::avx512,
                         CpuBackend::neon, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        for (std::size_t n = 0; n < 132; ++n) {
            // The source allocation ends at the last valid sample (ASan catches overreads).
            std::vector<T> a(n + 1), b(n + 1), d(n + 2, T(99));
            for (std::size_t i = 0; i < n; ++i) {
                a[i + 1] = T(int(i % 17) - 8) * T(.03125);
                b[i + 1] = T(int(i % 13) - 6) * T(.0625);
            }
            auto x = std::span<const T>(a).subspan(1), y = std::span<const T>(b).subspan(1);
            auto out = std::span<T>(d).subspan(1, n);
            ok(ops.copy(x, out));
            for (std::size_t i = 0; i < n; ++i)
                require(out[i] == x[i], "copy tail");
            ok(ops.scale(x, out, T(.75)));
            for (std::size_t i = 0; i < n; ++i)
                require(out[i] == x[i] * T(.75), "scale tail");
            ok(ops.multiply(x, y, out));
            for (std::size_t i = 0; i < n; ++i)
                require(out[i] == x[i] * y[i], "multiply tail");
            ok(ops.addScaled(x, out, T(.25)));
            for (std::size_t i = 0; i < n; ++i)
                require(out[i] == x[i] * y[i] + x[i] * T(.25), "addScaled tail");
            ops.fill(out, T(.375));
            for (T v : out)
                require(v == T(.375), "fill tail");
            require(d.front() == T(99) && d.back() == T(99), "vector tail wrote outside span");
            std::vector<std::complex<T>> ca(n + 1), cb(n + 1), cd(n + 2, {T(99), T(-99)});
            for (std::size_t i = 1; i <= n; ++i) {
                ca[i] = {a[i], b[i]};
                cb[i] = {b[i], a[i]};
                cd[i] = {T(.125), T(-.25)};
            }
            ok(ops.complexMultiplyAccumulate(std::span<const std::complex<T>>(ca).subspan(1),
                                             std::span<const std::complex<T>>(cb).subspan(1),
                                             std::span<std::complex<T>>(cd).subspan(1, n)));
            for (std::size_t i = 1; i <= n; ++i)
                require(cd[i] == std::complex<T>(T(.125), T(-.25)) + ca[i] * cb[i],
                        "complex MAC tail");
            require(cd.front() == std::complex<T>(T(99), T(-99)) && cd.back() == cd.front(),
                    "complex tail wrote outside span");
        }
    }
}
} // namespace
void performanceRegression() {
    dynamics<float>();
    dynamics<double>();
    wavetable<float>();
    wavetable<double>();
    vectorBounds<float>();
    vectorBounds<double>();
}
