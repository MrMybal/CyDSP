#pragma once
#include "cydsp/core/Cpu.h"
#include "cydsp/processors/FilterDesign.h"
namespace cydsp {
struct IirBandDesign {
    IirDesign prototype;
    double lowHz = 500, highHz = 2000;
    bool bandStop = false;
};
// An order-N lowpass prototype becomes an order-2N band filter (N SOS sections).
// Low/high edges are prewarped separately. All output is committed transactionally.
[[nodiscard]] Status designBandIir(const IirBandDesign&, std::span<BiquadCoefficients>,
                                   std::size_t& count) noexcept;
struct FirEqPoint {
    double frequencyHz = 0, gainDb = 0;
};
enum class FirPhase { linear, minimum };
// Preparation-only frequency-sampled FIR EQ, odd tap count in [3,8193]. Points
// must include DC and Nyquist, sorted strictly, gain in [-120,120] dB.
[[nodiscard]] Status designFirEqualizer(double sampleRate, std::span<const FirEqPoint>,
                                        std::span<double> taps, FirPhase = FirPhase::linear,
                                        CpuBackend = CpuBackend::automatic) noexcept;
// Real-cepstral minimum-phase approximation using zero padding and a relative
// spectral floor. Truncation/floor can cause small magnitude deviations.
[[nodiscard]] Status minimumPhaseFir(std::span<const double> input, std::span<double> output,
                                     double floorDb = -180,
                                     CpuBackend = CpuBackend::automatic) noexcept;
} // namespace cydsp
