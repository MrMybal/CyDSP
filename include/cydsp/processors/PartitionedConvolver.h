#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/processors/RealFft.h"
#include <utility>
namespace cydsp {
// Same IR independently on every channel. Uniform partitions, exact P-frame latency.
// Host blocks may have any size <= maxBlockFrames, including sizes not dividing P.
template <Sample T>
    requires(!std::is_const_v<T>)
class PartitionedConvolver {
  public:
    PartitionedConvolver() = default;
    PartitionedConvolver(const PartitionedConvolver&) = delete;
    PartitionedConvolver& operator=(const PartitionedConvolver&) = delete;
    PartitionedConvolver(PartitionedConvolver&& other) noexcept {
        swap(other);
    }
    PartitionedConvolver& operator=(PartitionedConvolver&& other) noexcept {
        if (this != &other) {
            PartitionedConvolver next(std::move(other));
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
        return partition_;
    }
    [[nodiscard]] std::size_t tailFrames() const noexcept {
        return tail_;
    }
    [[nodiscard]] CpuBackend backend() const noexcept {
        return ops_.backend();
    }

  private:
    void swap(PartitionedConvolver&) noexcept;
    void processPartition() noexcept;
    void accumulateUntil(std::size_t) noexcept;
    ProcessSpec spec_{};
    std::size_t partition_ = 0, partitions_ = 0, position_ = 0, head_ = 0, tail_ = 0, jobsDone_ = 0;
    std::size_t jobChannel_ = 0, jobPart_ = 1, jobsPerFrame_ = 0, extraJobs_ = 0, workDebt_ = 0;
    RealFft<T> fft_;
    VectorOps ops_;
    std::vector<T> input_, ready_, overlap_, time_;
    std::vector<std::complex<T>> impulses_, history_, sum_;
};
extern template class PartitionedConvolver<float>;
extern template class PartitionedConvolver<double>;
} // namespace cydsp
