#pragma once
#include <cstddef>
#include <immintrin.h>
namespace cydsp::detail {
namespace {
// Disjoint buffers; input holds frames+taps-1 samples. Four tap chains per
// output, no FMA, masked over-read, or horizontal reduction per output.
void firHeadF32(const float* x, const float* h, float* y, std::size_t frames,
                std::size_t taps) noexcept {
    std::size_t f = 0;
    for (; frames - f >= 16; f += 16) {
        auto s00 = _mm256_setzero_ps(), s01 = s00, s02 = s00, s03 = s00;
        auto s10 = _mm256_setzero_ps(), s11 = s10, s12 = s10, s13 = s10;
        std::size_t k = 0;
        for (; taps - k >= 4; k += 4) {
            const auto c0 = _mm256_set1_ps(h[k + 0]);
            s00 = _mm256_add_ps(s00, _mm256_mul_ps(_mm256_loadu_ps(x + f + k + 0), c0));
            s10 = _mm256_add_ps(s10, _mm256_mul_ps(_mm256_loadu_ps(x + f + k + 8), c0));
            const auto c1 = _mm256_set1_ps(h[k + 1]);
            s01 = _mm256_add_ps(s01, _mm256_mul_ps(_mm256_loadu_ps(x + f + k + 1), c1));
            s11 = _mm256_add_ps(s11, _mm256_mul_ps(_mm256_loadu_ps(x + f + k + 9), c1));
            const auto c2 = _mm256_set1_ps(h[k + 2]);
            s02 = _mm256_add_ps(s02, _mm256_mul_ps(_mm256_loadu_ps(x + f + k + 2), c2));
            s12 = _mm256_add_ps(s12, _mm256_mul_ps(_mm256_loadu_ps(x + f + k + 10), c2));
            const auto c3 = _mm256_set1_ps(h[k + 3]);
            s03 = _mm256_add_ps(s03, _mm256_mul_ps(_mm256_loadu_ps(x + f + k + 3), c3));
            s13 = _mm256_add_ps(s13, _mm256_mul_ps(_mm256_loadu_ps(x + f + k + 11), c3));
        }
        for (; k < taps; ++k) {
            s00 = _mm256_add_ps(
                s00, _mm256_mul_ps(_mm256_loadu_ps(x + f + k + 0), _mm256_set1_ps(h[k])));
            s10 = _mm256_add_ps(
                s10, _mm256_mul_ps(_mm256_loadu_ps(x + f + k + 8), _mm256_set1_ps(h[k])));
        }
        _mm256_storeu_ps(y + f + 0, _mm256_add_ps(_mm256_loadu_ps(y + f + 0),
                                                  _mm256_add_ps(_mm256_add_ps(s00, s01),
                                                                _mm256_add_ps(s02, s03))));
        _mm256_storeu_ps(y + f + 8, _mm256_add_ps(_mm256_loadu_ps(y + f + 8),
                                                  _mm256_add_ps(_mm256_add_ps(s10, s11),
                                                                _mm256_add_ps(s12, s13))));
    }
    for (; f < frames; ++f) {
        float sum{};
        for (std::size_t k = 0; k < taps; ++k)
            sum += x[f + k] * h[k];
        y[f] += sum;
    }
}
void firHeadF64(const double* x, const double* h, double* y, std::size_t frames,
                std::size_t taps) noexcept {
    std::size_t f = 0;
    for (; frames - f >= 8; f += 8) {
        auto s00 = _mm256_setzero_pd(), s01 = s00, s02 = s00, s03 = s00;
        auto s10 = _mm256_setzero_pd(), s11 = s10, s12 = s10, s13 = s10;
        std::size_t k = 0;
        for (; taps - k >= 4; k += 4) {
            const auto c0 = _mm256_set1_pd(h[k + 0]);
            s00 = _mm256_add_pd(s00, _mm256_mul_pd(_mm256_loadu_pd(x + f + k + 0), c0));
            s10 = _mm256_add_pd(s10, _mm256_mul_pd(_mm256_loadu_pd(x + f + k + 4), c0));
            const auto c1 = _mm256_set1_pd(h[k + 1]);
            s01 = _mm256_add_pd(s01, _mm256_mul_pd(_mm256_loadu_pd(x + f + k + 1), c1));
            s11 = _mm256_add_pd(s11, _mm256_mul_pd(_mm256_loadu_pd(x + f + k + 5), c1));
            const auto c2 = _mm256_set1_pd(h[k + 2]);
            s02 = _mm256_add_pd(s02, _mm256_mul_pd(_mm256_loadu_pd(x + f + k + 2), c2));
            s12 = _mm256_add_pd(s12, _mm256_mul_pd(_mm256_loadu_pd(x + f + k + 6), c2));
            const auto c3 = _mm256_set1_pd(h[k + 3]);
            s03 = _mm256_add_pd(s03, _mm256_mul_pd(_mm256_loadu_pd(x + f + k + 3), c3));
            s13 = _mm256_add_pd(s13, _mm256_mul_pd(_mm256_loadu_pd(x + f + k + 7), c3));
        }
        for (; k < taps; ++k) {
            s00 = _mm256_add_pd(
                s00, _mm256_mul_pd(_mm256_loadu_pd(x + f + k + 0), _mm256_set1_pd(h[k])));
            s10 = _mm256_add_pd(
                s10, _mm256_mul_pd(_mm256_loadu_pd(x + f + k + 4), _mm256_set1_pd(h[k])));
        }
        _mm256_storeu_pd(y + f + 0, _mm256_add_pd(_mm256_loadu_pd(y + f + 0),
                                                  _mm256_add_pd(_mm256_add_pd(s00, s01),
                                                                _mm256_add_pd(s02, s03))));
        _mm256_storeu_pd(y + f + 4, _mm256_add_pd(_mm256_loadu_pd(y + f + 4),
                                                  _mm256_add_pd(_mm256_add_pd(s10, s11),
                                                                _mm256_add_pd(s12, s13))));
    }
    for (; f < frames; ++f) {
        double sum{};
        for (std::size_t k = 0; k < taps; ++k)
            sum += x[f + k] * h[k];
        y[f] += sum;
    }
}
} // namespace
} // namespace cydsp::detail
