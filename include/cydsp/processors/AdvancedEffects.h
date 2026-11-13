#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include <memory>
#include <span>
namespace cydsp {
enum class DelayEffectMode { echo, flanger, vibrato, wowFlutter };
struct EchoTap {
    double delayMs = 250, gain = 1;
};
struct DelayEffectParameters {
    DelayEffectMode mode = DelayEffectMode::echo;
    double delayMs = 250, depthMs = 0, rateHz = .5, feedback = .3, lowPassHz = 10000,
           feedbackDrive = 1, duckDb = 0, wet = .5, dry = 1;
    bool pingPong = false;
};
// Shared circular history: up to 16 feed-forward taps plus a filtered/saturated
// feedback read. Delay modulation changes pitch intentionally. Wet tap delays
// and the feedback read are bounded by the prepared capacity.
template <Sample T> class DelayEffect {
  public:
    DelayEffect();
    ~DelayEffect();
    DelayEffect(DelayEffect&&) noexcept;
    DelayEffect& operator=(DelayEffect&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, double maximumDelayMs = 2000) noexcept;
    [[nodiscard]] Status setParameters(const DelayEffectParameters&) noexcept;
    [[nodiscard]] Status setTaps(std::span<const EchoTap>) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
struct AmplitudeModulationParameters {
    double rateHz = 4, depth = .5;
    bool autopan = false;
};
template <Sample T> class AmplitudeModulator {
  public:
    [[nodiscard]] Status prepare(const ProcessSpec&) noexcept;
    [[nodiscard]] Status setParameters(const AmplitudeModulationParameters&) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept {
        phase_ = 0;
    }

  private:
    ProcessSpec spec_{};
    AmplitudeModulationParameters p_;
    double phase_ = 0;
    bool prepared_ = false;
};
struct PlateParameters {
    double decay = .7, diffusion = .7, dampingHz = 6000, wet = .5, dry = 1, shimmer = 0;
    bool freeze = false;
};
// Original realization of the Dattorro figure-eight topology: input allpasses,
// two cross-fed allpass/delay/damping/allpass/delay tanks. Scaled CyDSP tuning;
// shimmer is a dual-head octave-up injection in the feedback path.
template <Sample T> class PlateReverb {
  public:
    PlateReverb();
    ~PlateReverb();
    PlateReverb(PlateReverb&&) noexcept;
    PlateReverb& operator=(PlateReverb&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, double size = 1) noexcept;
    [[nodiscard]] Status setParameters(const PlateParameters&) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
struct SpringParameters {
    double decaySeconds = 2, dispersion = .6, dampingHz = 5000, wet = .5, dry = 1;
};
// Dispersive feedback waveguide spring: frequency-dependent allpass propagation,
// damped round trips, independent left/right tap phases. Not a mechanical FEM.
template <Sample T> class SpringReverb {
  public:
    SpringReverb();
    ~SpringReverb();
    SpringReverb(SpringReverb&&) noexcept;
    SpringReverb& operator=(SpringReverb&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, double lengthSeconds = .035) noexcept;
    [[nodiscard]] Status setParameters(const SpringParameters&) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class DelayEffect<float>;
extern template class DelayEffect<double>;
extern template class AmplitudeModulator<float>;
extern template class AmplitudeModulator<double>;
extern template class PlateReverb<float>;
extern template class PlateReverb<double>;
extern template class SpringReverb<float>;
extern template class SpringReverb<double>;
} // namespace cydsp
