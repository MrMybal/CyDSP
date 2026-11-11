#pragma once
#include "cydsp/core/Types.h"
#include <span>
namespace cydsp {
enum class WindowType { rectangular, hann, hamming, blackman };
enum class WindowSymmetry { symmetric, periodic };
// Preparation-time coefficients; symmetric uses N-1, periodic uses N.
[[nodiscard]] Status makeWindow(WindowType, std::span<float>,
                                WindowSymmetry = WindowSymmetry::symmetric) noexcept;
[[nodiscard]] Status makeWindow(WindowType, std::span<double>,
                                WindowSymmetry = WindowSymmetry::symmetric) noexcept;
} // namespace cydsp
