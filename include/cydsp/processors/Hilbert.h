#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include <memory>
namespace cydsp {
// Windowed ideal Hilbert FIR; I is the matching delayed real input and Q its
// quadrature. Approximation accuracy decreases near DC/Nyquist. All blocks disjoint.
template <Sample T> class HilbertTransform {
  public:
    HilbertTransform();
    ~HilbertTransform();
    HilbertTransform(HilbertTransform&&) noexcept;
    HilbertTransform& operator=(HilbertTransform&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, std::size_t taps = 129,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T> inPhase,
                                 AudioBlock<T> quadrature) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class HilbertTransform<float>;
extern template class HilbertTransform<double>;
} // namespace cydsp
