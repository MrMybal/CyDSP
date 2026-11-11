#pragma once
#include "cydsp/core/AudioBlock.h"
#include <utility>
#include <vector>
namespace cydsp {
enum class BiquadType { lowPass, highPass, bandPass, notch, allPass };
// a0 normalized to 1; feedback is -a1*y[n-1] - a2*y[n-2].
struct BiquadCoefficients {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    [[nodiscard]] constexpr bool isStable() const noexcept {
        return isFinite(b0) && isFinite(b1) && isFinite(b2) && isFinite(a1) && isFinite(a2) &&
               a2 > -1 && a2 < 1 && 1 + a1 + a2 > 0 && 1 - a1 + a2 > 0;
    }
};
[[nodiscard]] Status designBiquad(BiquadType, double sampleRate, double frequency, double q,
                                  BiquadCoefficients& result) noexcept;
// EQ designs: finite gain in [-120,120] dB. Shelf slope is in (0,1].
[[nodiscard]] Status designPeakingBiquad(double sampleRate, double frequency, double q,
                                         double gainDb, BiquadCoefficients&) noexcept;
[[nodiscard]] Status designShelfBiquad(bool highShelf, double sampleRate, double frequency,
                                       double slope, double gainDb, BiquadCoefficients&) noexcept;
template <Sample T>
    requires(!std::is_const_v<T>)
class Biquad {
  public:
    Biquad() = default;
    Biquad(const Biquad&) = delete;
    Biquad& operator=(const Biquad&) = delete;
    Biquad(Biquad&& other) noexcept {
        swap(other);
    }
    Biquad& operator=(Biquad&& other) noexcept {
        if (this != &other) {
            Biquad next(std::move(other));
            swap(next);
        }
        return *this;
    }
    [[nodiscard]] Status prepare(const ProcessSpec&) noexcept;
    void reset() noexcept;
    // Processing owner only, between blocks. No implicit parameter smoothing.
    [[nodiscard]] Status setCoefficients(const BiquadCoefficients&) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;

  private:
    void swap(Biquad& other) noexcept {
        states_.swap(other.states_);
        std::swap(spec_, other.spec_);
        std::swap(coefficients_, other.coefficients_);
        std::swap(prepared_, other.prepared_);
    }
    struct State {
        T z1 = 0, z2 = 0;
    };
    std::vector<State> states_;
    ProcessSpec spec_{};
    BiquadCoefficients coefficients_{};
    bool prepared_ = false;
};
extern template class Biquad<float>;
extern template class Biquad<double>;
} // namespace cydsp
