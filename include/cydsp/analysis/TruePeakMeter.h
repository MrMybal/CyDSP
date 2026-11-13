#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include <memory>
namespace cydsp {
// Windowed-sinc intersample peak estimate, 4x/8x, double accumulation. Not a
// compliance certification. Peak holds until reset; flush includes the FIR tail.
template <Sample T> class TruePeakMeter {
  public:
    TruePeakMeter();
    ~TruePeakMeter();
    TruePeakMeter(TruePeakMeter&&) noexcept;
    TruePeakMeter& operator=(TruePeakMeter&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, unsigned oversampling = 4,
                                 std::size_t tapsPerPhase = 64,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>) noexcept;
    void flush() noexcept; // End-of-stream; reset before starting a new stream.
    void reset() noexcept;
    [[nodiscard]] double samplePeak(std::size_t channel) const noexcept;
    [[nodiscard]] double truePeak(std::size_t channel) const noexcept;
    [[nodiscard]] double truePeakDbtp(std::size_t channel) const noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept;
    [[nodiscard]] CpuBackend backend() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class TruePeakMeter<float>;
extern template class TruePeakMeter<double>;
} // namespace cydsp
