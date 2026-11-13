#pragma once
#include "cydsp/core/AudioBlock.h"
#include <cmath>
namespace cydsp::detail {
// Retain small energies while large samples leave a sliding measurement window.
struct CompensatedSum {
    double sum = 0, correction = 0;
    void add(double value) noexcept {
        const double next = sum + value;
        correction +=
            std::abs(sum) >= std::abs(value) ? (sum - next) + value : (value - next) + sum;
        sum = next;
    }
    [[nodiscard]] double value() const noexcept {
        return sum + correction;
    }
};
inline bool validAnalysisSpec(const ProcessSpec& spec) noexcept {
    return spec.isValid() && spec.sampleRate >= 8000 && spec.sampleRate <= 768000 &&
           spec.channels <= 64 && spec.maxBlockFrames <= 65536;
}
template <Sample T>
Status validateAnalysis(const ProcessSpec& spec, AudioBlock<const T> input) noexcept {
    if (!input.isValid())
        return Status::invalid_argument;
    if (input.channels() != spec.channels)
        return Status::channel_mismatch;
    if (input.frames() > spec.maxBlockFrames)
        return Status::block_too_large;
    // Keep squared/weighted accumulation representable for extreme double input.
    for (std::size_t ch = 0; ch < input.channels(); ++ch)
        for (T x : input.channel(ch))
            if (!isFinite(x) || std::abs(double(x)) > 1e100)
                return Status::invalid_argument;
    return Status::ok;
}
inline double amplitudeDb(double value) noexcept {
    return value > 0 ? 20 * std::log10(value) : -std::numeric_limits<double>::infinity();
}
} // namespace cydsp::detail
