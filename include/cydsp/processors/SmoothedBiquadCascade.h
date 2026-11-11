#pragma once
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/processors/BiquadBank.h"
#include <array>
namespace cydsp {
// Preallocated full-EQ output transitions; at most one following target is coalesced.
template <Sample T>
    requires(!std::is_const_v<T>)
class SmoothedBiquadCascade {
  public:
    SmoothedBiquadCascade() = default;
    SmoothedBiquadCascade(const SmoothedBiquadCascade&) = delete;
    SmoothedBiquadCascade& operator=(const SmoothedBiquadCascade&) = delete;
    SmoothedBiquadCascade(SmoothedBiquadCascade&& other) noexcept {
        swap(other);
    }
    SmoothedBiquadCascade& operator=(SmoothedBiquadCascade&& other) noexcept {
        if (this != &other) {
            SmoothedBiquadCascade next(std::move(other));
            swap(next);
        }
        return *this;
    }
    [[nodiscard]] Status prepare(const ProcessSpec&, std::size_t maxSections = 16,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setCoefficients(std::span<const BiquadCoefficients>,
                                         std::size_t transitionFrames = 128) noexcept;
    void reset() noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    [[nodiscard]] std::size_t maxSections() const noexcept {
        return maximum_;
    }
    [[nodiscard]] std::size_t targetSections() const noexcept {
        return targetCount_;
    }
    [[nodiscard]] bool isTransitioning() const noexcept {
        return remaining_ || queued_;
    }
    [[nodiscard]] std::size_t transitionRemainingFrames() const noexcept {
        return remaining_;
    }
    [[nodiscard]] CpuBackend backend() const noexcept {
        return ops_.backend();
    }

  private:
    void swap(SmoothedBiquadCascade&) noexcept;
    void configure(std::size_t, bool) noexcept;
    void run(std::size_t, AudioBlock<const T>, AudioBlock<T>) noexcept;
    ProcessSpec spec_{};
    std::array<std::vector<BiquadBank<T>>, 2> banks_;
    std::array<AudioBuffer<T>, 2> scratch_;
    std::array<std::array<BiquadCoefficients, 64>, 2> bankCoefficients_{};
    std::array<BiquadCoefficients, 64> target_{};
    std::array<std::size_t, 2> counts_{};
    VectorOps ops_;
    std::size_t active_ = 0, maximum_ = 0, tile_ = 0, targetCount_ = 0, remaining_ = 0,
                queuedFrames_ = 0;
    double blend_ = 0;
    bool queued_ = false;
};
extern template class SmoothedBiquadCascade<float>;
extern template class SmoothedBiquadCascade<double>;
} // namespace cydsp
