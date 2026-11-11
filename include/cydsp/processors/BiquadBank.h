#pragma once
#include "cydsp/math/VectorOps.h"
#include "cydsp/processors/Biquad.h"
namespace cydsp {
// One independent voice per planar channel, with independent coefficients/states.
// SIMD runs across voices. Internal frame-major tiles contain at most 64 frames.
template <Sample T>
    requires(!std::is_const_v<T>)
class BiquadBank {
  public:
    BiquadBank() = default;
    BiquadBank(const BiquadBank&) = delete;
    BiquadBank& operator=(const BiquadBank&) = delete;
    BiquadBank(BiquadBank&& other) noexcept {
        swap(other);
    }
    BiquadBank& operator=(BiquadBank&& other) noexcept {
        if (this != &other) {
            BiquadBank next(std::move(other));
            swap(next);
        }
        return *this;
    }
    [[nodiscard]] Status prepare(const ProcessSpec&, CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setCoefficients(std::size_t voice, const BiquadCoefficients&) noexcept;
    [[nodiscard]] Status setAllCoefficients(const BiquadCoefficients&) noexcept;
    void reset() noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    [[nodiscard]] std::size_t voices() const noexcept {
        return states_.size() / 2;
    }
    [[nodiscard]] CpuBackend backend() const noexcept {
        return ops_.backend();
    }

  private:
    void swap(BiquadBank&) noexcept;
    ProcessSpec spec_{};
    VectorOps ops_;
    std::vector<T> coefficients_, states_, workspace_;
    std::size_t tileFrames_ = 0;
};
extern template class BiquadBank<float>;
extern template class BiquadBank<double>;
} // namespace cydsp
