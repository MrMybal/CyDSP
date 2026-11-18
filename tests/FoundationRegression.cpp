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
void require(bool value, const char* why) {
    if (!value)
        throw std::runtime_error(why);
}
void ok(Status s) {
    require(s == Status::ok, "foundation status");
}
template <class T> void history() {
    CircularHistory<T> h;
    ok(h.prepare(2, 17));
    std::array<T, 2> x{};
    for (int i = 0; i < 101; ++i) {
        x = {T(i), T(-i)};
        ok(h.pushFrame(x));
        require(h.read(0, 0) == T(i), "history current frame");
    }
    for (int age = 0; age < 17; ++age)
        require(h.read(1, double(age)) == T(-(100 - age)), "history wrap oracle");
    require(h.read(0, -1) == 0 && h.read(0, 17) == 0, "history invalid age");
    for (auto interpolation : {HistoryInterpolation::linear, HistoryInterpolation::lagrange3,
                               HistoryInterpolation::catmullRom})
        require(std::abs(double(h.read(0, 4.25, interpolation)) - 95.75) < 1e-6,
                "history linear-ramp interpolation oracle");
    x[1] = std::numeric_limits<T>::infinity();
    require(h.pushFrame(x) == Status::invalid_argument && h.read(0, 0) == 100,
            "history rejected frame transaction");
    require(h.prepare(0, 10) == Status::invalid_spec && h.capacity() == 17,
            "history preparation transaction");
    auto moved = std::move(h);
    require(!h.capacity() && moved.capacity() == 17, "history move");
    moved.reset();
    require(!moved.availableFrames() && moved.read(0, 0) == 0, "history reset");
    ThiranDelay<T> thiran;
    ok(thiran.setDelay(.5));
    double inputEnergy = 0, outputEnergy = 0;
    for (int i = 0; i < 2048; ++i) {
        const T sample = i == 0 ? T{1} : T{};
        const double y = thiran.processSample(sample);
        inputEnergy += double(sample) * sample;
        outputEnergy += y * y;
    }
    require(std::abs(outputEnergy - inputEnergy) < 1e-6, "Thiran allpass impulse energy");
}
template <class T> void taps() {
    MultiTapDelay<T> delay;
    ok(delay.prepare({48000, 64, 1}, 2, 32, 4));
    std::array<DelayTap, 2> taps{{{0, 0, 3, .5}, {0, 1, 7, -.25}}};
    ok(delay.setTaps(taps));
    AudioBuffer<T> in, out;
    ok(in.resize(1, 64));
    ok(out.resize(2, 64));
    in.block().channel(0)[0] = 1;
    ok(delay.process(in.block(), out.block()));
    for (std::size_t i = 0; i < 64; ++i)
        require(out.block().channel(0)[i] == (i == 3 ? T(.5) : T{}) &&
                    out.block().channel(1)[i] == (i == 7 ? T(-.25) : T{}),
                "multitap impulse positions");
    taps[0].delayFrames = 2;
    ok(delay.setTaps(taps, 8));
    require(delay.setTaps(taps) == Status::transition_in_progress, "tap transition serialization");
    ok(delay.process(in.block(0), out.block(0)));
    require(delay.transitionRemainingFrames() == 8, "empty tap block advances transition");
    ok(delay.process(in.block(3), out.block(3)));
    require(delay.transitionRemainingFrames() == 5, "tap transition progress");
    struct C {
        MultiTapDelay<T>* delay;
        AudioBlock<const T> in;
        AudioBlock<T> out;
        bool passed = true;
    } c{&delay, in.block(), out.block()};
    runRealtimeAudit(
        [](void* raw) noexcept {
            auto& c = *static_cast<C*>(raw);
            c.passed = c.delay->process(c.in, c.out) == Status::ok;
            c.delay->reset();
        },
        &c);
    require(c.passed, "multitap realtime");
}
void parameters() {
    const std::array<ParameterDescriptor, 2> descriptors{
        {{7, "gain", ParameterUnit::linearGain, 0, 1, 0},
         {42, "frequency", ParameterUnit::hertz, 20, 20000, 1000, ParameterScale::logarithmic}}};
    ParameterBank bank;
    ok(bank.prepare(descriptors, 32));
    AudioBuffer<double> output;
    ok(output.resize(2, 16));
    std::array<ParameterEvent, 2> events{{{7, 2, 1, 4}, {7, 8, .5, 0}}};
    ok(bank.render(events, output.block()));
    for (std::size_t i = 0; i < 16; ++i) {
        const double expected = i < 2 ? 0 : i < 6 ? double(i - 1) * .25 : i < 8 ? 1 : .5;
        require(output.block().channel(0)[i] == expected, "parameter sample offset/ramp oracle");
    }
    const auto before = bank.value(7);
    events[1].id = 123;
    require(bank.render(events, output.block()) == Status::invalid_argument &&
                bank.value(7) == before,
            "parameter event transaction");
    require(std::abs(parameterFromNormalized(descriptors[1],
                                             parameterToNormalized(descriptors[1], 440)) -
                     440) < 1e-10,
            "parameter logarithmic conversion");
    const auto& hz = descriptors[1];
    const std::array<ParameterEvent, 2> limits{{
        {hz.id, 0, parameterFromNormalized(hz, 0), 0},
        {hz.id, 1, parameterFromNormalized(hz, 1), 0},
    }};
    ok(bank.render(limits, output.block(2)));
    require(output.block().channel(1)[0] == hz.minimum &&
                output.block().channel(1)[1] == hz.maximum,
            "logarithmic endpoints produce valid automation events");
    auto narrow = hz;
    narrow.minimum = narrow.initial = 20000;
    const double middle = std::nextafter(narrow.minimum, 20001.);
    narrow.maximum = std::nextafter(middle, 20001.);
    require(parameterFromNormalized(narrow, .5) == middle &&
                std::abs(parameterToNormalized(narrow, middle) - .5) < 1e-14 &&
                parameterToNormalized(narrow, narrow.maximum) == 1,
            "logarithmic ranges retain adjacent representable values");
    struct C {
        ParameterBank* bank;
        AudioBlock<double> out;
        bool passed = true;
    } c{&bank, output.block()};
    runRealtimeAudit(
        [](void* raw) noexcept {
            auto& c = *static_cast<C*>(raw);
            c.bank->reset();
            c.passed = c.bank->render({}, c.out) == Status::ok;
        },
        &c);
    require(c.passed, "parameter realtime");
}
template <class T> void stft(CpuBackend backend) {
    constexpr std::size_t length = 1003, n = 64, hop = 16;
    AudioBuffer<T> in, out, tail;
    ok(in.resize(2, length));
    ok(out.resize(2, length));
    ok(tail.resize(2, 2 * n));
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t i = 0; i < length; ++i)
            in.block().channel(ch)[i] = T(.3 * std::sin(double(i) * .13 + double(ch)) + .1);
    Stft<T> stft;
    ok(stft.prepare({48000, 128, 2}, {n, hop, WindowType::hann}, backend));
    const double tolerance = std::same_as<T, float> ? 2e-6 : 1e-12;
    for (std::size_t at = 0; at < length;) {
        auto count = std::min<std::size_t>(1 + at % 113, length - at);
        ok(stft.process(in.block().subBlock(at, count), out.block().subBlock(at, count)));
        at += count;
    }
    require(stft.latencyFrames() == n && stft.frameCount() == length / hop, "STFT timing");
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t i = 0; i < length; ++i)
            require(std::abs(double(out.block().channel(ch)[i]) -
                             (i >= n ? double(in.block().channel(ch)[i - n]) : 0)) < tolerance,
                    "STFT weighted overlap-add reconstruction");
    require(stft.flush(tail.block()) == Status::end_of_stream, "STFT flush length");
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t i = 0; i < 2 * n; ++i)
            require(std::abs(double(tail.block().channel(ch)[i]) -
                             (i < n ? double(in.block().channel(ch)[length - n + i]) : 0)) <
                        tolerance,
                    "STFT tail reconstruction");
    stft.reset();
    stft.setProcessor([](void*, std::size_t, std::span<std::complex<T>> bins) noexcept {
        for (auto& bin : bins)
            bin *= T(.5);
    });
    struct C {
        Stft<T>* stft;
        AudioBlock<const T> in;
        AudioBlock<T> out;
        bool passed = true;
    } c{&stft, in.block(128), out.block(128)};
    runRealtimeAudit(
        [](void* raw) noexcept {
            auto& c = *static_cast<C*>(raw);
            for (int i = 0; i < 4; ++i)
                c.passed &= c.stft->process(c.in, c.out) == Status::ok;
            c.stft->reset();
        },
        &c);
    require(c.passed, "STFT realtime");
    const auto count = stft.frameCount();
    in.block().channel(1)[127] = std::numeric_limits<T>::quiet_NaN();
    require(stft.process(in.block(128), out.block(128)) == Status::invalid_argument &&
                stft.frameCount() == count,
            "STFT input preflight");
}
} // namespace
void foundationRegression() {
    history<float>();
    history<double>();
    taps<float>();
    taps<double>();
    parameters();
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::avx512,
                         CpuBackend::neon, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        stft<float>(backend);
        stft<double>(backend);
    }
}
