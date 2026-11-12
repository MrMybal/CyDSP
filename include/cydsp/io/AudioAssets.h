#pragma once
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/core/Cpu.h"
#include "cydsp/io/AudioFile.h"
namespace cydsp {
struct AudioPeak {
    float minimum = 0, maximum = 0, rms = 0;
};
// Immutable peak pyramid. Build/save/load are control/worker operations.
// Views are allocation-free and borrowed until move/rebuild/destruction.
class WaveformPeakCache {
  public:
    WaveformPeakCache();
    ~WaveformPeakCache();
    WaveformPeakCache(WaveformPeakCache&&) noexcept;
    WaveformPeakCache& operator=(WaveformPeakCache&&) noexcept;
    Status build(const std::filesystem::path& audio, std::size_t framesPerBucket = 256,
                 std::size_t maximumBuckets = 1048576) noexcept;
    Status save(const std::filesystem::path& cache) const noexcept;
    Status load(const std::filesystem::path& cache, const std::filesystem::path& source) noexcept;
    [[nodiscard]] const AudioFileInfo* info() const noexcept;
    [[nodiscard]] std::size_t levels() const noexcept;
    [[nodiscard]] std::uint64_t bucketFrames(std::size_t level) const noexcept;
    [[nodiscard]] std::span<const AudioPeak> peaks(std::size_t channel,
                                                   std::size_t level = 0) const noexcept;
    [[nodiscard]] std::size_t selectLevel(double framesPerPixel) const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
struct ImpulseLoadOptions {
    double sampleRate = 0;                             // Zero preserves source rate.
    std::size_t channels = 0, maximumFrames = 1048576; // Limit input and output allocation.
    bool normalize = false, trimSilence = false;
    double trimThresholdDb = -120;
    CpuBackend backend = CpuBackend::automatic;
};
template <Sample T> class ImpulseResponse {
  public:
    ImpulseResponse() = default;
    ImpulseResponse(ImpulseResponse&& other) noexcept
        : audio_(std::move(other.audio_)), frames_(std::exchange(other.frames_, 0)),
          rate_(std::exchange(other.rate_, 0)) {}
    ImpulseResponse& operator=(ImpulseResponse&& other) noexcept {
        if (this != &other) {
            audio_ = std::move(other.audio_);
            frames_ = std::exchange(other.frames_, 0);
            rate_ = std::exchange(other.rate_, 0);
        }
        return *this;
    }
    Status load(const std::filesystem::path&, const ImpulseLoadOptions& = {}) noexcept;
    [[nodiscard]] AudioBlock<const T> samples() const noexcept {
        return audio_.block().subBlock(0, frames_);
    }
    [[nodiscard]] double sampleRate() const noexcept {
        return rate_;
    }

  private:
    AudioBuffer<T> audio_;
    std::size_t frames_ = 0;
    double rate_ = 0;
};
extern template class ImpulseResponse<float>;
extern template class ImpulseResponse<double>;
} // namespace cydsp
