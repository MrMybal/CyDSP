#pragma once
#include "CurveAvx.h"
#include <algorithm>
#include <cmath>
#include <immintrin.h>
#include <numbers>
namespace cydsp::detail {
namespace {
// Envelope is finite in [0,1e100], output is disjoint; gain is always normal.
inline void compressorGainAvx2(const double* envelope, double* gains, std::size_t count,
                               double thresholdDb, double ratio, double kneeDb,
                               bool limiter) noexcept {
    const double half = kneeDb * .5, slope = limiter ? -1 : 1 / ratio - 1;
    const double lower = std::pow(10., (thresholdDb - half) / 20),
                 upper = std::pow(10., (thresholdDb + half) / 20),
                 threshold = std::pow(10., thresholdDb / 20);
    std::size_t i = 0;
    for (; count - i >= 4; i += 4) {
        const auto env = _mm256_max_pd(_mm256_loadu_pd(envelope + i), _mm256_set1_pd(1e-12));
        if (_mm256_movemask_pd(
                _mm256_cmp_pd(env, _mm256_set1_pd(lower * (1 - 1e-12)), _CMP_LT_OQ)) == 15) {
            _mm256_storeu_pd(gains + i, _mm256_set1_pd(1));
            continue;
        }
        if ((limiter || ratio == 2 || ratio == 4) &&
            _mm256_movemask_pd(
                _mm256_cmp_pd(env, _mm256_set1_pd(upper * (1 + 1e-12)), _CMP_GT_OQ)) == 15) {
            auto value = _mm256_div_pd(_mm256_set1_pd(threshold), env);
            if (!limiter) {
                value = _mm256_sqrt_pd(value);
                if (ratio == 4)
                    value = _mm256_mul_pd(value, _mm256_sqrt_pd(value));
            }
            _mm256_storeu_pd(gains + i, value);
            continue;
        }
        const auto over =
            _mm256_sub_pd(_mm256_mul_pd(curveLog(env), _mm256_set1_pd(20 / std::numbers::ln10)),
                          _mm256_set1_pd(thresholdDb));
        auto db = _mm256_mul_pd(_mm256_max_pd(over, _mm256_setzero_pd()), _mm256_set1_pd(slope));
        if (kneeDb > 0) {
            const auto t =
                _mm256_max_pd(_mm256_add_pd(over, _mm256_set1_pd(half)), _mm256_setzero_pd());
            const auto knee =
                _mm256_mul_pd(_mm256_mul_pd(t, t), _mm256_set1_pd(slope / (2 * kneeDb)));
            db = _mm256_blendv_pd(knee, db, _mm256_cmp_pd(over, _mm256_set1_pd(half), _CMP_GE_OQ));
        }
        _mm256_storeu_pd(gains + i,
                         curveExp(_mm256_mul_pd(db, _mm256_set1_pd(std::numbers::ln10 / 20))));
    }
    for (; i < count; ++i) {
        const double over = 20 * std::log10(std::max(1e-12, envelope[i])) - thresholdDb;
        double db = 0;
        if (kneeDb > 0 && std::abs(over) < half) {
            const double t = over + half;
            db = slope * t * t / (2 * kneeDb);
        } else if (over >= half)
            db = slope * over;
        gains[i] = std::pow(10., db / 20);
    }
}
} // namespace
} // namespace cydsp::detail
