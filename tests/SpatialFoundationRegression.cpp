#include <algorithm>
#include <array>
#include <cmath>
#include <cydsp/CyDSP.h>
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
    require(s == Status::ok, "spatial foundation status");
}
void geometry() {
    SpatialSource source;
    SpatialListener listener;
    source.pose.position = {0, 10, 0};
    RelativeSource result;
    ok(relativeSource(source, listener, result));
    require(std::abs(result.azimuthRadians - std::numbers::pi / 2) < 1e-14 &&
                result.distanceMetres == 10,
            "spatial axes/azimuth");
    listener.pose.orientation = {std::sqrt(.5), 0, 0, std::sqrt(.5)};
    ok(relativeSource(source, listener, result));
    require(std::abs(result.azimuthRadians) < 1e-14, "listener inverse orientation");
    Pose3 a, b, out;
    b.position = {10, 0, 0};
    b.orientation = {0, 0, 0, 1};
    ok(interpolatePose(a, b, .5, out));
    const auto direction = rotate(out.orientation, {1, 0, 0});
    require(std::abs(out.position.x - 5) < 1e-14 && std::abs(direction.y - 1) < 1e-14,
            "pose interpolation/SLERP");
    AcousticMedium air;
    result.sourceRadialVelocity = -34.3;
    result.listenerRadialVelocity = 0;
    double ratio = 0;
    ok(dopplerRatio(result, air, ratio));
    require(std::abs(ratio - 1 / .9) < 1e-14, "approaching source Doppler");
    result.sourceRadialVelocity = result.listenerRadialVelocity = 100;
    ok(dopplerRatio(result, air, ratio));
    require(ratio == 1, "co-moving source/listener Doppler");
    const std::array<SpatialSource, 2> sources{{source, source}};
    const std::array<SpatialListener, 2> listeners{{listener, {}}};
    std::array<RelativeSource, 4> results;
    ok(relativeSources(sources, listeners, results));
    require(results[0].distanceMetres == 10 &&
                std::abs(results[2].azimuthRadians - std::numbers::pi / 2) < 1e-14,
            "multiple listener layout");
    source.pose.position = {10, 0, 0};
    source.pose.orientation = {0, 0, 0, 1};
    Directivity directivity;
    directivity.pattern = DirectivityPattern::cardioid;
    PropagationResult propagation;
    ok(evaluatePropagation(source, SpatialListener{}, air, {}, directivity, propagation));
    require(std::abs(propagation.travel.seconds - 10. / 343) < 1e-14 &&
                std::abs(propagation.travel.bandGains[0] - .1) < 1e-14,
            "propagation distance/directivity");
}
void media() {
    double value = 0;
    ok(atmosphericAbsorption(1000, 45, 20, value));
    require(std::abs(value - .010102) < 5e-7, "NPL atmospheric absorption fixture");
    AcousticMedium air;
    ok(airMedium(20, 50, air));
    require(air.speedMetresPerSecond > 343 && air.speedMetresPerSecond < 344 &&
                air.absorptionDbPerMetre[2] > air.absorptionDbPerMetre[0],
            "air medium ranges");
    AcousticMedium custom{100, 1, {.1, .2, .3}};
    const std::array<MediumSegment, 2> segments{{{100, custom}, {1480, waterMedium()}}};
    AcousticTravel travel;
    ok(acousticTravel(segments, travel));
    require(travel.seconds == 2 && std::abs(travel.bandGains[0] - std::pow(10., -10. / 20)) < 1e-14,
            "multi-medium travel time/loss");
    Directivity directivity;
    directivity.pattern = DirectivityPattern::cardioid;
    std::array<double, 3> gains;
    ok(directivityGains(0, directivity, gains));
    require(gains[0] == .5, "cardioid lateral response");
    directivity.pattern = DirectivityPattern::figureEight;
    directivity.preservePolarity = true;
    ok(directivityGains(-1, directivity, gains));
    require(gains[0] == -1, "dipole polarity");
    DistanceAttenuation attenuation;
    ok(distanceGain(10, attenuation, value));
    require(std::abs(value - .1) < 1e-15, "inverse distance law");
    attenuation.model = DistanceModel::exponential;
    attenuation.rolloff = 2;
    ok(distanceGain(10, attenuation, value));
    require(std::abs(value - .01) < 1e-15, "exponential distance law");
    AcousticMaterial material;
    MaterialResponse thin, thick, empty;
    ok(materialResponse(material, .2, thin));
    ok(materialResponse(material, .4, thick));
    ok(materialResponse(material, 0, empty));
    for (std::size_t b = 0; b < 3; ++b) {
        require(thick.transmission[b] < thin.transmission[b], "material thickness transmission");
        require(thin.transmission[b] * thin.transmission[b] +
                        thin.reflection[b] * thin.reflection[b] <=
                    1,
                "material passive energy");
        require(empty.transmission[b] == 1 && empty.reflection[b] == 0,
                "zero thickness transparency");
    }
}
template <class T> void bandGain() {
    BandGain<T> gain;
    ok(gain.prepare({48000, 128, 2}));
    AudioBuffer<T> in, out;
    ok(in.resize(2, 128));
    ok(out.resize(2, 128));
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t i = 0; i < 128; ++i)
            in.block().channel(ch)[i] = T(.5 * std::sin(double(i)));
    ok(gain.process(in.block(), out.block()));
    for (std::size_t ch = 0; ch < 2; ++ch)
        require(std::equal(in.block().channel(ch).begin(), in.block().channel(ch).end(),
                           out.block().channel(ch).begin()),
                "complementary band-gain unity reconstruction");
    ok(gain.setGains({.1, .2, .3}, 32));
    struct C {
        BandGain<T>* p;
        AudioBlock<const T> in;
        AudioBlock<T> out;
        bool passed = true;
    } c{&gain, in.block(), out.block()};
    runRealtimeAudit(
        [](void* raw) noexcept {
            auto& c = *static_cast<C*>(raw);
            c.passed = c.p->process(c.in, c.out) == Status::ok;
            c.p->reset();
        },
        &c);
    require(c.passed, "band gain realtime");
}
template <class T> void doppler() {
    DopplerResampler<T> resampler;
    ResampleSpec spec;
    spec.inputRate = spec.outputRate = 48000;
    spec.channels = 1;
    spec.maxInputFrames = spec.maxOutputFrames = 256;
    ok(resampler.prepare(spec, 12));
    RelativeSource motion;
    motion.sourceRadialVelocity = -34.3;
    ok(resampler.setMotion(motion, {}));
    AudioBuffer<T> input, output;
    ok(input.resize(1, 8192));
    ok(output.resize(1, 256));
    for (std::size_t i = 0; i < 8192; ++i)
        input.block().channel(0)[i] = T(.4 * std::sin(2 * std::numbers::pi * .05 * double(i)));
    std::vector<double> rendered;
    rendered.reserve(8192);
    std::size_t at = 0;
    while (at < 8192) {
        const auto amount = std::min<std::size_t>(256, 8192 - at);
        const auto result = resampler.process(input.block().subBlock(at, amount), output.block());
        ok(result.status);
        require(result.inputConsumed || result.outputProduced, "Doppler resampler progress");
        at += result.inputConsumed;
        for (std::size_t i = 0; i < result.outputProduced; ++i)
            rendered.push_back(output.block().channel(0)[i]);
    }
    double first = 0, last = 0;
    std::size_t crossings = 0;
    for (std::size_t i = 1025; i < rendered.size(); ++i)
        if (rendered[i - 1] <= 0 && rendered[i] > 0) {
            const double position =
                double(i - 1) - rendered[i - 1] / (rendered[i] - rendered[i - 1]);
            if (!crossings)
                first = position;
            last = position;
            ++crossings;
        }
    const double frequency = 48000 * double(crossings - 1) / (last - first);
    require(crossings > 100 && std::abs(frequency - 2400 / .9) < 1,
            "Doppler rendered tone frequency");
}
} // namespace
void spatialFoundationRegression() {
    geometry();
    media();
    bandGain<float>();
    bandGain<double>();
    doppler<float>();
    doppler<double>();
}
