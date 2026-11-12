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
    case WindowType::blackmanHarris:
        return .35875 + .48829 * std::cos(angle) + .14128 * std::cos(2 * angle) +
               .01168 * std::cos(3 * angle);
    case WindowType::nuttall:
        return .355768 + .487396 * std::cos(angle) + .144232 * std::cos(2 * angle) +
               .012604 * std::cos(3 * angle);
    case WindowType::flatTop:
        return .21557895 + .41663158 * std::cos(angle) + .277263158 * std::cos(2 * angle) +
               .083578947 * std::cos(3 * angle) + .006947368 * std::cos(4 * angle);
    case WindowType::bartlett:
    case WindowType::triangular:
        return std::max(0.0, 1 - std::abs(angle) / std::numbers::pi);
    case WindowType::cosine:
        return std::max(0.0, std::cos(angle * .5));
    case WindowType::kaiser:
        break;
    }
    return 0;
}
double i0(double x) noexcept {
    double value = 1, term = 1;
    for (unsigned n = 1; n < 160; ++n) {
        term *= x * x / (4 * double(n) * n);
        value += term;
        if (term < value * 1e-16)
            break;
    }
    return value;
}
template <class T> Status fillWindow(const WindowParameters& p, std::span<T> values) noexcept {
    if (values.empty() ||
        static_cast<unsigned>(p.type) > static_cast<unsigned>(WindowType::kaiser) ||
        (p.symmetry != WindowSymmetry::symmetric && p.symmetry != WindowSymmetry::periodic) ||
        static_cast<unsigned>(p.normalization) >
            static_cast<unsigned>(WindowNormalization::energy) ||
        !isFinite(p.beta) || p.beta < 0 || p.beta > 50)
        return Status::invalid_argument;
    if (values.size() == 1) {
        values[0] = T{1};
        return Status::ok;
    }
    const auto denominator =
        double(p.symmetry == WindowSymmetry::symmetric ? values.size() - 1 : values.size());
    const auto kaiserDenom = p.type == WindowType::kaiser ? i0(p.beta) : 1;
    auto value = [&](std::size_t i) {
        const auto angle = 2 * std::numbers::pi * double(i) / denominator - std::numbers::pi;
        if (p.type == WindowType::kaiser) {
            const auto x = 2 * double(i) / denominator - 1;
            return i0(p.beta * std::sqrt(std::max(0.0, 1 - x * x))) / kaiserDenom;
        }
        if (p.type == WindowType::triangular) {
            const auto width = p.symmetry == WindowSymmetry::symmetric ? double(values.size() + 1)
                                                                       : double(values.size() + 2);
            return 1 - std::abs(2 * double(i) - denominator) / width;
        }
        return cosineWindow(p.type, angle);
    };
    double norm = 0;
    for (std::size_t i = 0; i < values.size(); ++i) {
        const auto v = value(i);
        if (p.normalization == WindowNormalization::peak)
            norm = std::max(norm, std::abs(v));
        else if (p.normalization == WindowNormalization::energy)
            norm += v * v;
        else
            norm += v;
    }
    if (p.normalization == WindowNormalization::coherentGain)
        norm /= double(values.size());
    if (p.normalization == WindowNormalization::energy)
        norm = std::sqrt(norm / double(values.size()));
    if (p.normalization == WindowNormalization::none)
        norm = 1;
    if (!isFinite(norm) || std::abs(norm) < 1e-30)
        return Status::invalid_argument;
    for (std::size_t i = 0; i < values.size(); ++i)
        values[i] = static_cast<T>(value(i) / norm);
    return Status::ok;
}
} // namespace
Status makeWindow(WindowType type, std::span<float> values, WindowSymmetry symmetry) noexcept {
    return fillWindow(WindowParameters{type, symmetry}, values);
}
Status makeWindow(WindowType type, std::span<double> values, WindowSymmetry symmetry) noexcept {
    return fillWindow(WindowParameters{type, symmetry}, values);
}
Status makeWindow(const WindowParameters& p, std::span<float> out) noexcept {
    return fillWindow(p, out);
}
Status makeWindow(const WindowParameters& p, std::span<double> out) noexcept {
    return fillWindow(p, out);
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
