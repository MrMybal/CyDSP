#pragma once
#include <cstddef>
#include <immintrin.h>
namespace cydsp::detail {
namespace {
inline __m256 fftProductF32(__m256 a, __m256 b) noexcept {
    return _mm256_addsub_ps(_mm256_mul_ps(_mm256_moveldup_ps(a), b),
                            _mm256_mul_ps(_mm256_movehdup_ps(a), _mm256_permute_ps(b, 0xb1)));
}
void fftPairF32(float* data, const float* w1, const float* w2, std::size_t count, std::size_t half,
                bool inverse) noexcept {
    const auto sign = _mm256_setr_ps(0.f, -0.f, 0.f, -0.f, 0.f, -0.f, 0.f, -0.f);
    for (std::size_t base = 0; base < count; base += 4 * half) {
        for (std::size_t k = 0; k < half; k += 4) {
            auto u = _mm256_loadu_ps(w1 + 2 * k), v = _mm256_loadu_ps(w2 + 2 * k),
                 w = _mm256_loadu_ps(w2 + 2 * (half + k));
            if (inverse) {
                u = _mm256_xor_ps(u, sign);
                v = _mm256_xor_ps(v, sign);
                w = _mm256_xor_ps(w, sign);
            }
            const auto a = _mm256_loadu_ps(data + 2 * (base + k)),
                       b = fftProductF32(_mm256_loadu_ps(data + 2 * (base + half + k)), u),
                       c = _mm256_loadu_ps(data + 2 * (base + 2 * half + k)),
                       d = fftProductF32(_mm256_loadu_ps(data + 2 * (base + 3 * half + k)), u);
            const auto p = _mm256_add_ps(a, b), q = _mm256_sub_ps(a, b),
                       r = fftProductF32(_mm256_add_ps(c, d), v),
                       t = fftProductF32(_mm256_sub_ps(c, d), w);
            _mm256_storeu_ps(data + 2 * (base + k), _mm256_add_ps(p, r));
            _mm256_storeu_ps(data + 2 * (base + 2 * half + k), _mm256_sub_ps(p, r));
            _mm256_storeu_ps(data + 2 * (base + half + k), _mm256_add_ps(q, t));
            _mm256_storeu_ps(data + 2 * (base + 3 * half + k), _mm256_sub_ps(q, t));
        }
    }
}
void realSplitF32(const float* data, const float* weights, float* out, std::size_t half) noexcept {
    out[0] = data[0] + data[1];
    out[1] = 0;
    out[2 * half] = data[0] - data[1];
    out[2 * half + 1] = 0;
    const auto sign = _mm256_setr_ps(0.f, -0.f, 0.f, -0.f, 0.f, -0.f, 0.f, -0.f);
    std::size_t k = 1;
    for (; half - k >= 4; k += 4) {
        const auto a = _mm256_loadu_ps(data + 2 * k),
                   b = _mm256_xor_ps(
                       _mm256_permutevar8x32_ps(_mm256_loadu_ps(data + 2 * (half - k - 3)),
                                                _mm256_setr_epi32(6, 7, 4, 5, 2, 3, 0, 1)),
                       sign),
                   p = fftProductF32(_mm256_sub_ps(a, b), _mm256_loadu_ps(weights + 2 * (k - 1))),
                   result =
                       _mm256_mul_ps(_mm256_add_ps(_mm256_add_ps(a, b),
                                                   _mm256_xor_ps(_mm256_permute_ps(p, 0xb1), sign)),
                                     _mm256_set1_ps(.5));
        _mm256_storeu_ps(out + 2 * k, result);
    }
    for (; k < half; ++k) {
        const float ar = data[2 * k], ai = data[2 * k + 1], br = data[2 * (half - k)],
                    bi = -data[2 * (half - k) + 1];
        const float dr = ar - br, di = ai - bi, wr = weights[2 * (k - 1)],
                    wi = weights[2 * (k - 1) + 1];
        out[2 * k] = (ar + br + (wr * di + wi * dr)) * float(.5);
        out[2 * k + 1] = (ai + bi - (wr * dr - wi * di)) * float(.5);
    }
}
inline __m256d fftProductF64(__m256d a, __m256d b) noexcept {
    return _mm256_addsub_pd(_mm256_mul_pd(_mm256_movedup_pd(a), b),
                            _mm256_mul_pd(_mm256_permute_pd(a, 15), _mm256_permute_pd(b, 5)));
}
void fftPairF64(double* data, const double* w1, const double* w2, std::size_t count,
                std::size_t half, bool inverse) noexcept {
    const auto sign = _mm256_setr_pd(0., -0., 0., -0.);
    for (std::size_t base = 0; base < count; base += 4 * half) {
        for (std::size_t k = 0; k < half; k += 2) {
            auto u = _mm256_loadu_pd(w1 + 2 * k), v = _mm256_loadu_pd(w2 + 2 * k),
                 w = _mm256_loadu_pd(w2 + 2 * (half + k));
            if (inverse) {
                u = _mm256_xor_pd(u, sign);
                v = _mm256_xor_pd(v, sign);
                w = _mm256_xor_pd(w, sign);
            }
            const auto a = _mm256_loadu_pd(data + 2 * (base + k)),
                       b = fftProductF64(_mm256_loadu_pd(data + 2 * (base + half + k)), u),
                       c = _mm256_loadu_pd(data + 2 * (base + 2 * half + k)),
                       d = fftProductF64(_mm256_loadu_pd(data + 2 * (base + 3 * half + k)), u);
            const auto p = _mm256_add_pd(a, b), q = _mm256_sub_pd(a, b),
                       r = fftProductF64(_mm256_add_pd(c, d), v),
                       t = fftProductF64(_mm256_sub_pd(c, d), w);
            _mm256_storeu_pd(data + 2 * (base + k), _mm256_add_pd(p, r));
            _mm256_storeu_pd(data + 2 * (base + 2 * half + k), _mm256_sub_pd(p, r));
            _mm256_storeu_pd(data + 2 * (base + half + k), _mm256_add_pd(q, t));
            _mm256_storeu_pd(data + 2 * (base + 3 * half + k), _mm256_sub_pd(q, t));
        }
    }
}
void realSplitF64(const double* data, const double* weights, double* out,
                  std::size_t half) noexcept {
    out[0] = data[0] + data[1];
    out[1] = 0;
    out[2 * half] = data[0] - data[1];
    out[2 * half + 1] = 0;
    const auto sign = _mm256_setr_pd(0., -0., 0., -0.);
    std::size_t k = 1;
    for (; half - k >= 2; k += 2) {
        const auto a = _mm256_loadu_pd(data + 2 * k),
                   b = _mm256_xor_pd(
                       _mm256_permute4x64_pd(_mm256_loadu_pd(data + 2 * (half - k - 1)), 0x4e),
                       sign),
                   p = fftProductF64(_mm256_sub_pd(a, b), _mm256_loadu_pd(weights + 2 * (k - 1))),
                   result =
                       _mm256_mul_pd(_mm256_add_pd(_mm256_add_pd(a, b),
                                                   _mm256_xor_pd(_mm256_permute_pd(p, 5), sign)),
                                     _mm256_set1_pd(.5));
        _mm256_storeu_pd(out + 2 * k, result);
    }
    for (; k < half; ++k) {
        const double ar = data[2 * k], ai = data[2 * k + 1], br = data[2 * (half - k)],
                     bi = -data[2 * (half - k) + 1];
        const double dr = ar - br, di = ai - bi, wr = weights[2 * (k - 1)],
                     wi = weights[2 * (k - 1) + 1];
        out[2 * k] = (ar + br + (wr * di + wi * dr)) * double(.5);
        out[2 * k + 1] = (ai + bi - (wr * dr - wi * di)) * double(.5);
    }
}
} // namespace
} // namespace cydsp::detail
