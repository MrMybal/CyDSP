// Compile-only integration example. Preparation is performed by the owner before
// entering its audio callback. No hardware, file loading or UI dependency.
#include <array>
#include <cydsp/CyDSP.h>
namespace cydsp::examples {
Status advancedAudioExample() {
    constexpr ProcessSpec mono{48000, 128, 1}, stereo{48000, 128, 2};
    QualitySettings quality;
    auto status = qualitySettings(QualityProfile::realtimeHigh, 48000, quality);
    if (status != Status::ok)
        return status;
    std::array<float, 128> left{}, right{};
    left[12] = 1;
    right[20] = .8f;
    const std::array<HrirPoint<float>, 2> measurements{
        {{{0, 1, 0}, left, right}, {{0, -1, 0}, right, left}}};
    HrtfSet<float> shared;
    status = shared.prepare(measurements, quality.hrtf);
    if (status != Status::ok)
        return status;
    HrirBinaural<float> sourceA, sourceB;
    for (auto* renderer : {&sourceA, &sourceB}) {
        status = renderer->prepare(mono, shared);
        if (status != Status::ok)
            return status;
    }
    // Both renderers share immutable IRs; histories/transitions remain independent.
    status = sourceA.setDirection({0, 1, 0}, 0);
    if (status != Status::ok)
        return status;
    status = sourceB.setDirection({0, -1, 0}, 0);
    if (status != Status::ok)
        return status;
    SourceBatch<float> synthesis;
    status = synthesis.prepare(mono, 64);
    if (status != Status::ok)
        return status;
    BatchSource source;
    source.frequencyHz = 220;
    source.gain = .1;
    status = synthesis.start(0, source);
    if (status != Status::ok)
        return status;
    AudioBuffer<float> input, output;
    status = input.resize(1, 128);
    if (status != Status::ok)
        return status;
    status = output.resize(2, 128);
    if (status != Status::ok)
        return status;
    SpectralPitchShifter<float> pitch;
    status = pitch.prepare(stereo, quality.spectral);
    if (status != Status::ok)
        return status;
    // The following operations may run in the owner's audio callback.
    status = synthesis.process(input.block());
    if (status != Status::ok)
        return status;
    status = sourceA.process(input.block(), output.block());
    if (status != Status::ok)
        return status;
    return pitch.process(output.block(), output.block());
}
} // namespace cydsp::examples
