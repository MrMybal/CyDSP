#include "detail/Scalar.h"
#include "detail/VectorTable.h"
#include <arm_sve.h>
namespace cydsp::detail {
namespace {
void copyF32(const float* a, float* d, std::size_t n) noexcept {
    for (std::size_t i = 0; i < n; i += svcntw()) {
        auto pg = svwhilelt_b32(i, n);
        auto x = svld1_f32(pg, a + i);
        svst1_f32(pg, d + i, x);
    }
}
void scaleF32(const float* a, float* d, std::size_t n, float gain) noexcept {
    for (std::size_t i = 0; i < n; i += svcntw()) {
        auto pg = svwhilelt_b32(i, n);
        auto x = svld1_f32(pg, a + i);
        svst1_f32(pg, d + i, svmul_n_f32_x(pg, x, gain));
    }
}
void addScaledF32(const float* a, float* d, std::size_t n, float gain) noexcept {
    for (std::size_t i = 0; i < n; i += svcntw()) {
        auto pg = svwhilelt_b32(i, n);
        auto x = svld1_f32(pg, a + i);
        svst1_f32(pg, d + i, svadd_f32_x(pg, svld1_f32(pg, d + i), svmul_n_f32_x(pg, x, gain)));
    }
}
void multiplyF32(const float* a, const float* b, float* d, std::size_t n) noexcept {
    for (std::size_t i = 0; i < n; i += svcntw()) {
        auto pg = svwhilelt_b32(i, n);
        auto x = svld1_f32(pg, a + i);
        svst1_f32(pg, d + i, svmul_f32_x(pg, x, svld1_f32(pg, b + i)));
    }
}
void fillF32(float* d, std::size_t n, float value) noexcept {
    for (std::size_t i = 0; i < n; i += svcntw()) {
        auto pg = svwhilelt_b32(i, n);
        svst1_f32(pg, d + i, svdup_n_f32(value));
    }
}
float dotF32(const float* a, const float* b, std::size_t n) noexcept {
    const auto width = svcntw();
    const auto all = svptrue_b32();
    auto s0 = svdup_n_f32(0), s1 = s0, s2 = s0, s3 = s0;
    std::size_t i = 0;
    for (; n - i >= 4 * width; i += 4 * width) {
        s0 = svadd_f32_x(
            all, s0,
            svmul_f32_x(all, svld1_f32(all, a + i + 0 * width), svld1_f32(all, b + i + 0 * width)));
        s1 = svadd_f32_x(
            all, s1,
            svmul_f32_x(all, svld1_f32(all, a + i + 1 * width), svld1_f32(all, b + i + 1 * width)));
        s2 = svadd_f32_x(
            all, s2,
            svmul_f32_x(all, svld1_f32(all, a + i + 2 * width), svld1_f32(all, b + i + 2 * width)));
        s3 = svadd_f32_x(
            all, s3,
            svmul_f32_x(all, svld1_f32(all, a + i + 3 * width), svld1_f32(all, b + i + 3 * width)));
    }
    auto sum = svadd_f32_x(all, svadd_f32_x(all, s0, s1), svadd_f32_x(all, s2, s3));
    for (; i < n; i += width) {
        auto pg = svwhilelt_b32(i, n);
        sum = svadd_f32_m(pg, sum, svmul_f32_x(pg, svld1_f32(pg, a + i), svld1_f32(pg, b + i)));
    }
    return svaddv_f32(all, sum);
}

void complexMacF32(const float* a, const float* b, float* d, std::size_t n) noexcept {
    for (std::size_t i = 0; i < n; i += svcntw()) {
        auto pg = svwhilelt_b32(i, n);
        auto av = svld2_f32(pg, a + 2 * i), bv = svld2_f32(pg, b + 2 * i),
             dv = svld2_f32(pg, d + 2 * i);
        auto ar = svget2_f32(av, 0), ai = svget2_f32(av, 1), br = svget2_f32(bv, 0),
             bi = svget2_f32(bv, 1);
        auto re = svsub_f32_x(pg, svmul_f32_x(pg, ar, br), svmul_f32_x(pg, ai, bi));
        auto im = svadd_f32_x(pg, svmul_f32_x(pg, ar, bi), svmul_f32_x(pg, ai, br));
        svst2_f32(pg, d + 2 * i,
                  svcreate2_f32(svadd_f32_x(pg, svget2_f32(dv, 0), re),
                                svadd_f32_x(pg, svget2_f32(dv, 1), im)));
    }
}
void butterflyF32(float* e, float* o, const float* w, std::size_t n, bool inverse) noexcept {
    for (std::size_t i = 0; i < n; i += svcntw()) {
        auto pg = svwhilelt_b32(i, n);
        auto ev = svld2_f32(pg, e + 2 * i), av = svld2_f32(pg, o + 2 * i),
             bv = svld2_f32(pg, w + 2 * i);
        auto ar = svget2_f32(av, 0), ai = svget2_f32(av, 1), br = svget2_f32(bv, 0),
             bi = svget2_f32(bv, 1);
        if (inverse)
            bi = svneg_f32_x(pg, bi);
        auto re = svsub_f32_x(pg, svmul_f32_x(pg, ar, br), svmul_f32_x(pg, ai, bi));
        auto im = svadd_f32_x(pg, svmul_f32_x(pg, ar, bi), svmul_f32_x(pg, ai, br));
        svst2_f32(pg, e + 2 * i,
                  svcreate2_f32(svadd_f32_x(pg, svget2_f32(ev, 0), re),
                                svadd_f32_x(pg, svget2_f32(ev, 1), im)));
        svst2_f32(pg, o + 2 * i,
                  svcreate2_f32(svsub_f32_x(pg, svget2_f32(ev, 0), re),
                                svsub_f32_x(pg, svget2_f32(ev, 1), im)));
    }
}
void biquadBankF32(float* data, std::size_t voices, std::size_t frames, const float* c,
                   float* state) noexcept {
    for (std::size_t i = 0; i < voices; i += svcntw()) {
        auto pg = svwhilelt_b32(i, voices);
        auto c0 = svld1_f32(pg, c + 0 * voices + i);
        auto c1 = svld1_f32(pg, c + 1 * voices + i);
        auto c2 = svld1_f32(pg, c + 2 * voices + i);
        auto c3 = svld1_f32(pg, c + 3 * voices + i);
        auto c4 = svld1_f32(pg, c + 4 * voices + i);
        auto z1 = svld1_f32(pg, state + i), z2 = svld1_f32(pg, state + voices + i);
        for (std::size_t f = 0; f < frames; ++f) {
            auto x = svld1_f32(pg, data + f * voices + i);
            auto y = svadd_f32_x(pg, svmul_f32_x(pg, c0, x), z1);
            z1 = svadd_f32_x(pg, svsub_f32_x(pg, svmul_f32_x(pg, c1, x), svmul_f32_x(pg, c3, y)),
                             z2);
            z2 = svsub_f32_x(pg, svmul_f32_x(pg, c2, x), svmul_f32_x(pg, c4, y));
            svst1_f32(pg, data + f * voices + i, y);
        }
        svst1_f32(pg, state + i, z1);
        svst1_f32(pg, state + voices + i, z2);
    }
}
void copyF64(const double* a, double* d, std::size_t n) noexcept {
    for (std::size_t i = 0; i < n; i += svcntd()) {
        auto pg = svwhilelt_b64(i, n);
        auto x = svld1_f64(pg, a + i);
        svst1_f64(pg, d + i, x);
    }
}
void scaleF64(const double* a, double* d, std::size_t n, double gain) noexcept {
    for (std::size_t i = 0; i < n; i += svcntd()) {
        auto pg = svwhilelt_b64(i, n);
        auto x = svld1_f64(pg, a + i);
        svst1_f64(pg, d + i, svmul_n_f64_x(pg, x, gain));
    }
}
void addScaledF64(const double* a, double* d, std::size_t n, double gain) noexcept {
    for (std::size_t i = 0; i < n; i += svcntd()) {
        auto pg = svwhilelt_b64(i, n);
        auto x = svld1_f64(pg, a + i);
        svst1_f64(pg, d + i, svadd_f64_x(pg, svld1_f64(pg, d + i), svmul_n_f64_x(pg, x, gain)));
    }
}
void multiplyF64(const double* a, const double* b, double* d, std::size_t n) noexcept {
    for (std::size_t i = 0; i < n; i += svcntd()) {
        auto pg = svwhilelt_b64(i, n);
        auto x = svld1_f64(pg, a + i);
        svst1_f64(pg, d + i, svmul_f64_x(pg, x, svld1_f64(pg, b + i)));
    }
}
void fillF64(double* d, std::size_t n, double value) noexcept {
    for (std::size_t i = 0; i < n; i += svcntd()) {
        auto pg = svwhilelt_b64(i, n);
        svst1_f64(pg, d + i, svdup_n_f64(value));
    }
}
double dotF64(const double* a, const double* b, std::size_t n) noexcept {
    const auto width = svcntd();
    const auto all = svptrue_b64();
    auto s0 = svdup_n_f64(0), s1 = s0, s2 = s0, s3 = s0;
    std::size_t i = 0;
    for (; n - i >= 4 * width; i += 4 * width) {
        s0 = svadd_f64_x(
            all, s0,
            svmul_f64_x(all, svld1_f64(all, a + i + 0 * width), svld1_f64(all, b + i + 0 * width)));
        s1 = svadd_f64_x(
            all, s1,
            svmul_f64_x(all, svld1_f64(all, a + i + 1 * width), svld1_f64(all, b + i + 1 * width)));
        s2 = svadd_f64_x(
            all, s2,
            svmul_f64_x(all, svld1_f64(all, a + i + 2 * width), svld1_f64(all, b + i + 2 * width)));
        s3 = svadd_f64_x(
            all, s3,
            svmul_f64_x(all, svld1_f64(all, a + i + 3 * width), svld1_f64(all, b + i + 3 * width)));
    }
    auto sum = svadd_f64_x(all, svadd_f64_x(all, s0, s1), svadd_f64_x(all, s2, s3));
    for (; i < n; i += width) {
        auto pg = svwhilelt_b64(i, n);
        sum = svadd_f64_m(pg, sum, svmul_f64_x(pg, svld1_f64(pg, a + i), svld1_f64(pg, b + i)));
    }
    return svaddv_f64(all, sum);
}

void complexMacF64(const double* a, const double* b, double* d, std::size_t n) noexcept {
    for (std::size_t i = 0; i < n; i += svcntd()) {
        auto pg = svwhilelt_b64(i, n);
        auto av = svld2_f64(pg, a + 2 * i), bv = svld2_f64(pg, b + 2 * i),
             dv = svld2_f64(pg, d + 2 * i);
        auto ar = svget2_f64(av, 0), ai = svget2_f64(av, 1), br = svget2_f64(bv, 0),
             bi = svget2_f64(bv, 1);
        auto re = svsub_f64_x(pg, svmul_f64_x(pg, ar, br), svmul_f64_x(pg, ai, bi));
        auto im = svadd_f64_x(pg, svmul_f64_x(pg, ar, bi), svmul_f64_x(pg, ai, br));
        svst2_f64(pg, d + 2 * i,
                  svcreate2_f64(svadd_f64_x(pg, svget2_f64(dv, 0), re),
                                svadd_f64_x(pg, svget2_f64(dv, 1), im)));
    }
}
void butterflyF64(double* e, double* o, const double* w, std::size_t n, bool inverse) noexcept {
    for (std::size_t i = 0; i < n; i += svcntd()) {
        auto pg = svwhilelt_b64(i, n);
        auto ev = svld2_f64(pg, e + 2 * i), av = svld2_f64(pg, o + 2 * i),
             bv = svld2_f64(pg, w + 2 * i);
        auto ar = svget2_f64(av, 0), ai = svget2_f64(av, 1), br = svget2_f64(bv, 0),
             bi = svget2_f64(bv, 1);
        if (inverse)
            bi = svneg_f64_x(pg, bi);
        auto re = svsub_f64_x(pg, svmul_f64_x(pg, ar, br), svmul_f64_x(pg, ai, bi));
        auto im = svadd_f64_x(pg, svmul_f64_x(pg, ar, bi), svmul_f64_x(pg, ai, br));
        svst2_f64(pg, e + 2 * i,
                  svcreate2_f64(svadd_f64_x(pg, svget2_f64(ev, 0), re),
                                svadd_f64_x(pg, svget2_f64(ev, 1), im)));
        svst2_f64(pg, o + 2 * i,
                  svcreate2_f64(svsub_f64_x(pg, svget2_f64(ev, 0), re),
                                svsub_f64_x(pg, svget2_f64(ev, 1), im)));
    }
}
void biquadBankF64(double* data, std::size_t voices, std::size_t frames, const double* c,
                   double* state) noexcept {
    for (std::size_t i = 0; i < voices; i += svcntd()) {
        auto pg = svwhilelt_b64(i, voices);
        auto c0 = svld1_f64(pg, c + 0 * voices + i);
        auto c1 = svld1_f64(pg, c + 1 * voices + i);
        auto c2 = svld1_f64(pg, c + 2 * voices + i);
        auto c3 = svld1_f64(pg, c + 3 * voices + i);
        auto c4 = svld1_f64(pg, c + 4 * voices + i);
        auto z1 = svld1_f64(pg, state + i), z2 = svld1_f64(pg, state + voices + i);
        for (std::size_t f = 0; f < frames; ++f) {
            auto x = svld1_f64(pg, data + f * voices + i);
            auto y = svadd_f64_x(pg, svmul_f64_x(pg, c0, x), z1);
            z1 = svadd_f64_x(pg, svsub_f64_x(pg, svmul_f64_x(pg, c1, x), svmul_f64_x(pg, c3, y)),
                             z2);
            z2 = svsub_f64_x(pg, svmul_f64_x(pg, c2, x), svmul_f64_x(pg, c4, y));
            svst1_f64(pg, data + f * voices + i, y);
        }
        svst1_f64(pg, state + i, z1);
        svst1_f64(pg, state + voices + i, z2);
    }
}
double dotMixed(const float* a, const double* b, std::size_t n) noexcept {
    auto sum = svdup_n_f64(0);
    for (std::size_t i = 0; i < n; i += svcntd()) {
        const auto count = n - i < svcntd() ? n - i : svcntd();
        const auto pg = svwhilelt_b64(std::size_t{0}, count);
        const auto source = svld1_f32(svwhilelt_b32(std::size_t{0}, count), a + i);
        const auto spread = svreinterpret_f32_u64(svunpklo_u64(svreinterpret_u32_f32(source)));
        const auto x = svcvt_f64_f32_x(pg, spread);
        sum = svadd_f64_m(pg, sum, svmul_f64_x(pg, x, svld1_f64(pg, b + i)));
    }
    return svaddv_f64(svptrue_b64(), sum);
}
void gainChainF32(const float* a, float* d, std::size_t n, const float* gains,
                  std::size_t stages) noexcept {
    const auto width = svcntw();
    const auto all = svptrue_b32();
    const auto zero = svdup_n_f32(0);
    std::size_t i = 0;
    for (; n - i >= 4 * width; i += 4 * width) {
        auto v0 = svld1_f32(all, a + i + 0 * width), v1 = svld1_f32(all, a + i + 1 * width),
             v2 = svld1_f32(all, a + i + 2 * width), v3 = svld1_f32(all, a + i + 3 * width);
        for (std::size_t stage = 0; stage < stages; ++stage) {
            if (gains[stage] == float{})
                v0 = v1 = v2 = v3 = zero;
            else {
                const auto gain = svdup_n_f32(gains[stage]);
                v0 = svadd_f32_x(all, zero, svmul_f32_x(all, v0, gain));
                v1 = svadd_f32_x(all, zero, svmul_f32_x(all, v1, gain));
                v2 = svadd_f32_x(all, zero, svmul_f32_x(all, v2, gain));
                v3 = svadd_f32_x(all, zero, svmul_f32_x(all, v3, gain));
            }
        }
        svst1_f32(all, d + i + 0 * width, v0);
        svst1_f32(all, d + i + 1 * width, v1);
        svst1_f32(all, d + i + 2 * width, v2);
        svst1_f32(all, d + i + 3 * width, v3);
    }
    for (; i < n; i += width) {
        const auto pg = svwhilelt_b32(i, n);
        auto value = svld1_f32(pg, a + i);
        for (std::size_t stage = 0; stage < stages; ++stage)
            value = gains[stage] == float{}
                        ? zero
                        : svadd_f32_x(pg, zero, svmul_n_f32_x(pg, value, gains[stage]));
        svst1_f32(pg, d + i, value);
    }
}
void gainChainF64(const double* a, double* d, std::size_t n, const double* gains,
                  std::size_t stages) noexcept {
    const auto width = svcntd();
    const auto all = svptrue_b64();
    const auto zero = svdup_n_f64(0);
    std::size_t i = 0;
    for (; n - i >= 4 * width; i += 4 * width) {
        auto v0 = svld1_f64(all, a + i + 0 * width), v1 = svld1_f64(all, a + i + 1 * width),
             v2 = svld1_f64(all, a + i + 2 * width), v3 = svld1_f64(all, a + i + 3 * width);
        for (std::size_t stage = 0; stage < stages; ++stage) {
            if (gains[stage] == double{})
                v0 = v1 = v2 = v3 = zero;
            else {
                const auto gain = svdup_n_f64(gains[stage]);
                v0 = svadd_f64_x(all, zero, svmul_f64_x(all, v0, gain));
                v1 = svadd_f64_x(all, zero, svmul_f64_x(all, v1, gain));
                v2 = svadd_f64_x(all, zero, svmul_f64_x(all, v2, gain));
                v3 = svadd_f64_x(all, zero, svmul_f64_x(all, v3, gain));
            }
        }
        svst1_f64(all, d + i + 0 * width, v0);
        svst1_f64(all, d + i + 1 * width, v1);
        svst1_f64(all, d + i + 2 * width, v2);
        svst1_f64(all, d + i + 3 * width, v3);
    }
    for (; i < n; i += width) {
        const auto pg = svwhilelt_b64(i, n);
        auto value = svld1_f64(pg, a + i);
        for (std::size_t stage = 0; stage < stages; ++stage)
            value = gains[stage] == double{}
                        ? zero
                        : svadd_f64_x(pg, zero, svmul_n_f64_x(pg, value, gains[stage]));
        svst1_f64(pg, d + i, value);
    }
}
void fftStageF32(float* data, const float* weights, std::size_t count, std::size_t half,
                 bool inverse) noexcept {
    for (std::size_t base = 0; base < count; base += 2 * half)
        butterflyF32(data + 2 * base, data + 2 * (base + half), weights, half, inverse);
}
void fftStageF64(double* data, const double* weights, std::size_t count, std::size_t half,
                 bool inverse) noexcept {
    for (std::size_t base = 0; base < count; base += 2 * half)
        butterflyF64(data + 2 * base, data + 2 * (base + half), weights, half, inverse);
}
} // namespace
const VectorTable& sveTable() noexcept {
    static constexpr VectorTable table{
        CpuBackend::sve,
        {multiplyF32, complexMacF32, butterflyF32, copyF32, scaleF32, addScaledF32, fillF32, dotF32,
         biquadBankF32, gainChainF32, fftStageF32},
        {multiplyF64, complexMacF64, butterflyF64, copyF64, scaleF64, addScaledF64, fillF64, dotF64,
         biquadBankF64, gainChainF64, fftStageF64},
        dotMixed};
    return table;
}
} // namespace cydsp::detail
