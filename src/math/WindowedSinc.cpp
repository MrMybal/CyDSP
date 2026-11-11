// Cosine-window/windowed-sinc equations adapted from CDSPSincFilterGen.h.
// r8brain-free-src Copyright (c) 2013-2025 Aleksey Vaneev.
// MIT: third_party/licenses/r8brain-free-src.txt; exact original in manifest.
// CyDSP uses direct coefficient evaluation, not r8brain's oscillator/whole SRC pipeline.
#include "detail/WindowedSinc.h"
#include "cydsp/math/Window.h"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace cydsp {
namespace {
double cosineWindow(WindowType type, double angle) noexcept {
    switch (type) {
    case WindowType::rectangular:
        return 1;
    case WindowType::hann:
        return 0.5 + 0.5 * std::cos(angle);
    case WindowType::hamming:
        return 0.54 + 0.46 * std::cos(angle);
    case WindowType::blackman:
        return std::max(0.0, 0.42 + 0.5 * std::cos(angle) + 0.08 * std::cos(2 * angle));
    }
    return 0;
}
template <class T>
Status fillWindow(WindowType type, std::span<T> values, WindowSymmetry symmetry) noexcept {
    if (values.empty() ||
        (type != WindowType::rectangular && type != WindowType::hann &&
         type != WindowType::hamming && type != WindowType::blackman) ||
        (symmetry != WindowSymmetry::symmetric && symmetry != WindowSymmetry::periodic))
        return Status::invalid_argument;
    if (values.size() == 1) {
        values[0] = T{1};
        return Status::ok;
    }
    const auto divisor = static_cast<double>(
        symmetry == WindowSymmetry::symmetric ? values.size() - 1 : values.size());
    for (std::size_t i = 0; i < values.size(); ++i)
        values[i] = static_cast<T>(cosineWindow(
            type, 2 * std::numbers::pi * static_cast<double>(i) / divisor - std::numbers::pi));
    return Status::ok;
}
} // namespace
Status makeWindow(WindowType type, std::span<float> values, WindowSymmetry symmetry) noexcept {
    return fillWindow(type, values, symmetry);
}
Status makeWindow(WindowType type, std::span<double> values, WindowSymmetry symmetry) noexcept {
    return fillWindow(type, values, symmetry);
}
namespace detail {
double windowedSinc(double distance, double cutoff, double halfLength) noexcept {
    if (std::abs(distance) > halfLength)
        return 0;
    const auto x = std::numbers::pi * distance * cutoff;
    const auto sinc = x == 0 ? 1.0 : std::sin(x) / x;
    return cutoff * sinc *
           cosineWindow(WindowType::blackman, std::numbers::pi * distance / halfLength);
}
} // namespace detail
} // namespace cydsp
