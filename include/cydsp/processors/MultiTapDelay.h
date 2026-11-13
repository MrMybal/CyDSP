#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/CircularHistory.h"
#include "cydsp/core/Cpu.h"
#include <memory>
namespace cydsp {
struct DelayTap {
    std::size_t inputChannel = 0, outputChannel = 0;
    double delayFrames = 0, gain = 1;
    HistoryInterpolation interpolation = HistoryInterpolation::linear;
};
// One history for all taps. Input and output must be disjoint. Tap updates crossfade
// complete old/new read configurations, preserving pitch during a delay change.
template <Sample T> class MultiTapDelay {
  public:
    MultiTapDelay();
    ~MultiTapDelay();
    MultiTapDelay(MultiTapDelay&&) noexcept;
    MultiTapDelay& operator=(MultiTapDelay&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec& inputSpec, std::size_t outputChannels,
                                 std::size_t maximumDelayFrames, std::size_t maximumTaps = 32,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setTaps(std::span<const DelayTap>,
                                 std::size_t transitionFrames = 0) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t transitionRemainingFrames() const noexcept;
    [[nodiscard]] std::size_t tailFrames() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
// First-order Thiran all-pass: a unity-magnitude fractional delay D in (0,1].
// Stateful sequential interpolation, distinct from random-access history reading.
template <Sample T> class ThiranDelay {
  public:
    [[nodiscard]] Status setDelay(double fractionalFrames) noexcept {
        if (!isFinite(fractionalFrames) || fractionalFrames <= 0 || fractionalFrames > 1)
            return Status::invalid_argument;
        const double next = (1 - fractionalFrames) / (1 + fractionalFrames);
        if (next >= 1)
            return Status::invalid_argument;
        coefficient_ = next;
        return Status::ok;
    }
    [[nodiscard]] T processSample(T input) noexcept {
        const double out =
            coefficient_ * double(input) + previousInput_ - coefficient_ * previousOutput_;
        previousInput_ = double(input);
        previousOutput_ = out;
        return T(out);
    }
    void reset() noexcept {
        previousInput_ = previousOutput_ = 0;
    }

  private:
    double coefficient_ = 0, previousInput_ = 0, previousOutput_ = 0;
};
extern template class MultiTapDelay<float>;
extern template class MultiTapDelay<double>;
} // namespace cydsp
