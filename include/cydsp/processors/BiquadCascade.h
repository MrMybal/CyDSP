#pragma once
#include "cydsp/math/VectorOps.h"
#include "cydsp/processors/Biquad.h"
#include <utility>
namespace cydsp {
// Up to 64 sections. Empty cascade is a checked bypass; parameter edits are not smoothed.
template <Sample T>
    requires(!std::is_const_v<T>)
class BiquadCascade {
  public:
    BiquadCascade() = default;
    BiquadCascade(const BiquadCascade&) = delete;
    BiquadCascade& operator=(const BiquadCascade&) = delete;
    BiquadCascade(BiquadCascade&& other) noexcept {
        swap(other);
    }
    BiquadCascade& operator=(BiquadCascade&& other) noexcept {
        if (this != &other) {
            BiquadCascade next(std::move(other));
            swap(next);
        }
        return *this;
    }
    [[nodiscard]] Status prepare(const ProcessSpec&, std::span<const BiquadCoefficients>) noexcept;
    [[nodiscard]] Status setSection(std::size_t, const BiquadCoefficients&) noexcept;
    void reset() noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    [[nodiscard]] std::size_t sections() const noexcept {
        return sections_.size();
    }

  private:
    void swap(BiquadCascade&) noexcept;
    ProcessSpec spec_{};
    std::vector<Biquad<T>> sections_;
    VectorOps ops_;
    bool prepared_ = false;
};
extern template class BiquadCascade<float>;
extern template class BiquadCascade<double>;
} // namespace cydsp
