#pragma once
#include "cydsp/processors/HybridConvolver.h"
namespace cydsp {
// Two partition sizes. The late tier starts at latePartition, matching its latency.
template <Sample T>
    requires(!std::is_const_v<T>)
class NonuniformConvolver {
  public:
    NonuniformConvolver() = default;
    NonuniformConvolver(const NonuniformConvolver&) = delete;
    NonuniformConvolver& operator=(const NonuniformConvolver&) = delete;
    NonuniformConvolver(NonuniformConvolver&& other) noexcept {
        swap(other);
    }
    NonuniformConvolver& operator=(NonuniformConvolver&& other) noexcept {
        if (this != &other) {
            NonuniformConvolver next(std::move(other));
            swap(next);
        }
        return *this;
    }
    [[nodiscard]] Status prepare(const ProcessSpec&, std::span<const T>,
                                 std::size_t earlyPartition = 128, std::size_t latePartition = 1024,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    void reset() noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept {
        return 0;
    }
    [[nodiscard]] std::size_t tailFrames() const noexcept {
        return impulse_ ? impulse_ - 1 : 0;
    }
    [[nodiscard]] CpuBackend backend() const noexcept {
        return ops_.backend();
    }

  private:
    void swap(NonuniformConvolver&) noexcept;
    ProcessSpec spec_{};
    HybridConvolver<T> early_;
    PartitionedConvolver<T> late_;
    AudioBuffer<T> scratch_;
    VectorOps ops_;
    std::size_t impulse_ = 0, latePartition_ = 0;
};
extern template class NonuniformConvolver<float>;
extern template class NonuniformConvolver<double>;
} // namespace cydsp
