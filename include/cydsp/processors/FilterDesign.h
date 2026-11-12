#pragma once
#include "cydsp/processors/Biquad.h"
namespace cydsp {
enum class IirFamily { butterworth, chebyshevI, chebyshevII, elliptic, bessel };
struct IirDesign {
    IirFamily family = IirFamily::butterworth;
    BiquadType response = BiquadType::lowPass;
    unsigned order = 4;
    double sampleRate = 48000, cutoff = 1000, rippleDb = 1, stopBandDb = 80;
    double ellipticRolloff = 0; // DSPFilters transition-shape parameter, 0..4.
};
// Preparation only; LP/HP, order1..16. ChebyshevII cutoff is its stop-band edge.
// Bessel uses -3dB normalization. Other ripple families use the pass-band edge.
// On failure neither output coefficients nor count changes.
Status designIir(const IirDesign&, std::span<BiquadCoefficients>, std::size_t& count) noexcept;
} // namespace cydsp
