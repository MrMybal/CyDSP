#pragma once
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/processors/BiquadBank.h"
#include <array>
namespace cydsp {
// Crossfades outputs of two stable filter banks; coefficients are not interpolated.
// While a transition runs, one queued target is retained: the latest request wins.
template <Sample T>
    requires(!std::is_const_v<T>)
class SmoothedBiquad {
  public:
    SmoothedBiquad() = default;
    SmoothedBiquad(const SmoothedBiquad&) = delete;
    SmoothedBiquad& operator=(const SmoothedBiquad&) = delete;
    SmoothedBiquad(SmoothedBiquad&& other) noexcept {
        swap(other);
    }
    SmoothedBiquad& operator=(SmoothedBiquad&& other) noexcept {
        if (this != &other) {
            SmoothedBiquad next(std::move(other));
            swap(next);
        }
        return *this;
    }
    [[nodiscard]] Status prepare(const ProcessSpec&, CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setCoefficients(const BiquadCoefficients&,
                                         std::size_t transitionFrames = 64) noexcept;
    void reset() noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    [[nodiscard]] bool isTransitioning() const noexcept {
        return remaining_ != 0 || queued_;
    }
    [[nodiscard]] bool hasQueuedTransition() const noexcept {
        return queued_;
    }
    [[nodiscard]] std::size_t transitionRemainingFrames() const noexcept {
        return remaining_;
    }
    [[nodiscard]] const BiquadCoefficients& targetCoefficients() const noexcept {
        return target_;
    }
    [[nodiscard]] CpuBackend backend() const noexcept {
        return banks_[0].backend();
    }

  private:
    void swap(SmoothedBiquad&) noexcept;
    void beginTransition(const BiquadCoefficients&, std::size_t) noexcept;
    ProcessSpec spec_{};
    std::array<BiquadBank<T>, 2> banks_;
    std::array<AudioBuffer<T>, 2> scratch_;
    BiquadCoefficients target_{};
    std::array<BiquadCoefficients, 2> bankCoefficients_{};
    std::size_t active_ = 0, tileFrames_ = 0, remaining_ = 0, queuedFrames_ = 0;
    double blend_ = 0;
    bool queued_ = false;
};
extern template class SmoothedBiquad<float>;
extern template class SmoothedBiquad<double>;
} // namespace cydsp
