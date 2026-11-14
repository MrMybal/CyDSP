#pragma once
#include "cydsp/core/AudioBlock.h"
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <span>
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
// True when every sample is finite and, for double, at most 1e100 in magnitude.
// The test is integer arithmetic on the IEEE bit patterns: it has no branch per
// sample and no floating-point exception or rounding dependence, so any
// baseline compiler can vectorize it. Float cannot exceed 1e100 when finite.
template <Sample T> bool boundedSamples(std::span<const T> samples) noexcept {
    const auto* data = samples.data();
    const auto count = samples.size();
    if constexpr (std::same_as<std::remove_const_t<T>, float>) {
        // An all-ones exponent (infinity/NaN) carries into the sign bit.
        std::uint32_t flags = 0;
        for (std::size_t i = 0; i < count; ++i) {
            std::uint32_t bits;
            std::memcpy(&bits, data + i, sizeof(bits));
            flags |= (bits & 0x7f800000u) + 0x00800000u;
        }
        return !(flags & 0x80000000u);
    } else {
        // Magnitude bits are ordered like the magnitudes; above the limit the
        // subtraction borrows into the sign bit. Infinity/NaN are above it.
        constexpr auto limit = std::bit_cast<std::uint64_t>(1e100);
        std::uint64_t flags = 0;
        for (std::size_t i = 0; i < count; ++i) {
            std::uint64_t bits;
            std::memcpy(&bits, data + i, sizeof(bits));
            flags |= limit - (bits & 0x7fffffffffffffffull);
        }
        return !(flags >> 63);
    }
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
        if (!boundedSamples(input.channel(ch)))
            return Status::invalid_argument;
    return Status::ok;
}
inline double amplitudeDb(double value) noexcept {
    return value > 0 ? 20 * std::log10(value) : -std::numeric_limits<double>::infinity();
}
} // namespace cydsp::detail
