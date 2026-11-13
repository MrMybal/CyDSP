#pragma once
#include "cydsp/core/AudioBlock.h"
#include <memory>
namespace cydsp {
enum class DetectorMode { peak, rms };
enum class DynamicsMode { compressor, expander, noiseGate, limiter };
struct DynamicsParameters {
    double thresholdDb = -18, ratio = 4, kneeDb = 6, attackMs = 10, releaseMs = 100, makeupDb = 0;
    bool linkedChannels = true;
    DetectorMode detector = DetectorMode::peak;
    double holdMs = 0, hysteresisDb = 0, lookAheadMs = 0;
};
template <Sample T> class BallisticsFilter {
  public:
    BallisticsFilter();
    ~BallisticsFilter();
    BallisticsFilter(BallisticsFilter&&) noexcept;
    BallisticsFilter& operator=(BallisticsFilter&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, DetectorMode = DetectorMode::peak) noexcept;
    [[nodiscard]] Status setTimes(double attackMs, double releaseMs) noexcept;
    void reset() noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
template <Sample T> class Dynamics {
  public:
    Dynamics();
    ~Dynamics();
    Dynamics(Dynamics&&) noexcept;
    Dynamics& operator=(Dynamics&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, DynamicsMode = DynamicsMode::compressor,
                                 double maximumLookAheadMs = 20) noexcept;
    [[nodiscard]] Status setParameters(const DynamicsParameters&) noexcept;
    void reset() noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    // External detector: mono broadcast or one channel per program channel.
    // Sidechain must be disjoint from output unless it is the program block.
    [[nodiscard]] Status process(AudioBlock<const T> program, AudioBlock<const T> sidechain,
                                 AudioBlock<T>) noexcept;
    [[nodiscard]] double gainReductionDb() const noexcept;
    // Minimum detector gain including makeup in the last nonempty successful block,
    // before the limiter ceiling. Owner-serialized; no output/input division needed.
    [[nodiscard]] double minimumAppliedGain() const noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class BallisticsFilter<float>;
extern template class BallisticsFilter<double>;
extern template class Dynamics<float>;
extern template class Dynamics<double>;
} // namespace cydsp
