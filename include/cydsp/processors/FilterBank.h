#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include <memory>
namespace cydsp {
// Complementary linear-phase FIR bands. Output channel = band*inputChannels+channel.
// Their sum reconstructs the input delayed (taps-1)/2 samples, without crossover
// phase mismatch. Crossovers are fixed during processing and prepared in Hz.
template <Sample T> class FirFilterBank {
  public:
    FirFilterBank();
    ~FirFilterBank();
    FirFilterBank(FirFilterBank&&) noexcept;
    FirFilterBank& operator=(FirFilterBank&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, std::span<const double> crossoversHz,
                                 std::size_t taps = 129,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T> bands) noexcept;
    [[nodiscard]] Status sumBands(AudioBlock<const T> bands, AudioBlock<T> output) const noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t bands() const noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept;
    [[nodiscard]] CpuBackend backend() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class FirFilterBank<float>;
extern template class FirFilterBank<double>;
} // namespace cydsp
