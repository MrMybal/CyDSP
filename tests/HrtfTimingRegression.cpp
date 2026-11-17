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
void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}
void ok(Status status) {
    require(status == Status::ok, "HRTF timing status");
}
template <class T> struct Fixture {
    std::array<std::array<T, 24>, 4> fir{};
    std::array<HrirPoint<T>, 2> points;
    Fixture() {
        const std::array<std::size_t, 4> starts{5, 8, 3, 7};
        for (std::size_t e = 0; e < 4; ++e) {
            fir[e][starts[e]] = T(1);
            fir[e][starts[e] + 1] = T(.25);
            fir[e][starts[e] + 2] = T(-.125);
        }
        points = {
            {{{1, 0, 0}, fir[0], fir[1], 100.25, 99.5}, {{0, 1, 0}, fir[2], fir[3], 101.75, 100}}};
    }
};
// Independent known arrivals, mixed explicit/embedded delays and SRC units.
template <class T> void metadata() {
    Fixture<T> f;
    for (double rate : {48000., 96000., 44100.})
        for (bool detect : {false, true}) {
            HrtfPreparation p;
            p.targetSampleRate = rate;
            p.detectOnsets = detect;
            HrtfSet<T> original, aligned;
            require(aligned.removedCommonDelayFrames() == 0, "unprepared timing report");
            ok(original.prepare(f.points, p));
            p.removeCommonDelay = true;
            ok(aligned.prepare(f.points, p));
            const double common = (detect ? 104.75 : 99.5) * rate / 48000;
            require(std::abs(aligned.removedCommonDelayFrames() - common) < 1e-11 &&
                        original.removedCommonDelayFrames() == 0,
                    "global earliest arrival in target-rate frames");
            require(original.taps() == aligned.taps() &&
                        original.latencyFrames() == aligned.latencyFrames(),
                    "delay removal preserves conversion FIR and declared latency");
            std::vector<T> a(original.taps()), b(a.size()), c(a.size()), d(a.size());
            for (auto direction : {Vec3{1, 0, 0}, Vec3{0, 1, 0}, Vec3{1, 1, 0}}) {
                std::array<double, 2> oldDelay{}, newDelay{};
                ok(original.interpolate(direction, a, b, oldDelay));
                ok(aligned.interpolate(direction, c, d, newDelay));
                require(a == c && b == d, "common offset leaves aligned FIRs unchanged");
                for (std::size_t e = 0; e < 2; ++e)
                    require(newDelay[e] >= 0 &&
                                std::abs(oldDelay[e] - newDelay[e] - common) < 1e-11,
                            "one common offset preserves all directional arrivals");
                require(std::abs((oldDelay[1] - oldDelay[0]) - (newDelay[1] - newDelay[0])) < 1e-11,
                        "ITD preserved for measurements and interpolated directions");
            }
            const auto tail = aligned.storageReport();
            auto invalid = f.points;
            invalid[1].rightDelayFrames = std::numeric_limits<double>::quiet_NaN();
            require(aligned.prepare(invalid, p) == Status::invalid_argument &&
                        std::abs(aligned.removedCommonDelayFrames() - common) < 1e-11 &&
                        aligned.storageReport().taps == tail.taps,
                    "failed reprepare preserves timing and prepared asset");
            p.removeCommonDelay = false;
            ok(aligned.prepare(f.points, p));
            require(aligned.removedCommonDelayFrames() == 0, "report reset on preserving prepare");
        }
    // Silence has no detected arrival; never infer an advance from silence.
    std::array<T, 12> silence{};
    std::array<HrirPoint<T>, 2> silentPoints{
        {{{1, 0, 0}, silence, silence, 4.25, 6.75}, {{0, 1, 0}, silence, silence, 9.5, 4.5}}};
    HrtfPreparation p;
    p.removeCommonDelay = true;
    HrtfSet<T> silent;
    ok(silent.prepare(silentPoints, p));
    require(silent.removedCommonDelayFrames() == 4.25, "silence uses explicit delay only");
    silentPoints[1].rightDelayFrames = 0;
    ok(silent.prepare(silentPoints, p));
    require(silent.removedCommonDelayFrames() == 0,
            "silent zero-delay ear prevents unsafe advance");
    // Threshold is an explicit quality choice: pre-onset data are discarded by
    // the existing detector, never by the new common-offset subtraction.
    std::array<T, 8> precursor{0, T(.02), 0, 0, T(1), T(.25), 0, 0};
    HrirPoint<T> point{{1, 0, 0}, precursor, precursor, 0, 0};
    ok(silent.prepare({&point, 1}, p));
    require(silent.removedCommonDelayFrames() == 1, "default threshold retains precursor");
    p.onsetThresholdDb = -20;
    ok(silent.prepare({&point, 1}, p));
    require(silent.removedCommonDelayFrames() == 4, "explicit aggressive threshold changes onset");
}
// Integer common advance is an exact shift of the old rendering, including
// FIR tails, fractional-delay reconstruction and each convolution engine.
template <class T> void rendering() {
    Fixture<T> f;
    for (auto& point : f.points)
        point.leftDelayFrames = point.rightDelayFrames = 96;
    const double tolerance = std::same_as<T, float> ? 2e-6 : 2e-12;
    for (std::size_t fractional : {0u, 17u}) {
        HrtfPreparation p;
        p.fractionalDelayTaps = fractional;
        HrtfSet<T> original, aligned;
        ok(original.prepare(f.points, p));
        p.removeCommonDelay = true;
        ok(aligned.prepare(f.points, p));
        require(aligned.removedCommonDelayFrames() == 99, "integer acoustic offset");
        for (auto engine : {HrirConvolution::direct, HrirConvolution::fft}) {
            HrirRenderConfig config;
            config.convolution = engine;
            HrirBinaural<T> oldVoice, newVoice;
            ok(oldVoice.prepare({48000, 256, 1}, original, config));
            ok(newVoice.prepare({48000, 256, 1}, aligned, config));
            require(oldVoice.tailFrames() == newVoice.tailFrames() + 99 &&
                        oldVoice.latencyFrames() == newVoice.latencyFrames(),
                    "renderer tail shrinks, processing latency unchanged");
            AudioBuffer<T> input, oldOutput, newOutput;
            ok(input.resize(1, 256));
            ok(oldOutput.resize(2, 256));
            ok(newOutput.resize(2, 256));
            input.block().channel(0)[0] = T(1);
            for (auto direction : {Vec3{1, 0, 0}, Vec3{0, 1, 0}, Vec3{1, 1, 0}}) {
                oldVoice.reset();
                newVoice.reset();
                ok(oldVoice.setDirection(direction, 0));
                ok(newVoice.setDirection(direction, 0));
                std::size_t at = 0;
                for (std::size_t frames : {1u, 7u, 31u, 64u, 128u, 25u}) {
                    ok(oldVoice.process(input.block().subBlock(at, frames),
                                        oldOutput.block().subBlock(at, frames)));
                    ok(newVoice.process(input.block().subBlock(at, frames),
                                        newOutput.block().subBlock(at, frames)));
                    at += frames;
                }
                for (unsigned ear = 0; ear < 2; ++ear)
                    for (std::size_t i = 0; i < 157; ++i)
                        require(std::abs(double(oldOutput.block().channel(ear)[i + 99]) -
                                         double(newOutput.block().channel(ear)[i])) < tolerance,
                                "common advance equals time-shifted measured rendering");
            }
            struct Audit {
                HrirBinaural<T>* voice;
                AudioBuffer<T>* in;
                AudioBuffer<T>* out;
                Status status = Status::not_prepared;
            } audit{&newVoice, &input, &newOutput};
            runRealtimeAudit(
                [](void* ptr) noexcept {
                    auto& a = *static_cast<Audit*>(ptr);
                    a.voice->reset();
                    a.status = a.voice->setDirection({1, 1, 0}, 17);
                    if (a.status == Status::ok)
                        a.status = a.voice->process(a.in->block(), a.out->block());
                    a.voice->reset();
                },
                &audit);
            ok(audit.status);
        }
        HoaBinaural<T> originalHoa, alignedHoa;
        ok(originalHoa.prepare({48000, 256, 1}, 0, original));
        ok(alignedHoa.prepare({48000, 256, 1}, 0, aligned));
        AudioBuffer<T> input, a, b;
        ok(input.resize(1, 256));
        ok(a.resize(2, 256));
        ok(b.resize(2, 256));
        input.block().channel(0)[0] = T(1);
        ok(originalHoa.process(input.block(), a.block()));
        ok(alignedHoa.process(input.block(), b.block()));
        require(originalHoa.latencyFrames() == alignedHoa.latencyFrames(), "HOA latency unchanged");
        for (unsigned ear = 0; ear < 2; ++ear)
            for (std::size_t i = 0; i < 157; ++i)
                require(std::abs(double(a.block().channel(ear)[i + 99]) -
                                 double(b.block().channel(ear)[i])) < tolerance,
                        "HOA LS consumes residual delays without restoring acoustic offset");
    }
}
} // namespace
void hrtfTimingRegression() {
    metadata<float>();
    metadata<double>();
    rendering<float>();
    rendering<double>();
}
