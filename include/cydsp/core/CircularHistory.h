#pragma once
#include "cydsp/core/AudioBlock.h"
#include <vector>
namespace cydsp {
enum class HistoryInterpolation { nearest, linear, lagrange3, catmullRom };
// Owner-serialized sample history. Age zero is the last frame pushed; unavailable
// past/future samples read as zero. Readers do not alter the write head.
template <Sample T> class CircularHistory {
  public:
    CircularHistory() = default;
    CircularHistory(const CircularHistory&) = delete;
    CircularHistory& operator=(const CircularHistory&) = delete;
    CircularHistory(CircularHistory&&) noexcept;
    CircularHistory& operator=(CircularHistory&&) noexcept;
    [[nodiscard]] Status prepare(std::size_t channels, std::size_t historyFrames) noexcept;
    [[nodiscard]] Status pushFrame(std::span<const T>) noexcept;
    [[nodiscard]] Status push(AudioBlock<const T>) noexcept;
    [[nodiscard]] T read(std::size_t channel, double ageFrames,
                         HistoryInterpolation = HistoryInterpolation::linear) const noexcept;
    // Batched integer ages: out[i] = sample(firstAge+i), missing samples zero.
    // Double conversion is performed once per sample, without per-tap modulo.
    [[nodiscard]] Status readWindow(std::size_t channel, std::ptrdiff_t firstAge,
                                    std::span<double> output) const noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t channels() const noexcept {
        return channels_;
    }
    [[nodiscard]] std::size_t capacity() const noexcept {
        return capacity_;
    }
    [[nodiscard]] std::size_t availableFrames() const noexcept {
        return filled_;
    }

  private:
    void swap(CircularHistory&) noexcept;
    [[nodiscard]] double integer(std::size_t channel, std::ptrdiff_t age) const noexcept;
    void pushUnchecked(std::span<const T>) noexcept;
    std::vector<T> storage_;
    std::size_t channels_ = 0, capacity_ = 0, head_ = 0, filled_ = 0;
};
extern template class CircularHistory<float>;
extern template class CircularHistory<double>;
} // namespace cydsp
