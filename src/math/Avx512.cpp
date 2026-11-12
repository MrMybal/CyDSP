#include "detail/Scalar.h"
#include "detail/VectorTable.h"
#include <array>
#include <cstdint>
#include <immintrin.h>
namespace cydsp::detail {
namespace {
__mmask16 maskF32(std::size_t n) noexcept {
    return static_cast<__mmask16>((1u << n) - 1u);
}
void copyF32(const float* a, float* d, std::size_t n) noexcept {
    std::size_t i = 0;
    for (; n - i >= 16; i += 16)
        _mm512_storeu_ps(d + i, _mm512_loadu_ps(a + i));
    if (i < n) {
        const auto pg = maskF32(n - i);
        _mm512_mask_storeu_ps(d + i, pg, _mm512_maskz_loadu_ps(pg, a + i));
    }
}
void scaleF32(const float* a, float* d, std::size_t n, float gain) noexcept {
    std::size_t i = 0;
    for (; n - i >= 16; i += 16)
        _mm512_storeu_ps(d + i, _mm512_mul_ps(_mm512_loadu_ps(a + i), _mm512_set1_ps(gain)));
    if (i < n) {
        const auto pg = maskF32(n - i);
        _mm512_mask_storeu_ps(
            d + i, pg, _mm512_mul_ps(_mm512_maskz_loadu_ps(pg, a + i), _mm512_set1_ps(gain)));
    }
}
void addScaledF32(const float* a, float* d, std::size_t n, float gain) noexcept {
    std::size_t i = 0;
    for (; n - i >= 16; i += 16)
        _mm512_storeu_ps(
            d + i, _mm512_add_ps(_mm512_loadu_ps(d + i),
                                 _mm512_mul_ps(_mm512_loadu_ps(a + i), _mm512_set1_ps(gain))));
    if (i < n) {
        const auto pg = maskF32(n - i);
        _mm512_mask_storeu_ps(
            d + i, pg,
            _mm512_add_ps(_mm512_maskz_loadu_ps(pg, d + i),
                          _mm512_mul_ps(_mm512_maskz_loadu_ps(pg, a + i), _mm512_set1_ps(gain))));
    }
}
void multiplyF32(const float* a, const float* b, float* d, std::size_t n) noexcept {
    for (std::size_t i = 0; i < n; i += 16) {
        const auto pg = maskF32(n - i < 16 ? n - i : 16);
        const auto x = _mm512_maskz_loadu_ps(pg, a + i);
        _mm512_mask_storeu_ps(d + i, pg, _mm512_mul_ps(x, _mm512_maskz_loadu_ps(pg, b + i)));
    }
}
void fillF32(float* d, std::size_t n, float value) noexcept {
    std::size_t i = 0;
    for (; n - i >= 16; i += 16)
        _mm512_storeu_ps(d + i, _mm512_set1_ps(value));
    if (i < n) {
        const auto pg = maskF32(n - i);
        _mm512_mask_storeu_ps(d + i, pg, _mm512_set1_ps(value));
    }
}
float dotF32(const float* a, const float* b, std::size_t n) noexcept {
    if (!n)
        return float{};
    auto s0 = _mm512_setzero_ps(), s1 = s0, s2 = s0, s3 = s0;
    std::size_t i = 0;
    for (; n - i >= 64; i += 64) {
        s0 = _mm512_add_ps(s0,
                           _mm512_mul_ps(_mm512_loadu_ps(a + i + 0), _mm512_loadu_ps(b + i + 0)));
        s1 = _mm512_add_ps(s1,
                           _mm512_mul_ps(_mm512_loadu_ps(a + i + 16), _mm512_loadu_ps(b + i + 16)));
        s2 = _mm512_add_ps(s2,
                           _mm512_mul_ps(_mm512_loadu_ps(a + i + 32), _mm512_loadu_ps(b + i + 32)));
        s3 = _mm512_add_ps(s3,
                           _mm512_mul_ps(_mm512_loadu_ps(a + i + 48), _mm512_loadu_ps(b + i + 48)));
    }
    auto sum = _mm512_add_ps(_mm512_add_ps(s0, s1), _mm512_add_ps(s2, s3));
    for (; n - i >= 16; i += 16)
        sum = _mm512_add_ps(sum, _mm512_mul_ps(_mm512_loadu_ps(a + i), _mm512_loadu_ps(b + i)));
    if (i < n) {
        const auto pg = maskF32(n - i);
        sum = _mm512_add_ps(
            sum, _mm512_mul_ps(_mm512_maskz_loadu_ps(pg, a + i), _mm512_maskz_loadu_ps(pg, b + i)));
    }
    return _mm512_reduce_add_ps(sum);
}
__m512 conjugateF32(__m512 a) noexcept {
    return _mm512_castsi512_ps(_mm512_xor_si512(
        _mm512_castps_si512(a),
        _mm512_setr_epi32(0, -2147483647 - 1, 0, -2147483647 - 1, 0, -2147483647 - 1, 0,
                          -2147483647 - 1, 0, -2147483647 - 1, 0, -2147483647 - 1, 0,
                          -2147483647 - 1, 0, -2147483647 - 1)));
}
__m512 productF32(__m512 a, __m512 b) noexcept {
    auto cross = _mm512_mul_ps(_mm512_permute_ps(a, _MM_SHUFFLE(3, 3, 1, 1)),
                               _mm512_permute_ps(b, _MM_SHUFFLE(2, 3, 0, 1)));
    // Negate real lanes (the complementary sign pattern to conjugation).
    cross = _mm512_castsi512_ps(
        _mm512_xor_si512(_mm512_castps_si512(cross),
                         _mm512_xor_si512(_mm512_setr_epi32(0, -2147483647 - 1, 0, -2147483647 - 1,
                                                            0, -2147483647 - 1, 0, -2147483647 - 1,
                                                            0, -2147483647 - 1, 0, -2147483647 - 1,
                                                            0, -2147483647 - 1, 0, -2147483647 - 1),
                                          _mm512_set1_epi32(-2147483647 - 1))));
    return _mm512_add_ps(_mm512_mul_ps(_mm512_permute_ps(a, _MM_SHUFFLE(2, 2, 0, 0)), b), cross);
}
void complexMacF32(const float* a, const float* b, float* d, std::size_t n) noexcept {
    for (std::size_t i = 0; i < n; i += 8) {
        auto pg = maskF32(2 * (n - i < 8 ? n - i : 8));
        auto result = _mm512_add_ps(
            _mm512_maskz_loadu_ps(pg, d + 2 * i),
            productF32(_mm512_maskz_loadu_ps(pg, a + 2 * i), _mm512_maskz_loadu_ps(pg, b + 2 * i)));
        _mm512_mask_storeu_ps(d + 2 * i, pg, result);
    }
}
void butterflyF32(float* e, float* o, const float* w, std::size_t n, bool inverse) noexcept {
    for (std::size_t i = 0; i < n; i += 8) {
        auto pg = maskF32(2 * (n - i < 8 ? n - i : 8));
        auto tw = _mm512_maskz_loadu_ps(pg, w + 2 * i);
        if (inverse)
            tw = conjugateF32(tw);
        auto even = _mm512_maskz_loadu_ps(pg, e + 2 * i),
             prod = productF32(_mm512_maskz_loadu_ps(pg, o + 2 * i), tw);
        _mm512_mask_storeu_ps(e + 2 * i, pg, _mm512_add_ps(even, prod));
        _mm512_mask_storeu_ps(o + 2 * i, pg, _mm512_sub_ps(even, prod));
    }
}
void biquadBankF32(float* data, std::size_t voices, std::size_t frames, const float* c,
                   float* state) noexcept {
    for (std::size_t i = 0; i < voices; i += 16) {
        const auto pg = maskF32(voices - i < 16 ? voices - i : 16);
        auto c0 = _mm512_maskz_loadu_ps(pg, c + 0 * voices + i);
        auto c1 = _mm512_maskz_loadu_ps(pg, c + 1 * voices + i);
        auto c2 = _mm512_maskz_loadu_ps(pg, c + 2 * voices + i);
        auto c3 = _mm512_maskz_loadu_ps(pg, c + 3 * voices + i);
        auto c4 = _mm512_maskz_loadu_ps(pg, c + 4 * voices + i);
        auto z1 = _mm512_maskz_loadu_ps(pg, state + i),
             z2 = _mm512_maskz_loadu_ps(pg, state + voices + i);
        for (std::size_t f = 0; f < frames; ++f) {
            auto x = _mm512_maskz_loadu_ps(pg, data + f * voices + i);
            auto y = _mm512_add_ps(_mm512_mul_ps(c0, x), z1);
            z1 = _mm512_add_ps(_mm512_sub_ps(_mm512_mul_ps(c1, x), _mm512_mul_ps(c3, y)), z2);
            z2 = _mm512_sub_ps(_mm512_mul_ps(c2, x), _mm512_mul_ps(c4, y));
            _mm512_mask_storeu_ps(data + f * voices + i, pg, y);
        }
        _mm512_mask_storeu_ps(state + i, pg, z1);
        _mm512_mask_storeu_ps(state + voices + i, pg, z2);
    }
}
__mmask8 maskF64(std::size_t n) noexcept {
    return static_cast<__mmask8>((1u << n) - 1u);
}
void copyF64(const double* a, double* d, std::size_t n) noexcept {
    std::size_t i = 0;
    for (; n - i >= 8; i += 8)
        _mm512_storeu_pd(d + i, _mm512_loadu_pd(a + i));
    if (i < n) {
        const auto pg = maskF64(n - i);
        _mm512_mask_storeu_pd(d + i, pg, _mm512_maskz_loadu_pd(pg, a + i));
    }
}
void scaleF64(const double* a, double* d, std::size_t n, double gain) noexcept {
    std::size_t i = 0;
    for (; n - i >= 8; i += 8)
        _mm512_storeu_pd(d + i, _mm512_mul_pd(_mm512_loadu_pd(a + i), _mm512_set1_pd(gain)));
    if (i < n) {
        const auto pg = maskF64(n - i);
        _mm512_mask_storeu_pd(
            d + i, pg, _mm512_mul_pd(_mm512_maskz_loadu_pd(pg, a + i), _mm512_set1_pd(gain)));
    }
}
void addScaledF64(const double* a, double* d, std::size_t n, double gain) noexcept {
    std::size_t i = 0;
    for (; n - i >= 8; i += 8)
        _mm512_storeu_pd(
            d + i, _mm512_add_pd(_mm512_loadu_pd(d + i),
                                 _mm512_mul_pd(_mm512_loadu_pd(a + i), _mm512_set1_pd(gain))));
    if (i < n) {
        const auto pg = maskF64(n - i);
        _mm512_mask_storeu_pd(
            d + i, pg,
            _mm512_add_pd(_mm512_maskz_loadu_pd(pg, d + i),
                          _mm512_mul_pd(_mm512_maskz_loadu_pd(pg, a + i), _mm512_set1_pd(gain))));
    }
}
void multiplyF64(const double* a, const double* b, double* d, std::size_t n) noexcept {
    for (std::size_t i = 0; i < n; i += 8) {
        const auto pg = maskF64(n - i < 8 ? n - i : 8);
        const auto x = _mm512_maskz_loadu_pd(pg, a + i);
        _mm512_mask_storeu_pd(d + i, pg, _mm512_mul_pd(x, _mm512_maskz_loadu_pd(pg, b + i)));
    }
}
void fillF64(double* d, std::size_t n, double value) noexcept {
    std::size_t i = 0;
    for (; n - i >= 8; i += 8)
        _mm512_storeu_pd(d + i, _mm512_set1_pd(value));
    if (i < n) {
        const auto pg = maskF64(n - i);
        _mm512_mask_storeu_pd(d + i, pg, _mm512_set1_pd(value));
    }
}
double dotF64(const double* a, const double* b, std::size_t n) noexcept {
    if (!n)
        return double{};
    auto s0 = _mm512_setzero_pd(), s1 = s0, s2 = s0, s3 = s0;
    std::size_t i = 0;
    for (; n - i >= 32; i += 32) {
        s0 = _mm512_add_pd(s0,
                           _mm512_mul_pd(_mm512_loadu_pd(a + i + 0), _mm512_loadu_pd(b + i + 0)));
        s1 = _mm512_add_pd(s1,
                           _mm512_mul_pd(_mm512_loadu_pd(a + i + 8), _mm512_loadu_pd(b + i + 8)));
        s2 = _mm512_add_pd(s2,
                           _mm512_mul_pd(_mm512_loadu_pd(a + i + 16), _mm512_loadu_pd(b + i + 16)));
        s3 = _mm512_add_pd(s3,
                           _mm512_mul_pd(_mm512_loadu_pd(a + i + 24), _mm512_loadu_pd(b + i + 24)));
    }
    auto sum = _mm512_add_pd(_mm512_add_pd(s0, s1), _mm512_add_pd(s2, s3));
    for (; n - i >= 8; i += 8)
        sum = _mm512_add_pd(sum, _mm512_mul_pd(_mm512_loadu_pd(a + i), _mm512_loadu_pd(b + i)));
    if (i < n) {
        const auto pg = maskF64(n - i);
        sum = _mm512_add_pd(
            sum, _mm512_mul_pd(_mm512_maskz_loadu_pd(pg, a + i), _mm512_maskz_loadu_pd(pg, b + i)));
    }
    return _mm512_reduce_add_pd(sum);
}
__m512d conjugateF64(__m512d a) noexcept {
    return _mm512_castsi512_pd(_mm512_xor_si512(
        _mm512_castpd_si512(a),
        _mm512_setr_epi64(0, (-9223372036854775807LL - 1), 0, (-9223372036854775807LL - 1), 0,
                          (-9223372036854775807LL - 1), 0, (-9223372036854775807LL - 1))));
}
__m512d productF64(__m512d a, __m512d b) noexcept {
    auto cross = _mm512_mul_pd(_mm512_permute_pd(a, 255), _mm512_permute_pd(b, 85));
    // Negate real lanes (the complementary sign pattern to conjugation).
    cross = _mm512_castsi512_pd(_mm512_xor_si512(
        _mm512_castpd_si512(cross),
        _mm512_xor_si512(
            _mm512_setr_epi64(0, (-9223372036854775807LL - 1), 0, (-9223372036854775807LL - 1), 0,
                              (-9223372036854775807LL - 1), 0, (-9223372036854775807LL - 1)),
            _mm512_set1_epi64((-9223372036854775807LL - 1)))));
    return _mm512_add_pd(_mm512_mul_pd(_mm512_permute_pd(a, 0), b), cross);
}
void complexMacF64(const double* a, const double* b, double* d, std::size_t n) noexcept {
    for (std::size_t i = 0; i < n; i += 4) {
        auto pg = maskF64(2 * (n - i < 4 ? n - i : 4));
        auto result = _mm512_add_pd(
            _mm512_maskz_loadu_pd(pg, d + 2 * i),
            productF64(_mm512_maskz_loadu_pd(pg, a + 2 * i), _mm512_maskz_loadu_pd(pg, b + 2 * i)));
        _mm512_mask_storeu_pd(d + 2 * i, pg, result);
    }
}
void butterflyF64(double* e, double* o, const double* w, std::size_t n, bool inverse) noexcept {
    for (std::size_t i = 0; i < n; i += 4) {
        auto pg = maskF64(2 * (n - i < 4 ? n - i : 4));
        auto tw = _mm512_maskz_loadu_pd(pg, w + 2 * i);
        if (inverse)
            tw = conjugateF64(tw);
        auto even = _mm512_maskz_loadu_pd(pg, e + 2 * i),
             prod = productF64(_mm512_maskz_loadu_pd(pg, o + 2 * i), tw);
        _mm512_mask_storeu_pd(e + 2 * i, pg, _mm512_add_pd(even, prod));
        _mm512_mask_storeu_pd(o + 2 * i, pg, _mm512_sub_pd(even, prod));
    }
}
void biquadBankF64(double* data, std::size_t voices, std::size_t frames, const double* c,
                   double* state) noexcept {
    for (std::size_t i = 0; i < voices; i += 8) {
        const auto pg = maskF64(voices - i < 8 ? voices - i : 8);
        auto c0 = _mm512_maskz_loadu_pd(pg, c + 0 * voices + i);
        auto c1 = _mm512_maskz_loadu_pd(pg, c + 1 * voices + i);
        auto c2 = _mm512_maskz_loadu_pd(pg, c + 2 * voices + i);
        auto c3 = _mm512_maskz_loadu_pd(pg, c + 3 * voices + i);
        auto c4 = _mm512_maskz_loadu_pd(pg, c + 4 * voices + i);
        auto z1 = _mm512_maskz_loadu_pd(pg, state + i),
             z2 = _mm512_maskz_loadu_pd(pg, state + voices + i);
        for (std::size_t f = 0; f < frames; ++f) {
            auto x = _mm512_maskz_loadu_pd(pg, data + f * voices + i);
            auto y = _mm512_add_pd(_mm512_mul_pd(c0, x), z1);
            z1 = _mm512_add_pd(_mm512_sub_pd(_mm512_mul_pd(c1, x), _mm512_mul_pd(c3, y)), z2);
            z2 = _mm512_sub_pd(_mm512_mul_pd(c2, x), _mm512_mul_pd(c4, y));
            _mm512_mask_storeu_pd(data + f * voices + i, pg, y);
        }
        _mm512_mask_storeu_pd(state + i, pg, z1);
        _mm512_mask_storeu_pd(state + voices + i, pg, z2);
    }
}
double dotMixed(const float* a, const double* b, std::size_t n) noexcept {
    if (!n)
        return 0;
    auto s0 = _mm512_setzero_pd(), s1 = s0, s2 = s0, s3 = s0;
    std::size_t i = 0;
    for (; n - i >= 32; i += 32) {
        s0 = _mm512_add_pd(s0, _mm512_mul_pd(_mm512_cvtps_pd(_mm256_loadu_ps(a + i + 0)),
                                             _mm512_loadu_pd(b + i + 0)));
        s1 = _mm512_add_pd(s1, _mm512_mul_pd(_mm512_cvtps_pd(_mm256_loadu_ps(a + i + 8)),
                                             _mm512_loadu_pd(b + i + 8)));
        s2 = _mm512_add_pd(s2, _mm512_mul_pd(_mm512_cvtps_pd(_mm256_loadu_ps(a + i + 16)),
                                             _mm512_loadu_pd(b + i + 16)));
        s3 = _mm512_add_pd(s3, _mm512_mul_pd(_mm512_cvtps_pd(_mm256_loadu_ps(a + i + 24)),
                                             _mm512_loadu_pd(b + i + 24)));
    }
    auto sum = _mm512_add_pd(_mm512_add_pd(s0, s1), _mm512_add_pd(s2, s3));
    for (; n - i >= 8; i += 8)
        sum = _mm512_add_pd(sum, _mm512_mul_pd(_mm512_cvtps_pd(_mm256_loadu_ps(a + i + 0)),
                                               _mm512_loadu_pd(b + i)));
    if (i < n) {
        auto pg = maskF64(n - i);
        sum = _mm512_add_pd(sum, _mm512_mul_pd(_mm512_cvtps_pd(_mm256_maskz_loadu_ps(pg, a + i)),
                                               _mm512_maskz_loadu_pd(pg, b + i)));
    }
    return _mm512_reduce_add_pd(sum);
}
void gainChainF32(const float* a, float* d, std::size_t n, const float* gains,
                  std::size_t stages) noexcept {
    const auto zero = _mm512_setzero_ps();
    std::size_t i = 0;
    for (; n - i >= 4 * 16; i += 4 * 16) {
        auto v0 = _mm512_loadu_ps(a + i + 0 * 16), v1 = _mm512_loadu_ps(a + i + 1 * 16),
             v2 = _mm512_loadu_ps(a + i + 2 * 16), v3 = _mm512_loadu_ps(a + i + 3 * 16);
        for (std::size_t stage = 0; stage < stages; ++stage) {
            if (gains[stage] == float{})
                v0 = v1 = v2 = v3 = zero;
            else {
                const auto gain = _mm512_set1_ps(gains[stage]);
                v0 = _mm512_add_ps(zero, _mm512_mul_ps(v0, gain));
                v1 = _mm512_add_ps(zero, _mm512_mul_ps(v1, gain));
                v2 = _mm512_add_ps(zero, _mm512_mul_ps(v2, gain));
                v3 = _mm512_add_ps(zero, _mm512_mul_ps(v3, gain));
            }
        }
        _mm512_storeu_ps(d + i + 0 * 16, v0);
        _mm512_storeu_ps(d + i + 1 * 16, v1);
        _mm512_storeu_ps(d + i + 2 * 16, v2);
        _mm512_storeu_ps(d + i + 3 * 16, v3);
    }
    for (; n - i >= 16; i += 16) {
        auto value = _mm512_loadu_ps(a + i);
        for (std::size_t stage = 0; stage < stages; ++stage) {
            if (gains[stage] == float{})
                value = zero;
            else {
                const auto gain = _mm512_set1_ps(gains[stage]);
                value = _mm512_add_ps(zero, _mm512_mul_ps(value, gain));
            }
        }
        _mm512_storeu_ps(d + i, value);
    }
    if (i < n)
        scalarGainChain<float>(a + i, d + i, n - i, gains, stages);
}
void gainChainF64(const double* a, double* d, std::size_t n, const double* gains,
                  std::size_t stages) noexcept {
    const auto zero = _mm512_setzero_pd();
    std::size_t i = 0;
    for (; n - i >= 4 * 8; i += 4 * 8) {
        auto v0 = _mm512_loadu_pd(a + i + 0 * 8), v1 = _mm512_loadu_pd(a + i + 1 * 8),
             v2 = _mm512_loadu_pd(a + i + 2 * 8), v3 = _mm512_loadu_pd(a + i + 3 * 8);
        for (std::size_t stage = 0; stage < stages; ++stage) {
            if (gains[stage] == double{})
                v0 = v1 = v2 = v3 = zero;
            else {
                const auto gain = _mm512_set1_pd(gains[stage]);
                v0 = _mm512_add_pd(zero, _mm512_mul_pd(v0, gain));
                v1 = _mm512_add_pd(zero, _mm512_mul_pd(v1, gain));
                v2 = _mm512_add_pd(zero, _mm512_mul_pd(v2, gain));
                v3 = _mm512_add_pd(zero, _mm512_mul_pd(v3, gain));
            }
        }
        _mm512_storeu_pd(d + i + 0 * 8, v0);
        _mm512_storeu_pd(d + i + 1 * 8, v1);
        _mm512_storeu_pd(d + i + 2 * 8, v2);
        _mm512_storeu_pd(d + i + 3 * 8, v3);
    }
    for (; n - i >= 8; i += 8) {
        auto value = _mm512_loadu_pd(a + i);
        for (std::size_t stage = 0; stage < stages; ++stage) {
            if (gains[stage] == double{})
                value = zero;
            else {
                const auto gain = _mm512_set1_pd(gains[stage]);
                value = _mm512_add_pd(zero, _mm512_mul_pd(value, gain));
            }
        }
        _mm512_storeu_pd(d + i, value);
    }
    if (i < n)
        scalarGainChain<double>(a + i, d + i, n - i, gains, stages);
}
template <class I, std::size_t Lanes, std::size_t Half> struct FftSmallIndices {
    std::array<I, Lanes> even{}, odd{}, first{}, second{}, twiddle{};
    constexpr FftSmallIndices() {
        for (std::size_t lane = 0; lane < Lanes; ++lane) {
            const auto complex = lane / 2, component = lane % 2;
            even[lane] =
                static_cast<I>(2 * ((complex / Half) * 2 * Half + complex % Half) + component);
            odd[lane] = static_cast<I>(even[lane] + 2 * Half);
            twiddle[lane] = static_cast<I>(2 * (complex % Half) + component);
            for (std::size_t segment = 0; segment < 2; ++segment) {
                const auto scalar = lane + segment * Lanes, c = scalar / 2, within = c % (2 * Half);
                const auto packed = 2 * ((c / (2 * Half)) * Half + within % Half) + scalar % 2 +
                                    (within >= Half ? Lanes : 0);
                (segment ? second : first)[lane] = static_cast<I>(packed);
            }
        }
    }
};
template <std::size_t Half>
void smallStageF32(float* data, const float* weights, std::size_t count, bool inverse) noexcept {
    static constexpr FftSmallIndices<std::int32_t, 16, Half> indices;
    const auto ie = _mm512_loadu_si512(indices.even.data()),
               io = _mm512_loadu_si512(indices.odd.data());
    const auto first = _mm512_loadu_si512(indices.first.data()),
               second = _mm512_loadu_si512(indices.second.data());
    auto w = _mm512_i32gather_ps(_mm512_loadu_si512(indices.twiddle.data()), weights, 4);
    if (inverse)
        w = conjugateF32(w);
    std::size_t base = 0;
    for (; count - base >= 16; base += 16) {
        const auto a = _mm512_loadu_ps(data + 2 * base), b = _mm512_loadu_ps(data + 2 * base + 16);
        const auto e = _mm512_permutex2var_ps(a, ie, b), o = _mm512_permutex2var_ps(a, io, b),
                   p = productF32(o, w);
        const auto sum = _mm512_add_ps(e, p), difference = _mm512_sub_ps(e, p);
        _mm512_storeu_ps(data + 2 * base, _mm512_permutex2var_ps(sum, first, difference));
        _mm512_storeu_ps(data + 2 * base + 16, _mm512_permutex2var_ps(sum, second, difference));
    }
    for (; base < count; base += 2 * Half)
        scalarButterfly(data + 2 * base, data + 2 * (base + Half), weights, Half, inverse);
}
template <std::size_t Half>
void smallStageF64(double* data, const double* weights, std::size_t count, bool inverse) noexcept {
    static constexpr FftSmallIndices<std::int64_t, 8, Half> indices;
    const auto ie = _mm512_loadu_si512(indices.even.data()),
               io = _mm512_loadu_si512(indices.odd.data());
    const auto first = _mm512_loadu_si512(indices.first.data()),
               second = _mm512_loadu_si512(indices.second.data());
    auto w = _mm512_i64gather_pd(_mm512_loadu_si512(indices.twiddle.data()), weights, 8);
    if (inverse)
        w = conjugateF64(w);
    std::size_t base = 0;
    for (; count - base >= 8; base += 8) {
        const auto a = _mm512_loadu_pd(data + 2 * base), b = _mm512_loadu_pd(data + 2 * base + 8);
        const auto e = _mm512_permutex2var_pd(a, ie, b), o = _mm512_permutex2var_pd(a, io, b),
                   p = productF64(o, w);
        const auto sum = _mm512_add_pd(e, p), difference = _mm512_sub_pd(e, p);
        _mm512_storeu_pd(data + 2 * base, _mm512_permutex2var_pd(sum, first, difference));
        _mm512_storeu_pd(data + 2 * base + 8, _mm512_permutex2var_pd(sum, second, difference));
    }
    for (; base < count; base += 2 * Half)
        scalarButterfly(data + 2 * base, data + 2 * (base + Half), weights, Half, inverse);
}
void fftStageF32(float* data, const float* weights, std::size_t count, std::size_t half,
                 bool inverse) noexcept {
    if (half == 1) {
        smallStageF32<1>(data, weights, count, inverse);
        return;
    }
    if (half == 2) {
        smallStageF32<2>(data, weights, count, inverse);
        return;
    }
    if (half == 4) {
        smallStageF32<4>(data, weights, count, inverse);
        return;
    }
    for (std::size_t base = 0; base < count; base += 2 * half)
        butterflyF32(data + 2 * base, data + 2 * (base + half), weights, half, inverse);
}
void fftStageF64(double* data, const double* weights, std::size_t count, std::size_t half,
                 bool inverse) noexcept {
    if (half == 1) {
        smallStageF64<1>(data, weights, count, inverse);
        return;
    }
    if (half == 2) {
        smallStageF64<2>(data, weights, count, inverse);
        return;
    }
    for (std::size_t base = 0; base < count; base += 2 * half)
        butterflyF64(data + 2 * base, data + 2 * (base + half), weights, half, inverse);
}
} // namespace
const VectorTable& avx512Table() noexcept {
    static constexpr VectorTable table{
        CpuBackend::avx512,
        {multiplyF32, complexMacF32, butterflyF32, copyF32, scaleF32, addScaledF32, fillF32, dotF32,
         biquadBankF32, gainChainF32, fftStageF32},
        {multiplyF64, complexMacF64, butterflyF64, copyF64, scaleF64, addScaledF64, fillF64, dotF64,
         biquadBankF64, gainChainF64, fftStageF64},
        dotMixed};
    return table;
}
} // namespace cydsp::detail
