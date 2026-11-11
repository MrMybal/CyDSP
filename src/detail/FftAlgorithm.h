#pragma once
#include "detail/Scalar.h"
#include <complex>
#include <cstddef>
#include <span>
#include <utility>
namespace cydsp::detail {
// Shared production ordering/normalization; callable selects scalar or SIMD arithmetic.
template <class T, class Stage>
constexpr void
fftTransformStages(std::span<const std::complex<T>> input, std::span<std::complex<T>> output,
                   std::span<const std::size_t> reverse, std::span<const std::complex<T>> twiddles,
                   bool inverse, Stage stage) noexcept {
    const auto count = input.size();
    if (input.data() == output.data()) {
        for (std::size_t i = 0; i < count; ++i)
            if (reverse[i] > i)
                std::swap(output[i], output[reverse[i]]);
    } else
        for (std::size_t i = 0; i < count; ++i)
            output[reverse[i]] = input[i];
    for (std::size_t half = 1; half < count; half *= 2)
        stage(output, twiddles.subspan(half - 1, half), half, inverse);
    if (inverse) {
        const auto scale = T{1} / static_cast<T>(count);
        for (auto& value : output)
            value *= scale;
    }
}
// Original group dispatcher retained as an independent constexpr/numerical/benchmark reference.
template <class T, class Butterfly>
constexpr void fftTransform(std::span<const std::complex<T>> input,
                            std::span<std::complex<T>> output, std::span<const std::size_t> reverse,
                            std::span<const std::complex<T>> twiddles, bool inverse,
                            Butterfly butterfly) noexcept {
    const auto count = input.size();
    if (input.data() == output.data()) {
        for (std::size_t i = 0; i < count; ++i)
            if (reverse[i] > i)
                std::swap(output[i], output[reverse[i]]);
    } else
        for (std::size_t i = 0; i < count; ++i)
            output[reverse[i]] = input[i];
    for (std::size_t half = 1; half < count; half *= 2) {
        const auto weights = twiddles.subspan(half - 1, half);
        for (std::size_t base = 0; base < count; base += 2 * half) {
            if (half < 4)
                scalarComplexButterfly<T>(output.subspan(base, half),
                                          output.subspan(base + half, half), weights, inverse);
            else
                butterfly(output.subspan(base, half), output.subspan(base + half, half), weights,
                          inverse);
        }
    }
    if (inverse) {
        const auto scale = T{1} / static_cast<T>(count);
        for (auto& value : output)
            value *= scale;
    }
}
constexpr void fillBitReverse(std::span<std::size_t> reverse) noexcept {
    if (reverse.empty())
        return;
    reverse[0] = 0;
    const auto high = reverse.size() / 2;
    for (std::size_t i = 1; i < reverse.size(); ++i)
        reverse[i] = (reverse[i / 2] >> 1) | ((i & 1) ? high : 0);
}
} // namespace cydsp::detail
