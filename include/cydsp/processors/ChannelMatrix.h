#pragma once
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/math/VectorOps.h"
namespace cydsp {
// Rectangular channel mixer: coefficients are output-major, then input channel.
// Exact whole-channel aliases, including permutations, use prepared input snapshots.
template <Sample T>
    requires(!std::is_const_v<T>)
class ChannelMatrix {
  public:
    ChannelMatrix() = default;
    ChannelMatrix(const ChannelMatrix&) = delete;
    ChannelMatrix& operator=(const ChannelMatrix&) = delete;
    ChannelMatrix(ChannelMatrix&& other) noexcept {
        swap(other);
    }
    ChannelMatrix& operator=(ChannelMatrix&& other) noexcept {
        if (this != &other) {
            ChannelMatrix next(std::move(other));
            swap(next);
        }
        return *this;
    }
    [[nodiscard]] Status prepare(const ProcessSpec& inputSpec, std::size_t outputChannels,
                                 std::span<const double> coefficients,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    // No allocation. Finite coefficients must fit T; ramp differences must fit double.
    [[nodiscard]] Status setMatrix(std::span<const double>, std::size_t rampFrames = 0) noexcept;
    void reset() noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    [[nodiscard]] std::size_t inputChannels() const noexcept {
        return tileFrames_ ? spec_.channels : 0;
    }
    [[nodiscard]] std::size_t outputChannels() const noexcept {
        return outputs_;
    }
    [[nodiscard]] std::size_t rampRemainingFrames() const noexcept {
        return remaining_;
    }
    [[nodiscard]] CpuBackend backend() const noexcept {
        return ops_.backend();
    }

  private:
    void swap(ChannelMatrix&) noexcept;
    ProcessSpec spec_{};
    AudioBuffer<T> inputScratch_;
    std::vector<double> current_, target_;
    VectorOps ops_;
    std::size_t outputs_ = 0, tileFrames_ = 0, remaining_ = 0;
};
extern template class ChannelMatrix<float>;
extern template class ChannelMatrix<double>;
} // namespace cydsp
