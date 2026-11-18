#include <array>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <numbers>
#include <stdexcept>
#include <vector>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool v, const char* m) {
    if (!v)
        throw std::runtime_error(m);
}
void ok(Status s) {
    require(s == Status::ok, "creative DSP status");
}
void controls() {
    MultiSegmentEnvelope e;
    ok(e.prepare(8000));
    const std::array<EnvelopeSegment, 3> segments{{{.001, 1, 0}, {.001, .5, 0}, {.001, 0, 0}}};
    ok(e.setSegments(segments, 2, 1, 2));
    e.trigger();
    for (int i = 0; i < 8; ++i)
        (void)e.next();
    require(e.next() < 1, "MSEG transition");
    for (int i = 0; i < 40; ++i)
        (void)e.next();
    require(e.active(), "MSEG sustain loop");
    e.release();
    for (int i = 0; i < 8; ++i)
        (void)e.next();
    require(!e.active() && e.next() == 0, "MSEG continuous release");
    ModulationLfo a, b;
    ok(a.prepare(8000));
    ok(b.prepare(8000));
    ok(a.setParameters(100, LfoShape::sampleAndHold));
    ok(b.setParameters(100, LfoShape::sampleAndHold));
    a.reset(123);
    b.reset(123);
    const double first = a.next();
    require(first == b.next(), "S&H deterministic seed");
    for (int i = 0; i < 79; ++i)
        require(a.next() == b.next(), "S&H seeded sequence");
    require(a.next() != first, "S&H step at period");
}
template <class T> void oscillatorRejectedOutput() {
    const ProcessSpec spec{48000, 64, 2};
    ModulatedOscillator<T> oscillator, reference;
    ok(oscillator.prepare(spec));
    ok(reference.prepare(spec));
    std::array<T, 128> actual{}, expected{};
    T* channels[]{actual.data(), actual.data() + 64};
    T* referenceChannels[]{expected.data(), expected.data() + 64};
    T* overlapping[]{actual.data(), actual.data() + 1};
    ok(oscillator.process({channels, 2, 64}));
    ok(reference.process({referenceChannels, 2, 64}));
    require(oscillator.process({overlapping, 2, 64}) == Status::partial_overlap &&
                actual == expected,
            "modulated oscillator rejects overlap before writing");
    ok(oscillator.process({channels, 2, 64}));
    ok(reference.process({referenceChannels, 2, 64}));
    require(actual == expected, "modulated oscillator preserves phase and filter on error");
}
template <class T> void test() {
    const ProcessSpec spec{8000, 64, 1};
    AudioBuffer<T> in, out, side;
    ok(in.resize(1, 64));
    ok(out.resize(1, 64));
    ok(side.resize(1, 64));
    in.block().channel(0)[0] = 1;
    DelayEffect<T> delay;
    ok(delay.prepare(spec, 500));
    DelayEffectParameters dp;
    dp.delayMs = .5;
    dp.feedback = 0;
    dp.lowPassHz = 3000;
    dp.wet = 1;
    dp.dry = 0;
    ok(delay.setParameters(dp));
    ok(delay.process(in.block(), out.block()));
    for (std::size_t i = 0; i < 64; ++i)
        require(std::abs(double(out.block().channel(0)[i]) - (i == 4 ? 1. : 0.)) < 1e-6,
                "echo exact delay");
    SpectralEffects<T> spectral;
    ok(spectral.prepare(spec, {128, 32, WindowType::hann}));
    for (unsigned block = 0; block < 6; ++block) {
        std::fill(in.block().channel(0).begin(), in.block().channel(0).end(), T{});
        if (block == 0)
            in.block().channel(0)[17] = 1;
        ok(spectral.process(in.block(), out.block()));
        for (std::size_t i = 0; i < 64; ++i)
            require(std::abs(double(out.block().channel(0)[i]) -
                             (block * 64 + i == 145 ? 1. : 0.)) < 2e-5,
                    "spectral delayed reconstruction");
    }
    spectral.reset();
    ok(spectral.setParameters({SpectralEffectMode::vocoder, 1, 4}));
    for (std::size_t i = 0; i < 64; ++i)
        in.block().channel(0)[i] = T(std::sin(.2 * i));
    for (int b = 0; b < 6; ++b) {
        ok(spectral.process(in.block(), side.block(), out.block()));
        for (T x : out.block().channel(0))
            require(std::abs(double(x)) < 1e-9, "silent modulator vocoder");
    }
    ok(spectral.setParameters({}));
    for (int b = 0; b < 8; ++b)
        ok(spectral.process(in.block(), out.block()));
    ok(spectral.setParameters({SpectralEffectMode::freeze, 1, 4}));
    ok(spectral.process(in.block(), out.block()));
    std::fill(in.block().channel(0).begin(), in.block().channel(0).end(), T{});
    double energy = 0;
    for (int b = 0; b < 16; ++b) {
        ok(spectral.process(in.block(), out.block()));
        if (b > 8)
            for (T x : out.block().channel(0))
                energy += double(x) * x;
    }
    require(energy > 1, "spectral frozen tail");
    FrequencyShifter<T> shift;
    ok(shift.prepare(spec, 129));
    ok(shift.setFrequency(100));
    double crossings = 0, firstCross = 0, lastCross = 0;
    T previous = 0;
    for (unsigned b = 0; b < 100; ++b) {
        for (std::size_t i = 0; i < 64; ++i)
            in.block().channel(0)[i] =
                T(std::cos(2 * std::numbers::pi * 1000 * double(b * 64 + i) / 8000));
        ok(shift.process(in.block(), out.block()));
        for (std::size_t i = 0; i < 64; ++i) {
            const T x = out.block().channel(0)[i];
            if (b > 10 && x >= 0 && previous < 0) {
                const double time = double(b * 64 + i) - double(x) / (double(x) - previous);
                if (crossings == 0)
                    firstCross = time;
                lastCross = time;
                ++crossings;
            }
            previous = x;
        }
    }
    require(std::abs((crossings - 1) * 8000 / (lastCross - firstCross) - 1100) < .3,
            "Hilbert single-sideband shift");
    PlateReverb<T> plate;
    SpringReverb<T> spring;
    ok(plate.prepare(spec));
    ok(spring.prepare(spec));
    PlateParameters pp;
    pp.dampingHz = 3000;
    pp.dry = 0;
    pp.wet = 1;
    pp.shimmer = .3;
    ok(plate.setParameters(pp));
    SpringParameters sp;
    sp.dampingHz = 3000;
    sp.dry = 0;
    sp.wet = 1;
    ok(spring.setParameters(sp));
    std::fill(in.block().channel(0).begin(), in.block().channel(0).end(), T{});
    in.block().channel(0)[0] = 1;
    double pe = 0, se = 0;
    for (unsigned b = 0; b < 100; ++b) {
        ok(plate.process(in.block(), out.block()));
        for (T x : out.block().channel(0)) {
            require(isFinite(x) && std::abs(double(x)) < 10, "plate stability");
            pe += double(x) * x;
        }
        ok(spring.process(in.block(), out.block()));
        for (T x : out.block().channel(0)) {
            require(isFinite(x) && std::abs(double(x)) < 10, "spring stability");
            se += double(x) * x;
        }
        in.block().channel(0)[0] = 0;
    }
    require(pe > .001 && se > .001, "reverb tails absent");
    LiveGranulator<T> live;
    ok(live.prepare(spec, 3, 1, 8));
    AudioBuffer<T> grains;
    ok(grains.resize(3, 64));
    for (auto& x : in.block().channel(0))
        x = 1;
    for (unsigned b = 0; b < 32; ++b)
        ok(live.process(in.block(), grains.block()));
    live.setFrozen(true);
    LiveGrainParameters gp;
    gp.durationMs = 16;
    gp.ageSeconds = .1;
    gp.channelGains = {1, .5, -.25, 0, 0, 0, 0, 0};
    ok(live.trigger(gp));
    double ge = 0;
    for (int b = 0; b < 2; ++b) {
        ok(live.process(in.block(), grains.block()));
        for (std::size_t i = 0; i < 64; ++i) {
            const double x = grains.block().channel(0)[i];
            ge += x * x;
            require(std::abs(double(grains.block().channel(1)[i]) - .5 * x) < 1e-7 &&
                        std::abs(double(grains.block().channel(2)[i]) + .25 * x) < 1e-7,
                    "live grain spatial gains");
        }
    }
    require(ge > 20 && live.activeGrains() == 0, "live grain capture freeze lifetime");
    gp.pitchRatio = -1;
    ok(live.trigger(gp));
    ok(live.process(in.block(), grains.block()));
    ModulatedOscillator<T> osc;
    ok(osc.prepare(spec));
    ModulatedOscillatorParameters op;
    op.frequencyHz = 400;
    op.unison = 3;
    op.syncHz = 60;
    op.waveform = Waveform::saw;
    ok(osc.setParameters(op));
    ok(osc.process(out.block()));
    for (T x : out.block().channel(0))
        require(isFinite(x), "FM sync synthesis");
    ProceduralSource<T> source;
    ok(source.prepare(spec));
    ok(source.setParameters({ProceduralKind::impact, 100, 1, .7, .1}));
    source.trigger();
    ok(source.process(out.block()));
    double pro = 0;
    for (T x : out.block().channel(0))
        pro += double(x) * x;
    require(pro > 0, "procedural impact excitation");
    struct Audit {
        DelayEffect<T>* d;
        SpectralEffects<T>* s;
        FrequencyShifter<T>* f;
        PlateReverb<T>* p;
        SpringReverb<T>* r;
        LiveGranulator<T>* g;
        ModulatedOscillator<T>* o;
        AudioBlock<const T> in;
        AudioBlock<T> out, grains;
        bool pass = true;
    } a{&delay, &spectral, &shift,     &plate,      &spring,
        &live,  &osc,      in.block(), out.block(), grains.block()};
    runRealtimeAudit(
        [](void* raw) noexcept {
            auto& c = *static_cast<Audit*>(raw);
            c.pass = c.d->process(c.in, c.out) == Status::ok &&
                     c.s->process(c.in, c.out) == Status::ok &&
                     c.f->process(c.in, c.out) == Status::ok &&
                     c.p->process(c.in, c.out) == Status::ok &&
                     c.r->process(c.in, c.out) == Status::ok &&
                     c.g->process(c.in, c.grains) == Status::ok &&
                     c.o->process(c.out) == Status::ok;
            c.d->reset();
            c.s->reset();
            c.f->reset();
            c.p->reset();
            c.r->reset();
            c.g->reset();
            c.o->reset();
        },
        &a);
    require(a.pass, "creative DSP realtime audit");
}
} // namespace
void creativeRegression() {
    controls();
    oscillatorRejectedOutput<float>();
    oscillatorRejectedOutput<double>();
    test<float>();
    test<double>();
}
