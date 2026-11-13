#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include "cydsp/math/Window.h"
#include <complex>
#include <cstdint>
#include <memory>
namespace cydsp {
struct StftConfig {
    std::size_t fftFrames = 1024, hopFrames = 256;
    WindowType window = WindowType::hann;
};
// Causal weighted overlap-add, with zero-padded startup and N-frame latency.
// A null callback is exact delayed reconstruction (within FFT rounding).
// The callback must be noexcept, allocation-free, owner-serialized and leave bins
// finite. DC/Nyquist imaginary parts are discarded after callback processing.
template <Sample T> class Stft {
  public:
    using Processor = void (*)(void*, std::size_t channel,
                               std::span<std::complex<T>> bins) noexcept;
    Stft();
    ~Stft();
    Stft(Stft&&) noexcept;
    Stft& operator=(Stft&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, const StftConfig& = {},
                                 CpuBackend = CpuBackend::automatic) noexcept;
    void setProcessor(Processor, void* context = nullptr) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    // Feeds zeros for 2*N frames, then zero-fills additional output. End-of-stream
    // is returned once the prepared STFT tail has drained; reset before new input.
    [[nodiscard]] Status flush(AudioBlock<T>) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept;
    [[nodiscard]] std::size_t tailFrames() const noexcept;
    [[nodiscard]] std::size_t bins() const noexcept;
    [[nodiscard]] std::uint64_t frameCount() const noexcept;
    [[nodiscard]] CpuBackend backend() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class Stft<float>;
extern template class Stft<double>;
} // namespace cydsp
