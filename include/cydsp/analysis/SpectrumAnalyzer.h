#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include "cydsp/math/Window.h"
#include <complex>
#include <cstdint>
#include <memory>
namespace cydsp {
struct SpectrumConfig {
    std::size_t fftFrames = 2048, hopFrames = 512;
    WindowType window = WindowType::hann;
};
struct SpectralFeatures {
    double rms = 0, centroidHz = 0, rolloffHz = 0, flatness = 0;
    double positiveFlux = 0, dominantHz = 0;
};
// Streaming overlapped real FFT. Latest complete window only; copied/snapshot
// delivery to other threads is the caller's responsibility. Internal FFT is double.
template <Sample T> class SpectrumAnalyzer {
  public:
    SpectrumAnalyzer();
    ~SpectrumAnalyzer();
    SpectrumAnalyzer(SpectrumAnalyzer&&) noexcept;
    SpectrumAnalyzer& operator=(SpectrumAnalyzer&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, const SpectrumConfig& = {},
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>) noexcept;
    void reset() noexcept;
    [[nodiscard]] bool hasFrame() const noexcept;
    [[nodiscard]] std::uint64_t frameCount() const noexcept;
    [[nodiscard]] std::size_t bins() const noexcept;
    [[nodiscard]] double binFrequency(std::size_t bin) const noexcept;
    // One-sided peak amplitude and power spectral density (units squared / Hz).
    [[nodiscard]] std::span<const double> magnitudes(std::size_t channel) const noexcept;
    [[nodiscard]] std::span<const double> powerDensity(std::size_t channel) const noexcept;
    [[nodiscard]] std::span<const std::complex<double>>
    complexSpectrum(std::size_t channel) const noexcept;
    [[nodiscard]] SpectralFeatures features(std::size_t channel) const noexcept;
    [[nodiscard]] CpuBackend backend() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
enum class SpectralBandScale { mel, logarithmic };
struct SpectralBandConfig {
    std::size_t bands = 40;
    double minimumHz = 20, maximumHz = 0; // Zero maximum means Nyquist.
    SpectralBandScale scale = SpectralBandScale::mel;
};
// Prepared sparse triangular filters. Produces weighted means, in input units;
// does not reinterpret magnitude as power or implement an MFCC transform.
class SpectralBands {
  public:
    SpectralBands();
    ~SpectralBands();
    SpectralBands(SpectralBands&&) noexcept;
    SpectralBands& operator=(SpectralBands&&) noexcept;
    [[nodiscard]] Status prepare(double sampleRate, std::size_t fftFrames,
                                 const SpectralBandConfig& = {},
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status process(std::span<const double> bins,
                                 std::span<double> bands) const noexcept;
    [[nodiscard]] std::span<const double> centerFrequencies() const noexcept;
    [[nodiscard]] CpuBackend backend() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class SpectrumAnalyzer<float>;
extern template class SpectrumAnalyzer<double>;
} // namespace cydsp
