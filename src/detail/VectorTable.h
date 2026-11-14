#pragma once
#include "cydsp/core/Cpu.h"
#include "cydsp/math/VectorOps.h"
#include <cstddef>
namespace cydsp::detail {
template <class T> struct Kernels {
    void (*multiply)(const T*, const T*, T*, std::size_t) noexcept;
    void (*complexMac)(const T*, const T*, T*, std::size_t) noexcept;
    void (*butterfly)(T*, T*, const T*, std::size_t, bool) noexcept;

    void (*copy)(const T*, T*, std::size_t) noexcept;
    void (*scale)(const T*, T*, std::size_t, T) noexcept;
    void (*addScaled)(const T*, T*, std::size_t, T) noexcept;
    void (*fill)(T*, std::size_t, T) noexcept;
    T (*dot)(const T*, const T*, std::size_t) noexcept;
    void (*biquadBank)(T*, std::size_t, std::size_t, const T*, T*) noexcept;
    void (*gainChain)(const T*, T*, std::size_t, const T*, std::size_t) noexcept;
    void (*fftStage)(T*, const T*, std::size_t, std::size_t, bool) noexcept;
    // Optional time-parallel direct convolution; adds into disjoint output.
    void (*firHead)(const T*, const T*, T*, std::size_t, std::size_t) noexcept = nullptr;
};
struct VectorTable {
    CpuBackend backend;
    Kernels<float> f32;
    Kernels<double> f64;
    double (*dotMixed)(const float*, const double*, std::size_t) noexcept;
    // Optional double-precision, time-parallel ADAA wavefolder. Disjoint driven
    // input/output, finite input clamped to +/-1e6; previous is the driven sample.
    void (*adaaFold)(const double*, double*, std::size_t, double, bool, double,
                     double) noexcept = nullptr;
    void (*adaaClip)(const double*, double*, std::size_t, double, bool, double,
                     double) noexcept = nullptr;
    void (*adaaTanh)(const double*, double*, std::size_t, double, bool, double,
                     double) noexcept = nullptr;
    void (*compressorGain)(const double*, double*, std::size_t, double, double, double,
                           bool) noexcept = nullptr;
};
struct VectorAccess {
    template <class T> static const Kernels<T>& kernels(const VectorOps& ops) noexcept {
        if constexpr (std::same_as<T, float>)
            return ops.table_->f32;
        else
            return ops.table_->f64;
    }
    static const VectorTable& table(const VectorOps& ops) noexcept {
        return *ops.table_;
    }
};
const VectorTable& scalarTable() noexcept;
#if CYDSP_HAS_SSE2
const VectorTable& sse2Table() noexcept;
#endif
#if CYDSP_HAS_AVX2
const VectorTable& avx2Table() noexcept;
#endif
#if CYDSP_HAS_NEON
const VectorTable& neonTable() noexcept;
#endif
#if CYDSP_HAS_AVX512
const VectorTable& avx512Table() noexcept;
#endif
#if CYDSP_HAS_SVE
const VectorTable& sveTable() noexcept;
#endif
} // namespace cydsp::detail
