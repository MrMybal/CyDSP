#pragma once
#include "cydsp/io/AudioFile.h"
#include "cydsp/sources/AudioSource.h"
#include "cydsp/sources/SourceStretch.h"
#include <memory>
namespace cydsp {
struct ReadAheadOptions {
    std::uint32_t outputRate = 0;
    std::size_t outputChannels = 0, pageFrames = 1024, pages = 8;
    double durationRatio = 1, pitchSemitones = 0, formant = 1;
    bool cheaperStretch = false;
    CpuBackend backend = CpuBackend::automatic;
    SourceStretchMode mode = SourceStretchMode::automatic;
    SourceSeekPolicy seek = SourceSeekPolicy::primeHistory;
    SourceTailPolicy tails = SourceTailPolicy::trimToTimeline;
    std::vector<SourceTimePoint> timeMap;
    std::size_t controlQuantumFrames = 64;
    double tonalityLimitHz = 3000;
    SourceStretchProvider* provider = nullptr; // Factory borrowed only during prepare.
};
// Link CyDSP::Sources. Worker owns decoding/resampling; readAt is single-consumer realtime.
template <Sample T> class ReadAheadSource : public AudioSource<T> {
  public:
    ReadAheadSource();
    ~ReadAheadSource();
    ReadAheadSource(ReadAheadSource&&) noexcept;
    ReadAheadSource& operator=(ReadAheadSource&&) noexcept;
    Status prepare(const std::filesystem::path&, std::uint32_t outputRate = 0,
                   std::size_t pageFrames = 1024, std::size_t pages = 8,
                   CpuBackend = CpuBackend::automatic) noexcept;
    Status prepare(const std::filesystem::path&, const ReadAheadOptions&) noexcept;
    AudioSourceInfo info() const noexcept override;
    SourceStretchInfo stretchInfo() const noexcept;
    Status readAt(std::uint64_t, AudioBlock<T>, std::size_t& framesRead) noexcept override;
    // Control thread only, while readAt is idle. Initiates a seek and waits up to timeoutMs.
    Status prefill(std::uint64_t first, std::size_t minimumFrames = 1024,
                   unsigned timeoutMs = 2000) noexcept;
    [[nodiscard]] std::uint32_t underruns() const noexcept;
    void close() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
template <Sample T> class BackgroundRecorder {
  public:
    BackgroundRecorder();
    ~BackgroundRecorder();
    BackgroundRecorder(BackgroundRecorder&&) noexcept;
    BackgroundRecorder& operator=(BackgroundRecorder&&) noexcept;
    Status prepare(const std::filesystem::path&, AudioFileFormat, const AudioFileInfo&,
                   std::size_t maxBlockFrames = 1024, std::size_t queuedBlocks = 16,
                   CpuBackend = CpuBackend::automatic) noexcept;
    Status
        push(AudioBlock<const T>) noexcept; // Audio producer; whole block is accepted or rejected.
    Status
    finish() noexcept; // Control thread, after producer is idle; drains/joins and finalises file.
    [[nodiscard]] std::uint32_t droppedBlocks() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class ReadAheadSource<float>;
extern template class ReadAheadSource<double>;
extern template class BackgroundRecorder<float>;
extern template class BackgroundRecorder<double>;
} // namespace cydsp
