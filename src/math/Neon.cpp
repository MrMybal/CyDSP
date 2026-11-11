#include "detail/Scalar.h"
#include "detail/VectorTable.h"
#include <cstdint>
#if defined(_MSC_VER) && !defined(__clang__) && defined(_M_ARM64)
#include <arm64_neon.h>
#else
#include <arm_neon.h>
#endif
namespace cydsp::detail {
namespace {
float32x4_t productF32(float32x4_t a, float32x4_t b) noexcept {
    const std::uint32_t sign[4]{0x80000000u, 0, 0x80000000u, 0};
    const auto cross = vmulq_f32(vtrnq_f32(a, a).val[1], vrev64q_f32(b));
    return vaddq_f32(
        vmulq_f32(vtrnq_f32(a, a).val[0], b),
        vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(cross), vld1q_u32(sign))));
}
float32x4_t conjugateF32(float32x4_t value) noexcept {
    const std::uint32_t sign[4]{0, 0x80000000u, 0, 0x80000000u};
    return vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(value), vld1q_u32(sign)));
}
#if defined(__aarch64__) || defined(_M_ARM64)
double dotMixed(const float* a, const double* b, std::size_t n) noexcept {
    if (!n)
        return 0;
    auto low = vdupq_n_f64(0), high = vdupq_n_f64(0);
    std::size_t i = 0;
    for (; n - i >= 4; i += 4) {
        const auto value = vld1q_f32(a + i);
        low = vaddq_f64(low, vmulq_f64(vcvt_f64_f32(vget_low_f32(value)), vld1q_f64(b + i)));
        high = vaddq_f64(high, vmulq_f64(vcvt_f64_f32(vget_high_f32(value)), vld1q_f64(b + i + 2)));
    }
    double lanes[2];
    vst1q_f64(lanes, vaddq_f64(low, high));
    return (lanes[0] + lanes[1]) + scalarDotMixed(a + i, b + i, n - i);
}
#else
double dotMixed(const float* a, const double* b, std::size_t n) noexcept {
    return scalarDotMixed(a, b, n);
}
#endif
void biquadBankF32(float* data, std::size_t voices, std::size_t frames, const float* c,
                   float* states) noexcept {
    std::size_t voice = 0;
    for (; voices - voice >= 4; voice += 4) {
        const auto c0 = vld1q_f32(c + 0 * voices + voice);
        const auto c1 = vld1q_f32(c + 1 * voices + voice);
        const auto c2 = vld1q_f32(c + 2 * voices + voice);
        const auto c3 = vld1q_f32(c + 3 * voices + voice);
        const auto c4 = vld1q_f32(c + 4 * voices + voice);
        auto z1 = vld1q_f32(states + voice), z2 = vld1q_f32(states + voices + voice);
        for (std::size_t frame = 0; frame < frames; ++frame) {
            auto* samples = data + frame * voices + voice;
            const auto x = vld1q_f32(samples);
            const auto y = vaddq_f32(vmulq_f32(c0, x), z1);
            const auto next1 = vaddq_f32(vsubq_f32(vmulq_f32(c1, x), vmulq_f32(c3, y)), z2);
            z2 = vsubq_f32(vmulq_f32(c2, x), vmulq_f32(c4, y));
            z1 = next1;
            vst1q_f32(samples, y);
        }
        vst1q_f32(states + voice, z1);
        vst1q_f32(states + voices + voice, z2);
    }
    for (; voice < voices; ++voice)
        scalarBiquadVoice(data, voices, frames, c, states, voice);
}
#if defined(__aarch64__) || defined(_M_ARM64)
void biquadBankF64(double* data, std::size_t voices, std::size_t frames, const double* c,
                   double* states) noexcept {
    std::size_t voice = 0;
    for (; voices - voice >= 2; voice += 2) {
        const auto c0 = vld1q_f64(c + 0 * voices + voice);
        const auto c1 = vld1q_f64(c + 1 * voices + voice);
        const auto c2 = vld1q_f64(c + 2 * voices + voice);
        const auto c3 = vld1q_f64(c + 3 * voices + voice);
        const auto c4 = vld1q_f64(c + 4 * voices + voice);
        auto z1 = vld1q_f64(states + voice), z2 = vld1q_f64(states + voices + voice);
        for (std::size_t frame = 0; frame < frames; ++frame) {
            auto* samples = data + frame * voices + voice;
            const auto x = vld1q_f64(samples);
            const auto y = vaddq_f64(vmulq_f64(c0, x), z1);
            const auto next1 = vaddq_f64(vsubq_f64(vmulq_f64(c1, x), vmulq_f64(c3, y)), z2);
            z2 = vsubq_f64(vmulq_f64(c2, x), vmulq_f64(c4, y));
            z1 = next1;
            vst1q_f64(samples, y);
        }
        vst1q_f64(states + voice, z1);
        vst1q_f64(states + voices + voice, z2);
    }
    for (; voice < voices; ++voice)
        scalarBiquadVoice(data, voices, frames, c, states, voice);
}
#else
void biquadBankF64(double* data, std::size_t voices, std::size_t frames, const double* c,
                   double* states) noexcept {
    scalarBiquadBank(data, voices, frames, c, states);
}
#endif
float dotF32(const float* a, const float* b, std::size_t n) noexcept {
    if (!n)
        return float{};
    auto s0 = vdupq_n_f32(0), s1 = s0, s2 = s0, s3 = s0;
    std::size_t i = 0;
    for (; n - i >= 16; i += 16) {
        s0 = vaddq_f32(s0, vmulq_f32(vld1q_f32(a + i + 0), vld1q_f32(b + i + 0)));
        s1 = vaddq_f32(s1, vmulq_f32(vld1q_f32(a + i + 4), vld1q_f32(b + i + 4)));
        s2 = vaddq_f32(s2, vmulq_f32(vld1q_f32(a + i + 8), vld1q_f32(b + i + 8)));
        s3 = vaddq_f32(s3, vmulq_f32(vld1q_f32(a + i + 12), vld1q_f32(b + i + 12)));
    }
    auto sum = vaddq_f32(vaddq_f32(s0, s1), vaddq_f32(s2, s3));
    for (; n - i >= 4; i += 4)
        sum = vaddq_f32(sum, vmulq_f32(vld1q_f32(a + i), vld1q_f32(b + i)));
    const auto pair = vadd_f32(vget_low_f32(sum), vget_high_f32(sum));
    const auto result = vget_lane_f32(vpadd_f32(pair, pair), 0);
    return result + scalarDot(a + i, b + i, n - i);
}
void copyF32(const float* s, float* d, std::size_t n) noexcept {
    if (!n)
        return;

    std::size_t i = 0;
    for (; n - i >= 4; i += 4) {
        vst1q_f32(d + i, vld1q_f32(s + i));
    }
    scalarCopy(s + i, d + i, n - i);
}
void scaleF32(const float* s, float* d, std::size_t n, float gain) noexcept {
    if (!n)
        return;
    const auto factor = vdupq_n_f32(gain);
    std::size_t i = 0;
    for (; n - i >= 4; i += 4) {
        vst1q_f32(d + i, vmulq_f32(vld1q_f32(s + i), factor));
    }
    scalarScale(s + i, d + i, n - i, gain);
}
void addScaledF32(const float* s, float* d, std::size_t n, float gain) noexcept {
    if (!n)
        return;
    const auto factor = vdupq_n_f32(gain);
    std::size_t i = 0;
    for (; n - i >= 4; i += 4) {
        vst1q_f32(d + i, vaddq_f32(vld1q_f32(d + i), vmulq_f32(vld1q_f32(s + i), factor)));
    }
    scalarAddScaled(s + i, d + i, n - i, gain);
}
void fillF32(float* d, std::size_t n, float value) noexcept {
    if (!n)
        return;
    const auto factor = vdupq_n_f32(value);
    std::size_t i = 0;
    for (; n - i >= 4; i += 4) {
        vst1q_f32(d + i, factor);
    }
    scalarFill(d + i, n - i, value);
}
void multiplyF32(const float* a, const float* b, float* d, std::size_t n) noexcept {
    if (!n)
        return;

    std::size_t i = 0;
    for (; n - i >= 4; i += 4) {
        vst1q_f32(d + i, vmulq_f32(vld1q_f32(a + i), vld1q_f32(b + i)));
    }
    scalarMultiply(a + i, b + i, d + i, n - i);
}
void complexMacF32(const float* a, const float* b, float* d, std::size_t n) noexcept {
    if (!n)
        return;

    std::size_t i = 0;
    for (; n - i >= 2; i += 2) {
        vst1q_f32(d + 2 * i, vaddq_f32(vld1q_f32(d + 2 * i),
                                       productF32(vld1q_f32(a + 2 * i), vld1q_f32(b + 2 * i))));
    }
    scalarComplexMac(a + 2 * i, b + 2 * i, d + 2 * i, n - i);
}
void butterflyF32(float* even, float* odd, const float* w, std::size_t n, bool inverse) noexcept {
    if (!n)
        return;

    std::size_t i = 0;
    for (; n - i >= 2; i += 2) {
        auto twiddle = vld1q_f32(w + 2 * i);
        if (inverse)
            twiddle = conjugateF32(twiddle);
        const auto e = vld1q_f32(even + 2 * i), p = productF32(vld1q_f32(odd + 2 * i), twiddle);
        vst1q_f32(even + 2 * i, vaddq_f32(e, p));
        vst1q_f32(odd + 2 * i, vsubq_f32(e, p));
    }
    scalarButterfly(even + 2 * i, odd + 2 * i, w + 2 * i, n - i, inverse);
}
#if defined(__aarch64__) || defined(_M_ARM64)
float64x2_t productF64(float64x2_t a, float64x2_t b) noexcept {
    const std::uint64_t sign[2]{0x8000000000000000ull, 0};
    const auto cross = vmulq_f64(vzip2q_f64(a, a), vextq_f64(b, b, 1));
    return vaddq_f64(
        vmulq_f64(vzip1q_f64(a, a), b),
        vreinterpretq_f64_u64(veorq_u64(vreinterpretq_u64_f64(cross), vld1q_u64(sign))));
}
float64x2_t conjugateF64(float64x2_t value) noexcept {
    const std::uint64_t sign[2]{0, 0x8000000000000000ull};
    return vreinterpretq_f64_u64(veorq_u64(vreinterpretq_u64_f64(value), vld1q_u64(sign)));
}
double dotF64(const double* a, const double* b, std::size_t n) noexcept {
    if (!n)
        return double{};
    auto s0 = vdupq_n_f64(0), s1 = s0, s2 = s0, s3 = s0;
    std::size_t i = 0;
    for (; n - i >= 8; i += 8) {
        s0 = vaddq_f64(s0, vmulq_f64(vld1q_f64(a + i + 0), vld1q_f64(b + i + 0)));
        s1 = vaddq_f64(s1, vmulq_f64(vld1q_f64(a + i + 2), vld1q_f64(b + i + 2)));
        s2 = vaddq_f64(s2, vmulq_f64(vld1q_f64(a + i + 4), vld1q_f64(b + i + 4)));
        s3 = vaddq_f64(s3, vmulq_f64(vld1q_f64(a + i + 6), vld1q_f64(b + i + 6)));
    }
    auto sum = vaddq_f64(vaddq_f64(s0, s1), vaddq_f64(s2, s3));
    for (; n - i >= 2; i += 2)
        sum = vaddq_f64(sum, vmulq_f64(vld1q_f64(a + i), vld1q_f64(b + i)));
    const auto result = vgetq_lane_f64(sum, 0) + vgetq_lane_f64(sum, 1);
    return result + scalarDot(a + i, b + i, n - i);
}
void copyF64(const double* s, double* d, std::size_t n) noexcept {
    if (!n)
        return;

    std::size_t i = 0;
    for (; n - i >= 2; i += 2) {
        vst1q_f64(d + i, vld1q_f64(s + i));
    }
    scalarCopy(s + i, d + i, n - i);
}
void scaleF64(const double* s, double* d, std::size_t n, double gain) noexcept {
    if (!n)
        return;
    const auto factor = vdupq_n_f64(gain);
    std::size_t i = 0;
    for (; n - i >= 2; i += 2) {
        vst1q_f64(d + i, vmulq_f64(vld1q_f64(s + i), factor));
    }
    scalarScale(s + i, d + i, n - i, gain);
}
void addScaledF64(const double* s, double* d, std::size_t n, double gain) noexcept {
    if (!n)
        return;
    const auto factor = vdupq_n_f64(gain);
    std::size_t i = 0;
    for (; n - i >= 2; i += 2) {
        vst1q_f64(d + i, vaddq_f64(vld1q_f64(d + i), vmulq_f64(vld1q_f64(s + i), factor)));
    }
    scalarAddScaled(s + i, d + i, n - i, gain);
}
void fillF64(double* d, std::size_t n, double value) noexcept {
    if (!n)
        return;
    const auto factor = vdupq_n_f64(value);
    std::size_t i = 0;
    for (; n - i >= 2; i += 2) {
        vst1q_f64(d + i, factor);
    }
    scalarFill(d + i, n - i, value);
}
void multiplyF64(const double* a, const double* b, double* d, std::size_t n) noexcept {
    if (!n)
        return;

    std::size_t i = 0;
    for (; n - i >= 2; i += 2) {
        vst1q_f64(d + i, vmulq_f64(vld1q_f64(a + i), vld1q_f64(b + i)));
    }
    scalarMultiply(a + i, b + i, d + i, n - i);
}
void complexMacF64(const double* a, const double* b, double* d, std::size_t n) noexcept {
    if (!n)
        return;

    std::size_t i = 0;
    for (; n - i >= 1; i += 1) {
        vst1q_f64(d + 2 * i, vaddq_f64(vld1q_f64(d + 2 * i),
                                       productF64(vld1q_f64(a + 2 * i), vld1q_f64(b + 2 * i))));
    }
    scalarComplexMac(a + 2 * i, b + 2 * i, d + 2 * i, n - i);
}
void butterflyF64(double* even, double* odd, const double* w, std::size_t n,
                  bool inverse) noexcept {
    if (!n)
        return;

    std::size_t i = 0;
    for (; n - i >= 1; i += 1) {
        auto twiddle = vld1q_f64(w + 2 * i);
        if (inverse)
            twiddle = conjugateF64(twiddle);
        const auto e = vld1q_f64(even + 2 * i), p = productF64(vld1q_f64(odd + 2 * i), twiddle);
        vst1q_f64(even + 2 * i, vaddq_f64(e, p));
        vst1q_f64(odd + 2 * i, vsubq_f64(e, p));
    }
    scalarButterfly(even + 2 * i, odd + 2 * i, w + 2 * i, n - i, inverse);
}
#else
double dotF64(const double* a, const double* b, std::size_t n) noexcept {
    return scalarDot(a, b, n);
}
void copyF64(const double* s, double* d, std::size_t n) noexcept {
    scalarCopy(s, d, n);
}
void scaleF64(const double* s, double* d, std::size_t n, double gain) noexcept {
    scalarScale(s, d, n, gain);
}
void addScaledF64(const double* s, double* d, std::size_t n, double gain) noexcept {
    scalarAddScaled(s, d, n, gain);
}
void fillF64(double* d, std::size_t n, double value) noexcept {
    scalarFill(d, n, value);
}
void multiplyF64(const double* a, const double* b, double* d, std::size_t n) noexcept {
    scalarMultiply(a, b, d, n);
}
void complexMacF64(const double* a, const double* b, double* d, std::size_t n) noexcept {
    scalarComplexMac(a, b, d, n);
}
void butterflyF64(double* even, double* odd, const double* w, std::size_t n,
                  bool inverse) noexcept {
    scalarButterfly(even, odd, w, n, inverse);
}
#endif
void gainChainF32(const float* a, float* d, std::size_t n, const float* gains,
                  std::size_t stages) noexcept {
    const auto zero = vdupq_n_f32(0);
    std::size_t i = 0;
    for (; n - i >= 4 * 4; i += 4 * 4) {
        auto v0 = vld1q_f32(a + i + 0 * 4), v1 = vld1q_f32(a + i + 1 * 4),
             v2 = vld1q_f32(a + i + 2 * 4), v3 = vld1q_f32(a + i + 3 * 4);
        for (std::size_t stage = 0; stage < stages; ++stage) {
            if (gains[stage] == float{})
                v0 = v1 = v2 = v3 = zero;
            else {
                const auto gain = vdupq_n_f32(gains[stage]);
                v0 = vaddq_f32(zero, vmulq_f32(v0, gain));
                v1 = vaddq_f32(zero, vmulq_f32(v1, gain));
                v2 = vaddq_f32(zero, vmulq_f32(v2, gain));
                v3 = vaddq_f32(zero, vmulq_f32(v3, gain));
            }
        }
        vst1q_f32(d + i + 0 * 4, v0);
        vst1q_f32(d + i + 1 * 4, v1);
        vst1q_f32(d + i + 2 * 4, v2);
        vst1q_f32(d + i + 3 * 4, v3);
    }
    for (; n - i >= 4; i += 4) {
        auto value = vld1q_f32(a + i);
        for (std::size_t stage = 0; stage < stages; ++stage) {
            if (gains[stage] == float{})
                value = zero;
            else {
                const auto gain = vdupq_n_f32(gains[stage]);
                value = vaddq_f32(zero, vmulq_f32(value, gain));
            }
        }
        vst1q_f32(d + i, value);
    }
    if (i < n)
        scalarGainChain<float>(a + i, d + i, n - i, gains, stages);
}
#if defined(__aarch64__) || defined(_M_ARM64)
void gainChainF64(const double* a, double* d, std::size_t n, const double* gains,
                  std::size_t stages) noexcept {
    const auto zero = vdupq_n_f64(0);
    std::size_t i = 0;
    for (; n - i >= 4 * 2; i += 4 * 2) {
        auto v0 = vld1q_f64(a + i + 0 * 2), v1 = vld1q_f64(a + i + 1 * 2),
             v2 = vld1q_f64(a + i + 2 * 2), v3 = vld1q_f64(a + i + 3 * 2);
        for (std::size_t stage = 0; stage < stages; ++stage) {
            if (gains[stage] == double{})
                v0 = v1 = v2 = v3 = zero;
            else {
                const auto gain = vdupq_n_f64(gains[stage]);
                v0 = vaddq_f64(zero, vmulq_f64(v0, gain));
                v1 = vaddq_f64(zero, vmulq_f64(v1, gain));
                v2 = vaddq_f64(zero, vmulq_f64(v2, gain));
                v3 = vaddq_f64(zero, vmulq_f64(v3, gain));
            }
        }
        vst1q_f64(d + i + 0 * 2, v0);
        vst1q_f64(d + i + 1 * 2, v1);
        vst1q_f64(d + i + 2 * 2, v2);
        vst1q_f64(d + i + 3 * 2, v3);
    }
    for (; n - i >= 2; i += 2) {
        auto value = vld1q_f64(a + i);
        for (std::size_t stage = 0; stage < stages; ++stage) {
            if (gains[stage] == double{})
                value = zero;
            else {
                const auto gain = vdupq_n_f64(gains[stage]);
                value = vaddq_f64(zero, vmulq_f64(value, gain));
            }
        }
        vst1q_f64(d + i, value);
    }
    if (i < n)
        scalarGainChain<double>(a + i, d + i, n - i, gains, stages);
}
#else
void gainChainF64(const double* a, double* d, std::size_t n, const double* gains,
                  std::size_t stages) noexcept {
    scalarGainChain<double>(a, d, n, gains, stages);
}
#endif
void fftStageF32(float* data, const float* weights, std::size_t count, std::size_t half,
                 bool inverse) noexcept {
    if (half == 1) {
        const auto low = vld1_f32(weights);
        auto w = vcombine_f32(low, low);
        if (inverse)
            w = conjugateF32(w);
        std::size_t base = 0;
        for (; count - base >= 4; base += 4) {
            const auto a = vld1q_f32(data + 2 * base), b = vld1q_f32(data + 2 * base + 4);
            const auto e = vcombine_f32(vget_low_f32(a), vget_low_f32(b));
            const auto o = vcombine_f32(vget_high_f32(a), vget_high_f32(b)), p = productF32(o, w);
            const auto sum = vaddq_f32(e, p), difference = vsubq_f32(e, p);
            vst1q_f32(data + 2 * base, vcombine_f32(vget_low_f32(sum), vget_low_f32(difference)));
            vst1q_f32(data + 2 * base + 4,
                      vcombine_f32(vget_high_f32(sum), vget_high_f32(difference)));
        }
        for (; base < count; base += 2)
            scalarButterfly(data + 2 * base, data + 2 * (base + 1), weights, std::size_t{1},
                            inverse);
        return;
    }
    for (std::size_t base = 0; base < count; base += 2 * half)
        butterflyF32(data + 2 * base, data + 2 * (base + half), weights, half, inverse);
}
void fftStageF64(double* data, const double* weights, std::size_t count, std::size_t half,
                 bool inverse) noexcept {
    for (std::size_t base = 0; base < count; base += 2 * half)
        butterflyF64(data + 2 * base, data + 2 * (base + half), weights, half, inverse);
}
} // namespace
const VectorTable& neonTable() noexcept {
    static constexpr VectorTable table{
        CpuBackend::neon,
        {multiplyF32, complexMacF32, butterflyF32, copyF32, scaleF32, addScaledF32, fillF32, dotF32,
         biquadBankF32, gainChainF32, fftStageF32},
        {multiplyF64, complexMacF64, butterflyF64, copyF64, scaleF64, addScaledF64, fillF64, dotF64,
         biquadBankF64, gainChainF64, fftStageF64},
        dotMixed};
    return table;
}
} // namespace cydsp::detail
