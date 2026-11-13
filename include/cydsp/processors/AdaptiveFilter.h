#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include <cstdint>
#include <memory>
#include <span>
namespace cydsp {
enum class AdaptiveAlgorithm { lms, nlms };
struct AdaptiveParameters {
    double step = .1, leak = 0, regularization = 1e-6, coefficientLimit = 100;
    bool adapt = true;
};
// Independent FIR identification/cancellation per channel. desired-predicted is
// the residual output. LMS step depends on reference power; NLMS normalizes it.
// A coefficient guard bounds divergence and exposes the number of clipped updates.
template <Sample T> class AdaptiveFilter {
  public:
    AdaptiveFilter();
    ~AdaptiveFilter();
    AdaptiveFilter(AdaptiveFilter&&) noexcept;
    AdaptiveFilter& operator=(AdaptiveFilter&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, std::size_t taps = 128,
                                 AdaptiveAlgorithm = AdaptiveAlgorithm::nlms,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setParameters(const AdaptiveParameters&) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T> reference, AudioBlock<const T> desired,
                                 AudioBlock<T> predicted, AudioBlock<T> residual) noexcept;
    [[nodiscard]] std::span<const double> coefficients(std::size_t channel) const noexcept;
    [[nodiscard]] std::uint64_t clippedUpdates() const noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class AdaptiveFilter<float>;
extern template class AdaptiveFilter<double>;
} // namespace cydsp
