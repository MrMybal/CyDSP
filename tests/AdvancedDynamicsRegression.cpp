#include <array>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <numbers>
#include <stdexcept>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool v, const char* m) {
    if (!v)
        throw std::runtime_error(m);
}
void ok(Status s) {
    require(s == Status::ok, "advanced dynamics status");
}
template <class T> void test() {
    const ProcessSpec spec{48000, 256, 1};
    AudioBuffer<T> in, out, side;
    ok(in.resize(1, 256));
    ok(out.resize(1, 256));
    ok(side.resize(1, 256));
    Dynamics<T> d;
    ok(d.prepare(spec, DynamicsMode::compressor, 0));
    DynamicsParameters p;
    p.thresholdDb = -20;
    p.ratio = 4;
    p.kneeDb = 0;
    p.attackMs = p.releaseMs = 0;
    ok(d.setParameters(p));
    for (std::size_t i = 0; i < 256; ++i) {
        in.block().channel(0)[i] = T(.1);
        side.block().channel(0)[i] = T(1);
    }
    ok(d.process(in.block(), side.block(), out.block()));
    require(std::abs(double(out.block().channel(0)[200]) - .1 * std::pow(10., -15. / 20)) < 1e-7,
            "external sidechain gain curve");
    AdaaSaturator<T> s;
    ok(s.prepare(spec));
    SaturationParameters sat;
    sat.shape = AdaaShape::hardClip;
    ok(s.setParameters(sat));
    in.block().channel(0)[0] = T(-2);
    in.block().channel(0)[1] = T(2);
    ok(s.process(in.block(2), out.block(2)));
    require(std::abs(double(out.block().channel(0)[1])) < 1e-8, "ADAA clipped ramp integral");
    sat.shape = AdaaShape::wavefolder;
    sat.antialias = false;
    ok(s.setParameters(sat));
    in.block().channel(0)[0] = T(1.5);
    in.block().channel(0)[1] = T(3);
    ok(s.process(in.block(2), out.block(2)));
    require(out.block().channel(0)[0] == T(.5) && out.block().channel(0)[1] == T(-1),
            "wavefolder transfer");
    for (auto mode : {AdaaShape::tanh, AdaaShape::atan, AdaaShape::cubic, AdaaShape::hardClip,
                      AdaaShape::wavefolder, AdaaShape::hysteresis}) {
        sat.shape = mode;
        sat.antialias = true;
        ok(s.setParameters(sat));
        s.reset();
        for (std::size_t i = 0; i < 256; ++i)
            in.block().channel(0)[i] = T(2 * std::sin(.47 * double(i)));
        ok(s.process(in.block(), out.block()));
        for (T x : out.block().channel(0))
            require(isFinite(x) && std::abs(double(x)) < 2, "nonlinearity finite bound");
    }
    DynamicEqualizer<T> eq;
    ok(eq.prepare(spec, 2));
    DynamicEqBand band{3000, 2, -60, -24, 0, 0, 0};
    ok(eq.setBands({&band, 1}));
    double ein = 0, eout = 0;
    for (unsigned block = 0; block < 20; ++block) {
        for (std::size_t i = 0; i < 256; ++i)
            in.block().channel(0)[i] =
                T(.5 * std::sin(2 * std::numbers::pi * 3000 * double(block * 256 + i) / 48000));
        ok(eq.process(in.block(), out.block()));
        if (block > 15)
            for (std::size_t i = 0; i < 256; ++i) {
                ein += double(in.block().channel(0)[i]) * in.block().channel(0)[i];
                eout += double(out.block().channel(0)[i]) * out.block().channel(0)[i];
            }
    }
    require(eout < ein * .01, "dynamic EQ center attenuation");
    DeEsser<T> de;
    ok(de.prepare(spec));
    ok(de.setParameters());
    ok(de.process(in.block(), out.block()));
    TransientShaper<T> transient;
    ok(transient.prepare(spec));
    ok(transient.process(in.block(), out.block()));
    for (std::size_t i = 0; i < 256; ++i)
        require(in.block().channel(0)[i] == out.block().channel(0)[i],
                "neutral transient identity");
    MultibandDynamics<T> multi;
    const std::array<double, 2> cross{1000, 5000};
    ok(multi.prepare(spec, cross, 65));
    for (std::size_t i = 0; i < 256; ++i)
        in.block().channel(0)[i] = T{};
    in.block().channel(0)[0] = 1;
    ok(multi.process(in.block(), out.block()));
    for (std::size_t i = 0; i < 256; ++i)
        require(std::abs(double(out.block().channel(0)[i]) - (i == 32 ? 1. : 0.)) < 1e-6,
                "multiband unity reconstruction");
    TruePeakLimiter<T> limiter;
    ok(limiter.prepare(spec, 4, 2));
    ok(limiter.setParameters(-1, 100, 1));
    TruePeakMeter<T> meter;
    ok(meter.prepare(spec, 8));
    for (unsigned b = 0; b < 30; ++b) {
        for (std::size_t i = 0; i < 256; ++i)
            in.block().channel(0)[i] =
                T(2 * std::sin(2 * std::numbers::pi * 997 * double(b * 256 + i) / 48000));
        ok(limiter.process(in.block(), out.block()));
        ok(meter.process(out.block()));
    }
    meter.flush();
    require(meter.truePeakDbtp(0) < -0.8, "oversampled limiter reconstructed sine ceiling");
    struct Audit {
        Dynamics<T>* d;
        AdaaSaturator<T>* s;
        DynamicEqualizer<T>* eq;
        TransientShaper<T>* t;
        MultibandDynamics<T>* m;
        TruePeakLimiter<T>* l;
        AudioBlock<const T> in, side;
        AudioBlock<T> out;
        bool pass = true;
    } a{&d, &s, &eq, &transient, &multi, &limiter, in.block(), side.block(), out.block()};
    runRealtimeAudit(
        [](void* p) noexcept {
            auto& c = *static_cast<Audit*>(p);
            c.pass = c.d->process(c.in, c.side, c.out) == Status::ok &&
                     c.s->process(c.in, c.out) == Status::ok &&
                     c.eq->process(c.in, c.out) == Status::ok &&
                     c.t->process(c.in, c.out) == Status::ok &&
                     c.m->process(c.in, c.out) == Status::ok &&
                     c.l->process(c.in, c.out) == Status::ok;
            c.d->reset();
            c.s->reset();
            c.eq->reset();
            c.t->reset();
            c.m->reset();
            c.l->reset();
        },
        &a);
    require(a.pass, "advanced dynamics realtime");
}
} // namespace
void advancedDynamicsRegression() {
    test<float>();
    test<double>();
}
