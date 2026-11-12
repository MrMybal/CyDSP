#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include <memory>
namespace cydsp {
// Prepared immutable harmonic mip bank; cycle must have a power-of-two length.
// Processing borrows the table; serialize preparation/move/destruction with readers.
template <Sample T> class Wavetable {
  public:
    Wavetable();
    ~Wavetable();
    Wavetable(Wavetable&&) noexcept;
    Wavetable& operator=(Wavetable&&) noexcept;
    Status prepare(std::span<const T> cycle, CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] T value(double phase, double frequency, double sampleRate) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::size_t levels() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
template <Sample T> class WavetableOscillator {
  public:
    Status prepare(const ProcessSpec&, const Wavetable<T>&) noexcept;
    Status setFrequency(double) noexcept;
    void reset(double phase = 0) noexcept;
    T next() noexcept;
    Status process(AudioBlock<T>, bool add = false) noexcept;

  private:
    ProcessSpec spec_{};
    const Wavetable<T>* table_ = nullptr;
    double frequency_ = 440, phase_ = 0;
};
extern template class Wavetable<float>;
extern template class Wavetable<double>;
extern template class WavetableOscillator<float>;
extern template class WavetableOscillator<double>;
} // namespace cydsp
