#pragma once
#include "CurveAvx.h"
#include <algorithm>
#include <cmath>
#include <immintrin.h>
namespace cydsp::detail {
namespace {
// Original time-parallel ADAA kernel. All arithmetic remains double precision;
// no FMA, approximate reciprocal, vector transcendental runtime or task launch.
inline __m256d foldAbs(__m256d x) noexcept {
    return _mm256_andnot_pd(_mm256_set1_pd(-0.), x);
}
inline __m256d foldPhase(__m256d x) noexcept {
    const auto q =
        _mm256_floor_pd(_mm256_add_pd(_mm256_mul_pd(x, _mm256_set1_pd(.25)), _mm256_set1_pd(.5)));
    return _mm256_sub_pd(x, _mm256_mul_pd(_mm256_set1_pd(4), q));
}
inline __m256d foldShape(__m256d x) noexcept {
    const auto r = foldPhase(x), a = foldAbs(r);
    const auto magnitude = foldAbs(_mm256_sub_pd(_mm256_set1_pd(2), a));
    const auto reflected = _mm256_or_pd(magnitude, _mm256_and_pd(r, _mm256_set1_pd(-0.)));
    return _mm256_blendv_pd(reflected, r, _mm256_cmp_pd(a, _mm256_set1_pd(1), _CMP_LE_OQ));
}
inline __m256d foldPrimitive(__m256d x) noexcept {
    const auto r = foldAbs(foldPhase(x));
    const auto halfSquare = _mm256_mul_pd(_mm256_mul_pd(_mm256_set1_pd(.5), r), r);
    const auto second = _mm256_sub_pd(
        _mm256_sub_pd(_mm256_mul_pd(_mm256_set1_pd(2), r), halfSquare), _mm256_set1_pd(1));
    return _mm256_blendv_pd(second, halfSquare, _mm256_cmp_pd(r, _mm256_set1_pd(1), _CMP_LE_OQ));
}
inline void adaaFoldAvx2(const double* x, double* y, std::size_t n, double previous, bool active,
                         double dc, double gain) noexcept {
    std::size_t i = 0;
    for (; n - i >= 4; i += 4) {
        const auto current = _mm256_loadu_pd(x + i);
        const auto before =
            i ? _mm256_loadu_pd(x + i - 1) : _mm256_setr_pd(previous, x[0], x[1], x[2]);
        const auto delta = _mm256_sub_pd(current, before);
        const auto threshold = _mm256_mul_pd(
            _mm256_set1_pd(1e-5),
            _mm256_max_pd(_mm256_set1_pd(1), _mm256_max_pd(foldAbs(current), foldAbs(before))));
        const auto distinct = _mm256_cmp_pd(foldAbs(delta), threshold, _CMP_GT_OQ);
        const auto safeDelta = _mm256_blendv_pd(_mm256_set1_pd(1), delta, distinct);
        const auto integral =
            _mm256_div_pd(_mm256_sub_pd(foldPrimitive(current), foldPrimitive(before)), safeDelta);
        const auto midpoint =
            foldShape(_mm256_mul_pd(_mm256_set1_pd(.5), _mm256_add_pd(current, before)));
        const auto result = _mm256_mul_pd(
            _mm256_sub_pd(_mm256_blendv_pd(midpoint, integral, distinct), _mm256_set1_pd(dc)),
            _mm256_set1_pd(gain));
        _mm256_storeu_pd(y + i, result);
    }
    const auto triangle = [](double v) {
        const double r = v - 4 * std::floor(v * .25 + .5);
        return std::abs(r) <= 1 ? r : std::copysign(2 - std::abs(r), r);
    };
    const auto primitive = [](double v) {
        const double r = std::abs(v - 4 * std::floor(v * .25 + .5));
        return r <= 1 ? .5 * r * r : 2 * r - .5 * r * r - 1;
    };
    for (; i < n; ++i) {
        const auto before = i ? x[i - 1] : previous, delta = x[i] - before;
        const auto v = std::abs(delta) > 1e-5 * std::max({1., std::abs(x[i]), std::abs(before)})
                           ? (primitive(x[i]) - primitive(before)) / delta
                           : triangle(.5 * (x[i] + before));
        y[i] = (v - dc) * gain;
    }
    if (n && !active)
        y[0] = (triangle(x[0]) - dc) * gain;
}
inline void adaaClipAvx2(const double* x, double* y, std::size_t n, double previous, bool active,
                         double dc, double gain) noexcept {
    const auto clipped = [](__m256d v) {
        return _mm256_min_pd(_mm256_set1_pd(1), _mm256_max_pd(_mm256_set1_pd(-1), v));
    };
    const auto integral = [](__m256d v) {
        const auto a = foldAbs(v);
        return _mm256_blendv_pd(_mm256_sub_pd(a, _mm256_set1_pd(.5)),
                                _mm256_mul_pd(_mm256_set1_pd(.5), _mm256_mul_pd(v, v)),
                                _mm256_cmp_pd(a, _mm256_set1_pd(1), _CMP_LE_OQ));
    };
    std::size_t i = 0;
    for (; n - i >= 4; i += 4) {
        const auto current = _mm256_loadu_pd(x + i);
        const auto before =
            i ? _mm256_loadu_pd(x + i - 1) : _mm256_setr_pd(previous, x[0], x[1], x[2]);
        const auto delta = _mm256_sub_pd(current, before);
        const auto threshold = _mm256_mul_pd(
            _mm256_set1_pd(1e-5),
            _mm256_max_pd(_mm256_set1_pd(1), _mm256_max_pd(foldAbs(current), foldAbs(before))));
        const auto distinct = _mm256_cmp_pd(foldAbs(delta), threshold, _CMP_GT_OQ);
        const auto safe = _mm256_blendv_pd(_mm256_set1_pd(1), delta, distinct);
        const auto secant = _mm256_div_pd(_mm256_sub_pd(integral(current), integral(before)), safe);
        const auto middle =
            clipped(_mm256_mul_pd(_mm256_set1_pd(.5), _mm256_add_pd(current, before)));
        _mm256_storeu_pd(y + i,
                         _mm256_mul_pd(_mm256_sub_pd(_mm256_blendv_pd(middle, secant, distinct),
                                                     _mm256_set1_pd(dc)),
                                       _mm256_set1_pd(gain)));
    }
    for (; i < n; ++i) {
        const auto before = i ? x[i - 1] : previous, delta = x[i] - before;
        const auto primitive = [](double v) {
            return std::abs(v) <= 1 ? .5 * v * v : std::abs(v) - .5;
        };
        const auto value = std::abs(delta) > 1e-5 * std::max({1., std::abs(x[i]), std::abs(before)})
                               ? (primitive(x[i]) - primitive(before)) / delta
                               : std::clamp(.5 * (x[i] + before), -1., 1.);
        y[i] = (value - dc) * gain;
    }
    if (n && !active)
        y[0] = (std::clamp(x[0], -1., 1.) - dc) * gain;
}
inline void adaaTanhAvx2(const double* x, double* y, std::size_t n, double previous, bool active,
                         double dc, double gain) noexcept {
    const auto exactPrimitive = [](double v) {
        const auto a = std::abs(v);
        return a + std::log1p(std::exp(-2 * a)) - std::numbers::ln2;
    };
    double previousPrimitive = exactPrimitive(previous);
    bool previousValid = true, previousExact = true;
    std::size_t i = 0;
    for (; n - i >= 4; i += 4) {
        const auto current = _mm256_loadu_pd(x + i), magnitude = foldAbs(current);
        const auto before =
            i ? _mm256_loadu_pd(x + i - 1) : _mm256_setr_pd(previous, x[0], x[1], x[2]);
        const auto delta = _mm256_sub_pd(current, before);
        const auto scale =
            _mm256_max_pd(_mm256_set1_pd(1), _mm256_max_pd(magnitude, foldAbs(before)));
        const auto close =
            _mm256_cmp_pd(foldAbs(delta), _mm256_mul_pd(_mm256_set1_pd(.01), scale), _CMP_LT_OQ);
        if (_mm256_movemask_pd(close) == 15) {
            // Silence and slowly moving inputs retain the cached scalar integral,
            // avoiding vector transcendental work that would immediately be discarded.
            for (std::size_t j = 0; j < 4; ++j) {
                const auto index = i + j;
                const double beforeSample = index ? x[index - 1] : previous;
                const double d = x[index] - beforeSample;
                const double localScale =
                    std::max({1., std::abs(x[index]), std::abs(beforeSample)});
                double value;
                if (!index && !active) {
                    value = std::tanh(x[index]);
                    previousValid = previousExact = false;
                } else if (std::abs(d) > 1e-5 * localScale) {
                    if (!previousExact)
                        previousPrimitive = exactPrimitive(beforeSample);
                    const auto currentPrimitive = exactPrimitive(x[index]);
                    value = (currentPrimitive - previousPrimitive) / d;
                    previousPrimitive = currentPrimitive;
                    previousValid = previousExact = true;
                } else {
                    value = std::tanh(.5 * (x[index] + beforeSample));
                    previousValid = previousExact = false;
                }
                y[index] = (value - dc) * gain;
            }
            continue;
        }
        if (!previousValid)
            previousPrimitive = exactPrimitive(i ? x[i - 1] : previous);
        const auto argument =
            _mm256_max_pd(_mm256_set1_pd(-64), _mm256_mul_pd(magnitude, _mm256_set1_pd(-2)));
        const auto primitives = _mm256_sub_pd(
            _mm256_add_pd(magnitude,
                          curveLog(_mm256_add_pd(_mm256_set1_pd(1), curveExp(argument)))),
            _mm256_set1_pd(std::numbers::ln2));
        double values[4];
        _mm256_storeu_pd(values, primitives);

        const auto beforePrimitive =
            _mm256_setr_pd(previousPrimitive, values[0], values[1], values[2]);

        const auto distinct = _mm256_cmp_pd(foldAbs(delta), _mm256_set1_pd(0), _CMP_GT_OQ);
        const auto safe = _mm256_blendv_pd(_mm256_set1_pd(1), delta, distinct);
        _mm256_storeu_pd(
            y + i,
            _mm256_mul_pd(
                _mm256_sub_pd(_mm256_div_pd(_mm256_sub_pd(primitives, beforePrimitive), safe),
                              _mm256_set1_pd(dc)),
                _mm256_set1_pd(gain)));
        // Close endpoints retain the exact libm secant/midpoint, bounding the
        // amplification of the vector primitive's rounding error.
        for (std::size_t j = 0; j < 4; ++j) {
            const auto index = i + j;
            const double beforeSample = index ? x[index - 1] : previous;
            const double d = x[index] - beforeSample;
            const double scale = std::max({1., std::abs(x[index]), std::abs(beforeSample)});
            if (!index && !active)
                y[index] = (std::tanh(x[index]) - dc) * gain;
            else if (std::abs(d) < .01 * scale) {
                const double v = std::abs(d) > 1e-5 * scale
                                     ? (exactPrimitive(x[index]) - exactPrimitive(beforeSample)) / d
                                     : std::tanh(.5 * (x[index] + beforeSample));
                y[index] = (v - dc) * gain;
            }
        }
        previousPrimitive = values[3];
        previousValid = true;
        previousExact = false;
    }
    for (; i < n; ++i) {
        const auto before = i ? x[i - 1] : previous, d = x[i] - before;
        const auto scale = std::max({1., std::abs(x[i]), std::abs(before)});
        const auto v = !i && !active ? std::tanh(x[i])
                       : std::abs(d) > 1e-5 * scale
                           ? (exactPrimitive(x[i]) - exactPrimitive(before)) / d
                           : std::tanh(.5 * (x[i] + before));
        y[i] = (v - dc) * gain;
    }
}
} // namespace
} // namespace cydsp::detail
