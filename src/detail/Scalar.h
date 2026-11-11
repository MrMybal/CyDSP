#pragma once
#include "cydsp/core/Types.h"
#include "detail/BiquadAlgorithm.h"
#include <complex>
#include <span>
namespace cydsp::detail {
template <Sample T> constexpr void scalarCopy(const T* s, T* d, std::size_t n) noexcept {
    for (std::size_t i = 0; i < n; ++i)
        d[i] = s[i];
}
template <Sample T> constexpr void scalarScale(const T* s, T* d, std::size_t n, T gain) noexcept {
    for (std::size_t i = 0; i < n; ++i)
        d[i] = s[i] * gain;
}
template <Sample T>
constexpr void scalarAddScaled(const T* s, T* d, std::size_t n, T gain) noexcept {
    for (std::size_t i = 0; i < n; ++i)
        d[i] = d[i] + s[i] * gain;
}
template <Sample T> constexpr void scalarFill(T* d, std::size_t n, T value) noexcept {
    for (std::size_t i = 0; i < n; ++i)
        d[i] = value;
}

constexpr double scalarDotMixed(const float* a, const double* b, std::size_t n) noexcept {
    double sum{};
    for (std::size_t i = 0; i < n; ++i)
        sum = sum + static_cast<double>(a[i]) * b[i];
    return sum;
}

template <Sample T> constexpr T scalarDot(const T* a, const T* b, std::size_t n) noexcept {
    T sum{};
    for (std::size_t i = 0; i < n; ++i)
        sum = sum + a[i] * b[i];
    return sum;
}

template <Sample T>
constexpr void scalarMultiply(const T* a, const T* b, T* d, std::size_t n) noexcept {
    for (std::size_t i = 0; i < n; ++i)
        d[i] = a[i] * b[i];
}
template <Sample T>
constexpr void scalarComplexMac(const T* a, const T* b, T* d, std::size_t n) noexcept {
    for (std::size_t i = 0; i < n; ++i) {
        const auto ar = a[2 * i], ai = a[2 * i + 1], br = b[2 * i], bi = b[2 * i + 1];
        const auto real = d[2 * i] + (ar * br - ai * bi), imag = d[2 * i + 1] + (ar * bi + ai * br);
        d[2 * i] = real;
        d[2 * i + 1] = imag;
    }
}

template <class T> struct ComplexPair {
    T real, imag;
};
template <class T> struct ButterflyPair {
    ComplexPair<T> even, odd;
};
template <class T>
constexpr ButterflyPair<T> butterflyPair(ComplexPair<T> even, ComplexPair<T> odd, ComplexPair<T> w,
                                         bool inverse) noexcept {
    const auto wi = inverse ? -w.imag : w.imag;
    const auto real = odd.real * w.real - odd.imag * wi, imag = odd.real * wi + odd.imag * w.real;
    return {{even.real + real, even.imag + imag}, {even.real - real, even.imag - imag}};
}
template <Sample T>
constexpr void scalarButterfly(T* even, T* odd, const T* w, std::size_t n, bool inverse) noexcept {
    for (std::size_t i = 0; i < n; ++i) {
        const auto value = butterflyPair(ComplexPair<T>{even[2 * i], even[2 * i + 1]},
                                         ComplexPair<T>{odd[2 * i], odd[2 * i + 1]},
                                         ComplexPair<T>{w[2 * i], w[2 * i + 1]}, inverse);
        even[2 * i] = value.even.real;
        even[2 * i + 1] = value.even.imag;
        odd[2 * i] = value.odd.real;
        odd[2 * i + 1] = value.odd.imag;
    }
}
template <class T>
constexpr void scalarComplexButterfly(std::span<std::complex<T>> even,
                                      std::span<std::complex<T>> odd,
                                      std::span<const std::complex<T>> w, bool inverse) noexcept {
    for (std::size_t i = 0; i < even.size(); ++i) {
        const auto value = butterflyPair(ComplexPair<T>{even[i].real(), even[i].imag()},
                                         ComplexPair<T>{odd[i].real(), odd[i].imag()},
                                         ComplexPair<T>{w[i].real(), w[i].imag()}, inverse);
        even[i] = {value.even.real, value.even.imag};
        odd[i] = {value.odd.real, value.odd.imag};
    }
}
template <Sample T>
constexpr void scalarGainChain(const T* source, T* target, std::size_t count, const T* gains,
                               std::size_t stages) noexcept {
    std::size_t i = 0;
    for (; count - i >= 4; i += 4) {
        auto v0 = source[i], v1 = source[i + 1], v2 = source[i + 2], v3 = source[i + 3];
        for (std::size_t stage = 0; stage < stages; ++stage) {
            const auto gain = gains[stage];
            if (gain == T{})
                v0 = v1 = v2 = v3 = T{};
            else {
                v0 = T{} + v0 * gain;
                v1 = T{} + v1 * gain;
                v2 = T{} + v2 * gain;
                v3 = T{} + v3 * gain;
            }
        }
        target[i] = v0;
        target[i + 1] = v1;
        target[i + 2] = v2;
        target[i + 3] = v3;
    }
    for (; i < count; ++i) {
        auto value = source[i];
        for (std::size_t stage = 0; stage < stages; ++stage)
            value = gains[stage] == T{} ? T{} : T{} + value * gains[stage];
        target[i] = value;
    }
}
template <Sample T>
constexpr void scalarFftStage(T* data, const T* twiddles, std::size_t count, std::size_t half,
                              bool inverse) noexcept {
    for (std::size_t base = 0; base < count; base += 2 * half)
        scalarButterfly(data + 2 * base, data + 2 * (base + half), twiddles, half, inverse);
}
} // namespace cydsp::detail
