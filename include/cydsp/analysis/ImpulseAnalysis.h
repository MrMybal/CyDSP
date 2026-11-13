#pragma once
#include "cydsp/core/Types.h"
#include "cydsp/spatial/FdnReverb.h"
#include <array>
#include <span>
namespace cydsp {
struct DecayFit {
    double seconds = 0, rSquared = 0;
    bool valid = false;
};
struct ImpulseBandAnalysis {
    double centerHz = 0, energy = 0;
    DecayFit edt, t20, t30;
};
struct EarlyReflection {
    double seconds = 0, amplitude = 0;
};
struct ImpulseAnalysisConfig {
    double onsetThresholdDb = -40, directWindowMs = 2.5, reflectionThresholdDb = -20;
    bool subtractTailNoise = false;
};
struct ImpulseReport {
    bool silent = false, noiseCompensated = false;
    double onsetSeconds = 0, peakSeconds = 0, energy = 0, centerTimeSeconds = 0;
    double clarity50Db = 0, clarity80Db = 0, definition50 = 0, directToReverberantDb = 0;
    ImpulseBandAnalysis broadband;
    std::array<ImpulseBandAnalysis, 9> octaveBands{}; // 63..16000 Hz, unavailable above Nyquist.
    std::array<EarlyReflection, 32> reflections{};
    std::size_t reflectionCount = 0;
};
// Offline analysis of one channel, no file handling. Schroeder backward energy
// integration and least-squares EDT/T20/T30. Invalid fits remain explicitly
// invalid when the IR is too short/noisy. Not an ISO measurement certification.
[[nodiscard]] Status analyzeImpulse(std::span<const float>, double sampleRate, ImpulseReport&,
                                    const ImpulseAnalysisConfig& = {}) noexcept;
[[nodiscard]] Status analyzeImpulse(std::span<const double>, double sampleRate, ImpulseReport&,
                                    const ImpulseAnalysisConfig& = {}) noexcept;
// Maps valid band decays to nominal low/mid/high FDN targets; no claim of exact
// waveform/room reproduction. Output is unchanged if no useful fit is available.
[[nodiscard]] Status matchFdnToImpulse(const ImpulseReport&, FdnParameters&) noexcept;
} // namespace cydsp
