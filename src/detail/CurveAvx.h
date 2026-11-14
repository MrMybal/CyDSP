#pragma once
#include <immintrin.h>
#include <numbers>
namespace cydsp::detail {
namespace {
// Original double-precision range reductions, without FMA or vector libm.
inline __m256d curveLog(__m256d x) noexcept {
    const auto bits = _mm256_castpd_si256(x);
    const auto raw = _mm256_srli_epi64(bits, 52);
    auto e = _mm256_sub_pd(
        _mm256_castsi256_pd(_mm256_or_si256(raw, _mm256_set1_epi64x(0x4330000000000000ll))),
        _mm256_set1_pd(4503599627370496. + 1023));
    auto m = _mm256_castsi256_pd(
        _mm256_or_si256(_mm256_and_si256(bits, _mm256_set1_epi64x(0x000fffffffffffffll)),
                        _mm256_set1_epi64x(0x3ff0000000000000ll)));
    const auto high = _mm256_cmp_pd(m, _mm256_set1_pd(std::numbers::sqrt2), _CMP_GT_OQ);
    m = _mm256_blendv_pd(m, _mm256_mul_pd(m, _mm256_set1_pd(.5)), high);
    e = _mm256_add_pd(e, _mm256_and_pd(high, _mm256_set1_pd(1)));
    const auto z =
        _mm256_div_pd(_mm256_sub_pd(m, _mm256_set1_pd(1)), _mm256_add_pd(m, _mm256_set1_pd(1)));
    const auto square = _mm256_mul_pd(z, z);
    auto polynomial = _mm256_set1_pd(1. / 21);
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 19), _mm256_mul_pd(square, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 17), _mm256_mul_pd(square, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 15), _mm256_mul_pd(square, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 13), _mm256_mul_pd(square, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 11), _mm256_mul_pd(square, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 9), _mm256_mul_pd(square, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 7), _mm256_mul_pd(square, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 5), _mm256_mul_pd(square, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 3), _mm256_mul_pd(square, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 1), _mm256_mul_pd(square, polynomial));
    const auto correction =
        _mm256_add_pd(_mm256_mul_pd(_mm256_mul_pd(_mm256_set1_pd(2), z), polynomial),
                      _mm256_mul_pd(e, _mm256_set1_pd(2.3190468138462996e-17)));
    return _mm256_add_pd(_mm256_mul_pd(e, _mm256_set1_pd(std::numbers::ln2)), correction);
}
inline __m256d curveExp(__m256d x) noexcept {
    const auto nearest = _mm256_round_pd(_mm256_mul_pd(x, _mm256_set1_pd(1 / std::numbers::ln2)),
                                         _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
    constexpr double high = 0x1.62e42fefa0000p-1;
    constexpr double low = 0x1.cf79abc9e3b3ap-40;
    const auto r = _mm256_sub_pd(_mm256_sub_pd(x, _mm256_mul_pd(nearest, _mm256_set1_pd(high))),
                                 _mm256_mul_pd(nearest, _mm256_set1_pd(low)));
    auto polynomial = _mm256_set1_pd(1. / 6227020800.);
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 479001600.), _mm256_mul_pd(r, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 39916800.), _mm256_mul_pd(r, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 3628800.), _mm256_mul_pd(r, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 362880.), _mm256_mul_pd(r, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 40320.), _mm256_mul_pd(r, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 5040.), _mm256_mul_pd(r, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 720.), _mm256_mul_pd(r, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 120.), _mm256_mul_pd(r, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 24.), _mm256_mul_pd(r, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 6.), _mm256_mul_pd(r, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 2.), _mm256_mul_pd(r, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 1.), _mm256_mul_pd(r, polynomial));
    polynomial = _mm256_add_pd(_mm256_set1_pd(1. / 1.), _mm256_mul_pd(r, polynomial));
    const auto exponent = _mm256_add_epi64(_mm256_cvtepi32_epi64(_mm256_cvttpd_epi32(nearest)),
                                           _mm256_set1_epi64x(1023));
    return _mm256_mul_pd(polynomial, _mm256_castsi256_pd(_mm256_slli_epi64(exponent, 52)));
}
} // namespace
} // namespace cydsp::detail
