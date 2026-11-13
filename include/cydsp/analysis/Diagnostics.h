#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include "cydsp/processors/Biquad.h"
#include <complex>
#include <cstdint>
#include <memory>
namespace cydsp {
struct FilterResponse {
    std::complex<double> gain{1, 0};
    double phaseRadians = 0, groupDelayFrames = 0;
};
[[nodiscard]] Status firResponse(std::span<const double>, double sampleRate, double frequencyHz,
                                 FilterResponse&) noexcept;
[[nodiscard]] Status iirResponse(std::span<const BiquadCoefficients>, double sampleRate,
                                 double frequencyHz, FilterResponse&) noexcept;
struct TransferBin {
    std::complex<double> gain{};
    double phaseRadians = 0, groupDelayFrames = 0, coherence = 0;
};
// Paired Welch cross spectra, H1=conj(X)*Y / |X|²; bounded exponential averaging
// after the initial ensemble. Phase is unwrapped across frequency before delay.
template <Sample T> class TransferAnalyzer {
  public:
    TransferAnalyzer();
    ~TransferAnalyzer();
    TransferAnalyzer(TransferAnalyzer&&) noexcept;
    TransferAnalyzer& operator=(TransferAnalyzer&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, std::size_t fftFrames = 2048,
                                 std::size_t hopFrames = 512, std::size_t averages = 32,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T> reference,
                                 AudioBlock<const T> measured) noexcept;
    [[nodiscard]] TransferBin bin(std::size_t channel, std::size_t index) const noexcept;
    [[nodiscard]] std::size_t bins() const noexcept;
    [[nodiscard]] std::uint64_t analyzedFrames() const noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
struct DistortionReport {
    double fundamentalRms = 0, harmonicsRms = 0, noiseRms = 0, dc = 0, thd = 0, thdPlusNoise = 0;
    unsigned measuredHarmonics = 0;
};
// Non-RT coherent/non-coherent sinusoidal least-squares fit at known f0.
// DC and harmonics up to Nyquist are fitted together. Ratios are linear RMS.
template <Sample T>
[[nodiscard]] Status measureDistortion(std::span<const T>, double sampleRate, double fundamentalHz,
                                       DistortionReport&, unsigned harmonics = 10) noexcept;
enum class MeasurementSignal { sine, whiteNoise, impulse, linearSweep, logSweep };
struct MeasurementSignalParameters {
    MeasurementSignal kind = MeasurementSignal::logSweep;
    double startHz = 20, endHz = 20000, durationSeconds = 5, amplitude = .5, fadeSeconds = .01;
};
template <Sample T> class MeasurementGenerator {
  public:
    [[nodiscard]] Status prepare(const ProcessSpec&) noexcept;
    [[nodiscard]] Status setParameters(const MeasurementSignalParameters&) noexcept;
    [[nodiscard]] Status process(AudioBlock<T>) noexcept;
    void reset(std::uint64_t seed = 1) noexcept;

  private:
    ProcessSpec spec_{};
    MeasurementSignalParameters p_;
    std::uint64_t position_ = 0, random_ = 1;
    bool prepared_ = false;
};
// Preparation/offline tools. Sweep inversion uses reverse-time exponential
// amplitude compensation, normalized to unit self-convolution peak. Frequency
// deconvolution uses zero-padding and Tikhonov regularization in power dB.
template <Sample T>
[[nodiscard]] Status inverseLogSweep(std::span<const T> sweep, double startHz, double endHz,
                                     std::span<T> inverse) noexcept;
template <Sample T>
[[nodiscard]] Status deconvolveSweep(std::span<const T> recording, std::span<const T> excitation,
                                     std::span<T> impulse, double regularizationDb = -120,
                                     CpuBackend = CpuBackend::automatic) noexcept;
extern template class TransferAnalyzer<float>;
extern template class TransferAnalyzer<double>;
extern template class MeasurementGenerator<float>;
extern template class MeasurementGenerator<double>;
extern template Status measureDistortion<float>(std::span<const float>, double, double,
                                                DistortionReport&, unsigned) noexcept;
extern template Status measureDistortion<double>(std::span<const double>, double, double,
                                                 DistortionReport&, unsigned) noexcept;
extern template Status inverseLogSweep<float>(std::span<const float>, double, double,
                                              std::span<float>) noexcept;
extern template Status inverseLogSweep<double>(std::span<const double>, double, double,
                                               std::span<double>) noexcept;
extern template Status deconvolveSweep<float>(std::span<const float>, std::span<const float>,
                                              std::span<float>, double, CpuBackend) noexcept;
extern template Status deconvolveSweep<double>(std::span<const double>, std::span<const double>,
                                               std::span<double>, double, CpuBackend) noexcept;
} // namespace cydsp
