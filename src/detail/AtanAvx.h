#pragma once
#include "AdaaAvx.h"
#include <immintrin.h>
namespace cydsp::detail {
namespace {
// Optional bounded approximation. Inputs and endpoint policy match ADAA's
// precise path; no compiler fast-math or approximate reciprocal is enabled.
inline __m256d fastAtan(__m256d x) noexcept {
    const auto a = foldAbs(x), one = _mm256_set1_pd(1);
    const auto high = _mm256_cmp_pd(a, _mm256_set1_pd(2.414213562373095), _CMP_GT_OQ);
    const auto middle = _mm256_cmp_pd(a, _mm256_set1_pd(.41421356237309503), _CMP_GT_OQ);
    auto z =
        _mm256_blendv_pd(a, _mm256_div_pd(_mm256_sub_pd(a, one), _mm256_add_pd(a, one)), middle);
    z = _mm256_blendv_pd(z, _mm256_div_pd(_mm256_set1_pd(-1), _mm256_max_pd(a, one)), high);
    const auto square = _mm256_mul_pd(z, z);
    auto polynomial = _mm256_set1_pd(1. / 25);
    polynomial = _mm256_add_pd(_mm256_set1_pd(-1. / 23), _mm256_mul_pd(square, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 21), _mm256_mul_pd(square, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(-1. / 19), _mm256_mul_pd(square, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 17), _mm256_mul_pd(square, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(-1. / 15), _mm256_mul_pd(square, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 13), _mm256_mul_pd(square, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(-1. / 11), _mm256_mul_pd(square, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 9), _mm256_mul_pd(square, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(-1. / 7), _mm256_mul_pd(square, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 5), _mm256_mul_pd(square, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(-1. / 3), _mm256_mul_pd(square, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 1), _mm256_mul_pd(square, polynomial));
    const auto offset =
        _mm256_blendv_pd(_mm256_setzero_pd(), _mm256_set1_pd(std::numbers::pi / 4), middle);
    const auto base = _mm256_blendv_pd(offset, _mm256_set1_pd(std::numbers::pi / 2), high);
    return _mm256_or_pd(_mm256_add_pd(base, _mm256_mul_pd(z, polynomial)),
                        _mm256_and_pd(x, _mm256_set1_pd(-0.)));
}
inline void adaaAtanFastAvx2(const double* x, double* y, std::size_t n, double previous,
                             bool active, double dc, double gain) noexcept {
    const auto exactPrimitive = [](double v) {
        return 2 / std::numbers::pi * (v * std::atan(v) - .5 * std::log1p(v * v));
    };
    const auto exactShape = [](double v) { return 2 / std::numbers::pi * std::atan(v); };
    std::size_t i = 0;
    double beforePrimitive = exactPrimitive(previous);
    bool previousValid = true, previousExact = true;
    for (; n - i >= 4; i += 4) {
        const auto current = _mm256_loadu_pd(x + i);
        const auto previousSamples =
            i ? _mm256_loadu_pd(x + i - 1) : _mm256_setr_pd(previous, x[0], x[1], x[2]);
        const auto difference = _mm256_sub_pd(current, previousSamples);
        const auto scale = _mm256_max_pd(_mm256_set1_pd(1),
                                         _mm256_max_pd(foldAbs(current), foldAbs(previousSamples)));
        if (_mm256_movemask_pd(_mm256_cmp_pd(foldAbs(difference),
                                             _mm256_mul_pd(_mm256_set1_pd(.01), scale),
                                             _CMP_LT_OQ)) == 15) {
            for (std::size_t j = 0; j < 4; ++j) {
                const auto at = i + j;
                const double p = at ? x[at - 1] : previous, d = x[at] - p;
                const double localScale = std::max({1., std::abs(x[at]), std::abs(p)});
                double value;
                if (!at && !active) {
                    value = exactShape(x[at]);
                    previousValid = previousExact = false;
                } else if (std::abs(d) > 1e-5 * localScale) {
                    if (!previousExact)
                        beforePrimitive = exactPrimitive(p);
                    const auto next = exactPrimitive(x[at]);
                    value = (next - beforePrimitive) / d;
                    beforePrimitive = next;
                    previousValid = previousExact = true;
                } else {
                    value = exactShape(.5 * (x[at] + p));
                    previousValid = previousExact = false;
                }
                y[at] = (value - dc) * gain;
            }
            continue;
        }
        if (!previousValid)
            beforePrimitive = exactPrimitive(i ? x[i - 1] : previous);
        const auto v = _mm256_mul_pd(
            _mm256_set1_pd(2 / std::numbers::pi),
            _mm256_sub_pd(_mm256_mul_pd(current, fastAtan(current)),
                          _mm256_mul_pd(_mm256_set1_pd(.5),
                                        curveLog(_mm256_add_pd(_mm256_set1_pd(1),
                                                               _mm256_mul_pd(current, current))))));
        double values[4];
        _mm256_storeu_pd(values, v);
        const auto before =
            i ? _mm256_loadu_pd(x + i - 1) : _mm256_setr_pd(previous, x[0], x[1], x[2]);
        const auto delta = _mm256_sub_pd(current, before);
        const auto safe = _mm256_blendv_pd(delta, _mm256_set1_pd(1),
                                           _mm256_cmp_pd(delta, _mm256_setzero_pd(), _CMP_EQ_OQ));
        const auto primitiveBefore =
            _mm256_setr_pd(beforePrimitive, values[0], values[1], values[2]);
        _mm256_storeu_pd(
            y + i,
            _mm256_mul_pd(_mm256_sub_pd(_mm256_div_pd(_mm256_sub_pd(v, primitiveBefore), safe),
                                        _mm256_set1_pd(dc)),
                          _mm256_set1_pd(gain)));
        for (std::size_t j = 0; j < 4; ++j) {
            const auto at = i + j;
            const double p = at ? x[at - 1] : previous, d = x[at] - p;
            const double scale = std::max({1., std::abs(x[at]), std::abs(p)});
            if (!at && !active)
                y[at] = (exactShape(x[at]) - dc) * gain;
            else if (std::abs(d) < .01 * scale) {
                const double result = std::abs(d) > 1e-5 * scale
                                          ? (exactPrimitive(x[at]) - exactPrimitive(p)) / d
                                          : exactShape(.5 * (x[at] + p));
                y[at] = (result - dc) * gain;
            }
        }
        beforePrimitive = values[3];
        previousValid = true;
        previousExact = false;
    }
    for (; i < n; ++i) {
        const double p = i ? x[i - 1] : previous, d = x[i] - p,
                     scale = std::max({1., std::abs(x[i]), std::abs(p)});
        const double value = !i && !active ? exactShape(x[i])
                             : std::abs(d) > 1e-5 * scale
                                 ? (exactPrimitive(x[i]) - exactPrimitive(p)) / d
                                 : exactShape(.5 * (x[i] + p));
        y[i] = (value - dc) * gain;
    }
}
} // namespace
} // namespace cydsp::detail
