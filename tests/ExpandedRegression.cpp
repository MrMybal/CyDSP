#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <complex>
#include <cydsp/CyDSP.h>
#include <numbers>
#include <stdexcept>
#include <thread>
#include <vector>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool v, const char* message) {
    if (!v)
        throw std::runtime_error(message);
}
template <Sample T> void clear(AudioBlock<T> block) {
    for (std::size_t ch = 0; ch < block.channels(); ++ch)
        std::fill(block.channel(ch).begin(), block.channel(ch).end(), T{});
}
void ok(Status s) {
    require(s == Status::ok, "expanded status");
}
std::complex<double> response(std::span<const BiquadCoefficients> coeff, double hz) {
    const auto z = std::polar(1.0, -2 * std::numbers::pi * hz / 48000);
    std::complex<double> h = 1;
    for (const auto& c : coeff)
        h *= (c.b0 + c.b1 * z + c.b2 * z * z) / (1.0 + c.a1 * z + c.a2 * z * z);
    return h;
}
void designs() {
    for (unsigned order = 1; order <= 32; ++order)
        for (auto type : {BiquadType::lowPass, BiquadType::highPass}) {
            std::array<BiquadCoefficients, 16> sections;
            std::size_t n = 0;
            ok(designButterworth(type, order, 48000, 3000, sections, n));
            require(n == (order + 1) / 2, "Butterworth section count");
            require(std::abs(std::abs(response({sections.data(), n}, 3000)) - std::sqrt(.5)) <
                        1e-10,
                    "Butterworth cutoff response");
            require(std::abs(std::abs(response({sections.data(), n},
                                               type == BiquadType::lowPass ? 0 : 24000)) -
                             1) < 1e-10,
                    "Butterworth passband");
        }
    std::array<double, 129> taps;
    ok(designWindowedFir(taps, .15));
    double dc = 0;
    std::complex<double> stop{};
    for (std::size_t i = 0; i < taps.size(); ++i) {
        dc += taps[i];
        stop += taps[i] * std::polar(1.0, -2 * std::numbers::pi * .35 * static_cast<double>(i));
        require(taps[i] == taps[taps.size() - 1 - i], "FIR symmetry");
    }
    require(std::abs(dc - 1) < 1e-12 && std::abs(stop) < 1e-5, "FIR DC/stopband");
    const auto before = taps;
    require(designWindowedFir(taps, 0) == Status::invalid_argument && taps == before,
            "FIR invalid transactional");
    Adsr envelope;
    ok(envelope.prepare(1000));
    ok(envelope.setParameters({.004, .002, .5, .004}));
    envelope.noteOn();
    for (double expected : {.25, .5, .75, 1., .75, .5, .5})
        require(std::abs(envelope.next() - expected) < 1e-12, "ADSR endpoints");
    envelope.noteOff();
    for (double expected : {.375, .25, .125, 0.})
        require(std::abs(envelope.next() - expected) < 1e-12, "ADSR release");
    require(!envelope.active(), "ADSR inactive");
    // The block form must return the values of next() across stages and note commands.
    for (const auto& shape : {AdsrParameters{.004, .002, .5, .004}, AdsrParameters{0, 0, 1, 0},
                              AdsrParameters{.0105, .0033, .25, .0071}}) {
        Adsr single, block;
        ok(single.prepare(1000));
        ok(block.prepare(1000));
        ok(single.setParameters(shape));
        ok(block.setParameters(shape));
        single.noteOn();
        block.noteOn();
        std::array<double, 7> wide{};
        std::array<float, 5> narrow{};
        for (unsigned pass = 0; pass < 12; ++pass) {
            if (pass == 5) {
                single.noteOff();
                block.noteOff();
            }
            if (pass & 1) {
                block.process(std::span<float>(narrow));
                for (const float value : narrow)
                    require(value == static_cast<float>(single.next()), "ADSR float block");
            } else {
                block.process(std::span<double>(wide));
                for (const double value : wide)
                    require(value == single.next(), "ADSR double block");
            }
            block.process(std::span<double>{});
            require(block.active() == single.active() && block.value() == single.value(),
                    "ADSR block state");
        }
    }
    SmoothedValue ramp;
    ok(ramp.reset(1, SmoothingCurve::multiplicative));
    ok(ramp.setTarget(16, 4));
    for (double expected : {2., 4., 8., 16.})
        require(std::abs(ramp.next() - expected) < 1e-12, "multiplicative ramp");
    LookupTable<double> table;
    ok(table.prepare(-1, 1, 1001, [](double x, void*) noexcept { return x * x; }));
    require(std::abs(table.lookup(.42) - .1764) < 1e-6, "lookup numerical reference");
}
template <Sample T> void processors() {
    const ProcessSpec spec{48000, 4096, 2};
    const double tolerance = std::same_as<T, float> ? 2e-5 : 2e-11;
    AudioBuffer<T> in, out, other;
    ok(in.resize(2, 4096));
    ok(out.resize(2, 4096));
    ok(other.resize(2, 4096));
    Oscillator<T> osc;
    ok(osc.prepare(spec));
    ok(osc.setFrequency(1000));
    ok(osc.process(out.block()));
    for (std::size_t i = 0; i < 4096; ++i)
        require(std::abs(out.block().channel(0)[i] -
                         std::sin(2 * std::numbers::pi * 1000 * static_cast<double>(i) / 48000)) <
                    tolerance,
                "oscillator frequency/phase");
    for (std::size_t ch = 0; ch < 2; ++ch)
        std::fill(in.block().channel(ch).begin(), in.block().channel(ch).end(), T{1});
    BallisticsFilter<T> envelope;
    ok(envelope.prepare(spec));
    ok(envelope.setTimes(10, 100));
    ok(envelope.process(in.block(), out.block()));
    require(std::abs(out.block().channel(0)[479] - (1 - std::exp(-1.0))) < tolerance,
            "ballistics time constant");
    Dynamics<T> dynamics;
    ok(dynamics.prepare(spec));
    DynamicsParameters p;
    p.attackMs = 0;
    p.releaseMs = 0;
    p.kneeDb = 0;
    p.thresholdDb = -20;
    p.ratio = 4;
    ok(dynamics.setParameters(p));
    ok(dynamics.process(in.block(), out.block()));
    const auto expected = std::pow(10.0, -15.0 / 20);
    require(std::abs(out.block().channel(0)[0] - expected) < tolerance,
            "compressor transfer curve");
    Dynamics<T> limiter;
    ok(limiter.prepare(spec, DynamicsMode::limiter));
    ok(limiter.setParameters(p));
    ok(limiter.process(in.block(), out.block()));
    require(std::abs(out.block().channel(0)[0]) <= .100001, "limiter ceiling");
    TptFilter<T> tpt, split;
    ok(tpt.prepare(spec));
    ok(split.prepare(spec));
    ok(tpt.setParameters(3000, std::sqrt(.5), TptFilterMode::lowPass));
    ok(split.setParameters(3000, std::sqrt(.5), TptFilterMode::lowPass));
    clear(in.block());
    in.block().channel(0)[0] = 1;
    in.block().channel(1)[0] = 1;
    ok(tpt.process(in.block(), out.block()));
    for (std::size_t i = 0; i < 4096; i += 37) {
        auto n = std::min<std::size_t>(37, 4096 - i);
        ok(split.process(in.block().subBlock(i, n), other.block().subBlock(i, n)));
    }
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t i = 0; i < 4096; ++i)
            require(out.block().channel(ch)[i] == other.block().channel(ch)[i],
                    "TPT split invariance");
    BiquadCoefficients biquad;
    ok(designBiquad(BiquadType::lowPass, 48000, 3000, std::sqrt(.5), biquad));
    Biquad<T> direct;
    ok(direct.prepare(spec));
    ok(direct.setCoefficients(biquad));
    ok(direct.process(in.block(), other.block()));
    for (std::size_t i = 0; i < 4096; ++i)
        require(std::abs(out.block().channel(0)[i] - other.block().channel(0)[i]) < tolerance,
                "TPT equivalent bilinear response");
    LinkwitzRiley<T> crossover;
    ok(crossover.prepare(spec));
    ok(crossover.setFrequency(3000));
    ok(crossover.process(in.block(), out.block(), other.block()));
    std::complex<double> sum{};
    for (std::size_t i = 0; i < 4096; ++i)
        sum += (static_cast<double>(out.block().channel(0)[i]) + other.block().channel(0)[i]) *
               std::polar(1.0, -2 * std::numbers::pi * 3000 * static_cast<double>(i) / 48000);
    require(std::abs(std::abs(sum) - 1) < tolerance, "LR summed allpass response");
    if constexpr (std::same_as<T, float>) {
        std::array<T, 64> reference;
        std::copy_n(out.block().channel(0).begin(), reference.size(), reference.begin());
        require(crossover.setFrequency(1e-5) == Status::invalid_argument,
                "LR rounded stability validation");
        crossover.reset();
        ok(crossover.process(in.block(), out.block(), other.block()));
        require(std::equal(reference.begin(), reference.end(), out.block().channel(0).begin()),
                "LR failed update preserves coefficients");
    }
    require(crossover.process(in.block(), out.block(), out.block()) == Status::partial_overlap,
            "LR output alias rejection");
    LadderFilter<T> ladder;
    ok(ladder.prepare(spec));
    ok(ladder.setParameters(18000, 1, 8));
    ok(ladder.process(in.block(), out.block()));
    for (auto v : out.block().channel(0))
        require(isFinite(v), "ladder stability");
    Panner<T> pan;
    ok(pan.prepare({48000, 4096, 1}));
    const T* mono[]{in.block().channel(0).data()};
    ok(pan.setPan(0));
    for (auto invalid : {-1, 25})
        require(pan.setPan(-1, static_cast<PanLaw>(invalid)) == Status::invalid_argument,
                "unknown pan law accepted");
    ok(pan.process({mono, 1, 4096}, out.block()));
    require(std::abs(out.block().channel(0)[0] - std::sqrt(.5)) < tolerance, "pan centre power");
    DryWetMixer<T> mix;
    for (std::size_t latency : {0u, 32u}) {
        ok(mix.prepare(spec, latency));
        ok(mix.setWetLatency(latency));
        ok(mix.setMix(0));
        for (auto invalid : {-1, 25})
            require(mix.setMix(1, static_cast<MixLaw>(invalid)) == Status::invalid_argument,
                    "unknown mix law accepted");
        ok(mix.mixWetSamples(out.block(0)));
        ok(mix.pushDrySamples(in.block(0)));
        require(mix.mixWetSamples(out.block()) == Status::not_prepared,
                "empty dry push created a pending block");
        ok(mix.pushDrySamples(in.block()));
        ok(mix.pushDrySamples(in.block(0)));
        ok(mix.mixWetSamples(out.block(0)));
        require(mix.pushDrySamples({mono, 1, 0}) == Status::channel_mismatch,
                "empty dry push ignored a wrong channel count");
        require(mix.pushDrySamples(in.block()) == Status::transition_in_progress,
                "empty wet mix consumed the pending dry block");
        clear(out.block());
        ok(mix.mixWetSamples(out.block()));
        for (std::size_t i = 0; i < out.block().frames(); ++i)
            require(out.block().channel(0)[i] == (i == latency ? T{1} : T{}),
                    "dry latency alignment after empty block");
    }
    Oversampler<T> over;
    ok(over.prepare(spec, 4));
    AudioBlock<T> high;
    ok(over.processUp(in.block(), high));
    require(high.frames() == 16384, "oversampling shape");
    ok(over.processDown(out.block()));
    const auto peak = static_cast<std::size_t>(
        std::max_element(out.block().channel(0).begin(), out.block().channel(0).end()) -
        out.block().channel(0).begin());
    require(peak == over.latencyFrames(), "oversampling latency impulse");
    over.reset();
    clear(in.block());
    for (std::size_t ch = 0; ch < 2; ++ch)
        std::fill(in.block().channel(ch).begin(), in.block().channel(ch).end(), T{1});
    ok(over.processUp(in.block(), high));
    ok(over.processDown(out.block()));
    require(std::abs(out.block().channel(0)[1000] - 1) < 1e-4, "oversampling DC");
    WaveShaper<T> shaper;
    ok(shaper.prepare(spec));
    ok(shaper.setDrive(2));
    ok(shaper.process(in.block(), out.block()));
    require(std::abs(out.block().channel(0)[0] - std::tanh(2.0)) < tolerance,
            "waveshaper transfer");
    Bias<T> bias;
    ok(bias.prepare(spec));
    ok(bias.setBias(T{.25}));
    struct Audit {
        decltype(in)& input;
        decltype(out)& output;
        decltype(other)& second;
        decltype(tpt)& filter;
        decltype(ladder)& ladder;
        decltype(dynamics)& dynamics;
        decltype(envelope)& envelope;
        decltype(mix)& mixer;
        decltype(over)& over;
        decltype(shaper)& shape;
        decltype(osc)& osc;
        decltype(crossover)& crossover;
        decltype(bias)& bias;
        bool success = true;
    };
    Audit audit{in,  out,  other,  tpt, ladder,    dynamics, envelope,
                mix, over, shaper, osc, crossover, bias};
    runRealtimeAudit(
        [](void* ptr) noexcept {
            auto& a = *static_cast<Audit*>(ptr);
            auto capture = [&](Status s) { a.success = a.success && s == Status::ok; };
            capture(a.filter.process(a.input.block(), a.output.block()));
            capture(a.ladder.process(a.input.block(), a.output.block()));
            capture(a.dynamics.process(a.input.block(), a.output.block()));
            capture(a.envelope.process(a.input.block(), a.output.block()));
            capture(a.mixer.pushDrySamples(a.input.block()));
            capture(a.mixer.mixWetSamples(a.output.block()));
            AudioBlock<T> h;
            capture(a.over.processUp(a.input.block(), h));
            capture(a.over.processDown(a.output.block()));
            capture(a.shape.process(a.input.block(), a.output.block()));
            capture(a.osc.process(a.output.block()));
            capture(a.crossover.process(a.input.block(), a.output.block(), a.second.block()));
            capture(a.bias.process(a.output.block(), a.output.block()));
            a.filter.reset();
            a.ladder.reset();
            a.dynamics.reset();
            a.envelope.reset();
            a.mixer.reset();
            a.over.reset();
            a.osc.reset();
            a.crossover.reset();
            a.shape.reset();
            a.bias.reset();
        },
        &audit);
    require(audit.success, "expanded realtime statuses");
    auto moved = std::move(tpt);
    require(tpt.process(in.block(), out.block()) == Status::not_prepared, "moved TPT state");
    ok(moved.process(in.block(), out.block()));
}
template <Sample T> void effects() {
    const ProcessSpec spec{48000, 4096, 2};
    AudioBuffer<T> in, out, other;
    ok(in.resize(2, 4096));
    ok(out.resize(2, 4096));
    ok(other.resize(2, 4096));
    in.block().channel(0)[0] = 1;
    in.block().channel(1)[0] = 1;
    Chorus<T> chorus;
    ok(chorus.prepare(spec));
    ok(chorus.setParameters({0, 0, 10, 0, 1}));
    ok(chorus.process(in.block(), out.block()));
    for (std::size_t i = 0; i < 4096; ++i)
        require(out.block().channel(0)[i] == (i == 480 ? T{1} : T{}), "chorus exact static delay");
    Phaser<T> phaser;
    ok(phaser.prepare(spec));
    ok(phaser.setParameters({0, 0, 1000, 0, 1}));
    for (std::size_t ch = 0; ch < 2; ++ch)
        std::fill(in.block().channel(ch).begin(), in.block().channel(ch).end(), T{1});
    ok(phaser.process(in.block(), out.block()));
    require(std::abs(out.block().channel(0)[4095] - 1) < 1e-5, "phaser allpass DC");
    // Split blocks preserve LFO/feedback state exactly, including in-place operation.
    Chorus<T> split;
    ok(split.prepare(spec));
    ok(split.setParameters({2, .8, 15, .7, .6}));
    ok(chorus.setParameters({2, .8, 15, .7, .6}));
    chorus.reset();
    ok(chorus.process(in.block(), out.block()));
    for (std::size_t i = 0; i < 4096; i += 29) {
        const auto n = std::min<std::size_t>(29, 4096 - i);
        ok(split.process(in.block().subBlock(i, n), other.block().subBlock(i, n)));
    }
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t i = 0; i < 4096; ++i)
            require(other.block().channel(ch)[i] == out.block().channel(ch)[i],
                    "chorus split invariance");
    Reverb<T> reverb;
    ok(reverb.prepare(spec));
    ok(reverb.setParameters({.4, 0, 1, 1}));
    clear(in.block());
    in.block().channel(0)[0] = 1;
    double early = 0, late = 0;
    for (unsigned block = 0; block < 32; ++block) {
        ok(reverb.process(in.block(), out.block()));
        for (auto v : out.block().channel(0)) {
            require(isFinite(v), "reverb finite");
            if (block < 4)
                early += v * v;
            if (block > 24)
                late += v * v;
        }
        clear(in.block());
    }
    require(early > 0 && late < early * 1e-8, "reverb decaying tail");
    struct Audit {
        Chorus<T>& c;
        Phaser<T>& p;
        Reverb<T>& r;
        AudioBuffer<T>& a;
        AudioBuffer<T>& b;
        bool success = true;
    };
    Audit audit{chorus, phaser, reverb, in, out};
    runRealtimeAudit(
        [](void* p) noexcept {
            auto& a = *static_cast<Audit*>(p);
            a.success = a.c.process(a.a.block(), a.b.block()) == Status::ok &&
                        a.p.process(a.b.block(), a.b.block()) == Status::ok &&
                        a.r.process(a.b.block(), a.b.block()) == Status::ok;
            a.c.reset();
            a.p.reset();
            a.r.reset();
        },
        &audit);
    require(audit.success, "effects realtime statuses");
}
} // namespace
void expandedRegression() {
    designs();
    processors<float>();
    processors<double>();
    effects<float>();
    effects<double>();
}
