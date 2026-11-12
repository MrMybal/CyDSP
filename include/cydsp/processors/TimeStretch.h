#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include <memory>
namespace cydsp {
// Link CyDSP::Stretch. Input/output block lengths set the time ratio; caller owns scheduling.
template <Sample T> class TimeStretch {
  public:
    TimeStretch();
    ~TimeStretch();
    TimeStretch(TimeStretch&&) noexcept;
    TimeStretch& operator=(TimeStretch&&) noexcept;
    Status prepare(const ProcessSpec&, std::size_t maximumOutputFrames, bool cheaper = false,
                   bool splitComputation = true, CpuBackend = CpuBackend::automatic) noexcept;
    Status setPitchSemitones(double semitones, double tonalityLimitHz = 0) noexcept;
    Status setFormant(double multiplier, bool compensatePitch = false,
                      double fundamentalHz = 0) noexcept;
    // All overlap is rejected. Zero output with nonzero input is rejected to prevent discarded
    // input.
    Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    // Prepared seek: history contains inputLatency() past samples, followed by
    // playbackRate*outputLatency() future samples. Next input starts after history.
    // Next output is aligned to the target between past/future. No allocation.
    Status prime(AudioBlock<const T> history) noexcept;
    // Feed inputLatency() silence frames at the chosen ratio, then flush outputLatency() frames.
    // Flush starts end-of-stream; reset is required before new input.
    Status flush(AudioBlock<T>) noexcept;
    void reset() noexcept;
    [[nodiscard]] CpuBackend backend() const noexcept;
    [[nodiscard]] std::size_t inputLatency() const noexcept;
    [[nodiscard]] std::size_t outputLatency() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class TimeStretch<float>;
extern template class TimeStretch<double>;
} // namespace cydsp
