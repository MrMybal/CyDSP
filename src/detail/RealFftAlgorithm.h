#pragma once
#include <complex>
#include <span>
namespace cydsp::detail {
// N real samples are packed as N/2 even+i*odd values before the complex FFT.
// These allocation-free helpers are also evaluated by compile-time regressions.
template <class T>
constexpr void packReal(std::span<const T> input, std::span<std::complex<T>> packed) noexcept {
    for (std::size_t i = 0; i < packed.size(); ++i)
        packed[i] = {input[2 * i], input[2 * i + 1]};
}
template <class T>
constexpr void scatterReal(std::span<const std::complex<T>> packed, std::span<T> output) noexcept {
    for (std::size_t i = 0; i < packed.size(); ++i) {
        output[2 * i] = packed[i].real();
        output[2 * i + 1] = packed[i].imag();
    }
}
template <class T>
constexpr std::complex<T> splitRealBin(std::span<const std::complex<T>> packed,
                                       std::span<const std::complex<T>> twiddles,
                                       std::size_t k) noexcept {
    const auto half = packed.size();
    if (k == 0)
        return {packed[0].real() + packed[0].imag(), T{0}};
    if (k == half)
        return {packed[0].real() - packed[0].imag(), T{0}};
    const auto a = packed[k], b = std::conj(packed[half - k]);
    const auto sum = a + b, difference = a - b, w = twiddles[k - 1];
    const auto real = w.real() * difference.real() - w.imag() * difference.imag();
    const auto imag = w.real() * difference.imag() + w.imag() * difference.real();
    return {(sum.real() + imag) * T{0.5}, (sum.imag() - real) * T{0.5}};
}
template <class T>
constexpr std::complex<T> mergeRealBin(std::span<const std::complex<T>> input,
                                       std::span<const std::complex<T>> twiddles,
                                       std::size_t k) noexcept {
    const auto half = input.size() - 1;
    if (k == 0)
        return {(input[0].real() + input[half].real()) * T{0.5},
                (input[0].real() - input[half].real()) * T{0.5}};
    const auto a = input[k], b = std::conj(input[half - k]);
    const auto sum = a + b, difference = a - b, w = twiddles[k - 1];
    const auto real = w.real() * difference.real() + w.imag() * difference.imag();
    const auto imag = w.real() * difference.imag() - w.imag() * difference.real();
    return {(sum.real() - imag) * T{0.5}, (sum.imag() + real) * T{0.5}};
}
template <class T>
constexpr void splitRealSpectrum(std::span<const std::complex<T>> packed,
                                 std::span<const std::complex<T>> twiddles,
                                 std::span<std::complex<T>> output) noexcept {
    const auto half = packed.size();
    output[0] = splitRealBin<T>(packed, twiddles, 0);
    output[half] = splitRealBin<T>(packed, twiddles, half);
    for (std::size_t k = 1; k < half; ++k)
        output[k] = splitRealBin<T>(packed, twiddles, k);
}
template <class T>
constexpr void mergeRealSpectrum(std::span<const std::complex<T>> input,
                                 std::span<const std::complex<T>> twiddles,
                                 std::span<std::complex<T>> packed) noexcept {
    packed[0] = mergeRealBin<T>(input, twiddles, 0);
    for (std::size_t k = 1; k < packed.size(); ++k)
        packed[k] = mergeRealBin<T>(input, twiddles, k);
}
} // namespace cydsp::detail
