#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include "cydsp/sources/SourceTimeMap.h"
#include <memory>
namespace cydsp {
enum class SourceStretchMode { automatic, none, repitch, tonal, complex, beats, texture, external };
enum class SourceSeekPolicy { primeHistory, resetHistory };
enum class SourceTailPolicy { trimToTimeline, includeAlgorithmTail };
struct SourceStretchCapabilities {
    double minimumDurationRatio = 1.0 / 64, maximumDurationRatio = 64, minimumPitch = -48,
           maximumPitch = 48;
    double minimumFormant = .25, maximumFormant = 4;
    bool available = false, timeCurves = false, pitchCurves = false;
};
struct SourceStretchControl {
    const SourceTimeMap* mapping = nullptr;
    std::uint64_t outputFirst = 0, inputFirst = 0;
    double formant = 1, tonalityLimitHz = 0;
    std::size_t zeroPrefixFrames = 0;
};
template <Sample T> class SourceStretchProcessor {
  public:
    virtual ~SourceStretchProcessor() = default;
    virtual SourceStretchCapabilities capabilities() const noexcept = 0;
    // Control, may allocate. Subsequent calls have one worker owner.
    virtual Status prepare(const ProcessSpec& input, std::size_t maximumOutputFrames,
                           CpuBackend) noexcept = 0;
    virtual std::size_t inputLatency() const noexcept = 0;
    virtual std::size_t outputLatency() const noexcept = 0;
    virtual Status prime(AudioBlock<const T> history, const SourceStretchControl&) noexcept = 0;
    virtual Status process(AudioBlock<const T> input, AudioBlock<T> output,
                           const SourceStretchControl&) noexcept = 0;
};
// Factory borrowed only during ReadAheadSource::prepare; returned processors are
// owned by the source worker. This preserves application-specific beats/texture
// algorithms without importing their code or claiming identical sound.
class SourceStretchProvider {
  public:
    virtual ~SourceStretchProvider() = default;
    virtual Status create(SourceStretchMode,
                          std::unique_ptr<SourceStretchProcessor<float>>&) noexcept = 0;
    virtual Status create(SourceStretchMode,
                          std::unique_ptr<SourceStretchProcessor<double>>&) noexcept = 0;
};
struct SourceStretchInfo {
    SourceStretchMode mode = SourceStretchMode::none;
    SourceSeekPolicy seek = SourceSeekPolicy::primeHistory;
    SourceTailPolicy tails = SourceTailPolicy::trimToTimeline;
    SourceStretchCapabilities capabilities;
    std::uint64_t timelineFrames = 0, tailFrames = 0;
    std::size_t primePastFrames = 0, primeFutureFrames = 0, controlQuantumFrames = 0;
    double repitchAntiAliasMaximumStep = 1;
    bool externalProvider = false;
};
SourceStretchCapabilities sourceStretchCapabilities(SourceStretchMode) noexcept;
} // namespace cydsp
