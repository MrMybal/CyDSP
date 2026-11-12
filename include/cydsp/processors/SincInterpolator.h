#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include <memory>
namespace cydsp {
// Prepared centered sinc reconstruction for random access/cache jobs. No added timeline latency.
template <Sample T> class SincInterpolator {
  public:
    SincInterpolator();
    ~SincInterpolator();
    SincInterpolator(SincInterpolator&&) noexcept;
    SincInterpolator& operator=(SincInterpolator&&) noexcept;
    Status prepare(const ProcessSpec& output, double inputRate, std::size_t taps = 64,
                   std::size_t phases = 1024, CpuBackend = CpuBackend::automatic) noexcept;
    // Source includes taps/2-1 left and taps/2 right context (zero-pad file boundaries).
    Status process(AudioBlock<const T> source, double firstSourceFrame,
                   AudioBlock<T> output) noexcept;
    // Positions are prepared/caller-owned, increasing and no faster than the
    // inputRate/outputRate maximum selected at prepare (conservative anti-aliasing).
    Status processPositions(AudioBlock<const T> source, std::span<const double> positions,
                            AudioBlock<T> output) noexcept;
    [[nodiscard]] CpuBackend backend() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class SincInterpolator<float>;
extern template class SincInterpolator<double>;
} // namespace cydsp
