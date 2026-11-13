#include <algorithm>
#include <array>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <vector>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool v, const char* text) {
    if (!v)
        throw std::runtime_error(text);
}
void ok(Status s) {
    require(s == Status::ok, "granulation status");
}
template <class T> void exact() {
    std::vector<T> ramp(16);
    for (std::size_t i = 0; i < ramp.size(); ++i)
        ramp[i] = T(i);
    const T* input[]{ramp.data()};
    SampleData<T> sample;
    ok(sample.prepare({input, 1, ramp.size()}, 8000));
    Granulator<T> a, b;
    for (auto* p : {&a, &b})
        ok(p->prepare({8000, 32, 1}, sample, 2));
    GrainParameters grain;
    grain.positionSeconds = 4. / 8000;
    grain.durationMs = .5;
    grain.gain = .5;
    grain.envelope = GrainEnvelope::rectangular;
    for (auto* p : {&a, &b})
        ok(p->trigger(grain, 3));
    AudioBuffer<T> whole, split;
    ok(whole.resize(1, 32));
    ok(split.resize(1, 32));
    ok(a.process(whole.block()));
    std::size_t at = 0;
    for (std::size_t count : {2u, 1u, 4u, 0u, 25u}) {
        ok(b.process(split.block().subBlock(at, count)));
        at += count;
    }
    for (std::size_t i = 0; i < 32; ++i) {
        const T expected = i >= 3 && i < 7 ? T(double(i + 1) * .5) : T{};
        require(whole.block().channel(0)[i] == expected && split.block().channel(0)[i] == expected,
                "grain sample offset/split oracle");
    }
    require(!a.activeGrains() && !b.activeGrains(), "grain did not finish");
    grain.positionSeconds = 8. / 8000;
    grain.pitchRatio = -1;
    grain.gain = 1;
    ok(a.trigger(grain));
    ok(a.process(whole.block()));
    for (std::size_t i = 0; i < 4; ++i)
        require(whole.block().channel(0)[i] == T(8 - i), "reverse grain oracle");
    ok(sample.setLoop(4, 12));
    a.reset();
    grain.positionSeconds = 11. / 8000;
    grain.pitchRatio = 1;
    ok(a.trigger(grain));
    ok(a.process(whole.block()));
    const std::array<T, 4> wrapped{T(11), T(4), T(5), T(6)};
    for (std::size_t i = 0; i < 4; ++i)
        require(whole.block().channel(0)[i] == wrapped[i], "grain loop oracle");
    a.reset();
    grain.wrap = false;
    grain.positionSeconds = 14. / 8000;
    ok(a.trigger(grain));
    ok(a.process(whole.block()));
    require(whole.block().channel(0)[0] == T(14) && whole.block().channel(0)[1] == T(15) &&
                whole.block().channel(0)[2] == 0,
            "grain source end");
    a.reset();
    grain.wrap = true;
    grain.positionSeconds = 0;
    ok(a.trigger(grain));
    ok(a.trigger(grain));
    require(a.trigger(grain) == Status::update_queue_full && a.activeGrains() == 2,
            "bounded grain pool");
    require(a.process({nullptr, 1, 1}) == Status::invalid_argument && a.activeGrains() == 2,
            "invalid output changed grains");
    require(a.prepare({0, 32, 1}, sample) == Status::invalid_spec && a.activeGrains() == 2,
            "failed preparation destroyed grains");
    auto moved = std::move(a);
    require(a.process(whole.block()) == Status::not_prepared, "moved granulator prepared");
    ok(moved.process(whole.block()));
}
template <class T> void envelopeAndPan() {
    std::vector<T> ones(64, T(1));
    const T* pointers[]{ones.data()};
    SampleData<T> sample;
    ok(sample.prepare({pointers, 1, 64}, 8000));
    Granulator<T> p;
    ok(p.prepare({8000, 64, 2}, sample, 4));
    AudioBuffer<T> out;
    ok(out.resize(2, 64));
    GrainParameters g;
    g.durationMs = 33. * 1000 / 8000;
    ok(p.trigger(g));
    ok(p.process(out.block()));
    for (std::size_t i = 0; i < 33; ++i) {
        const auto expected =
            .5 * (1 - std::cos(2 * std::numbers::pi * double(i) / 32)) * std::sqrt(.5);
        for (std::size_t ch = 0; ch < 2; ++ch)
            require(std::abs(double(out.block().channel(ch)[i]) - expected) < 2e-7,
                    "Hann grain/pan analytical oracle");
    }
    p.reset();
    g.envelope = GrainEnvelope::rectangular;
    g.pan = -1;
    ok(p.trigger(g));
    for (std::size_t ch = 0; ch < 2; ++ch)
        std::fill(out.block().channel(ch).begin(), out.block().channel(ch).end(), T(.25));
    ok(p.process(out.block(), true));
    require(out.block().channel(0)[5] == T(1.25) && out.block().channel(1)[5] == T(.25),
            "grain additive hard pan");
    // In-place writes into the borrowed sample would corrupt future grain reads.
    p.reset();
    ok(p.prepare({8000, 64, 1}, sample));
    ok(p.trigger(g));
    T* alias[]{const_cast<T*>(sample.samples().channel(0).data())};
    require(p.process({alias, 1, 64}) == Status::partial_overlap && p.activeGrains() == 1,
            "source/output alias accepted");
}
template <class T> void cloud() {
    std::vector<T> cycle(4096);
    for (std::size_t i = 0; i < cycle.size(); ++i)
        cycle[i] = T(.4 * std::sin(.073 * double(i)));
    const T* pointers[]{cycle.data()};
    SampleData<T> sample;
    ok(sample.prepare({pointers, 1, cycle.size()}, 8000));
    Granulator<T> a, b;
    GrainCloudParameters cloud;
    cloud.grainsPerSecond = 137.1;
    cloud.grain.durationMs = 40;
    cloud.grain.positionSeconds = .1;
    cloud.positionSpreadSeconds = .05;
    cloud.pitchSpreadSemitones = 7;
    cloud.panSpread = 1;
    for (auto* p : {&a, &b}) {
        ok(p->prepare({8000, 256, 2}, sample, 64));
        ok(p->setCloud(cloud));
        p->reset(9381);
    }
    AudioBuffer<T> whole, split;
    ok(whole.resize(2, 4096));
    ok(split.resize(2, 4096));
    for (std::size_t at = 0; at < 4096; at += 256)
        ok(a.process(whole.block().subBlock(at, 256)));
    std::size_t at = 0, block = 0;
    const std::array<std::size_t, 5> sizes{1, 7, 31, 128, 3};
    while (at < 4096) {
        const auto n = std::min(sizes[block++ % sizes.size()], 4096 - at);
        ok(b.process(split.block().subBlock(at, n)));
        at += n;
    }
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t i = 0; i < 4096; ++i)
            require(std::abs(double(whole.block().channel(ch)[i]) - split.block().channel(ch)[i]) <
                        (std::same_as<T, float> ? 2e-7 : 2e-14),
                    "seeded cloud changed with block splitting");
    require(!a.droppedGrains() && !b.droppedGrains(), "unexpected cloud saturation");
    a.reset(9381);
    AudioBuffer<T> repeat;
    ok(repeat.resize(2, 256));
    ok(a.process(repeat.block()));
    for (std::size_t ch = 0; ch < 2; ++ch)
        require(std::equal(repeat.block().channel(ch).begin(), repeat.block().channel(ch).end(),
                           whole.block().channel(ch).begin()),
                "cloud seed reset");
    cloud.grainsPerSecond = 8000;
    cloud.grain.durationMs = 40;
    ok(b.prepare({8000, 256, 2}, sample, 1));
    ok(b.setCloud(cloud));
    ok(b.process(repeat.block(32)));
    require(b.droppedGrains() == 31 && b.activeGrains() == 1, "cloud overflow policy");
    // A density change starts a new schedule immediately, even after a long gap.
    a.reset();
    cloud.grainsPerSecond = .01;
    ok(a.setCloud(cloud));
    ok(a.process(repeat.block(1)));
    require(a.activeGrains() == 1, "cloud first event");
    cloud.grainsPerSecond = 8000;
    ok(a.setCloud(cloud));
    ok(a.process(repeat.block(1)));
    require(a.activeGrains() == 2, "cloud density change kept obsolete countdown");
    cloud.grainsPerSecond = std::numeric_limits<double>::denorm_min();
    require(a.setCloud(cloud) == Status::invalid_argument, "unrepresentable cloud interval");
    struct Context {
        Granulator<T>* p;
        AudioBlock<T> out;
        GrainParameters grain;
        bool passed = true;
    } context{&a, repeat.block(), cloud.grain};
    runRealtimeAudit(
        [](void* raw) noexcept {
            auto& c = *static_cast<Context*>(raw);
            c.p->reset();
            c.passed &= c.p->trigger(c.grain) == Status::ok;
            c.passed &= c.p->process(c.out) == Status::ok;
        },
        &context);
    require(context.passed, "granulation realtime");
}
template <class T> void aliasReduction() {
    AudioBuffer<T> source, a, b;
    ok(source.resize(1, 8192));
    ok(a.resize(1, 1024));
    ok(b.resize(1, 1024));
    for (std::size_t i = 0; i < 8192; ++i)
        source.block().channel(0)[i] = T(std::sin(2 * std::numbers::pi * .35 * double(i)));
    SampleData<T> sample;
    ok(sample.prepare(source.block(), 48000));
    Granulator<T> linear, limited;
    GrainParameters g;
    g.positionSeconds = 512. / 48000;
    g.durationMs = 1024. * 1000 / 48000;
    g.pitchRatio = 2;
    g.envelope = GrainEnvelope::rectangular;
    ok(linear.prepare({48000, 1024, 1}, sample));
    ok(limited.prepare({48000, 1024, 1}, sample));
    g.interpolation = SampleInterpolation::linear;
    ok(linear.trigger(g));
    g.interpolation = SampleInterpolation::bandLimited;
    ok(limited.trigger(g));
    ok(linear.process(a.block()));
    ok(limited.process(b.block()));
    double raw = 0, filtered = 0;
    for (std::size_t i = 0; i < 1024; ++i) {
        raw += double(a.block().channel(0)[i]) * a.block().channel(0)[i];
        filtered += double(b.block().channel(0)[i]) * b.block().channel(0)[i];
    }
    require(raw > 400 && filtered < raw * 1e-5, "grain antialias reader not integrated");
    std::cout << "QUALITY grain alias reduction dB=" << 10 * std::log10(raw / filtered) << '\n';
}
} // namespace
void granulationRegression() {
    exact<float>();
    exact<double>();
    envelopeAndPan<float>();
    envelopeAndPan<double>();
    cloud<float>();
    cloud<double>();
    aliasReduction<float>();
    aliasReduction<double>();
}
