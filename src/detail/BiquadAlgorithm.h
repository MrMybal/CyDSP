#pragma once
#include "cydsp/processors/Biquad.h"
namespace cydsp::detail {
template <class T>
constexpr bool roundBiquad(const BiquadCoefficients& value, BiquadCoefficients& result) noexcept {
    if (!value.isStable())
        return false;
    constexpr auto maximum = static_cast<double>(std::numeric_limits<T>::max());
    for (const auto coefficient : {value.b0, value.b1, value.b2, value.a1, value.a2})
        if (coefficient < -maximum || coefficient > maximum)
            return false;
    const BiquadCoefficients next{static_cast<T>(value.b0), static_cast<T>(value.b1),
                                  static_cast<T>(value.b2), static_cast<T>(value.a1),
                                  static_cast<T>(value.a2)};
    if (!next.isStable())
        return false;
    result = next;
    return true;
}
template <class T> struct BiquadSample {
    T output, z1, z2;
};
template <class T>
constexpr BiquadSample<T> biquadSample(T x, T z1, T z2, T b0, T b1, T b2, T a1, T a2) noexcept {
    const T y = b0 * x + z1;
    return {y, b1 * x - a1 * y + z2, b2 * x - a2 * y};
}
// Frame-major samples; coefficients and state use one contiguous array per field.
template <class T>
constexpr void scalarBiquadVoice(T* data, std::size_t voices, std::size_t frames, const T* c,
                                 T* states, std::size_t voice) noexcept {
    const auto b0 = c[voice], b1 = c[voices + voice], b2 = c[2 * voices + voice],
               a1 = c[3 * voices + voice], a2 = c[4 * voices + voice];
    auto z1 = states[voice], z2 = states[voices + voice];
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const auto sample = biquadSample(data[frame * voices + voice], z1, z2, b0, b1, b2, a1, a2);
        data[frame * voices + voice] = sample.output;
        z1 = sample.z1;
        z2 = sample.z2;
    }
    states[voice] = z1;
    states[voices + voice] = z2;
}
template <class T>
constexpr void scalarBiquadBank(T* data, std::size_t voices, std::size_t frames, const T* c,
                                T* states) noexcept {
    for (std::size_t voice = 0; voice < voices; ++voice)
        scalarBiquadVoice(data, voices, frames, c, states, voice);
}
} // namespace cydsp::detail
