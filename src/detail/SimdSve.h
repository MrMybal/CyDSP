#pragma once
#include <arm_sve.h>
namespace cydsp::detail {
namespace {
struct DoubleSve {
    using Reg = svfloat64_t;
    using Mask = svbool_t;
    static constexpr std::size_t maxWidth = 32;
    static std::size_t lanes() noexcept {
        return svcntd();
    }
    static CYDSP_SIMD_INLINE Reg set(double v) noexcept {
        return svdup_f64(v);
    }
    static CYDSP_SIMD_INLINE Reg load(const double* p) noexcept {
        return svld1_f64(svptrue_b64(), p);
    }
    static CYDSP_SIMD_INLINE void store(double* p, Reg v) noexcept {
        svst1_f64(svptrue_b64(), p, v);
    }
    static CYDSP_SIMD_INLINE Reg add(Reg a, Reg b) noexcept {
        return svadd_f64_x(svptrue_b64(), a, b);
    }
    static CYDSP_SIMD_INLINE Reg sub(Reg a, Reg b) noexcept {
        return svsub_f64_x(svptrue_b64(), a, b);
    }
    static CYDSP_SIMD_INLINE Reg mul(Reg a, Reg b) noexcept {
        return svmul_f64_x(svptrue_b64(), a, b);
    }
    static CYDSP_SIMD_INLINE Reg div(Reg a, Reg b) noexcept {
        return svdiv_f64_x(svptrue_b64(), a, b);
    }
    static CYDSP_SIMD_INLINE Reg min(Reg a, Reg b) noexcept {
        return svmin_f64_x(svptrue_b64(), a, b);
    }
    static CYDSP_SIMD_INLINE Reg max(Reg a, Reg b) noexcept {
        return svmax_f64_x(svptrue_b64(), a, b);
    }
    static CYDSP_SIMD_INLINE Reg sqrt(Reg a) noexcept {
        return svsqrt_f64_x(svptrue_b64(), a);
    }
    static CYDSP_SIMD_INLINE Reg abs(Reg a) noexcept {
        return svabs_f64_x(svptrue_b64(), a);
    }
    static CYDSP_SIMD_INLINE Reg nearest(Reg a) noexcept {
        return svrintn_f64_x(svptrue_b64(), a);
    }
    static CYDSP_SIMD_INLINE Reg floor(Reg a) noexcept {
        return svrintm_f64_x(svptrue_b64(), a);
    }
    static CYDSP_SIMD_INLINE Mask gt(Reg a, Reg b) noexcept {
        return svcmpgt_f64(svptrue_b64(), a, b);
    }
    static CYDSP_SIMD_INLINE Mask lt(Reg a, Reg b) noexcept {
        return svcmplt_f64(svptrue_b64(), a, b);
    }
    static CYDSP_SIMD_INLINE Mask le(Reg a, Reg b) noexcept {
        return svcmple_f64(svptrue_b64(), a, b);
    }
    static CYDSP_SIMD_INLINE Mask eq(Reg a, Reg b) noexcept {
        return svcmpeq_f64(svptrue_b64(), a, b);
    }
    static CYDSP_SIMD_INLINE Mask both(Mask a, Mask b) noexcept {
        return svand_b_z(svptrue_b64(), a, b);
    }
    static CYDSP_SIMD_INLINE Mask either(Mask a, Mask b) noexcept {
        return svorr_b_z(svptrue_b64(), a, b);
    }
    static CYDSP_SIMD_INLINE Mask without(Mask a, Mask b) noexcept {
        return svbic_b_z(svptrue_b64(), a, b);
    }
    static CYDSP_SIMD_INLINE bool all(Mask m) noexcept {
        return !svptest_any(svptrue_b64(), svnot_b_z(svptrue_b64(), m));
    }
    static CYDSP_SIMD_INLINE Reg select(Mask m, Reg yes, Reg no) noexcept {
        return svsel_f64(m, yes, no);
    }
    static CYDSP_SIMD_INLINE Reg signOf(Reg a) noexcept {
        return svreinterpret_f64_u64(
            svand_n_u64_x(svptrue_b64(), svreinterpret_u64_f64(a), 0x8000000000000000ull));
    }
    static CYDSP_SIMD_INLINE Reg orBits(Reg a, Reg b) noexcept {
        return svreinterpret_f64_u64(
            svorr_u64_x(svptrue_b64(), svreinterpret_u64_f64(a), svreinterpret_u64_f64(b)));
    }
    static CYDSP_SIMD_INLINE Reg xorBits(Reg a, Reg b) noexcept {
        return svreinterpret_f64_u64(
            sveor_u64_x(svptrue_b64(), svreinterpret_u64_f64(a), svreinterpret_u64_f64(b)));
    }
    static CYDSP_SIMD_INLINE Reg pow2(Reg n) noexcept {
        auto i = svadd_n_s64_x(svptrue_b64(), svcvt_s64_f64_x(svptrue_b64(), n), 1023);
        return svreinterpret_f64_s64(svlsl_n_s64_x(svptrue_b64(), i, 52));
    }
    static CYDSP_SIMD_INLINE void split(Reg u, Reg& m, Reg& e) noexcept {
        auto b = svreinterpret_u64_f64(u);
        e = svsub_n_f64_x(svptrue_b64(),
                          svcvt_f64_u64_x(svptrue_b64(), svlsr_n_u64_x(svptrue_b64(), b, 52)),
                          1023);
        m = svreinterpret_f64_u64(
            svorr_n_u64_x(svptrue_b64(), svand_n_u64_x(svptrue_b64(), b, 0x000fffffffffffffull),
                          0x3ff0000000000000ull));
    }
    static CYDSP_SIMD_INLINE Reg fromSamples(const float* p) noexcept {
        auto f = svld1_f32(svwhilelt_b32(std::uint64_t{0}, svcntd()), p);
        return svcvt_f64_f32_x(svptrue_b64(), svzip1_f32(f, f));
    }
    static CYDSP_SIMD_INLINE void toSamples(float* p, Reg v) noexcept {
        auto f = svcvt_f32_f64_x(svptrue_b64(), v);
        svst1_f32(svwhilelt_b32(std::uint64_t{0}, svcntd()), p, svuzp1_f32(f, f));
    }
    static CYDSP_SIMD_INLINE Reg rounded(Reg v, const float*) noexcept {
        return svcvt_f64_f32_x(svptrue_b64(), svcvt_f32_f64_x(svptrue_b64(), v));
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
