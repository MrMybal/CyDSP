#pragma once
#if defined(_MSC_VER) && !defined(__clang__)
#include <arm64_neon.h>
#else
#include <arm_neon.h>
#endif
namespace cydsp::detail {
namespace {
struct Double2 {
    using Reg = float64x2_t;
    using Mask = uint64x2_t;
    static constexpr std::size_t maxWidth = 2;
    static constexpr std::size_t lanes() noexcept {
        return 2;
    }
    static CYDSP_SIMD_INLINE Reg set(double v) noexcept {
        return vdupq_n_f64(v);
    }
    static CYDSP_SIMD_INLINE Reg load(const double* p) noexcept {
        return vld1q_f64(p);
    }
    static CYDSP_SIMD_INLINE void store(double* p, Reg v) noexcept {
        vst1q_f64(p, v);
    }
    static CYDSP_SIMD_INLINE Reg add(Reg a, Reg b) noexcept {
        return vaddq_f64(a, b);
    }
    static CYDSP_SIMD_INLINE Reg sub(Reg a, Reg b) noexcept {
        return vsubq_f64(a, b);
    }
    static CYDSP_SIMD_INLINE Reg mul(Reg a, Reg b) noexcept {
        return vmulq_f64(a, b);
    }
    static CYDSP_SIMD_INLINE Reg div(Reg a, Reg b) noexcept {
        return vdivq_f64(a, b);
    }
    static CYDSP_SIMD_INLINE Reg min(Reg a, Reg b) noexcept {
        return vminq_f64(a, b);
    }
    static CYDSP_SIMD_INLINE Reg max(Reg a, Reg b) noexcept {
        return vmaxq_f64(a, b);
    }
    static CYDSP_SIMD_INLINE Reg sqrt(Reg a) noexcept {
        return vsqrtq_f64(a);
    }
    static CYDSP_SIMD_INLINE Reg abs(Reg a) noexcept {
        return vabsq_f64(a);
    }
    static CYDSP_SIMD_INLINE Reg nearest(Reg a) noexcept {
        return vrndnq_f64(a);
    }
    static CYDSP_SIMD_INLINE Reg floor(Reg a) noexcept {
        return vrndmq_f64(a);
    }
    static CYDSP_SIMD_INLINE Mask gt(Reg a, Reg b) noexcept {
        return vcgtq_f64(a, b);
    }
    static CYDSP_SIMD_INLINE Mask lt(Reg a, Reg b) noexcept {
        return vcltq_f64(a, b);
    }
    static CYDSP_SIMD_INLINE Mask le(Reg a, Reg b) noexcept {
        return vcleq_f64(a, b);
    }
    static CYDSP_SIMD_INLINE Mask eq(Reg a, Reg b) noexcept {
        return vceqq_f64(a, b);
    }
    static CYDSP_SIMD_INLINE Mask both(Mask a, Mask b) noexcept {
        return vandq_u64(a, b);
    }
    static CYDSP_SIMD_INLINE Mask either(Mask a, Mask b) noexcept {
        return vorrq_u64(a, b);
    }
    static CYDSP_SIMD_INLINE Mask without(Mask a, Mask b) noexcept {
        return vbicq_u64(a, b);
    }
    static CYDSP_SIMD_INLINE bool all(Mask m) noexcept {
        return vminvq_u32(vreinterpretq_u32_u64(m)) == 0xffffffffu;
    }
    static CYDSP_SIMD_INLINE Reg select(Mask m, Reg yes, Reg no) noexcept {
        return vbslq_f64(m, yes, no);
    }
    static CYDSP_SIMD_INLINE Reg signOf(Reg a) noexcept {
        return vreinterpretq_f64_u64(
            vandq_u64(vreinterpretq_u64_f64(a), vdupq_n_u64(0x8000000000000000ull)));
    }
    static CYDSP_SIMD_INLINE Reg orBits(Reg a, Reg b) noexcept {
        return vreinterpretq_f64_u64(vorrq_u64(vreinterpretq_u64_f64(a), vreinterpretq_u64_f64(b)));
    }
    static CYDSP_SIMD_INLINE Reg xorBits(Reg a, Reg b) noexcept {
        return vreinterpretq_f64_u64(veorq_u64(vreinterpretq_u64_f64(a), vreinterpretq_u64_f64(b)));
    }
    static CYDSP_SIMD_INLINE Reg pow2(Reg n) noexcept {
        return vreinterpretq_f64_s64(
            vshlq_n_s64(vaddq_s64(vcvtq_s64_f64(n), vdupq_n_s64(1023)), 52));
    }
    static CYDSP_SIMD_INLINE void split(Reg u, Reg& m, Reg& e) noexcept {
        auto b = vreinterpretq_u64_f64(u);
        e = vsubq_f64(vcvtq_f64_u64(vshrq_n_u64(b, 52)), set(1023));
        m = vreinterpretq_f64_u64(vorrq_u64(vandq_u64(b, vdupq_n_u64(0x000fffffffffffffull)),
                                            vdupq_n_u64(0x3ff0000000000000ull)));
    }
    static CYDSP_SIMD_INLINE Reg fromSamples(const float* p) noexcept {
        return vcvt_f64_f32(vld1_f32(p));
    }
    static CYDSP_SIMD_INLINE void toSamples(float* p, Reg v) noexcept {
        vst1_f32(p, vcvt_f32_f64(v));
    }
    static CYDSP_SIMD_INLINE Reg rounded(Reg v, const float*) noexcept {
        return vcvt_f64_f32(vcvt_f32_f64(v));
    }
    static CYDSP_SIMD_INLINE Reg fromSamples(const double* p) noexcept {
        return load(p);
    }
    static CYDSP_SIMD_INLINE void toSamples(double* p, Reg v) noexcept {
        store(p, v);
    }
    static CYDSP_SIMD_INLINE Reg rounded(Reg v, const double*) noexcept {
        return v;
    }
};
} // namespace
} // namespace cydsp::detail
