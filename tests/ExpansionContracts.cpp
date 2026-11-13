#include <array>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <limits>
#include <stdexcept>
namespace {
using namespace cydsp;
void require(bool v, const char* m) {
    if (!v)
        throw std::runtime_error(m);
}
void ok(Status s) {
    require(s == Status::ok, "expansion contract status");
}
template <class P, class T> void standard() {
    const ProcessSpec spec{48000, 32, 2};
    P tested, reference;
    ok(tested.prepare(spec));
    ok(reference.prepare(spec));
    AudioBuffer<T> input, a, b;
    ok(input.resize(2, 32));
    ok(a.resize(2, 32));
    ok(b.resize(2, 32));
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t i = 0; i < 32; ++i)
            input.block().channel(ch)[i] = T(.2 * std::sin(.17 * double(i + ch)));
    ok(tested.process(input.block(), a.block()));
    ok(reference.process(input.block(), b.block()));
    require(tested.prepare({0, 32, 2}) != Status::ok, "failed prepare accepted");
    for (auto& x : a.block().channel(0))
        x = T(.75);
    const T saved = input.block().channel(1)[31];
    input.block().channel(1)[31] = std::numeric_limits<T>::quiet_NaN();
    require(tested.process(input.block(), a.block()) == Status::invalid_argument,
            "NaN preflight rejected too late");
    for (T x : a.block().channel(0))
        require(x == T(.75), "invalid input changed output");
    input.block().channel(1)[31] = saved;
    P moved(std::move(tested));
    require(tested.process(input.block(), a.block()) == Status::not_prepared,
            "moved-from processor remained prepared");
    ok(moved.process(input.block(), a.block()));
    ok(reference.process(input.block(), b.block()));
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t i = 0; i < 32; ++i)
            require(a.block().channel(ch)[i] == b.block().channel(ch)[i],
                    "failed request changed stream state");
    ok(moved.process(input.block(0), a.block(0)));
    ok(moved.process(input.block(), a.block()));
    ok(reference.process(input.block(), b.block()));
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t i = 0; i < 32; ++i)
            require(a.block().channel(ch)[i] == b.block().channel(ch)[i],
                    "empty block changed stream state");
}
template <class T> void processors() {
    standard<BandGain<T>, T>();
    standard<FdnReverb<T>, T>();
    standard<AdaaSaturator<T>, T>();
    standard<DynamicEqualizer<T>, T>();
    standard<DeEsser<T>, T>();
    standard<TransientShaper<T>, T>();
    standard<TruePeakLimiter<T>, T>();
    standard<DelayEffect<T>, T>();
    standard<PlateReverb<T>, T>();
    standard<SpringReverb<T>, T>();
    standard<SpectralEffects<T>, T>();
    standard<FrequencyShifter<T>, T>();
    standard<PitchEffects<T>, T>();
    standard<Waveguide<T>, T>();
    standard<SpectralDenoiser<T>, T>();
    standard<Dehum<T>, T>();
    standard<Declicker<T>, T>();
}
} // namespace
template <class T> void historyWindowContract() {
    cydsp::CircularHistory<T> history;
    ok(history.prepare(1, 10));
    for (int i = 0; i < 25; ++i) {
        const T x = T(i);
        ok(history.pushFrame({&x, 1}));
    }
    std::array<double, 16> values{};
    for (std::ptrdiff_t start : {-3, 0, 4, 15}) {
        ok(history.readWindow(0, start, values));
        for (std::size_t i = 0; i < values.size(); ++i) {
            const auto age = start + std::ptrdiff_t(i);
            require(values[i] == (age >= 0 && age < 10 ? double(24 - age) : 0.),
                    "batched history wrap/future/past");
        }
    }
}
void expansionContracts() {
    historyWindowContract<float>();
    historyWindowContract<double>();
    processors<float>();
    processors<double>();
    cydsp::MultiSegmentEnvelope envelope;
    ok(envelope.prepare(8000));
    const cydsp::EnvelopeSegment segment{.01, 1, 0};
    ok(envelope.setSegments({&segment, 1}, 0));
    envelope.trigger();
    (void)envelope.next();
    auto moved = std::move(envelope);
    require(!envelope.active() && envelope.next() == 0 && moved.active(), "MSEG move lifetime");
    cydsp::DyadicFilterBank<float> bank;
    ok(bank.prepare({8000, 8, 1}, 8));
    auto movedBank = std::move(bank);
    require(bank.analysisFrames(8) == 0 && movedBank.analysisFrames(8) == 1,
            "packet bank move lifetime");
}
