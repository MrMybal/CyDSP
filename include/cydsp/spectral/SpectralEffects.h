#pragma once
#include "cydsp/spectral/Stft.h"
#include <memory>
namespace cydsp {
enum class SpectralEffectMode { bypass, freeze, vocoder, morph, filter };
struct SpectralEffectParameters {
    SpectralEffectMode mode = SpectralEffectMode::bypass;
    double mix = 1;
    std::size_t envelopeRadiusBins = 8;
};
// Synchronous two-bus STFT: a carrier and modulator are captured at the same
// analysis instant. Freeze retains magnitudes and measured phase increments.
template <Sample T> class SpectralEffects {
  public:
    SpectralEffects();
    ~SpectralEffects();
    SpectralEffects(SpectralEffects&&) noexcept;
    SpectralEffects& operator=(SpectralEffects&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, const StftConfig& = {},
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setParameters(const SpectralEffectParameters&) noexcept;
    [[nodiscard]] Status setBinGains(std::span<const double>) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<const T> modulator,
                                 AudioBlock<T>) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
template <Sample T> class FrequencyShifter {
  public:
    FrequencyShifter();
    ~FrequencyShifter();
    FrequencyShifter(FrequencyShifter&&) noexcept;
    FrequencyShifter& operator=(FrequencyShifter&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, std::size_t hilbertTaps = 129,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setFrequency(double hz) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class SpectralEffects<float>;
extern template class SpectralEffects<double>;
extern template class FrequencyShifter<float>;
extern template class FrequencyShifter<double>;
} // namespace cydsp
