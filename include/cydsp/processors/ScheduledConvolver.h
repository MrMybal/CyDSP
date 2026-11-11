#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include <memory>
#include <span>
namespace cydsp {
// Zero added block latency. A hybrid prefix covers [0,2*latePartition); later IR
// work, including FFT stages, is divided across input frames on the audio thread.
template <Sample T>
    requires(!std::is_const_v<T>)
class ScheduledConvolver {
  public:
    ScheduledConvolver() noexcept;
    ~ScheduledConvolver();
    ScheduledConvolver(const ScheduledConvolver&) = delete;
    ScheduledConvolver& operator=(const ScheduledConvolver&) = delete;
    ScheduledConvolver(ScheduledConvolver&&) noexcept;
    ScheduledConvolver& operator=(ScheduledConvolver&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, std::span<const T>,
                                 std::size_t earlyPartition = 128, std::size_t latePartition = 1024,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    void reset() noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept {
        return 0;
    }
    [[nodiscard]] std::size_t tailFrames() const noexcept;
    [[nodiscard]] CpuBackend backend() const noexcept;
    [[nodiscard]] bool schedulingActive() const noexcept;
    // Counts bounded chunks, not CPU cycles or a hard deadline guarantee.
    [[nodiscard]] std::size_t workUnitsPerPartition() const noexcept;
    [[nodiscard]] std::size_t maximumWorkUnitsPerFrame() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class ScheduledConvolver<float>;
extern template class ScheduledConvolver<double>;
} // namespace cydsp
