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
void realSplitRangeF32(const float* data, const float* weights, float* out, std::size_t half,
                      std::size_t begin, std::size_t end) noexcept {
    const auto sign = _mm256_setr_ps(0.f, -0.f, 0.f, -0.f, 0.f, -0.f, 0.f, -0.f);
    std::size_t k = begin;
    for (; end - k >= 4; k += 4) {
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
        _mm256_storeu_ps(out + 2 * (k - begin), result);
    }
    for (; k < end; ++k) {
        const float ar = data[2 * k], ai = data[2 * k + 1], br = data[2 * (half - k)],
                    bi = -data[2 * (half - k) + 1];
        const float dr = ar - br, di = ai - bi, wr = weights[2 * (k - 1)],
                    wi = weights[2 * (k - 1) + 1];
        out[2 * (k - begin)] = (ar + br + (wr * di + wi * dr)) * float(.5);
        out[2 * (k - begin) + 1] = (ai + bi - (wr * dr - wi * di)) * float(.5);
    }
}
void realSplitF32(const float* data, const float* weights, float* out, std::size_t half) noexcept {
    out[0] = data[0] + data[1];
    out[1] = 0;
    out[2 * half] = data[0] - data[1];
    out[2 * half + 1] = 0;
    realSplitRangeF32(data, weights, out + 2, half, 1, half);
}
// Bit-reversal permutation fused with the radix-2 stages half=1 and half=2.
// x holds count >= 16 complex values in natural order; y is disjoint. Block
// reverse[j] of four permuted values depends only on x[j], x[j+count/4],
// x[j+count/2] and x[j+3*count/4], so the four quarter streams are read
// contiguously and each lane is one block. The butterflies, their twiddles and
// their operation order are exactly those of the two separate stages.
void fftHeadF32(const float* x, float* y, const std::size_t* reverse, const float* weights,
                std::size_t count, bool inverse) noexcept {
    const auto m = count / 4;
    const auto sign = _mm256_setr_ps(0.f, -0.f, 0.f, -0.f, 0.f, -0.f, 0.f, -0.f);
    const auto twiddle = [&](const float* w) noexcept {
        const auto value = _mm256_setr_ps(w[0], w[1], w[0], w[1], w[0], w[1], w[0], w[1]);
        return inverse ? _mm256_xor_ps(value, sign) : value;
    };
    const auto w1 = twiddle(weights), w20 = twiddle(weights + 2), w21 = twiddle(weights + 4);
    for (std::size_t j = 0; j < m; j += 4) {
        const auto x0 = _mm256_loadu_ps(x + 2 * j), x1 = _mm256_loadu_ps(x + 2 * (j + m)),
                   x2 = _mm256_loadu_ps(x + 2 * (j + 2 * m)),
                   x3 = _mm256_loadu_ps(x + 2 * (j + 3 * m));
        const auto t0 = fftProductF32(x2, w1), t1 = fftProductF32(x3, w1);
        const auto a0 = _mm256_add_ps(x0, t0), a1 = _mm256_sub_ps(x0, t0),
                   a2 = _mm256_add_ps(x1, t1), a3 = _mm256_sub_ps(x1, t1);
        const auto u0 = fftProductF32(a2, w20), u1 = fftProductF32(a3, w21);
        const auto b0 = _mm256_castps_pd(_mm256_add_ps(a0, u0)),
                   b1 = _mm256_castps_pd(_mm256_add_ps(a1, u1)),
                   b2 = _mm256_castps_pd(_mm256_sub_ps(a0, u0)),
                   b3 = _mm256_castps_pd(_mm256_sub_ps(a1, u1));
        const auto r0 = _mm256_unpacklo_pd(b0, b1), r1 = _mm256_unpackhi_pd(b0, b1),
                   r2 = _mm256_unpacklo_pd(b2, b3), r3 = _mm256_unpackhi_pd(b2, b3);
        _mm256_storeu_pd(reinterpret_cast<double*>(y + 2 * reverse[j]),
                         _mm256_permute2f128_pd(r0, r2, 0x20));
        _mm256_storeu_pd(reinterpret_cast<double*>(y + 2 * reverse[j + 1]),
                         _mm256_permute2f128_pd(r1, r3, 0x20));
        _mm256_storeu_pd(reinterpret_cast<double*>(y + 2 * reverse[j + 2]),
                         _mm256_permute2f128_pd(r0, r2, 0x31));
        _mm256_storeu_pd(reinterpret_cast<double*>(y + 2 * reverse[j + 3]),
                         _mm256_permute2f128_pd(r1, r3, 0x31));
    }
}
// Inverse counterpart of realSplitRange: real-spectrum bins [begin,end), with
// 1 <= begin, to packed complex values in natural order; out receives bin begin.
// Same arithmetic as the scalar mergeRealBin.
void realMergeRangeF32(const float* in, const float* weights, float* out, std::size_t half,
                      std::size_t begin, std::size_t end) noexcept {
    const auto conjugate = _mm256_setr_ps(0.f, -0.f, 0.f, -0.f, 0.f, -0.f, 0.f, -0.f);
    const auto sign = _mm256_setr_ps(-0.f, 0.f, -0.f, 0.f, -0.f, 0.f, -0.f, 0.f);
    std::size_t k = begin;
    for (; end - k >= 4; k += 4) {
        const auto a = _mm256_loadu_ps(in + 2 * k),
                   b = _mm256_xor_ps(
                       _mm256_permutevar8x32_ps(_mm256_loadu_ps(in + 2 * (half - k - 3)),
                                                _mm256_setr_epi32(6, 7, 4, 5, 2, 3, 0, 1)),
                       conjugate),
                   p = fftProductF32(
                       _mm256_sub_ps(a, b),
                       _mm256_xor_ps(_mm256_loadu_ps(weights + 2 * (k - 1)), conjugate)),
                   result =
                       _mm256_mul_ps(_mm256_add_ps(_mm256_add_ps(a, b),
                                                   _mm256_xor_ps(_mm256_permute_ps(p, 0xb1), sign)),
                                     _mm256_set1_ps(.5));
        _mm256_storeu_ps(out + 2 * (k - begin), result);
    }
    for (; k < end; ++k) {
        const float ar = in[2 * k], ai = in[2 * k + 1], br = in[2 * (half - k)],
                    bi = -in[2 * (half - k) + 1];
        const float dr = ar - br, di = ai - bi, wr = weights[2 * (k - 1)],
                    wi = weights[2 * (k - 1) + 1];
        out[2 * (k - begin)] = (ar + br - (wr * di - wi * dr)) * float(.5);
        out[2 * (k - begin) + 1] = (ai + bi + (wr * dr + wi * di)) * float(.5);
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
void realSplitRangeF64(const double* data, const double* weights, double* out, std::size_t half,
                      std::size_t begin, std::size_t end) noexcept {
    const auto sign = _mm256_setr_pd(0., -0., 0., -0.);
    std::size_t k = begin;
    for (; end - k >= 2; k += 2) {
        const auto a = _mm256_loadu_pd(data + 2 * k),
                   b = _mm256_xor_pd(
                       _mm256_permute4x64_pd(_mm256_loadu_pd(data + 2 * (half - k - 1)), 0x4e),
                       sign),
                   p = fftProductF64(_mm256_sub_pd(a, b), _mm256_loadu_pd(weights + 2 * (k - 1))),
                   result =
                       _mm256_mul_pd(_mm256_add_pd(_mm256_add_pd(a, b),
                                                   _mm256_xor_pd(_mm256_permute_pd(p, 5), sign)),
                                     _mm256_set1_pd(.5));
        _mm256_storeu_pd(out + 2 * (k - begin), result);
    }
    for (; k < end; ++k) {
        const double ar = data[2 * k], ai = data[2 * k + 1], br = data[2 * (half - k)],
                     bi = -data[2 * (half - k) + 1];
        const double dr = ar - br, di = ai - bi, wr = weights[2 * (k - 1)],
                     wi = weights[2 * (k - 1) + 1];
        out[2 * (k - begin)] = (ar + br + (wr * di + wi * dr)) * double(.5);
        out[2 * (k - begin) + 1] = (ai + bi - (wr * dr - wi * di)) * double(.5);
    }
}
void realSplitF64(const double* data, const double* weights, double* out, std::size_t half) noexcept {
    out[0] = data[0] + data[1];
    out[1] = 0;
    out[2 * half] = data[0] - data[1];
    out[2 * half + 1] = 0;
    realSplitRangeF64(data, weights, out + 2, half, 1, half);
}
void fftHeadF64(const double* x, double* y, const std::size_t* reverse, const double* weights,
                std::size_t count, bool inverse) noexcept {
    const auto m = count / 4;
    const auto sign = _mm256_setr_pd(0., -0., 0., -0.);
    const auto twiddle = [&](const double* w) noexcept {
        const auto value = _mm256_setr_pd(w[0], w[1], w[0], w[1]);
        return inverse ? _mm256_xor_pd(value, sign) : value;
    };
    const auto w1 = twiddle(weights), w20 = twiddle(weights + 2), w21 = twiddle(weights + 4);
    for (std::size_t j = 0; j < m; j += 2) {
        const auto x0 = _mm256_loadu_pd(x + 2 * j), x1 = _mm256_loadu_pd(x + 2 * (j + m)),
                   x2 = _mm256_loadu_pd(x + 2 * (j + 2 * m)),
                   x3 = _mm256_loadu_pd(x + 2 * (j + 3 * m));
        const auto t0 = fftProductF64(x2, w1), t1 = fftProductF64(x3, w1);
        const auto a0 = _mm256_add_pd(x0, t0), a1 = _mm256_sub_pd(x0, t0),
                   a2 = _mm256_add_pd(x1, t1), a3 = _mm256_sub_pd(x1, t1);
        const auto u0 = fftProductF64(a2, w20), u1 = fftProductF64(a3, w21);
        const auto b0 = _mm256_add_pd(a0, u0), b1 = _mm256_add_pd(a1, u1),
                   b2 = _mm256_sub_pd(a0, u0), b3 = _mm256_sub_pd(a1, u1);
        _mm256_storeu_pd(y + 2 * reverse[j], _mm256_permute2f128_pd(b0, b1, 0x20));
        _mm256_storeu_pd(y + 2 * reverse[j] + 4, _mm256_permute2f128_pd(b2, b3, 0x20));
        _mm256_storeu_pd(y + 2 * reverse[j + 1], _mm256_permute2f128_pd(b0, b1, 0x31));
        _mm256_storeu_pd(y + 2 * reverse[j + 1] + 4, _mm256_permute2f128_pd(b2, b3, 0x31));
    }
}
void realMergeRangeF64(const double* in, const double* weights, double* out, std::size_t half,
                      std::size_t begin, std::size_t end) noexcept {
    const auto conjugate = _mm256_setr_pd(0., -0., 0., -0.);
    const auto sign = _mm256_setr_pd(-0., 0., -0., 0.);
    std::size_t k = begin;
    for (; end - k >= 2; k += 2) {
        const auto a = _mm256_loadu_pd(in + 2 * k),
                   b = _mm256_xor_pd(
                       _mm256_permute4x64_pd(_mm256_loadu_pd(in + 2 * (half - k - 1)), 0x4e),
                       conjugate),
                   p = fftProductF64(
                       _mm256_sub_pd(a, b),
                       _mm256_xor_pd(_mm256_loadu_pd(weights + 2 * (k - 1)), conjugate)),
                   result =
                       _mm256_mul_pd(_mm256_add_pd(_mm256_add_pd(a, b),
                                                   _mm256_xor_pd(_mm256_permute_pd(p, 5), sign)),
                                     _mm256_set1_pd(.5));
        _mm256_storeu_pd(out + 2 * (k - begin), result);
    }
    for (; k < end; ++k) {
        const double ar = in[2 * k], ai = in[2 * k + 1], br = in[2 * (half - k)],
                     bi = -in[2 * (half - k) + 1];
        const double dr = ar - br, di = ai - bi, wr = weights[2 * (k - 1)],
                     wi = weights[2 * (k - 1) + 1];
        out[2 * (k - begin)] = (ar + br - (wr * di - wi * dr)) * double(.5);
        out[2 * (k - begin) + 1] = (ai + bi + (wr * dr + wi * di)) * double(.5);
    }
}
} // namespace
} // namespace cydsp::detail
