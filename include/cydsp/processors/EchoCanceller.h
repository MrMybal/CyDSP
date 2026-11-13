#pragma once
#include "cydsp/processors/AdaptiveFilter.h"
namespace cydsp {
struct EchoCancellationConfig {
    std::size_t taps = 512, referenceDelayFrames = 0;
    double initialLearningMs = 250;
};
struct EchoCancellationParameters {
    double step = .25, doubleTalkRatio = 2.5, hangoverMs = 100, minimumReferencePower = 1e-8;
    bool adaptationEnabled = true, forceAdaptation = false;
};
struct EchoCancellationMetrics {
    bool referenceActive = false, doubleTalk = false, adapting = false;
    double echoReturnLossEnhancementDb = 0, residualPower = 0;
    std::uint64_t adaptedFrames = 0, frozenFrames = 0;
};
// Mono far-end reference, 1..8 microphones, independent NLMS paths and linked
// residual/prediction-coherence double-talk gating. Delays are supplied by host;
// nonlinear loudspeakers, drift and a complete WebRTC speech pipeline are outside
// this linear AEC. reset clears paths; forceAdaptation allows deliberate relearning.
template <Sample T> class EchoCanceller {
  public:
    EchoCanceller();
    ~EchoCanceller();
    EchoCanceller(EchoCanceller&&) noexcept;
    EchoCanceller& operator=(EchoCanceller&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec& microphones, const EchoCancellationConfig& = {},
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setParameters(const EchoCancellationParameters&) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T> farEnd, AudioBlock<const T> microphones,
                                 AudioBlock<T> residual) noexcept;
    [[nodiscard]] EchoCancellationMetrics metrics() const noexcept;
    [[nodiscard]] std::span<const double> coefficients(std::size_t channel) const noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class EchoCanceller<float>;
extern template class EchoCanceller<double>;
} // namespace cydsp
