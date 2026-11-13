#pragma once
#include "cydsp/spectral/Stft.h"
namespace cydsp {
struct SpectralPitchParameters {
    double ratio = 1;
    bool preserveFormants = true, protectTransients = true, lockSpectralPeaks = true;
    double envelopeWidthHz = 150, transientFluxThreshold = .35;
};
// Constant-duration phase-vocoder transposition, .25..4x. Shared guide-channel
// phases preserve relative channel phase; spectral-peak locking retains local
// vertical coherence. A smoothed spectral envelope controls formant preservation;
// positive flux resets phase at attacks. This is not source separation.
// Exact delayed bypass at ratio 1. Feed 2*N zeros to drain after input ends.
template <Sample T> class SpectralPitchShifter {
  public:
    SpectralPitchShifter();
    ~SpectralPitchShifter();
    SpectralPitchShifter(SpectralPitchShifter&&) noexcept;
    SpectralPitchShifter& operator=(SpectralPitchShifter&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, const StftConfig& = {},
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setParameters(const SpectralPitchParameters&) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class SpectralPitchShifter<float>;
extern template class SpectralPitchShifter<double>;
} // namespace cydsp
