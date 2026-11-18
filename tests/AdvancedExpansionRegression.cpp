#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cydsp/CyDSP.h>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <vector>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
void ok(Status status) {
    require(status == Status::ok, "advanced extension status");
}
constexpr double pi = std::numbers::pi;
std::vector<Vec3> sphere(std::size_t n) {
    std::vector<Vec3> result;
    for (std::size_t i = 0; i < n; ++i) {
        const double z = 1 - 2 * (double(i) + .5) / double(n),
                     phi = double(i) * pi * (3 - std::sqrt(5.)), r = std::sqrt(1 - z * z);
        result.push_back({r * std::cos(phi), r * std::sin(phi), z});
    }
    return result;
}
void hoa7() {
    const Quaternion q{std::cos(.43), .2 * std::sin(.43), .4 * std::sin(.43),
                       std::sqrt(.8) * std::sin(.43)};
    for (unsigned order : {4u, 5u, 6u, 7u}) {
        const auto n = std::size_t((order + 1) * (order + 1));
        std::array<double, 64> a{}, b{};
        std::array<double, 4096> matrix{};
        ok(ambisonicRotation(order, q, {matrix.data(), n * n}));
        for (auto d : sphere(11)) {
            ok(ambisonicCoefficients(order, d, {a.data(), n}));
            ok(ambisonicCoefficients(order, rotate(q, d), {b.data(), n}));
            for (std::size_t row = 0; row < n; ++row) {
                double sum = 0;
                for (std::size_t c = 0; c < n; ++c)
                    sum += matrix[row * n + c] * a[c];
                require(std::abs(sum - b[row]) < 3e-12, "HOA7 active rotation covariance");
            }
        }
        for (std::size_t row = 0; row < n; ++row)
            for (std::size_t c = 0; c < n; ++c) {
                double sum = 0;
                for (std::size_t k = 0; k < n; ++k)
                    sum += matrix[row * n + k] * matrix[c * n + k];
                require(std::abs(sum - double(row == c)) < 3e-12, "HOA7 orthogonal rotation");
            }
    }
    std::array<double, 81> bad{};
    require(ambisonicCoefficients(8, {1, 0, 0}, bad) == Status::invalid_argument, "HOA bound");
}
template <class T> void batch() {
    const ProcessSpec spec{8000, 64, 2};
    AudioBuffer<T> out;
    ok(out.resize(2, 64));
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::avx512,
                         CpuBackend::neon, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        SourceBatch<T> p;
        ok(p.prepare(spec, 32, {}, {}, backend));
        for (std::size_t v = 0; v < 32; ++v) {
            BatchSource source;
            source.frequencyHz = 40 + 17 * double(v);
            source.phase = .01 * double(v);
            source.gain = .02;
            source.startOffsetFrames = v % 7;
            source.channelGains[1] = -.5;
            ok(p.start(v, source));
        }
        require(p.start(0, {}) == Status::would_block, "batch no stealing");
        for (std::size_t block = 0; block < 40; ++block) {
            ok(p.process(out.block()));
            for (std::size_t i = 0; i < 64; ++i) {
                double expected = 0;
                const auto t = block * 64 + i;
                for (std::size_t v = 0; v < 32; ++v)
                    if (t >= v % 7)
                        expected +=
                            .02 * std::sin(2 * pi *
                                           (.01 * double(v) +
                                            double(t - v % 7) * (40 + 17 * double(v)) / 8000));
                require(std::abs(double(out.block().channel(0)[i]) - expected) <
                            (std::same_as<T, float> ? 2e-5 : 2e-12),
                        "SIMD source batch oscillator oracle");
                require(std::abs(double(out.block().channel(1)[i]) +
                                 .5 * double(out.block().channel(0)[i])) < 1e-7,
                        "batch routing");
            }
        }
        struct Audit {
            SourceBatch<T>* p;
            AudioBlock<T> out;
            Status status;
        } audit{&p, out.block(), Status::ok};
        runRealtimeAudit(
            [](void* v) noexcept {
                auto& a = *static_cast<Audit*>(v);
                a.status = a.p->process(a.out);
                a.p->reset();
            },
            &audit);
        ok(audit.status);
        require(p.activeVoices() == 0, "batch reset");
        require(p.prepare({0, 64, 2}, 32) == Status::invalid_spec, "batch failed prepare");
        ok(p.start(0, {}));
        auto moved = std::move(p);
        require(p.process(out.block()) == Status::not_prepared, "batch moved from");
    }
    AudioBuffer<T> sample;
    ok(sample.resize(1, 2048));
    std::fill(sample.block().channel(0).begin(), sample.block().channel(0).end(), T{1});
    for (std::size_t i = 1024; i < 2048; ++i)
        sample.block().channel(0)[i] = T(double(i) / 2048 + (i % 2 ? .25 : -.25));
    SampleData<T> data;
    ok(data.prepare(sample.block(), 8000));
    const SampleData<T>* assets[]{&data};
    SourceBatch<T> grains;
    ok(grains.prepare(spec, 8, {}, assets));
    BatchSource g;
    g.kind = BatchSourceKind::grain;
    g.samplePosition = 512;
    g.durationFrames = 32;
    g.startOffsetFrames = 7;
    ok(grains.start(0, g));
    ok(grains.setPosition(0, {0, 2, 0}));
    ok(grains.process(out.block()));
    for (std::size_t i = 0; i < 64; ++i) {
        const double expected =
            i >= 7 && i < 39 ? .25 - .25 * std::cos(2 * pi * double(i - 7) / 32) : 0;
        require(std::abs(double(out.block().channel(0)[i]) - expected) < 2e-6,
                "shared sample grain window/spatial gain");
        require(std::abs(double(out.block().channel(1)[i])) < 1e-12, "left positioned grain");
    }
    require(grains.activeVoices() == 0, "grain lifetime");
    g.playbackRatio = -2;
    g.samplePosition = 1536;
    g.startOffsetFrames = 0;
    ok(grains.start(0, g));
    ok(grains.process(out.block()));
    for (std::size_t i = 0; i < g.durationFrames; ++i) {
        const double position = g.samplePosition + double(i) * g.playbackRatio;
        const T envelope = T(.5 - .5 * std::cos(2 * pi * double(i) / double(g.durationFrames)));
        const T expected = data.value(0, position, 2) * envelope;
        require(std::abs(double(out.block().channel(0)[i]) - expected) < 1e-7,
                "reverse grain position and antialias filter");
    }
    std::array<T, 256> cycle{};
    for (std::size_t i = 0; i < cycle.size(); ++i)
        cycle[i] = T(std::sin(2 * pi * double(i) / 256));
    Wavetable<T> table;
    ok(table.prepare(cycle));
    const Wavetable<T>* tables[]{&table};
    SourceBatch<T> tablesBatch;
    ok(tablesBatch.prepare(spec, 1, tables));
    BatchSource oscillator;
    oscillator.kind = BatchSourceKind::wavetable;
    oscillator.frequencyHz = 125;
    ok(tablesBatch.start(0, oscillator));
    ok(tablesBatch.process(out.block()));
    for (std::size_t i = 0; i < 64; ++i)
        require(std::abs(double(out.block().channel(0)[i]) -
                         std::sin(2 * pi * 125 * double(i) / 8000)) < 2e-5,
                "shared wavetable batch");
}
template <class T> void hrtf() {
    std::array<T, 64> l{}, r{};
    l[8] = 1;
    r[12] = T{.5};
    const std::array<HrirPoint<T>, 2> points{{{{1, 0, 0}, l, r}, {{-1, 0, 0}, l, r}}};
    HrtfSet<T> set;
    HrtfPreparation cfg;
    cfg.sourceSampleRate = cfg.targetSampleRate = 8000;
    ok(set.prepare(points, cfg));
    require(set.points() == 2 && !set.truncated(), "prepared HRTF data");
    HrirBinaural<T> renderer;
    ok(renderer.prepare({8000, 64, 1}, set));
    set = {}; // The renderer retains the immutable bank.
    AudioBuffer<T> in, out;
    ok(in.resize(1, 64));
    ok(out.resize(2, 64));
    in.block().channel(0)[0] = 1;
    ok(renderer.setDirection({0, 1, 0}, 0));
    ok(renderer.process(in.block(), out.block()));
    for (std::size_t i = 0; i < 64; ++i) {
        require(std::abs(double(out.block().channel(0)[i]) - double(i == 8)) < 2e-6,
                "HRTF onset alignment left");
        require(std::abs(double(out.block().channel(1)[i]) - (i == 12 ? .5 : 0)) < 2e-6,
                "HRTF onset alignment right");
    }
    cfg.targetSampleRate = 16000;
    ok(set.prepare(points, cfg));
    require(set.latencyFrames() == 64, "HRTF conversion latency");
    ok(renderer.prepare({16000, 64, 1}, set));
    double sums[2]{};
    for (int block = 0; block < 8; ++block) {
        std::fill(in.block().channel(0).begin(), in.block().channel(0).end(), T{});
        if (!block)
            in.block().channel(0)[0] = 1;
        ok(renderer.process(in.block(), out.block()));
        for (std::size_t ear = 0; ear < 2; ++ear)
            for (T x : out.block().channel(ear))
                sums[ear] += x;
    }
    require(std::abs(sums[0] - 1) < 2e-4 && std::abs(sums[1] - .5) < 2e-4,
            "HRTF sample-rate conversion DC gain");
    cfg.targetSampleRate = 8000;
    cfg.detectOnsets = false;
    cfg.angularWidthRadians = .05;
    ok(set.prepare(points, cfg));
    ok(renderer.prepare({8000, 64, 1}, set));
    ok(renderer.setDirection({0, 1, 0}, 0));
    renderer.reset();
    struct A {
        HrirBinaural<T>* p;
        AudioBlock<const T> in;
        AudioBlock<T> out;
        Status st;
    } a{&renderer, in.block(), out.block(), Status::ok};
    runRealtimeAudit(
        [](void* v) noexcept {
            auto& x = *static_cast<A*>(v);
            x.st = x.p->setDirection({0, 0, 1}, 64);
            if (x.st == Status::ok)
                x.st = x.p->process(x.in, x.out);
            x.p->reset();
        },
        &a);
    ok(a.st);
    auto dirs = sphere(32);
    std::vector<HrirPoint<T>> flat;
    const std::array<T, 1> left{1}, right{T{.5}};
    for (auto d : dirs)
        flat.push_back({d, left, right});
    ok(set.prepare(flat, cfg));
    HoaBinaural<T> hoa;
    ok(hoa.prepare({8000, 64, 16}, 3, set));
    AudioBuffer<T> field;
    ok(field.resize(16, 64));
    field.block().channel(0)[0] = 1;
    ok(hoa.setRotation({std::cos(.3), 0, 0, std::sin(.3)}));
    ok(hoa.process(field.block(), out.block()));
    require(std::abs(double(out.block().channel(0)[0]) - 1) < 2e-5 &&
                std::abs(double(out.block().channel(1)[0]) - .5) < 2e-5,
            "HOA binaural isotropic projection");
    AmbisonicRenderer<T> irregular;
    ok(irregular.prepare({8000, 64, 1}, 7, sphere(64), 1e-5));
    require(irregular.channels() == 64, "order7 irregular decoder");
}
void diffractionAndIr() {
    DiffractionPath p;
    double a = 0, b = 0;
    ok(diffractionGain(p, 1000, a));
    require(std::abs(a - .5) < .001, "knife-edge boundary loss");
    p.edgeOffsetMetres = .2;
    ok(diffractionGain(p, 125, a));
    ok(diffractionGain(p, 8000, b));
    require(b < a && b > 0, "diffraction shadow frequency dependence");
    p.edgeOffsetMetres = -1;
    ok(diffractionGain(p, 8000, a));
    require(a == 1, "visible path");
    p.openingWidthMetres = 1;
    p.openingAngleRadians = pi / 6;
    ok(diffractionGain(p, 686, b));
    require(b < 1e-12, "rectangular opening first null");
    const auto saved = b;
    p.soundSpeed = 0;
    require(diffractionGain(p, 1000, b) == Status::invalid_argument && b == saved,
            "diffraction transactional error");
    std::vector<double> ir(24000);
    for (std::size_t i = 0; i < ir.size(); ++i)
        ir[i] = std::exp(-std::log(1000.) * double(i) / (8000 * .4));
    ImpulseReport report;
    ok(analyzeImpulse(ir, 8000, report));
    require(report.broadband.t30.valid && std::abs(report.broadband.t30.seconds - .4) < 1e-5,
            "Schroeder analytic RT60");
    const double late = std::exp(-2 * std::log(1000.) * .05 / .4);
    require(std::abs(report.clarity50Db - 10 * std::log10((1 - late) / late)) < 1e-5,
            "analytic clarity C50");
    FdnParameters fdn;
    ImpulseReport matching = report;
    const std::array<double, 9> targetTimes{.2, .3, .4, .6, .9, 1.2, 1.5, 2, 2.5};
    for (std::size_t band = 0; band < 9; ++band)
        matching.octaveBands[band].t30 = {targetTimes[band], 1, true};
    ok(matchFdnToImpulse(matching, fdn));
    require(std::abs(fdn.decaySeconds[0] - .3) < 1e-12 &&
                std::abs(fdn.decaySeconds[1] - .9) < 1e-12 &&
                std::abs(fdn.decaySeconds[2] - 2) < 1e-12,
            "IR FDN nominal band match");
    std::fill(ir.begin(), ir.end(), 0);
    ir[40] = 1;
    ir[120] = .5;
    ok(analyzeImpulse(ir, 8000, report));
    require(report.reflectionCount == 1 && std::abs(report.reflections[0].seconds - .01) < 1e-12,
            "early reflection timing");
    require(std::abs(report.directToReverberantDb - 10 * std::log10(4.)) < 1e-12,
            "IR direct reverberant energy");
    require(!report.broadband.t30.valid, "short sparse IR does not claim valid decay");
}
template <class T> void spectral() {
    const ProcessSpec spec{8000, 64, 2};
    const StftConfig stft{256, 64, WindowType::hann};
    AudioBuffer<T> in, h, p, out;
    for (auto* b : {&in, &h, &p, &out})
        ok(b->resize(2, 64));
    HarmonicPercussive<T> split;
    ok(split.prepare(spec, {stft, 9, 17}));
    const auto latency = split.latencyFrames();
    std::vector<double> source(8192);
    for (std::size_t i = 0; i < source.size(); ++i)
        source[i] = .2 * std::sin(2 * pi * 500 * double(i) / 8000) + (i == 2000 ? .8 : 0);
    double maxError = 0, hEnergy = 0, pEnergy = 0;
    for (std::size_t block = 0; block < source.size() / 64; ++block) {
        for (std::size_t i = 0; i < 64; ++i) {
            in.block().channel(0)[i] = T(source[block * 64 + i]);
            in.block().channel(1)[i] = T(-2 * source[block * 64 + i]);
        }
        ok(split.process(in.block(), h.block(), p.block()));
        for (std::size_t i = 0; i < 64; ++i) {
            const auto t = block * 64 + i;
            const double expected = t >= latency ? source[t - latency] : 0;
            maxError = std::max(maxError, std::abs(double(h.block().channel(0)[i]) +
                                                   double(p.block().channel(0)[i]) - expected));
            require(std::abs(double(h.block().channel(1)[i]) +
                             2 * double(h.block().channel(0)[i])) < 1e-5,
                    "HPSS linked stereo mask");
            if (t > 4000) {
                hEnergy += std::pow(double(h.block().channel(0)[i]), 2);
                pEnergy += std::pow(double(p.block().channel(0)[i]), 2);
            }
        }
    }
    require(maxError < 2e-5, "HPSS complementary delayed reconstruction");
    require(hEnergy > 10 * pEnergy, "HPSS tonal discrimination");
    SpectralPitchShifter<T> pitch;
    ok(pitch.prepare(spec, stft));
    SpectralPitchParameters parameters;
    for (std::size_t block = 0; block < source.size() / 64; ++block) {
        for (std::size_t i = 0; i < 64; ++i)
            for (std::size_t ch = 0; ch < 2; ++ch)
                in.block().channel(ch)[i] = T((ch == 0 ? 1 : -2) * source[block * 64 + i]);
        ok(pitch.process(in.block(), out.block()));
        for (std::size_t i = 0; i < 64; ++i) {
            const auto t = block * 64 + i;
            require(std::abs(double(out.block().channel(0)[i]) - (t >= 256 ? source[t - 256] : 0)) <
                        2e-5,
                    "spectral pitch identity latency");
        }
    }
    pitch.reset();
    parameters.ratio = 1.5;
    parameters.preserveFormants = false;
    parameters.protectTransients = false;
    ok(pitch.setParameters(parameters));
    double targetPower = 0, originalPower = 0;
    std::complex<double> target{}, original{};
    for (std::size_t block = 0; block < 128; ++block) {
        for (std::size_t i = 0; i < 64; ++i) {
            const double x = .2 * std::sin(2 * pi * 250 * double(block * 64 + i) / 8000);
            in.block().channel(0)[i] = T(x);
            in.block().channel(1)[i] = T(-2 * x);
        }
        ok(pitch.process(in.block(), out.block()));
        for (std::size_t i = 0; i < 64; ++i) {
            const auto t = block * 64 + i;
            const double x = out.block().channel(0)[i];
            require(std::abs(double(out.block().channel(1)[i]) + 2 * x) < 2e-5,
                    "spectral pitch interchannel phase coherence");
            if (t >= 4096) {
                target += x * std::polar(1., -2 * pi * 375 * double(t) / 8000);
                original += x * std::polar(1., -2 * pi * 250 * double(t) / 8000);
            }
        }
    }
    targetPower = std::norm(target);
    originalPower = std::norm(original);
    require(targetPower > 100 * originalPower && targetPower > 10, "spectral pitch known ratio");
    PitchEffects<T> harmony;
    ok(harmony.prepareSpectral(spec, 2, stft, {512, 64, 20, 8, 60, 1000}));
    require(harmony.latencyFrames() == 256, "harmony spectral latency");
    ok(harmony.setVoices(std::array<HarmonyVoice, 2>{{{0, .5, 0, 0}, {12, .5, 0, 0}}}));
    ok(harmony.process(in.block(), out.block()));
    struct Audit {
        HarmonicPercussive<T>* h;
        SpectralPitchShifter<T>* p;
        AudioBlock<const T> in;
        AudioBlock<T> a, b, c;
        Status status;
    } audit{&split, &pitch, in.block(), h.block(), p.block(), out.block(), Status::ok};
    runRealtimeAudit(
        [](void* ptr) noexcept {
            auto& a = *static_cast<Audit*>(ptr);
            a.status = a.h->process(a.in, a.a, a.b);
            if (a.status == Status::ok)
                a.status = a.p->process(a.in, a.c);
            a.h->reset();
            a.p->reset();
        },
        &audit);
    ok(audit.status);
    in.block().channel(0)[63] = std::numeric_limits<T>::quiet_NaN();
    std::fill(out.block().channel(0).begin(), out.block().channel(0).end(), T{7});
    require(pitch.process(in.block(), out.block()) == Status::invalid_argument &&
                out.block().channel(0)[0] == 7,
            "spectral NaN preflight");
}
template <class T> void vowelRetention() {
    const auto envelope = [](double f) {
        return std::exp(-.5 * std::pow((f - 600) / 150, 2)) +
               .7 * std::exp(-.5 * std::pow((f - 1600) / 200, 2)) + .005;
    };
    std::array<double, 2> errors{};
    for (int mode = 0; mode < 2; ++mode) {
        SpectralPitchShifter<T> pitch;
        ok(pitch.prepare({16000, 128, 1}, {1024, 256, WindowType::hann}));
        SpectralPitchParameters parameters;
        parameters.ratio = 2;
        parameters.preserveFormants = mode != 0;
        parameters.protectTransients = false;
        ok(pitch.setParameters(parameters));
        AudioBuffer<T> x, y;
        ok(x.resize(1, 128));
        ok(y.resize(1, 128));
        std::array<std::complex<double>, 24> sums{};
        for (std::size_t block = 0; block < 128; ++block) {
            for (std::size_t i = 0; i < 128; ++i) {
                const auto t = block * 128 + i;
                double value = 0;
                for (unsigned harmonic = 1; harmonic <= 60; ++harmonic) {
                    const double f = 62.5 * harmonic;
                    value += .02 * envelope(f) * std::sin(2 * pi * f * double(t) / 16000);
                }
                x.block().channel(0)[i] = T(value);
            }
            ok(pitch.process(x.block(), y.block()));
            if (block >= 64)
                for (std::size_t i = 0; i < 128; ++i)
                    for (std::size_t k = 0; k < sums.size(); ++k)
                        sums[k] += double(y.block().channel(0)[i]) *
                                   std::polar(1., -2 * pi * 125 * double(k + 1) *
                                                      double(block * 128 + i) / 16000);
        }
        double dot = 0, power = 0, norm = 0, error = 0;
        for (std::size_t k = 0; k < sums.size(); ++k) {
            const double expected = envelope(125 * double(k + 1));
            dot += expected * std::abs(sums[k]);
            power += std::norm(sums[k]);
        }
        const double scale = dot / std::max(1e-100, power);
        for (std::size_t k = 0; k < sums.size(); ++k) {
            const double expected = envelope(125 * double(k + 1));
            error += std::pow(scale * std::abs(sums[k]) - expected, 2);
            norm += expected * expected;
        }
        errors[std::size_t(mode)] = std::sqrt(error / norm);
    }
    require(errors[1] < .35 && errors[1] < .5 * errors[0],
            "spectral pitch retains synthetic vowel envelope");
}
template <class T> void contactsAndEcho() {
    const ProcessSpec spec{8000, 64, 1};
    AudioBuffer<T> in, mic, out;
    for (auto* b : {&in, &mic, &out})
        ok(b->resize(1, 64));
    ContactExciter<T> exciter;
    ok(exciter.prepare(spec));
    ContactParameters cp;
    cp.kind = ContactKind::impact;
    ok(exciter.setParameters(cp));
    ok(exciter.strike(.001));
    double recovered = 0, integral = 0;
    const double pole = std::exp(-2 * pi * 20 / 8000);
    for (int block = 0; block < 16; ++block) {
        ok(exciter.render(out.block()));
        for (T y : out.block().channel(0)) {
            const double force = double(y) + (1 - pole) * integral;
            recovered += force / 8000;
            integral += y;
        }
    }
    require(std::abs(recovered - .001) < 2e-7, "contact physical impulse");
    for (auto kind : {ContactKind::sliding, ContactKind::rolling, ContactKind::bowed}) {
        cp.kind = kind;
        cp.roughness = .5;
        ok(exciter.setParameters(cp));
        exciter.reset(42);
        ok(exciter.render(out.block()));
        std::vector<T> saved(out.block().channel(0).begin(), out.block().channel(0).end());
        exciter.reset(42);
        ok(exciter.render(out.block()));
        require(std::equal(saved.begin(), saved.end(), out.block().channel(0).begin()),
                "seeded contact texture");
        for (T x : saved)
            require(isFinite(x), "physical finite output");
    }
    EchoCanceller<T> echo;
    ok(echo.prepare(spec, {64, 4, 100}));
    std::array<double, 128> reference{};
    std::uint64_t seed = 123;
    std::size_t head = 0;
    double lateError = 0, lateDesired = 0;
    bool doubleTalkSeen = false;
    std::vector<double> frozen;
    for (std::size_t block = 0; block < 320; ++block) {
        for (std::size_t i = 0; i < 64; ++i) {
            seed = seed * 6364136223846793005ULL + 1;
            const double x = (double(seed >> 11) / 9007199254740992. - .5) * .4;
            reference[head] = x;
            const auto read = [&](std::size_t age) {
                return reference[(head + reference.size() - age) % reference.size()];
            };
            double y = .6 * read(6) + .2 * read(9) - .1 * read(21);
            if (block >= 260)
                y += .8 * std::sin(2 * pi * 733 * double(block * 64 + i) / 8000);
            in.block().channel(0)[i] = T(x);
            mic.block().channel(0)[i] = T(y);
            head = (head + 1) % reference.size();
        }
        ok(echo.process(in.block(), mic.block(), out.block()));
        if (block >= 220 && block < 250)
            for (std::size_t i = 0; i < 64; ++i) {
                lateError += std::pow(double(out.block().channel(0)[i]), 2);
                lateDesired += std::pow(double(mic.block().channel(0)[i]), 2);
            }
        if (block >= 280) {
            const auto m = echo.metrics();
            doubleTalkSeen = doubleTalkSeen || m.doubleTalk;
            require(!m.adapting, "AEC freezes during double talk");
            const auto coefficients = echo.coefficients(0);
            if (frozen.empty())
                frozen.assign(coefficients.begin(), coefficients.end());
            else
                require(std::equal(frozen.begin(), frozen.end(), coefficients.begin()),
                        "AEC frozen coefficients");
        }
    }
    require(lateError < lateDesired * 1e-6, "AEC known FIR cancellation 60dB");
    require(doubleTalkSeen, "AEC detects near-end speech");
    struct A {
        EchoCanceller<T>* e;
        ContactExciter<T>* c;
        AudioBlock<const T> f, m;
        AudioBlock<T> out;
        Status st;
    } a{&echo, &exciter, in.block(), mic.block(), out.block(), Status::ok};
    runRealtimeAudit(
        [](void* p) noexcept {
            auto& x = *static_cast<A*>(p);
            x.st = x.e->process(x.f, x.m, x.out);
            if (x.st == Status::ok)
                x.st = x.c->render(x.out);
            x.e->reset();
            x.c->reset();
        },
        &a);
    ok(a.st);
}
template <class T> void qualities() {
    QualitySettings economy, high, offline;
    ok(qualitySettings(QualityProfile::economy, 48000, economy));
    ok(qualitySettings(QualityProfile::realtimeHigh, 48000, high));
    ok(qualitySettings(QualityProfile::offline, 48000, offline));
    require(economy.spectral.fftFrames < high.spectral.fftFrames &&
                high.spectral.fftFrames < offline.spectral.fftFrames,
            "quality spectral policy");
    require(economy.reverb.lines == 8 && offline.oversampling == 8, "quality common policy");
    const ProcessSpec spec{8000, 64, 1};
    const std::array<DspBus, 1> buses{{{0, 1}}};
    auto graph = [&](double gain) {
        MultiBusGraph<T> g;
        const std::array<DspConnection, 1> edges{{{graphInputNode, 0, graphOutputNode, 0, gain}}};
        ok(g.prepare(spec, buses, buses, {}, edges));
        return g;
    };
    QualityGraph<T> transition;
    ok(transition.prepare(spec, buses, buses, 16));
    ok(transition.setInitial(QualityProfile::economy, graph(1)));
    AudioBuffer<T> in, out;
    ok(in.resize(1, 64));
    ok(out.resize(1, 64));
    std::fill(in.block().channel(0).begin(), in.block().channel(0).end(), T{1});
    const std::array<AudioBlock<const T>, 1> inputs{in.block()};
    std::array<AudioBlock<T>, 1> outputs{out.block()};
    ok(transition.process(inputs, outputs));
    ok(transition.stage(QualityProfile::offline, graph(2)));
    ok(transition.startTransition(64));
    ok(transition.process(inputs, outputs));
    require(transition.activeProfile() == QualityProfile::economy && transition.transitioning(),
            "quality warmup and fade progress");
    ok(transition.process(inputs, outputs));
    require(transition.activeProfile() == QualityProfile::offline && !transition.transitioning(),
            "quality profile committed after fade");
    require(std::abs(double(out.block().channel(0)[63]) - 2) < 1e-6, "quality graph new endpoint");
    struct A {
        QualityGraph<T>* p;
        std::span<const AudioBlock<const T>> in;
        std::span<AudioBlock<T>> out;
        Status st;
    } a{&transition, inputs, outputs, Status::ok};
    runRealtimeAudit(
        [](void* p) noexcept {
            auto& x = *static_cast<A*>(p);
            x.st = x.p->process(x.in, x.out);
            x.p->reset();
        },
        &a);
    ok(a.st);
}
} // namespace
void advancedExpansionRegression() {
    hoa7();
    diffractionAndIr();
    batch<float>();
    batch<double>();
    hrtf<float>();
    hrtf<double>();
    spectral<float>();
    spectral<double>();
    vowelRetention<float>();
    vowelRetention<double>();
    contactsAndEcho<float>();
    contactsAndEcho<double>();
    qualities<float>();
    qualities<double>();
}
