#pragma once
#include "cydsp/core/AudioBlock.h"
#include <array>
#include <memory>
namespace cydsp {
// Complementary one-pole split: low=L1(x), mid=L2(x)-L1(x), high=x-L2(x).
// Unit gains reconstruct x exactly, zero added latency. Bands overlap and are not
// brick-wall crossovers. Used for low-cost acoustic colouring and feedback damping.
template <Sample T> class BandGain {
  public:
    BandGain();
    ~BandGain();
    BandGain(BandGain&&) noexcept;
    BandGain& operator=(BandGain&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, double lowCrossoverHz = 400,
                                 double highCrossoverHz = 4000) noexcept;
    [[nodiscard]] Status setGains(const std::array<double, 3>&,
                                  std::size_t rampFrames = 0) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class BandGain<float>;
extern template class BandGain<double>;
} // namespace cydsp
