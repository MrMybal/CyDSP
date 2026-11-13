#include <algorithm>
#include <array>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <limits>
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
    require(status == Status::ok, "advanced expansion contract status");
}
template <class T> void tests() {
    const ProcessSpec spec{8000, 64, 1};
    const StftConfig config{128, 32, WindowType::hann};
    AudioBuffer<T> in, a, b, c, d;
    for (auto* p : {&in, &a, &b, &c, &d})
        ok(p->resize(1, 64));
    SpectralPitchShifter<T> whole, split;
    ok(whole.prepare(spec, config));
    ok(split.prepare(spec, config));
    SpectralPitchParameters parameters;
    parameters.ratio = 1.3;
    ok(whole.setParameters(parameters));
    ok(split.setParameters(parameters));
    HarmonicPercussive<T> h1, h2;
    ok(h1.prepare(spec, {config, 7, 7}));
    ok(h2.prepare(spec, {config, 7, 7}));
    EchoCanceller<T> e1, e2;
    ok(e1.prepare(spec, {16, 3, 10}));
    ok(e2.prepare(spec, {16, 3, 10}));
    for (std::size_t block = 0; block < 40; ++block) {
        for (std::size_t i = 0; i < 64; ++i)
            in.block().channel(0)[i] = T(.2 * std::sin(.13 * double(block * 64 + i)));
        ok(whole.process(in.block(), a.block()));
        ok(split.process(in.block().subBlock(0, 11), b.block().subBlock(0, 11)));
        ok(split.process(in.block().subBlock(11, 53), b.block().subBlock(11, 53)));
        for (std::size_t i = 0; i < 64; ++i)
            require(a.block().channel(0)[i] == b.block().channel(0)[i],
                    "spectral pitch block invariance");
        ok(h1.process(in.block(), a.block(), c.block()));
        ok(h2.process(in.block().subBlock(0, 17), b.block().subBlock(0, 17),
                      d.block().subBlock(0, 17)));
        ok(h2.process(in.block().subBlock(17, 47), b.block().subBlock(17, 47),
                      d.block().subBlock(17, 47)));
        for (std::size_t i = 0; i < 64; ++i)
            require(a.block().channel(0)[i] == b.block().channel(0)[i] &&
                        c.block().channel(0)[i] == d.block().channel(0)[i],
                    "HPSS block invariance");
        ok(e1.process(in.block(), in.block(), a.block()));
        ok(e2.process(in.block().subBlock(0, 23), in.block().subBlock(0, 23),
                      b.block().subBlock(0, 23)));
        ok(e2.process(in.block().subBlock(23, 41), in.block().subBlock(23, 41),
                      b.block().subBlock(23, 41)));
        for (std::size_t i = 0; i < 64; ++i)
            require(a.block().channel(0)[i] == b.block().channel(0)[i], "AEC block invariance");
    }
    const auto previous = std::vector<double>(e1.coefficients(0).begin(), e1.coefficients(0).end());
    in.block().channel(0)[63] = std::numeric_limits<T>::infinity();
    for (auto* p : {&a, &c})
        std::fill(p->block().channel(0).begin(), p->block().channel(0).end(), T{9});
    require(h1.process(in.block(), a.block(), c.block()) == Status::invalid_argument &&
                a.block().channel(0)[0] == 9 && c.block().channel(0)[0] == 9,
            "HPSS preflight both outputs");
    require(e1.process(in.block(), in.block(), a.block()) == Status::invalid_argument &&
                a.block().channel(0)[0] == 9,
            "AEC invalid preflight");
    require(std::equal(previous.begin(), previous.end(), e1.coefficients(0).begin()),
            "AEC rejected input preserves coefficients");
    in.block().channel(0)[63] = 0;
    require(whole.prepare({0, 64, 1}, config) == Status::invalid_spec &&
                whole.latencyFrames() == 128,
            "spectral prepare transactional");
    require(h1.prepare(spec, {config, 8, 7}) == Status::invalid_spec && h1.latencyFrames() == 224,
            "HPSS prepare transactional");
    require(e1.prepare(spec, {0, 0, 10}) == Status::invalid_spec && e1.coefficients(0).size() == 16,
            "AEC prepare transactional");
    auto moved = std::move(whole);
    require(whole.process(in.block(), a.block()) == Status::not_prepared, "spectral moved from");
    ok(moved.process(in.block(), a.block()));
    auto movedH = std::move(h1);
    require(h1.process(in.block(), a.block(), c.block()) == Status::not_prepared,
            "HPSS moved from");
    auto movedE = std::move(e1);
    require(e1.process(in.block(), in.block(), a.block()) == Status::not_prepared,
            "AEC moved from");
    ok(movedH.process(in.block().subBlock(0, 0), a.block().subBlock(0, 0),
                      c.block().subBlock(0, 0)));
    const auto count = movedE.metrics().adaptedFrames;
    ok(movedE.process(in.block().subBlock(0, 0), in.block().subBlock(0, 0),
                      a.block().subBlock(0, 0)));
    require(movedE.metrics().adaptedFrames == count, "AEC zero block state");
    DiffractionFilter<T> diffraction;
    ok(diffraction.prepare(spec));
    ok(diffraction.setPath({1, 2, .3}, 32));
    struct A {
        DiffractionFilter<T>* p;
        AudioBlock<const T> in;
        AudioBlock<T> out;
        Status st;
    } audit{&diffraction, in.block(), a.block(), Status::ok};
    runRealtimeAudit(
        [](void* x) noexcept {
            auto& c = *static_cast<A*>(x);
            c.st = c.p->setPath({1, 2, .2}, 64);
            if (c.st == Status::ok)
                c.st = c.p->process(c.in, c.out);
            c.p->reset();
        },
        &audit);
    ok(audit.st);
    ContactExciter<T> contact;
    ok(contact.prepare(spec));
    for (unsigned i = 0; i < 8; ++i)
        ok(contact.strike(.001));
    require(contact.strike(.001) == Status::update_queue_full, "bounded physical contacts");
    std::array<T, 4> ir{1, T{.2}, 0, 0};
    const std::array<HrirPoint<T>, 1> points{{{{1, 0, 0}, ir, ir}}};
    HrtfSet<T> data;
    HrtfPreparation preparation;
    preparation.sourceSampleRate = preparation.targetSampleRate = 8000;
    ok(data.prepare(points, preparation));
    auto invalid = preparation;
    invalid.targetSampleRate = 0;
    require(data.prepare(points, invalid) == Status::invalid_spec && data.sampleRate() == 8000,
            "HRTF immutable prepare transactional");
    HrirBinaural<T> binaural;
    ok(binaural.prepare(spec, data));
    std::vector<T> oversized(2049);
    const std::array<HrirPoint<T>, 1> tooLong{{{{1, 0, 0}, oversized, oversized}}};
    require(binaural.prepare(spec, tooLong) == Status::invalid_argument,
            "legacy HRIR rejects silent truncation");
    require(binaural.prepare({16000, 64, 1}, data) == Status::invalid_spec, "HRTF rate mismatch");
    HoaBinaural<T> hoa;
    ok(hoa.prepare(spec, 0, data));
    AudioBuffer<T> stereo;
    ok(stereo.resize(2, 64));
    struct H {
        HoaBinaural<T>* p;
        AudioBlock<const T> in;
        AudioBlock<T> out;
        Status st;
    } ha{&hoa, in.block(), stereo.block(), Status::ok};
    runRealtimeAudit(
        [](void* x) noexcept {
            auto& h = *static_cast<H*>(x);
            h.st = h.p->setRotation({1, 0, 0, 0});
            if (h.st == Status::ok)
                h.st = h.p->process(h.in, h.out);
            h.p->reset();
        },
        &ha);
    ok(ha.st);
    ImpulseReport report;
    report.energy = 42;
    report.reflectionCount = 1;
    report.reflections[0].seconds = std::numeric_limits<double>::quiet_NaN();
    FdnParameters fdn;
    fdn.wet = .123;
    require(matchFdnToImpulse(report, fdn) == Status::invalid_argument && fdn.wet == .123,
            "IR matching rejects invalid reflection time");

    std::array<T, 2> bad{0, std::numeric_limits<T>::quiet_NaN()};
    require(analyzeImpulse(bad, 8000, report) == Status::invalid_argument && report.energy == 42,
            "IR output transactional");
}
} // namespace
void advancedExpansionContracts() {
    tests<float>();
    tests<double>();
}
