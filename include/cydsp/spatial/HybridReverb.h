#pragma once
#include "cydsp/processors/Convolver.h"
#include "cydsp/spatial/FdnReverb.h"
#include <memory>
namespace cydsp {
// Prepared IR contribution plus algorithmic late room, using existing convolution
// transitions and vector/gain kernels. IR loading remains an application service.
template <Sample T> class HybridReverb {
  public:
    HybridReverb();
    ~HybridReverb();
    HybridReverb(HybridReverb&&) noexcept;
    HybridReverb& operator=(HybridReverb&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, std::span<const T> impulse,
                                 const FdnConfig& = {}, const ConvolutionConfig& = {},
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setMix(double impulseGain, double lateGain,
                                std::size_t rampFrames = 256) noexcept;
    [[nodiscard]] Status setLateParameters(const FdnParameters&,
                                           std::size_t rampFrames = 256) noexcept;
    [[nodiscard]] Status stageImpulse(std::span<const T>) noexcept; // NON real-time.
    [[nodiscard]] Status startImpulseTransition(std::size_t frames = 256) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t nominalTailFrames() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class HybridReverb<float>;
extern template class HybridReverb<double>;
} // namespace cydsp
