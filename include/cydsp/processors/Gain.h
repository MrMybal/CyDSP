#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/math/VectorOps.h"
namespace cydsp {
template <Sample T>
    requires(!std::is_const_v<T>)
class Gain {
  public:
    [[nodiscard]] Status prepare(const ProcessSpec&, CpuBackend = CpuBackend::automatic) noexcept;
    void reset() noexcept;
    // Ramps count frames, shared across channels. First frame advances one step.
    [[nodiscard]] Status setGain(T linearGain, std::size_t rampFrames = 0) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    [[nodiscard]] CpuBackend backend() const noexcept {
        return ops_.backend();
    }
    [[nodiscard]] T currentGain() const noexcept {
        return static_cast<T>(current_);
    }

  private:
    ProcessSpec spec_{};
    VectorOps ops_{};
    double current_ = 1.0, target_ = 1.0;
    std::size_t remaining_ = 0;
    bool prepared_ = false;
};
extern template class Gain<float>;
extern template class Gain<double>;
} // namespace cydsp
