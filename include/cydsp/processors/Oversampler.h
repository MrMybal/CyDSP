#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include <memory>
namespace cydsp {
enum class OversamplingMode { fir, halfBandFir, iir };
// FIR modes have fixed linear-phase latency; IIR has frequency-dependent group delay.
// Linear-phase anti-imaging/anti-alias FIR pair; caller processes the returned
// high-rate block between processUp and processDown. Buffers are retained.
template <Sample T> class Oversampler {
  public:
    Oversampler();
    ~Oversampler();
    Oversampler(Oversampler&&) noexcept;
    Oversampler& operator=(Oversampler&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, unsigned factor = 2,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, unsigned factor, OversamplingMode,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] bool linearPhase() const noexcept;
    [[nodiscard]] double latencyAtDcFrames() const noexcept;
    [[nodiscard]] Status processUp(AudioBlock<const T>, AudioBlock<T>& highRateBlock) noexcept;
    [[nodiscard]] Status processDown(AudioBlock<T> output) noexcept;
    void reset() noexcept;
    [[nodiscard]] unsigned factor() const noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class Oversampler<float>;
extern template class Oversampler<double>;
} // namespace cydsp
