#pragma once
#include "cydsp/core/AudioBlock.h"
#include <utility>
#include <vector>
namespace cydsp {
// Independent planar channels; linear fractional interpolation, delay measured in frames.
template <Sample T>
    requires(!std::is_const_v<T>)
class DelayLine {
  public:
    DelayLine() = default;
    DelayLine(const DelayLine&) = delete;
    DelayLine& operator=(const DelayLine&) = delete;
    DelayLine(DelayLine&& other) noexcept {
        swap(other);
    }
    DelayLine& operator=(DelayLine&& other) noexcept {
        if (this != &other) {
            DelayLine next(std::move(other));
            swap(next);
        }
        return *this;
    }
    [[nodiscard]] Status prepare(const ProcessSpec&, std::size_t maximumDelayFrames) noexcept;
    [[nodiscard]] Status setDelay(double frames, std::size_t rampFrames = 0) noexcept;
    void reset() noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    [[nodiscard]] double targetDelayFrames() const noexcept {
        return target_;
    }
    [[nodiscard]] double delayFrames() const noexcept {
        return delay_;
    }

  private:
    void swap(DelayLine&) noexcept;
    void processConstant(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void processFrame(AudioBlock<const T>, AudioBlock<T>, std::size_t frame, std::size_t read,
                      T fraction) noexcept;
    ProcessSpec spec_{};
    std::vector<T> history_;
    std::size_t capacity_ = 0, write_ = 0, maximum_ = 0;
    double delay_ = 0, target_ = 0;
    std::size_t remaining_ = 0;
};
extern template class DelayLine<float>;
extern template class DelayLine<double>;
} // namespace cydsp
