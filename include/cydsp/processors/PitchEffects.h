#pragma once
#include "cydsp/analysis/MusicalAnalyzer.h"
#include "cydsp/spectral/SpectralPitch.h"
namespace cydsp {
struct HarmonyVoice {
    double semitones = 0, gain = 1, pan = 0, delayMs = 0;
};
struct PitchCorrectionParameters {
    bool enabled = false;
    double targetHz = 0, referenceA4Hz = 440, retuneMs = 20;
    unsigned scalePitchClasses = 0xfff;
};
// Dual complementary-window delay heads, 32-tap bandlimited history reads.
// Preserves duration and supports up to 8 harmony/doubling voices. Correction is
// monophonic YIN-driven; no formant lock. Algorithmic window is 40 ms; the read
// delay varies over that window (16-frame guard + optional voice delay), so it
// is not represented as one exact integer PDC latency.
template <Sample T> class PitchEffects {
  public:
    PitchEffects();
    ~PitchEffects();
    PitchEffects(PitchEffects&&) noexcept;
    PitchEffects& operator=(PitchEffects&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, std::size_t maximumVoices = 8,
                                 const MusicalAnalysisConfig& = {},
                                 CpuBackend = CpuBackend::automatic) noexcept;
    // Alternative fixed-latency mode using spectral formants, linked stereo phases
    // and attack protection. Same voices/correction controls; extra voice delays
    // are post-shift delays. latencyFrames reports the common FFT latency only.
    [[nodiscard]] Status prepareSpectral(const ProcessSpec&, std::size_t maximumVoices = 8,
                                         const StftConfig& = {}, const MusicalAnalysisConfig& = {},
                                         CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept;
    [[nodiscard]] Status setVoices(std::span<const HarmonyVoice>) noexcept;
    [[nodiscard]] Status setCorrection(const PitchCorrectionParameters&) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;
    [[nodiscard]] double detectedPitchHz() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class PitchEffects<float>;
extern template class PitchEffects<double>;
} // namespace cydsp
