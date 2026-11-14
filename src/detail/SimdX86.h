#pragma once
#include <immintrin.h>
namespace cydsp::detail {
namespace {
struct Double4 {
    using Reg = __m256d;
    using Mask = __m256d;
    static constexpr std::size_t maxWidth = 4;
    static constexpr std::size_t lanes() noexcept {
        return 4;
    }
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
    static constexpr std::size_t maxWidth = 8;
    static constexpr std::size_t lanes() noexcept {
        return 8;
    }
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
} // namespace
} // namespace cydsp::detail
