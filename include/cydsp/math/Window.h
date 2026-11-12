#pragma once
#include "cydsp/core/Types.h"
#include <span>
namespace cydsp {
enum class WindowType {
    rectangular,
    hann,
    hamming,
    blackman,
    blackmanHarris,
    nuttall,
    flatTop,
    bartlett,
    triangular,
    cosine,
    kaiser
};
enum class WindowSymmetry { symmetric, periodic };
enum class WindowNormalization { none, peak, coherentGain, energy };
struct WindowParameters {
    WindowType type = WindowType::hann;
    WindowSymmetry symmetry = WindowSymmetry::symmetric;
    WindowNormalization normalization = WindowNormalization::none;
    double beta = 8.6; // Kaiser 0..50.
};
Status makeWindow(const WindowParameters&, std::span<float>) noexcept;
Status makeWindow(const WindowParameters&, std::span<double>) noexcept;
// Preparation-time coefficients; symmetric uses N-1, periodic uses N.
[[nodiscard]] Status makeWindow(WindowType, std::span<float>,
                                WindowSymmetry = WindowSymmetry::symmetric) noexcept;
[[nodiscard]] Status makeWindow(WindowType, std::span<double>,
                                WindowSymmetry = WindowSymmetry::symmetric) noexcept;
} // namespace cydsp
