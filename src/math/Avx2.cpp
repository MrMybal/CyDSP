#include "detail/AdaaAvx.h"
#include "detail/DynamicsAvx.h"
#include "detail/FirAvx.h"
#include "detail/Scalar.h"
#include "detail/VectorTable.h"
#include <immintrin.h>
namespace cydsp::detail {
namespace {
__m256 productF32(__m256 a, __m256 b) noexcept {

    const auto cross = _mm256_mul_ps(_mm256_shuffle_ps(a, a, _MM_SHUFFLE(3, 3, 1, 1)),
                                     _mm256_shuffle_ps(b, b, _MM_SHUFFLE(2, 3, 0, 1)));
    return _mm256_add_ps(
        _mm256_mul_ps(_mm256_shuffle_ps(a, a, _MM_SHUFFLE(2, 2, 0, 0)), b),
        _mm256_xor_ps(cross, _mm256_set_ps(0.0, -0.0, 0.0, -0.0, 0.0, -0.0, 0.0, -0.0)));
}
__m256 conjugateF32(__m256 value) noexcept {
    return _mm256_xor_ps(value, _mm256_set_ps(-0.0, 0.0, -0.0, 0.0, -0.0, 0.0, -0.0, 0.0));
}
double dotMixed(const float* a, const double* b, std::size_t n) noexcept {
    if (!n)
        return 0;
    auto s0 = _mm256_setzero_pd(), s1 = s0, s2 = s0, s3 = s0;
    std::size_t i = 0;
    for (; n - i >= 16; i += 16) {
        s0 = _mm256_add_pd(s0, _mm256_mul_pd(_mm256_cvtps_pd(_mm_loadu_ps(a + i + 0)),
                                             _mm256_loadu_pd(b + i + 0)));
        s1 = _mm256_add_pd(s1, _mm256_mul_pd(_mm256_cvtps_pd(_mm_loadu_ps(a + i + 4)),
                                             _mm256_loadu_pd(b + i + 4)));
        s2 = _mm256_add_pd(s2, _mm256_mul_pd(_mm256_cvtps_pd(_mm_loadu_ps(a + i + 8)),
                                             _mm256_loadu_pd(b + i + 8)));
        s3 = _mm256_add_pd(s3, _mm256_mul_pd(_mm256_cvtps_pd(_mm_loadu_ps(a + i + 12)),
                                             _mm256_loadu_pd(b + i + 12)));
    }
    auto sum = _mm256_add_pd(_mm256_add_pd(s0, s1), _mm256_add_pd(s2, s3));
    for (; n - i >= 4; i += 4)
        sum = _mm256_add_pd(
            sum, _mm256_mul_pd(_mm256_cvtps_pd(_mm_loadu_ps(a + i + 0)), _mm256_loadu_pd(b + i)));
    auto low = _mm_add_pd(_mm256_castpd256_pd128(sum), _mm256_extractf128_pd(sum, 1));
    return _mm_cvtsd_f64(_mm_add_sd(low, _mm_unpackhi_pd(low, low))) +
           scalarDotMixed(a + i, b + i, n - i);
}
void biquadBankF32(float* data, std::size_t voices, std::size_t frames, const float* c,
                   float* states) noexcept {
    std::size_t voice = 0;
    for (; voices - voice >= 8; voice += 8) {
        const auto c0 = _mm256_loadu_ps(c + 0 * voices + voice);
        const auto c1 = _mm256_loadu_ps(c + 1 * voices + voice);
        const auto c2 = _mm256_loadu_ps(c + 2 * voices + voice);
        const auto c3 = _mm256_loadu_ps(c + 3 * voices + voice);
        const auto c4 = _mm256_loadu_ps(c + 4 * voices + voice);
        auto z1 = _mm256_loadu_ps(states + voice), z2 = _mm256_loadu_ps(states + voices + voice);
        for (std::size_t frame = 0; frame < frames; ++frame) {
            auto* samples = data + frame * voices + voice;
            const auto x = _mm256_loadu_ps(samples);
            const auto y = _mm256_add_ps(_mm256_mul_ps(c0, x), z1);
            const auto next1 =
                _mm256_add_ps(_mm256_sub_ps(_mm256_mul_ps(c1, x), _mm256_mul_ps(c3, y)), z2);
            z2 = _mm256_sub_ps(_mm256_mul_ps(c2, x), _mm256_mul_ps(c4, y));
            z1 = next1;
            _mm256_storeu_ps(samples, y);
        }
        _mm256_storeu_ps(states + voice, z1);
        _mm256_storeu_ps(states + voices + voice, z2);
    }
    for (; voices - voice >= 4; voice += 4) {
        const auto c0 = _mm_loadu_ps(c + 0 * voices + voice);
        const auto c1 = _mm_loadu_ps(c + 1 * voices + voice);
        const auto c2 = _mm_loadu_ps(c + 2 * voices + voice);
        const auto c3 = _mm_loadu_ps(c + 3 * voices + voice);
        const auto c4 = _mm_loadu_ps(c + 4 * voices + voice);
        auto z1 = _mm_loadu_ps(states + voice), z2 = _mm_loadu_ps(states + voices + voice);
        for (std::size_t frame = 0; frame < frames; ++frame) {
            auto* samples = data + frame * voices + voice;
            const auto x = _mm_loadu_ps(samples);
            const auto y = _mm_add_ps(_mm_mul_ps(c0, x), z1);
            const auto next1 = _mm_add_ps(_mm_sub_ps(_mm_mul_ps(c1, x), _mm_mul_ps(c3, y)), z2);
            z2 = _mm_sub_ps(_mm_mul_ps(c2, x), _mm_mul_ps(c4, y));
            z1 = next1;
            _mm_storeu_ps(samples, y);
        }
        _mm_storeu_ps(states + voice, z1);
        _mm_storeu_ps(states + voices + voice, z2);
    }
    for (; voice < voices; ++voice)
        scalarBiquadVoice(data, voices, frames, c, states, voice);
}
void biquadBankF64(double* data, std::size_t voices, std::size_t frames, const double* c,
                   double* states) noexcept {
    std::size_t voice = 0;
    for (; voices - voice >= 4; voice += 4) {
        const auto c0 = _mm256_loadu_pd(c + 0 * voices + voice);
        const auto c1 = _mm256_loadu_pd(c + 1 * voices + voice);
        const auto c2 = _mm256_loadu_pd(c + 2 * voices + voice);
        const auto c3 = _mm256_loadu_pd(c + 3 * voices + voice);
        const auto c4 = _mm256_loadu_pd(c + 4 * voices + voice);
        auto z1 = _mm256_loadu_pd(states + voice), z2 = _mm256_loadu_pd(states + voices + voice);
        for (std::size_t frame = 0; frame < frames; ++frame) {
            auto* samples = data + frame * voices + voice;
            const auto x = _mm256_loadu_pd(samples);
            const auto y = _mm256_add_pd(_mm256_mul_pd(c0, x), z1);
            const auto next1 =
                _mm256_add_pd(_mm256_sub_pd(_mm256_mul_pd(c1, x), _mm256_mul_pd(c3, y)), z2);
            z2 = _mm256_sub_pd(_mm256_mul_pd(c2, x), _mm256_mul_pd(c4, y));
            z1 = next1;
            _mm256_storeu_pd(samples, y);
        }
        _mm256_storeu_pd(states + voice, z1);
        _mm256_storeu_pd(states + voices + voice, z2);
    }
    for (; voices - voice >= 2; voice += 2) {
        const auto c0 = _mm_loadu_pd(c + 0 * voices + voice);
        const auto c1 = _mm_loadu_pd(c + 1 * voices + voice);
        const auto c2 = _mm_loadu_pd(c + 2 * voices + voice);
        const auto c3 = _mm_loadu_pd(c + 3 * voices + voice);
        const auto c4 = _mm_loadu_pd(c + 4 * voices + voice);
        auto z1 = _mm_loadu_pd(states + voice), z2 = _mm_loadu_pd(states + voices + voice);
        for (std::size_t frame = 0; frame < frames; ++frame) {
            auto* samples = data + frame * voices + voice;
            const auto x = _mm_loadu_pd(samples);
            const auto y = _mm_add_pd(_mm_mul_pd(c0, x), z1);
            const auto next1 = _mm_add_pd(_mm_sub_pd(_mm_mul_pd(c1, x), _mm_mul_pd(c3, y)), z2);
            z2 = _mm_sub_pd(_mm_mul_pd(c2, x), _mm_mul_pd(c4, y));
            z1 = next1;
            _mm_storeu_pd(samples, y);
        }
        _mm_storeu_pd(states + voice, z1);
        _mm_storeu_pd(states + voices + voice, z2);
    }
    for (; voice < voices; ++voice)
        scalarBiquadVoice(data, voices, frames, c, states, voice);
}
float dotF32(const float* a, const float* b, std::size_t n) noexcept {
    if (!n)
        return float{};
    auto s0 = _mm256_setzero_ps(), s1 = s0, s2 = s0, s3 = s0;
    std::size_t i = 0;
    for (; n - i >= 32; i += 32) {
        s0 = _mm256_add_ps(s0,
                           _mm256_mul_ps(_mm256_loadu_ps(a + i + 0), _mm256_loadu_ps(b + i + 0)));
        s1 = _mm256_add_ps(s1,
                           _mm256_mul_ps(_mm256_loadu_ps(a + i + 8), _mm256_loadu_ps(b + i + 8)));
        s2 = _mm256_add_ps(s2,
                           _mm256_mul_ps(_mm256_loadu_ps(a + i + 16), _mm256_loadu_ps(b + i + 16)));
        s3 = _mm256_add_ps(s3,
                           _mm256_mul_ps(_mm256_loadu_ps(a + i + 24), _mm256_loadu_ps(b + i + 24)));
    }
    auto sum = _mm256_add_ps(_mm256_add_ps(s0, s1), _mm256_add_ps(s2, s3));
    for (; n - i >= 8; i += 8)
        sum = _mm256_add_ps(sum, _mm256_mul_ps(_mm256_loadu_ps(a + i), _mm256_loadu_ps(b + i)));
    auto low = _mm_add_ps(_mm256_castps256_ps128(sum), _mm256_extractf128_ps(sum, 1));
    low = _mm_add_ps(low, _mm_movehl_ps(low, low));
    const auto value = _mm_cvtss_f32(_mm_add_ss(low, _mm_shuffle_ps(low, low, 1)));
    return value + scalarDot(a + i, b + i, n - i);
}
void copyF32(const float* s, float* d, std::size_t n) noexcept {
    if (!n)
        return;

    std::size_t i = 0;
    for (; n - i >= 8; i += 8) {
        _mm256_storeu_ps(d + i, _mm256_loadu_ps(s + i));
    }
    scalarCopy(s + i, d + i, n - i);
}
void scaleF32(const float* s, float* d, std::size_t n, float gain) noexcept {
    if (!n)
        return;
    const auto factor = _mm256_set1_ps(gain);
    std::size_t i = 0;
    for (; n - i >= 8; i += 8) {
        _mm256_storeu_ps(d + i, _mm256_mul_ps(_mm256_loadu_ps(s + i), factor));
    }
    scalarScale(s + i, d + i, n - i, gain);
}
void addScaledF32(const float* s, float* d, std::size_t n, float gain) noexcept {
    if (!n)
        return;
    const auto factor = _mm256_set1_ps(gain);
    std::size_t i = 0;
    for (; n - i >= 8; i += 8) {
        _mm256_storeu_ps(d + i, _mm256_add_ps(_mm256_loadu_ps(d + i),
                                              _mm256_mul_ps(_mm256_loadu_ps(s + i), factor)));
    }
    scalarAddScaled(s + i, d + i, n - i, gain);
}
void fillF32(float* d, std::size_t n, float value) noexcept {
    if (!n)
        return;
    const auto factor = _mm256_set1_ps(value);
    std::size_t i = 0;
    for (; n - i >= 8; i += 8) {
        _mm256_storeu_ps(d + i, factor);
    }
    scalarFill(d + i, n - i, value);
}
void multiplyF32(const float* a, const float* b, float* d, std::size_t n) noexcept {
    if (!n)
        return;

    std::size_t i = 0;
    for (; n - i >= 8; i += 8) {
        _mm256_storeu_ps(d + i, _mm256_mul_ps(_mm256_loadu_ps(a + i), _mm256_loadu_ps(b + i)));
    }
    scalarMultiply(a + i, b + i, d + i, n - i);
}
void complexMacF32(const float* a, const float* b, float* d, std::size_t n) noexcept {
    if (!n)
        return;

    std::size_t i = 0;
    for (; n - i >= 4; i += 4) {
        _mm256_storeu_ps(d + 2 * i, _mm256_add_ps(_mm256_loadu_ps(d + 2 * i),
                                                  productF32(_mm256_loadu_ps(a + 2 * i),
                                                             _mm256_loadu_ps(b + 2 * i))));
    }
    scalarComplexMac(a + 2 * i, b + 2 * i, d + 2 * i, n - i);
}
void butterflyF32(float* even, float* odd, const float* w, std::size_t n, bool inverse) noexcept {
    if (!n)
        return;

    std::size_t i = 0;
    for (; n - i >= 4; i += 4) {
        auto twiddle = _mm256_loadu_ps(w + 2 * i);
        if (inverse)
            twiddle = conjugateF32(twiddle);
        const auto e = _mm256_loadu_ps(even + 2 * i),
                   p = productF32(_mm256_loadu_ps(odd + 2 * i), twiddle);
        _mm256_storeu_ps(even + 2 * i, _mm256_add_ps(e, p));
        _mm256_storeu_ps(odd + 2 * i, _mm256_sub_ps(e, p));
    }
    scalarButterfly(even + 2 * i, odd + 2 * i, w + 2 * i, n - i, inverse);
}
__m256d productF64(__m256d a, __m256d b) noexcept {

    const auto cross = _mm256_mul_pd(_mm256_permute_pd(a, 15), _mm256_permute_pd(b, 5));
    return _mm256_add_pd(_mm256_mul_pd(_mm256_permute_pd(a, 0), b),
                         _mm256_xor_pd(cross, _mm256_set_pd(0.0, -0.0, 0.0, -0.0)));
}
__m256d conjugateF64(__m256d value) noexcept {
    return _mm256_xor_pd(value, _mm256_set_pd(-0.0, 0.0, -0.0, 0.0));
}
double dotF64(const double* a, const double* b, std::size_t n) noexcept {
    if (!n)
        return double{};
    auto s0 = _mm256_setzero_pd(), s1 = s0, s2 = s0, s3 = s0;
    std::size_t i = 0;
    for (; n - i >= 16; i += 16) {
        s0 = _mm256_add_pd(s0,
                           _mm256_mul_pd(_mm256_loadu_pd(a + i + 0), _mm256_loadu_pd(b + i + 0)));
        s1 = _mm256_add_pd(s1,
                           _mm256_mul_pd(_mm256_loadu_pd(a + i + 4), _mm256_loadu_pd(b + i + 4)));
        s2 = _mm256_add_pd(s2,
                           _mm256_mul_pd(_mm256_loadu_pd(a + i + 8), _mm256_loadu_pd(b + i + 8)));
        s3 = _mm256_add_pd(s3,
                           _mm256_mul_pd(_mm256_loadu_pd(a + i + 12), _mm256_loadu_pd(b + i + 12)));
    }
    auto sum = _mm256_add_pd(_mm256_add_pd(s0, s1), _mm256_add_pd(s2, s3));
    for (; n - i >= 4; i += 4)
        sum = _mm256_add_pd(sum, _mm256_mul_pd(_mm256_loadu_pd(a + i), _mm256_loadu_pd(b + i)));
    auto low = _mm_add_pd(_mm256_castpd256_pd128(sum), _mm256_extractf128_pd(sum, 1));
    const auto value = _mm_cvtsd_f64(_mm_add_sd(low, _mm_unpackhi_pd(low, low)));
    return value + scalarDot(a + i, b + i, n - i);
}
void copyF64(const double* s, double* d, std::size_t n) noexcept {
    if (!n)
        return;

    std::size_t i = 0;
    for (; n - i >= 4; i += 4) {
        _mm256_storeu_pd(d + i, _mm256_loadu_pd(s + i));
    }
    scalarCopy(s + i, d + i, n - i);
}
void scaleF64(const double* s, double* d, std::size_t n, double gain) noexcept {
    if (!n)
        return;
    const auto factor = _mm256_set1_pd(gain);
    std::size_t i = 0;
    for (; n - i >= 4; i += 4) {
        _mm256_storeu_pd(d + i, _mm256_mul_pd(_mm256_loadu_pd(s + i), factor));
    }
    scalarScale(s + i, d + i, n - i, gain);
}
void addScaledF64(const double* s, double* d, std::size_t n, double gain) noexcept {
    if (!n)
        return;
    const auto factor = _mm256_set1_pd(gain);
    std::size_t i = 0;
    for (; n - i >= 4; i += 4) {
        _mm256_storeu_pd(d + i, _mm256_add_pd(_mm256_loadu_pd(d + i),
                                              _mm256_mul_pd(_mm256_loadu_pd(s + i), factor)));
    }
    scalarAddScaled(s + i, d + i, n - i, gain);
}
void fillF64(double* d, std::size_t n, double value) noexcept {
    if (!n)
        return;
    const auto factor = _mm256_set1_pd(value);
    std::size_t i = 0;
    for (; n - i >= 4; i += 4) {
        _mm256_storeu_pd(d + i, factor);
    }
    scalarFill(d + i, n - i, value);
}
void multiplyF64(const double* a, const double* b, double* d, std::size_t n) noexcept {
    if (!n)
        return;

    std::size_t i = 0;
    for (; n - i >= 4; i += 4) {
        _mm256_storeu_pd(d + i, _mm256_mul_pd(_mm256_loadu_pd(a + i), _mm256_loadu_pd(b + i)));
    }
    scalarMultiply(a + i, b + i, d + i, n - i);
}
void complexMacF64(const double* a, const double* b, double* d, std::size_t n) noexcept {
    if (!n)
        return;

    std::size_t i = 0;
    for (; n - i >= 2; i += 2) {
        _mm256_storeu_pd(d + 2 * i, _mm256_add_pd(_mm256_loadu_pd(d + 2 * i),
                                                  productF64(_mm256_loadu_pd(a + 2 * i),
                                                             _mm256_loadu_pd(b + 2 * i))));
    }
    scalarComplexMac(a + 2 * i, b + 2 * i, d + 2 * i, n - i);
}
void butterflyF64(double* even, double* odd, const double* w, std::size_t n,
                  bool inverse) noexcept {
    if (!n)
        return;

    std::size_t i = 0;
    for (; n - i >= 2; i += 2) {
        auto twiddle = _mm256_loadu_pd(w + 2 * i);
        if (inverse)
            twiddle = conjugateF64(twiddle);
        const auto e = _mm256_loadu_pd(even + 2 * i),
                   p = productF64(_mm256_loadu_pd(odd + 2 * i), twiddle);
        _mm256_storeu_pd(even + 2 * i, _mm256_add_pd(e, p));
        _mm256_storeu_pd(odd + 2 * i, _mm256_sub_pd(e, p));
    }
    scalarButterfly(even + 2 * i, odd + 2 * i, w + 2 * i, n - i, inverse);
}
void gainChainF32(const float* a, float* d, std::size_t n, const float* gains,
                  std::size_t stages) noexcept {
    const auto zero = _mm256_setzero_ps();
    std::size_t i = 0;
    for (; n - i >= 4 * 8; i += 4 * 8) {
        auto v0 = _mm256_loadu_ps(a + i + 0 * 8), v1 = _mm256_loadu_ps(a + i + 1 * 8),
             v2 = _mm256_loadu_ps(a + i + 2 * 8), v3 = _mm256_loadu_ps(a + i + 3 * 8);
        for (std::size_t stage = 0; stage < stages; ++stage) {
            if (gains[stage] == float{})
                v0 = v1 = v2 = v3 = zero;
            else {
                const auto gain = _mm256_set1_ps(gains[stage]);
                v0 = _mm256_add_ps(zero, _mm256_mul_ps(v0, gain));
                v1 = _mm256_add_ps(zero, _mm256_mul_ps(v1, gain));
                v2 = _mm256_add_ps(zero, _mm256_mul_ps(v2, gain));
                v3 = _mm256_add_ps(zero, _mm256_mul_ps(v3, gain));
            }
        }
        _mm256_storeu_ps(d + i + 0 * 8, v0);
        _mm256_storeu_ps(d + i + 1 * 8, v1);
        _mm256_storeu_ps(d + i + 2 * 8, v2);
        _mm256_storeu_ps(d + i + 3 * 8, v3);
    }
    for (; n - i >= 8; i += 8) {
        auto value = _mm256_loadu_ps(a + i);
        for (std::size_t stage = 0; stage < stages; ++stage) {
            if (gains[stage] == float{})
                value = zero;
            else {
                const auto gain = _mm256_set1_ps(gains[stage]);
                value = _mm256_add_ps(zero, _mm256_mul_ps(value, gain));
            }
        }
        _mm256_storeu_ps(d + i, value);
    }
    if (i < n)
        scalarGainChain<float>(a + i, d + i, n - i, gains, stages);
}
void gainChainF64(const double* a, double* d, std::size_t n, const double* gains,
                  std::size_t stages) noexcept {
    const auto zero = _mm256_setzero_pd();
    std::size_t i = 0;
    for (; n - i >= 4 * 4; i += 4 * 4) {
        auto v0 = _mm256_loadu_pd(a + i + 0 * 4), v1 = _mm256_loadu_pd(a + i + 1 * 4),
             v2 = _mm256_loadu_pd(a + i + 2 * 4), v3 = _mm256_loadu_pd(a + i + 3 * 4);
        for (std::size_t stage = 0; stage < stages; ++stage) {
            if (gains[stage] == double{})
                v0 = v1 = v2 = v3 = zero;
            else {
                const auto gain = _mm256_set1_pd(gains[stage]);
                v0 = _mm256_add_pd(zero, _mm256_mul_pd(v0, gain));
                v1 = _mm256_add_pd(zero, _mm256_mul_pd(v1, gain));
                v2 = _mm256_add_pd(zero, _mm256_mul_pd(v2, gain));
                v3 = _mm256_add_pd(zero, _mm256_mul_pd(v3, gain));
            }
        }
        _mm256_storeu_pd(d + i + 0 * 4, v0);
        _mm256_storeu_pd(d + i + 1 * 4, v1);
        _mm256_storeu_pd(d + i + 2 * 4, v2);
        _mm256_storeu_pd(d + i + 3 * 4, v3);
    }
    for (; n - i >= 4; i += 4) {
        auto value = _mm256_loadu_pd(a + i);
        for (std::size_t stage = 0; stage < stages; ++stage) {
            if (gains[stage] == double{})
                value = zero;
            else {
                const auto gain = _mm256_set1_pd(gains[stage]);
                value = _mm256_add_pd(zero, _mm256_mul_pd(value, gain));
            }
        }
        _mm256_storeu_pd(d + i, value);
    }
    if (i < n)
        scalarGainChain<double>(a + i, d + i, n - i, gains, stages);
}
void fftStageF32(float* data, const float* weights, std::size_t count, std::size_t half,
                 bool inverse) noexcept {
    if (half <= 2) {
        __m256 w;
        if (half == 1)
            w = _mm256_setr_ps(weights[0], weights[1], weights[0], weights[1], weights[0],
                               weights[1], weights[0], weights[1]);
        else {
            const auto low = _mm_loadu_ps(weights);
            w = _mm256_insertf128_ps(_mm256_castps128_ps256(low), low, 1);
        }
        if (inverse)
            w = conjugateF32(w);
        std::size_t base = 0;
        for (; count - base >= 8; base += 8) {
            const auto a = _mm256_loadu_ps(data + 2 * base),
                       b = _mm256_loadu_ps(data + 2 * base + 8);
            const auto e =
                half == 1 ? _mm256_shuffle_ps(a, b, 0x44) : _mm256_permute2f128_ps(a, b, 0x20);
            const auto o =
                half == 1 ? _mm256_shuffle_ps(a, b, 0xee) : _mm256_permute2f128_ps(a, b, 0x31);
            const auto p = productF32(o, w), sum = _mm256_add_ps(e, p),
                       difference = _mm256_sub_ps(e, p);
            const auto first = half == 1 ? _mm256_shuffle_ps(sum, difference, 0x44)
                                         : _mm256_permute2f128_ps(sum, difference, 0x20);
            const auto second = half == 1 ? _mm256_shuffle_ps(sum, difference, 0xee)
                                          : _mm256_permute2f128_ps(sum, difference, 0x31);
            _mm256_storeu_ps(data + 2 * base, first);
            _mm256_storeu_ps(data + 2 * base + 8, second);
        }
        for (; base < count; base += 2 * half)
            scalarButterfly(data + 2 * base, data + 2 * (base + half), weights, half, inverse);
        return;
    }
    for (std::size_t base = 0; base < count; base += 2 * half)
        butterflyF32(data + 2 * base, data + 2 * (base + half), weights, half, inverse);
}
void fftStageF64(double* data, const double* weights, std::size_t count, std::size_t half,
                 bool inverse) noexcept {
    if (half == 1) {
        auto w = _mm256_setr_pd(weights[0], weights[1], weights[0], weights[1]);
        if (inverse)
            w = conjugateF64(w);
        std::size_t base = 0;
        for (; count - base >= 4; base += 4) {
            const auto a = _mm256_loadu_pd(data + 2 * base),
                       b = _mm256_loadu_pd(data + 2 * base + 4);
            const auto e = _mm256_permute2f128_pd(a, b, 0x20),
                       o = _mm256_permute2f128_pd(a, b, 0x31), p = productF64(o, w);
            const auto sum = _mm256_add_pd(e, p), difference = _mm256_sub_pd(e, p);
            _mm256_storeu_pd(data + 2 * base, _mm256_permute2f128_pd(sum, difference, 0x20));
            _mm256_storeu_pd(data + 2 * base + 4, _mm256_permute2f128_pd(sum, difference, 0x31));
        }
        for (; base < count; base += 2)
            scalarButterfly(data + 2 * base, data + 2 * (base + 1), weights, std::size_t{1},
                            inverse);
        return;
    }
    for (std::size_t base = 0; base < count; base += 2 * half)
        butterflyF64(data + 2 * base, data + 2 * (base + half), weights, half, inverse);
}
} // namespace
const VectorTable& avx2Table() noexcept {
    static constexpr VectorTable table{
        CpuBackend::avx2,
        {multiplyF32, complexMacF32, butterflyF32, copyF32, scaleF32, addScaledF32, fillF32, dotF32,
         biquadBankF32, gainChainF32, fftStageF32, firHeadF32},
        {multiplyF64, complexMacF64, butterflyF64, copyF64, scaleF64, addScaledF64, fillF64, dotF64,
         biquadBankF64, gainChainF64, fftStageF64, firHeadF64},
        dotMixed,
        adaaFoldAvx2,
        adaaClipAvx2,
        adaaTanhAvx2,
        compressorGainAvx2};
    return table;
}
} // namespace cydsp::detail
