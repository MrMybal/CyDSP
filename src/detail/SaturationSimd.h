#pragma once
#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <immintrin.h>
#include <numbers>
// Original width-generic saturation kernels. Included only by the private x86
// ISA translation units; every operation is explicit double arithmetic with no
// FMA, approximate reciprocal, vector libm or floating-point state change.
#if defined(_MSC_VER) && !defined(__clang__)
#define CYDSP_SIMD_INLINE __forceinline
#else
#define CYDSP_SIMD_INLINE inline __attribute__((always_inline))
#endif
namespace cydsp::detail {
namespace {
struct Double4 {
    using Reg = __m256d;
    using Mask = __m256d;
    static constexpr std::size_t width = 4;
    static CYDSP_SIMD_INLINE Reg set(double v) noexcept {
        return _mm256_set1_pd(v);
    }
    static CYDSP_SIMD_INLINE Reg load(const double* p) noexcept {
        return _mm256_loadu_pd(p);
    }
    static CYDSP_SIMD_INLINE void store(double* p, Reg v) noexcept {
        _mm256_storeu_pd(p, v);
    }
    static CYDSP_SIMD_INLINE Reg add(Reg a, Reg b) noexcept {
        return _mm256_add_pd(a, b);
    }
    static CYDSP_SIMD_INLINE Reg sub(Reg a, Reg b) noexcept {
        return _mm256_sub_pd(a, b);
    }
    static CYDSP_SIMD_INLINE Reg mul(Reg a, Reg b) noexcept {
        return _mm256_mul_pd(a, b);
    }
    static CYDSP_SIMD_INLINE Reg div(Reg a, Reg b) noexcept {
        return _mm256_div_pd(a, b);
    }
    static CYDSP_SIMD_INLINE Reg sqrt(Reg a) noexcept {
        return _mm256_sqrt_pd(a);
    }
    static CYDSP_SIMD_INLINE Reg min(Reg a, Reg b) noexcept {
        return _mm256_min_pd(a, b);
    }
    static CYDSP_SIMD_INLINE Reg max(Reg a, Reg b) noexcept {
        return _mm256_max_pd(a, b);
    }
    static CYDSP_SIMD_INLINE Reg abs(Reg a) noexcept {
        return _mm256_andnot_pd(_mm256_set1_pd(-0.), a);
    }
    static CYDSP_SIMD_INLINE Reg signOf(Reg a) noexcept {
        return _mm256_and_pd(a, _mm256_set1_pd(-0.));
    }
    static CYDSP_SIMD_INLINE Reg orBits(Reg a, Reg b) noexcept {
        return _mm256_or_pd(a, b);
    }
    static CYDSP_SIMD_INLINE Reg xorBits(Reg a, Reg b) noexcept {
        return _mm256_xor_pd(a, b);
    }
    static CYDSP_SIMD_INLINE Mask gt(Reg a, Reg b) noexcept {
        return _mm256_cmp_pd(a, b, _CMP_GT_OQ);
    }
    static CYDSP_SIMD_INLINE Mask lt(Reg a, Reg b) noexcept {
        return _mm256_cmp_pd(a, b, _CMP_LT_OQ);
    }
    static CYDSP_SIMD_INLINE Mask le(Reg a, Reg b) noexcept {
        return _mm256_cmp_pd(a, b, _CMP_LE_OQ);
    }
    static CYDSP_SIMD_INLINE Mask eq(Reg a, Reg b) noexcept {
        return _mm256_cmp_pd(a, b, _CMP_EQ_OQ);
    }
    static CYDSP_SIMD_INLINE Mask both(Mask a, Mask b) noexcept {
        return _mm256_and_pd(a, b);
    }
    static CYDSP_SIMD_INLINE Mask either(Mask a, Mask b) noexcept {
        return _mm256_or_pd(a, b);
    }
    // a and not b
    static CYDSP_SIMD_INLINE Mask without(Mask a, Mask b) noexcept {
        return _mm256_andnot_pd(b, a);
    }
    static CYDSP_SIMD_INLINE bool all(Mask m) noexcept {
        return _mm256_movemask_pd(m) == 15;
    }
    static CYDSP_SIMD_INLINE Reg select(Mask m, Reg yes, Reg no) noexcept {
        return _mm256_blendv_pd(no, yes, m);
    }
    static CYDSP_SIMD_INLINE Reg nearest(Reg a) noexcept {
        return _mm256_round_pd(a, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
    }
    static CYDSP_SIMD_INLINE Reg floor(Reg a) noexcept {
        return _mm256_floor_pd(a);
    }
    // 2^n for integral n in the normal exponent range.
    static CYDSP_SIMD_INLINE Reg pow2(Reg n) noexcept {
        const auto exponent = _mm256_add_epi64(_mm256_cvtepi32_epi64(_mm256_cvttpd_epi32(n)),
                                               _mm256_set1_epi64x(1023));
        return _mm256_castsi256_pd(_mm256_slli_epi64(exponent, 52));
    }
    // Positive normal u = mantissa * 2^exponent with mantissa in [1,2).
    static CYDSP_SIMD_INLINE void split(Reg u, Reg& mantissa, Reg& exponent) noexcept {
        const auto bits = _mm256_castpd_si256(u);
        exponent = _mm256_sub_pd(
            _mm256_castsi256_pd(_mm256_or_si256(_mm256_srli_epi64(bits, 52),
                                                _mm256_set1_epi64x(0x4330000000000000ll))),
            _mm256_set1_pd(4503599627370496. + 1023));
        mantissa = _mm256_castsi256_pd(
            _mm256_or_si256(_mm256_and_si256(bits, _mm256_set1_epi64x(0x000fffffffffffffll)),
                            _mm256_set1_epi64x(0x3ff0000000000000ll)));
    }
    // {carry[last], current[0], ..., current[width-2]}
    static CYDSP_SIMD_INLINE Reg shiftIn(Reg current, Reg carry) noexcept {
        return _mm256_blend_pd(_mm256_permute4x64_pd(current, _MM_SHUFFLE(2, 1, 0, 0)),
                               _mm256_permute4x64_pd(carry, _MM_SHUFFLE(3, 3, 3, 3)), 1);
    }
    static CYDSP_SIMD_INLINE Reg fromSamples(const float* p) noexcept {
        return _mm256_cvtps_pd(_mm_loadu_ps(p));
    }
    static CYDSP_SIMD_INLINE Reg fromSamples(const double* p) noexcept {
        return _mm256_loadu_pd(p);
    }
    static CYDSP_SIMD_INLINE void toSamples(float* p, Reg v) noexcept {
        _mm_storeu_ps(p, _mm256_cvtpd_ps(v));
    }
    static CYDSP_SIMD_INLINE void toSamples(double* p, Reg v) noexcept {
        _mm256_storeu_pd(p, v);
    }
    // The value after rounding to the sample type, as a double.
    static CYDSP_SIMD_INLINE Reg rounded(Reg v, const float*) noexcept {
        return _mm256_cvtps_pd(_mm256_cvtpd_ps(v));
    }
    static CYDSP_SIMD_INLINE Reg rounded(Reg v, const double*) noexcept {
        return v;
    }
};
#if defined(__AVX512F__) && defined(__AVX512DQ__)
struct Double8 {
    using Reg = __m512d;
    using Mask = __mmask8;
    static constexpr std::size_t width = 8;
    static CYDSP_SIMD_INLINE Reg set(double v) noexcept {
        return _mm512_set1_pd(v);
    }
    static CYDSP_SIMD_INLINE Reg load(const double* p) noexcept {
        return _mm512_loadu_pd(p);
    }
    static CYDSP_SIMD_INLINE void store(double* p, Reg v) noexcept {
        _mm512_storeu_pd(p, v);
    }
    static CYDSP_SIMD_INLINE Reg add(Reg a, Reg b) noexcept {
        return _mm512_add_pd(a, b);
    }
    static CYDSP_SIMD_INLINE Reg sub(Reg a, Reg b) noexcept {
        return _mm512_sub_pd(a, b);
    }
    static CYDSP_SIMD_INLINE Reg mul(Reg a, Reg b) noexcept {
        return _mm512_mul_pd(a, b);
    }
    static CYDSP_SIMD_INLINE Reg div(Reg a, Reg b) noexcept {
        return _mm512_div_pd(a, b);
    }
    static CYDSP_SIMD_INLINE Reg sqrt(Reg a) noexcept {
        return _mm512_sqrt_pd(a);
    }
    static CYDSP_SIMD_INLINE Reg min(Reg a, Reg b) noexcept {
        return _mm512_min_pd(a, b);
    }
    static CYDSP_SIMD_INLINE Reg max(Reg a, Reg b) noexcept {
        return _mm512_max_pd(a, b);
    }
    static CYDSP_SIMD_INLINE Reg abs(Reg a) noexcept {
        return _mm512_andnot_pd(_mm512_set1_pd(-0.), a);
    }
    static CYDSP_SIMD_INLINE Reg signOf(Reg a) noexcept {
        return _mm512_and_pd(a, _mm512_set1_pd(-0.));
    }
    static CYDSP_SIMD_INLINE Reg orBits(Reg a, Reg b) noexcept {
        return _mm512_or_pd(a, b);
    }
    static CYDSP_SIMD_INLINE Reg xorBits(Reg a, Reg b) noexcept {
        return _mm512_xor_pd(a, b);
    }
    static CYDSP_SIMD_INLINE Mask gt(Reg a, Reg b) noexcept {
        return _mm512_cmp_pd_mask(a, b, _CMP_GT_OQ);
    }
    static CYDSP_SIMD_INLINE Mask lt(Reg a, Reg b) noexcept {
        return _mm512_cmp_pd_mask(a, b, _CMP_LT_OQ);
    }
    static CYDSP_SIMD_INLINE Mask le(Reg a, Reg b) noexcept {
        return _mm512_cmp_pd_mask(a, b, _CMP_LE_OQ);
    }
    static CYDSP_SIMD_INLINE Mask eq(Reg a, Reg b) noexcept {
        return _mm512_cmp_pd_mask(a, b, _CMP_EQ_OQ);
    }
    static CYDSP_SIMD_INLINE Mask both(Mask a, Mask b) noexcept {
        return static_cast<Mask>(a & b);
    }
    static CYDSP_SIMD_INLINE Mask either(Mask a, Mask b) noexcept {
        return static_cast<Mask>(a | b);
    }
    static CYDSP_SIMD_INLINE Mask without(Mask a, Mask b) noexcept {
        return static_cast<Mask>(a & ~b);
    }
    static CYDSP_SIMD_INLINE bool all(Mask m) noexcept {
        return m == 0xff;
    }
    static CYDSP_SIMD_INLINE Reg select(Mask m, Reg yes, Reg no) noexcept {
        return _mm512_mask_blend_pd(m, no, yes);
    }
    static CYDSP_SIMD_INLINE Reg nearest(Reg a) noexcept {
        return _mm512_roundscale_pd(a, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
    }
    static CYDSP_SIMD_INLINE Reg floor(Reg a) noexcept {
        return _mm512_roundscale_pd(a, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
    }
    static CYDSP_SIMD_INLINE Reg pow2(Reg n) noexcept {
        const auto exponent = _mm512_add_epi64(_mm512_cvtepi32_epi64(_mm512_cvttpd_epi32(n)),
                                               _mm512_set1_epi64(1023));
        return _mm512_castsi512_pd(_mm512_slli_epi64(exponent, 52));
    }
    static CYDSP_SIMD_INLINE void split(Reg u, Reg& mantissa, Reg& exponent) noexcept {
        exponent = _mm512_getexp_pd(u);
        mantissa = _mm512_getmant_pd(u, _MM_MANT_NORM_1_2, _MM_MANT_SIGN_src);
    }
    static CYDSP_SIMD_INLINE Reg shiftIn(Reg current, Reg carry) noexcept {
        const auto shifted =
            _mm512_permutexvar_pd(_mm512_setr_epi64(0, 0, 1, 2, 3, 4, 5, 6), current);
        const auto last = _mm512_permutexvar_pd(_mm512_set1_epi64(7), carry);
        return _mm512_mask_blend_pd(1, shifted, last);
    }
    static CYDSP_SIMD_INLINE Reg fromSamples(const float* p) noexcept {
        return _mm512_cvtps_pd(_mm256_loadu_ps(p));
    }
    static CYDSP_SIMD_INLINE Reg fromSamples(const double* p) noexcept {
        return _mm512_loadu_pd(p);
    }
    static CYDSP_SIMD_INLINE void toSamples(float* p, Reg v) noexcept {
        _mm256_storeu_ps(p, _mm512_cvtpd_ps(v));
    }
    static CYDSP_SIMD_INLINE void toSamples(double* p, Reg v) noexcept {
        _mm512_storeu_pd(p, v);
    }
    static CYDSP_SIMD_INLINE Reg rounded(Reg v, const float*) noexcept {
        return _mm512_cvtps_pd(_mm512_cvtpd_ps(v));
    }
    static CYDSP_SIMD_INLINE Reg rounded(Reg v, const double*) noexcept {
        return v;
    }
};
#endif
// Coefficients are listed from the constant term upward.
inline constexpr double simdExpTaylor[]{1.,
                                        1.,
                                        1. / 2.,
                                        1. / 6.,
                                        1. / 24.,
                                        1. / 120.,
                                        1. / 720.,
                                        1. / 5040.,
                                        1. / 40320.,
                                        1. / 362880.,
                                        1. / 3628800.,
                                        1. / 39916800.,
                                        1. / 479001600.,
                                        1. / 6227020800.};
// expm1(x)/x; the remainder is below 1e-17 for |x| < 0.25.
inline constexpr double simdExpm1Ratio[]{1.,
                                         1. / 2.,
                                         1. / 6.,
                                         1. / 24.,
                                         1. / 120.,
                                         1. / 720.,
                                         1. / 5040.,
                                         1. / 40320.,
                                         1. / 362880.,
                                         1. / 3628800.,
                                         1. / 39916800.,
                                         1. / 479001600.};
// atanh(z)/z in z*z; |z| <= 3-2*sqrt(2) keeps the remainder below 3e-19.
inline constexpr double simdAtanhRatio[]{1.,      1. / 3,  1. / 5,  1. / 7,  1. / 9, 1. / 11,
                                         1. / 13, 1. / 15, 1. / 17, 1. / 19, 1. / 21};
// atan(z)/z in z*z; |z| <= tan(pi/16) keeps the remainder below 2e-17.
inline constexpr double simdAtanRatio[]{1.,      -1. / 3,  1. / 5,  -1. / 7,  1. / 9,  -1. / 11,
                                        1. / 13, -1. / 15, 1. / 17, -1. / 19, 1. / 21, -1. / 23};
// Estrin evaluation: the same products as Horner's rule, arranged as a balanced
// tree. A dependent chain of 2N operations becomes about 2*log2(N), which is
// what bounds throughput once a vector group fills the reorder window.
template <class V, std::size_t N>
CYDSP_SIMD_INLINE typename V::Reg simdEstrin(const double* c,
                                             const typename V::Reg* powers) noexcept {
    if constexpr (N == 1)
        return V::set(c[0]);
    else if constexpr (N == 2)
        return V::add(V::set(c[0]), V::mul(powers[0], V::set(c[1])));
    else {
        constexpr std::size_t half = std::bit_floor(N - 1);
        return V::add(simdEstrin<V, half>(c, powers),
                      V::mul(powers[std::countr_zero(half)],
                             simdEstrin<V, N - half>(c + half, powers)));
    }
}
template <class V, std::size_t N>
CYDSP_SIMD_INLINE typename V::Reg simdPolynomial(const double (&c)[N], typename V::Reg x) noexcept {
    static_assert(N <= 16);
    const auto x2 = V::mul(x, x), x4 = V::mul(x2, x2);
    const typename V::Reg powers[4]{x, x2, x4, V::mul(x4, x4)};
    return simdEstrin<V, N>(c, powers);
}
// exp(x) for finite x well inside the normal exponent range.
template <class V> CYDSP_SIMD_INLINE typename V::Reg simdExp(typename V::Reg x) noexcept {
    const auto n = V::nearest(V::mul(x, V::set(1 / std::numbers::ln2)));
    constexpr double high = 0x1.62e42fefa0000p-1;
    constexpr double low = 0x1.cf79abc9e3b3ap-40;
    const auto r = V::sub(V::sub(x, V::mul(n, V::set(high))), V::mul(n, V::set(low)));
    return V::mul(simdPolynomial<V>(simdExpTaylor, r), V::pow2(n));
}
// exp(-2|x|), floored sameOctave 1.6e-28 where it no longer changes 1 + exp(-2|x|).
template <class V> CYDSP_SIMD_INLINE typename V::Reg simdDecay(typename V::Reg x) noexcept {
    return simdExp<V>(V::max(V::set(-64), V::mul(V::abs(x), V::set(-2))));
}
// Tiles are evaluated as a sequence of short passes over small stack arrays.
// One fused expression per vector group is a dependency chain of well over a
// hundred operations, which fills the floating-point scheduler and prevents
// consecutive groups from overlapping. Each pass below is a short chain, so
// several groups are in flight at once and the kernel runs at its arithmetic
// throughput. The arithmetic and its rounding are those of the fused formula.
inline constexpr std::size_t simdTile = 64;
inline constexpr std::size_t simdSlots = simdTile + 16;
// entries[0] = first, entries[1..count] = x; later lanes repeat the last sample
// so padded lanes stay finite and consistent with their predecessor.
template <class V>
CYDSP_SIMD_INLINE std::size_t simdLoadTile(const double* x, std::size_t count, double first,
                                           double* entries) noexcept {
    const auto groups = (count + V::width - 1) / V::width;
    entries[0] = first;
    std::copy_n(x, count, entries + 1);
    std::fill(entries + 1 + count, entries + (groups + 1) * V::width, x[count - 1]);
    return groups;
}
// Shared terms of the tanh mean. log(cosh a) - log(cosh b) is rewritten as
// (|a|-|b|) + log((1+p)/(1+q)) with p = exp(-2|a|), q = exp(-2|b|). The ratio
// is reduced by at most one octave to [1/sqrt2, sqrt2] and its logarithm taken
// as 2*atanh((u-s)/(u+s)).
template <class V> struct SimdTanhTerms {
    typename V::Reg d, x2, u, scaled, octave;
    typename V::Mask close, reduced;
};
template <class V>
CYDSP_SIMD_INLINE SimdTanhTerms<V> simdTanhTerms(typename V::Reg a, typename V::Reg b,
                                                 typename V::Reg p, typename V::Reg q) noexcept {
    const auto one = V::set(1);
    const auto d = V::sub(V::abs(a), V::abs(b));
    const auto x2 = V::mul(d, V::set(-2));
    const auto u = V::add(one, p), v = V::add(one, q);
    const auto high = V::gt(u, V::mul(v, V::set(std::numbers::sqrt2)));
    const auto low = V::lt(V::mul(u, V::set(std::numbers::sqrt2)), v);
    return {d,
            x2,
            u,
            V::select(high, V::add(v, v), V::select(low, V::mul(v, V::set(.5)), v)),
            V::select(high, V::set(std::numbers::ln2),
                      V::select(low, V::set(-std::numbers::ln2), V::set(0))),
            V::lt(V::abs(x2), V::set(.25)),
            V::either(high, low)};
}
// Mean of tanh between consecutive driven samples (first-order ADAA). When the
// endpoints are close, p-q is formed as q*expm1(-2d): no difference of nearly
// equal primitives is ever divided by a small step, and close endpoints on one
// side of zero need no division by the step at all. Absolute error stays near
// 1e-15 for all finite endpoints, including equal ones (the limit tanh(a)).
template <class V>
CYDSP_SIMD_INLINE void simdAdaaTanhTile(const double* x, double* y, std::size_t count,
                                        double first, double dc, double gain) noexcept {
    constexpr auto w = V::width;
    alignas(64) double sample[simdSlots], decay[simdSlots], scaledDecay[simdSlots],
        ratio[simdSlots], inverse[simdSlots], series[simdSlots], output[simdSlots];
    const auto groups = simdLoadTile<V>(x, count, first, sample);
    for (std::size_t g = 0; g <= groups; ++g)
        V::store(decay + g * w, simdDecay<V>(V::load(sample + g * w)));
    for (std::size_t g = 0; g < groups; ++g) {
        const auto i = g * w;
        const auto x2 = V::mul(
            V::sub(V::abs(V::load(sample + 1 + i)), V::abs(V::load(sample + i))), V::set(-2));
        V::store(scaledDecay + i,
                 V::mul(V::load(decay + i), simdPolynomial<V>(simdExpm1Ratio, x2)));
    }
    for (std::size_t g = 0; g < groups; ++g) {
        const auto i = g * w;
        const auto p = V::load(decay + 1 + i), q = V::load(decay + i);
        const auto t = simdTanhTerms<V>(V::load(sample + 1 + i), V::load(sample + i), p, q);
        const auto difference =
            V::select(t.close, V::mul(V::load(scaledDecay + i), t.x2), V::sub(p, q));
        const auto numerator = V::select(t.reduced, V::sub(t.u, t.scaled), difference);
        const auto reciprocal = V::div(V::set(1), V::add(t.u, t.scaled));
        V::store(inverse + i, reciprocal);
        V::store(ratio + i, V::mul(numerator, reciprocal));
    }
    for (std::size_t g = 0; g < groups; ++g) {
        const auto z = V::load(ratio + g * w);
        V::store(series + g * w, simdPolynomial<V>(simdAtanhRatio, V::mul(z, z)));
    }
    const auto offset = V::set(dc), scale = V::set(gain);
    for (std::size_t g = 0; g < groups; ++g) {
        const auto i = g * w;
        const auto a = V::load(sample + 1 + i), b = V::load(sample + i);
        const auto t = simdTanhTerms<V>(a, b, V::load(decay + 1 + i), V::load(decay + i));
        const auto z = V::load(ratio + i), sum = V::load(series + i);
        const auto direct = V::without(t.close, V::lt(V::mul(a, b), V::set(0)));
        const auto limit = V::xorBits(
            V::mul(V::sub(V::add(t.u, t.scaled),
                          V::mul(V::set(4), V::mul(V::load(scaledDecay + i), sum))),
                   V::load(inverse + i)),
            V::signOf(V::add(a, b)));
        const auto integral = V::add(V::add(t.d, t.octave), V::mul(V::mul(V::set(2), z), sum));
        const auto step = V::select(direct, V::set(1), V::sub(a, b));
        const auto mean = V::select(direct, limit, V::div(integral, step));
        V::store(output + i, V::mul(V::sub(mean, offset), scale));
    }
    std::copy_n(output, count, y);
}
template <class V>
CYDSP_SIMD_INLINE void simdShapeTanhTile(const double* x, double* y, std::size_t count,
                                         double dc, double gain) noexcept {
    constexpr auto w = V::width;
    alignas(64) double sample[simdSlots], decay[simdSlots], output[simdSlots];
    const auto groups = (count + w - 1) / w;
    std::fill(std::copy_n(x, count, sample), sample + groups * w, 0.);
    for (std::size_t g = 0; g < groups; ++g)
        V::store(decay + g * w, simdDecay<V>(V::load(sample + g * w)));
    const auto offset = V::set(dc), scale = V::set(gain), one = V::set(1);
    for (std::size_t g = 0; g < groups; ++g) {
        const auto v = V::load(sample + g * w), p = V::load(decay + g * w);
        const auto magnitude = V::abs(v), doubled = V::add(magnitude, magnitude);
        // 1-p cancels near zero; expm1 keeps the relative accuracy of tiny inputs.
        const auto numerator = V::select(
            V::lt(magnitude, V::set(.125)),
            V::mul(doubled, simdPolynomial<V>(simdExpm1Ratio, V::sub(V::set(0), doubled))),
            V::sub(one, p));
        const auto value = V::xorBits(V::div(numerator, V::add(one, p)), V::signOf(v));
        V::store(output + g * w, V::mul(V::sub(value, offset), scale));
    }
    std::copy_n(output, count, y);
}
inline constexpr double simdAtanUnreduced = 0x1.975f5e0553158p-3; // tan(pi/16)
// Five-interval argument reduction around 0, tan(pi/8), 1, tan(3pi/8), infinity.
// reduce() yields z with atan|x| = base + atan z; unreduced lanes have z = |x|.
template <class V> struct SimdAtanReduction {
    typename V::Reg z, base;
};
template <class V>
CYDSP_SIMD_INLINE SimdAtanReduction<V> simdAtanReduce(typename V::Reg x) noexcept {
    constexpr double c1 = 0x1.a827999fcef32p-2, c3 = 0x1.3504f333f9de6p+1; // tan(pi/8), tan(3pi/8)
    constexpr double b1 = 0x1.921fb54442d18p-2, b3 = 0x1.2d97c7f3321d2p+0; // their arctangents
    const auto a = V::abs(x), one = V::set(1);
    const auto r1 = V::gt(a, V::set(simdAtanUnreduced)), r3 = V::gt(a, V::set(0x1.561b82ab7f990p-1)),
               r5 = V::gt(a, V::set(0x1.7f218e25a7461p+0)),
               r7 = V::gt(a, V::set(0x1.41bfee242476fp+2)); // tan(k*pi/16), k = 1,3,5,7
    auto numerator = V::select(r1, V::sub(a, V::set(c1)), a);
    auto denominator = V::select(r1, V::add(one, V::mul(a, V::set(c1))), one);
    auto base = V::select(r1, V::set(b1), V::set(0));
    numerator = V::select(r3, V::sub(a, one), numerator);
    denominator = V::select(r3, V::add(a, one), denominator);
    base = V::select(r3, V::set(std::numbers::pi / 4), base);
    numerator = V::select(r5, V::sub(a, V::set(c3)), numerator);
    denominator = V::select(r5, V::add(one, V::mul(a, V::set(c3))), denominator);
    base = V::select(r5, V::set(b3), base);
    numerator = V::select(r7, V::set(-1), numerator);
    denominator = V::select(r7, a, denominator);
    base = V::select(r7, V::set(std::numbers::pi / 2), base);
    return {V::div(numerator, denominator), base};
}
// atan over `groups` vector groups: value[] and, optionally, the series ratio
// atan(z)/z (equal to atan(x)/x on unreduced lanes).
template <class V>
CYDSP_SIMD_INLINE void simdAtanTile(const double* x, std::size_t groups, double* value,
                                    double* ratio) noexcept {
    constexpr auto w = V::width;
    alignas(64) double reduced[simdSlots], base[simdSlots];
    for (std::size_t g = 0; g < groups; ++g) {
        const auto r = simdAtanReduce<V>(V::load(x + g * w));
        V::store(reduced + g * w, r.z);
        V::store(base + g * w, r.base);
    }
    for (std::size_t g = 0; g < groups; ++g) {
        const auto z = V::load(reduced + g * w);
        const auto sum = simdPolynomial<V>(simdAtanRatio, V::mul(z, z));
        if (ratio)
            V::store(ratio + g * w, sum);
        V::store(value + g * w, V::xorBits(V::add(V::load(base + g * w), V::mul(z, sum)),
                                           V::signOf(V::load(x + g * w))));
    }
}
// Mean of (2/pi)atan between consecutive driven samples. With m the midpoint
// and 2h the step, the primitive difference is regrouped as
// h(atan a + atan b) + m(atan a - atan b) - log((1+a*a)/(1+b*b))/2. The two
// differences are formed from atan(2h/(1+ab)) and from the exact a*a-b*b, so
// close endpoints use series ratios and never divide a cancelled value by h.
// Absolute error stays near 1e-15 for all finite endpoints.
template <class V>
CYDSP_SIMD_INLINE void simdAdaaAtanTile(const double* x, double* y, std::size_t count,
                                        double first, double dc, double gain) noexcept {
    constexpr auto w = V::width;
    alignas(64) double sample[simdSlots], angle[simdSlots], crossInverse[simdSlots],
        turn[simdSlots], turnAngle[simdSlots], turnRatio[simdSlots], octaves[simdSlots],
        ratio[simdSlots], inverse[simdSlots], series[simdSlots], output[simdSlots];
    const auto groups = simdLoadTile<V>(x, count, first, sample);
    const auto one = V::set(1), zero = V::set(0);
    simdAtanTile<V>(sample, groups + 1, angle, nullptr);
    for (std::size_t g = 0; g < groups; ++g) {
        const auto i = g * w;
        const auto a = V::load(sample + 1 + i), b = V::load(sample + i);
        const auto reciprocal = V::div(one, V::add(one, V::mul(a, b)));
        V::store(crossInverse + i, reciprocal);
        V::store(turn + i, V::mul(V::sub(a, b), reciprocal));
    }
    simdAtanTile<V>(turn, groups, turnAngle, turnRatio);
    for (std::size_t g = 0; g < groups; ++g) {
        const auto i = g * w;
        const auto a = V::load(sample + 1 + i), b = V::load(sample + i);
        const auto u = V::add(one, V::mul(a, a)), v = V::add(one, V::mul(b, b));
        typename V::Reg mantissaU, exponentU, mantissaV, exponentV;
        V::split(u, mantissaU, exponentU);
        V::split(v, mantissaV, exponentV);
        const auto high = V::gt(mantissaU, V::mul(mantissaV, V::set(std::numbers::sqrt2)));
        const auto low = V::lt(V::mul(mantissaU, V::set(std::numbers::sqrt2)), mantissaV);
        const auto scaled = V::select(high, V::add(mantissaV, mantissaV),
                                      V::select(low, V::mul(mantissaV, V::set(.5)), mantissaV));
        const auto distance = V::add(V::sub(exponentU, exponentV),
                                     V::select(high, one, V::select(low, V::set(-1), zero)));
        const auto sameOctave = V::eq(distance, zero);
        const auto numerator =
            V::select(sameOctave, V::mul(V::add(a, b), V::sub(a, b)), V::sub(mantissaU, scaled));
        const auto reciprocal = V::div(
            one, V::select(sameOctave, V::add(u, v), V::add(mantissaU, scaled)));
        V::store(octaves + i, distance);
        V::store(inverse + i, reciprocal);
        V::store(ratio + i, V::mul(numerator, reciprocal));
    }
    for (std::size_t g = 0; g < groups; ++g) {
        const auto z = V::load(ratio + g * w);
        V::store(series + g * w, simdPolynomial<V>(simdAtanhRatio, V::mul(z, z)));
    }
    const auto offset = V::set(dc), scale = V::set(gain);
    for (std::size_t g = 0; g < groups; ++g) {
        const auto i = g * w;
        const auto a = V::load(sample + 1 + i), b = V::load(sample + i);
        const auto midpoint = V::mul(V::set(.5), V::add(a, b)), step = V::sub(a, b);
        const auto cross = V::add(one, V::mul(a, b));
        const auto unreduced =
            V::both(V::le(V::abs(V::load(turn + i)), V::set(simdAtanUnreduced)),
                    V::gt(cross, zero));
        const auto rotation =
            V::add(V::load(turnAngle + i),
                   V::select(V::lt(cross, zero),
                             V::orBits(V::set(std::numbers::pi), V::signOf(a)), zero));
        const auto distance = V::load(octaves + i);
        const auto sameOctave = V::eq(distance, zero);
        const auto direct = V::both(unreduced, sameOctave);
        const auto stepInverse = V::div(one, V::select(direct, one, step));
        const auto z = V::load(ratio + i), sum = V::load(series + i);
        const auto angleRatio =
            V::select(unreduced, V::mul(V::load(turnRatio + i), V::load(crossInverse + i)),
                      V::mul(rotation, stepInverse));
        const auto logarithmRatio = V::select(
            sameOctave, V::mul(V::mul(V::add(midpoint, midpoint), sum), V::load(inverse + i)),
            V::mul(V::add(V::mul(distance, V::set(std::numbers::ln2)),
                          V::mul(V::mul(V::set(2), z), sum)),
                   V::mul(V::set(.5), stepInverse)));
        const auto mean = V::mul(
            V::set(2 / std::numbers::pi),
            V::add(V::mul(V::set(.5), V::add(V::load(angle + 1 + i), V::load(angle + i))),
                   V::sub(V::mul(midpoint, angleRatio), logarithmRatio)));
        V::store(output + i, V::mul(V::sub(mean, offset), scale));
    }
    std::copy_n(output, count, y);
}
template <class V>
CYDSP_SIMD_INLINE void simdShapeAtanTile(const double* x, double* y, std::size_t count,
                                         double dc, double gain) noexcept {
    constexpr auto w = V::width;
    alignas(64) double sample[simdSlots], angle[simdSlots];
    const auto groups = (count + w - 1) / w;
    std::fill(std::copy_n(x, count, sample), sample + groups * w, 0.);
    simdAtanTile<V>(sample, groups, angle, nullptr);
    const auto offset = V::set(dc), scale = V::set(gain);
    for (std::size_t g = 0; g < groups; ++g)
        V::store(angle + g * w,
                 V::mul(V::sub(V::mul(V::set(2 / std::numbers::pi), V::load(angle + g * w)),
                               offset),
                        scale));
    std::copy_n(angle, count, y);
}
// previous precedes x[0]; an inactive history duplicates x[0], which selects
// the memoryless shape exactly. x and y are disjoint.
template <class V, class Tile>
CYDSP_SIMD_INLINE void simdAdaaTiles(const double* x, double* y, std::size_t n, double previous,
                                     bool active, double dc, double gain, Tile&& tile) noexcept {
    for (std::size_t offset = 0; offset < n;) {
        const auto count = std::min(simdTile, n - offset);
        tile(x + offset, y + offset, count,
             offset ? x[offset - 1] : active ? previous : x[0], dc, gain);
        offset += count;
    }
}
template <class V>
CYDSP_SIMD_INLINE void simdAdaaTanh(const double* x, double* y, std::size_t n, double previous,
                                    bool active, double dc, double gain) noexcept {
    simdAdaaTiles<V>(x, y, n, previous, active, dc, gain,
                     [](const double* in, double* out, std::size_t count, double first,
                        double offset, double scale) noexcept {
                         simdAdaaTanhTile<V>(in, out, count, first, offset, scale);
                     });
}
template <class V>
CYDSP_SIMD_INLINE void simdAdaaAtan(const double* x, double* y, std::size_t n, double previous,
                                    bool active, double dc, double gain) noexcept {
    simdAdaaTiles<V>(x, y, n, previous, active, dc, gain,
                     [](const double* in, double* out, std::size_t count, double first,
                        double offset, double scale) noexcept {
                         simdAdaaAtanTile<V>(in, out, count, first, offset, scale);
                     });
}
// Memoryless (shape(x)-dc)*gain on driven samples; x and y may be identical.
template <class V>
CYDSP_SIMD_INLINE void simdShapeTanh(const double* x, double* y, std::size_t n, double dc,
                                     double gain) noexcept {
    for (std::size_t offset = 0; offset < n; offset += simdTile)
        simdShapeTanhTile<V>(x + offset, y + offset, std::min(simdTile, n - offset), dc, gain);
}
template <class V>
CYDSP_SIMD_INLINE void simdShapeAtan(const double* x, double* y, std::size_t n, double dc,
                                     double gain) noexcept {
    for (std::size_t offset = 0; offset < n; offset += simdTile)
        simdShapeAtanTile<V>(x + offset, y + offset, std::min(simdTile, n - offset), dc, gain);
}
// stage = {inputGain, inputBias, inputScale, drive, bias}; the optional input
// stage and the +/-1e6 clamp keep the scalar operation order and rounding.
template <class V, class T>
CYDSP_SIMD_INLINE void simdSaturationDrive(const T* in, double* out, std::size_t n, const double* stage,
                                bool staged) noexcept {
    const auto drive = V::set(stage[3]), bias = V::set(stage[4]);
    const auto lower = V::set(-1e6), upper = V::set(1e6);
    std::size_t i = 0;
    for (; n - i >= V::width; i += V::width) {
        auto sample = V::fromSamples(in + i);
        if (staged)
            sample = V::mul(V::add(V::mul(sample, V::set(stage[0])), V::set(stage[1])),
                            V::set(stage[2]));
        V::store(out + i, V::min(V::max(V::add(V::mul(sample, drive), bias), lower), upper));
    }
    for (; i < n; ++i) {
        double sample = double(in[i]);
        if (staged)
            sample = (sample * stage[0] + stage[1]) * stage[2];
        out[i] = std::clamp(sample * stage[3] + stage[4], -1e6, 1e6);
    }
}
// out = dryGain*dry + wetGain*T(wet) when mixed, else T(wet). dry may alias out.
template <class V, class T>
CYDSP_SIMD_INLINE void simdSaturationPublish(const T* dry, const double* wet, T* out, std::size_t n,
                                  double dryGain, double wetGain, bool mixed) noexcept {
    std::size_t i = 0;
    for (; n - i >= V::width; i += V::width) {
        auto value = V::load(wet + i);
        if (mixed)
            value = V::add(V::mul(V::set(dryGain), V::fromSamples(dry + i)),
                           V::mul(V::set(wetGain), V::rounded(value, out)));
        V::toSamples(out + i, value);
    }
    for (; i < n; ++i)
        out[i] = mixed ? T(dryGain * double(dry[i]) + wetGain * double(T(wet[i]))) : T(wet[i]);
}
} // namespace
} // namespace cydsp::detail
