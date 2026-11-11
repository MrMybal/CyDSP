#pragma once
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/math/VectorOps.h"
#include <cstdint>
#include <utility>
#include <vector>
namespace cydsp {
struct ResampleSpec {
    std::uint32_t inputRate = 48000, outputRate = 44100;
    std::size_t channels = 2, maxInputFrames = 512, maxOutputFrames = 1024, taps = 128,
                phases = 1024;
    // Optional prepared envelope in INPUT frames per OUTPUT frame. Both zero: rational mode.
    double minimumRatio = 0, maximumRatio = 0;
    [[nodiscard]] constexpr bool isValid() const noexcept {
        return inputRate && outputRate && channels && maxInputFrames && maxOutputFrames &&
               taps >= 8 && taps <= 512 && taps % 2 == 0 && phases >= 16 && phases <= 4096 &&
               isPowerOfTwo(phases) &&
               ((minimumRatio == 0 && maximumRatio == 0) ||
                (isFinite(minimumRatio) && isFinite(maximumRatio) && minimumRatio >= 1.0 / 65536 &&
                 maximumRatio <= 65536 && minimumRatio <= maximumRatio &&
                 static_cast<double>(inputRate) / outputRate >= minimumRatio &&
                 static_cast<double>(inputRate) / outputRate <= maximumRatio));
    }
};
enum class ResampleQuality { economy, balanced, high };
// Presets trade preparation/memory/CPU for longer filtering; measure the intended rates.
[[nodiscard]] constexpr ResampleSpec resampleSpecForQuality(ResampleSpec spec,
                                                            ResampleQuality quality) noexcept {
    switch (quality) {
    case ResampleQuality::economy:
        spec.taps = 64;
        spec.phases = 256;
        break;
    case ResampleQuality::balanced:
        spec.taps = 128;
        spec.phases = 1024;
        break;
    case ResampleQuality::high:
        spec.taps = 256;
        spec.phases = 4096;
        break;
    default:
        spec.taps = 0;
        return spec;
    }
    // Increase support for the worst prepared downsampling ratio, including variable mode.
    const auto nominal =
        spec.outputRate && spec.inputRate / spec.outputRate >= 2
            ? (static_cast<std::uint64_t>(spec.inputRate) + spec.outputRate - 1) / spec.outputRate
            : 1;
    auto ratio = nominal;
    if (isFinite(spec.maximumRatio) && spec.maximumRatio > 0 && spec.maximumRatio <= 65536) {
        const auto whole = static_cast<std::uint64_t>(spec.maximumRatio);
        ratio = std::max(ratio, whole + (spec.maximumRatio > static_cast<double>(whole) ? 1u : 0u));
    }
    if (ratio >= 2) {
        const auto factor = static_cast<std::size_t>(std::min<std::uint64_t>(ratio, 8));
        spec.taps = std::min<std::size_t>(512, spec.taps * factor);
    }
    return spec;
}
struct ResampleResult {
    Status status = Status::ok;
    std::size_t inputConsumed = 0, outputProduced = 0;
};
// Fixed rational rates; Blackman-windowed sinc phase table with linear phase interpolation.
// Returned counts are mandatory: retain unconsumed input and use only produced output.
template <Sample T>
    requires(!std::is_const_v<T>)
class Resampler {
  public:
    Resampler() = default;
    Resampler(const Resampler&) = delete;
    Resampler& operator=(const Resampler&) = delete;
    Resampler(Resampler&& other) noexcept {
        swap(other);
    }
    Resampler& operator=(Resampler&& other) noexcept {
        if (this != &other) {
            Resampler next(std::move(other));
            swap(next);
        }
        return *this;
    }
    [[nodiscard]] Status prepare(const ResampleSpec&, CpuBackend = CpuBackend::automatic) noexcept;
    // Non-real-time, preserves FIR history and the already scheduled next output.
    [[nodiscard]] Status setRates(std::uint32_t inputRate, std::uint32_t outputRate) noexcept;
    // Real-time, within the prepared envelope. Ramp duration counts produced output frames.
    // Keeps the already scheduled next output; the table cutoff stays safe for the entire envelope.
    [[nodiscard]] Status setRatio(double inputPerOutput, std::size_t outputRampFrames = 0) noexcept;
    [[nodiscard]] double ratio() const noexcept {
        return ratio_;
    }
    [[nodiscard]] std::size_t ratioTransitionRemainingFrames() const noexcept {
        return ratioRemaining_;
    }
    void reset() noexcept;
    // Signals EOS and drains up to taps zero-input frames; inputConsumed is always zero.
    [[nodiscard]] ResampleResult finish(AudioBlock<T>) noexcept;
    [[nodiscard]] bool isFinished() const noexcept {
        return finished_;
    }
    [[nodiscard]] ResampleResult process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    [[nodiscard]] CpuBackend backend() const noexcept {
        return ops_.backend();
    }
    [[nodiscard]] double latencyInputFrames() const noexcept {
        return prepared_ ? static_cast<double>(spec_.taps - 1) * 0.5 : 0;
    }

  private:
    void swap(Resampler&) noexcept;
    [[nodiscard]] Status validateBlocks(AudioBlock<const T>, AudioBlock<T>) const noexcept;
    [[nodiscard]] ResampleResult processInternal(AudioBlock<const T>, AudioBlock<T>) noexcept;
    ResampleSpec spec_{};
    VectorOps ops_;
    std::vector<T> history_;
    AudioBuffer<T> zeros_;
    std::vector<double> table_, weights_;
    std::size_t write_ = 0, pending_ = 1;
    std::uint64_t phase_ = 0;
    double variablePhase_ = 0, ratio_ = 1, targetRatio_ = 1;
    std::size_t ratioRemaining_ = 0;
    std::size_t zerosRemaining_ = 0;
    bool prepared_ = false, hasInput_ = false, ending_ = false, finished_ = false;
};
extern template class Resampler<float>;
extern template class Resampler<double>;
} // namespace cydsp
