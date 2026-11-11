#pragma once
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/processors/Fir.h"
#include "cydsp/processors/PartitionedConvolver.h"
namespace cydsp {
// Direct FIR head plus partitioned FFT tail. No added block buffering latency.
// One finite impulse is applied independently to every channel.
template <Sample T>
    requires(!std::is_const_v<T>)
class HybridConvolver {
  public:
    HybridConvolver() = default;
    HybridConvolver(const HybridConvolver&) = delete;
    HybridConvolver& operator=(const HybridConvolver&) = delete;
    HybridConvolver(HybridConvolver&& other) noexcept {
        swap(other);
    }
    HybridConvolver& operator=(HybridConvolver&& other) noexcept {
        if (this != &other) {
            HybridConvolver next(std::move(other));
            swap(next);
        }
        return *this;
    }
    [[nodiscard]] Status prepare(const ProcessSpec&, std::span<const T> impulse,
                                 std::size_t partitionFrames = 128,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    void reset() noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept {
        return 0;
    }
    [[nodiscard]] std::size_t tailFrames() const noexcept {
        return impulseFrames_ ? impulseFrames_ - 1 : 0;
    }
    [[nodiscard]] std::size_t partitionFrames() const noexcept {
        return partition_;
    }
    [[nodiscard]] CpuBackend backend() const noexcept {
        return ops_.backend();
    }

  private:
    void swap(HybridConvolver&) noexcept;
    ProcessSpec spec_{};
    Fir<T> head_;
    PartitionedConvolver<T> tail_;
    AudioBuffer<T> tailBuffer_;
    VectorOps ops_;
    std::size_t partition_ = 0, impulseFrames_ = 0;
};
extern template class HybridConvolver<float>;
extern template class HybridConvolver<double>;
} // namespace cydsp
