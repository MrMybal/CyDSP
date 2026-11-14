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
    // Two adjacent radix-2 stages held in registers, with the same twiddles.
    void (*fftPair)(T*, const T*, const T*, std::size_t, std::size_t, bool) noexcept = nullptr;
    void (*realSplit)(const T*, const T*, T*, std::size_t) noexcept = nullptr;
    void (*clipDirect)(const T*, T*, std::size_t, double, double, double,
                       double) noexcept = nullptr;
    // Saturator tile stages. Drive: stage = {inputGain, inputBias, inputScale,
    // drive, bias} to clamped double. Publish: optional dry/wet mix and rounding.
    void (*saturationDrive)(const T*, double*, std::size_t, const double*, bool) noexcept = nullptr;
    void (*saturationPublish)(const T*, const double*, T*, std::size_t, double, double,
                              bool) noexcept = nullptr;
    // out = |in|, or max(out, |in|) when accumulating a linked detector.
    void (*magnitude)(const T*, double*, std::size_t, bool) noexcept = nullptr;
    // clamp(in*gain*scale, +/-ceiling), then T(value) or dryGain*dry+wetGain*T(value).
    void (*applyGains)(const T*, const double*, T*, std::size_t, double, double, const T*, double,
                       double, bool) noexcept = nullptr;
    // Bit reversal fused with stages half=1,2: (natural, permuted, reverse table,
    // stage-contiguous twiddles, count >= 16, inverse). Buffers are disjoint.
    void (*fftHead)(const T*, T*, const std::size_t*, const T*, std::size_t,
                    bool) noexcept = nullptr;
    // Bins [begin,end), 1 <= begin, of the packed<->real-spectrum recombination:
    // (source, real twiddles, destination of bin begin, half, begin, end).
    void (*realSplitRange)(const T*, const T*, T*, std::size_t, std::size_t,
                           std::size_t) noexcept = nullptr;
    void (*realMergeRange)(const T*, const T*, T*, std::size_t, std::size_t,
                           std::size_t) noexcept = nullptr;
    void (*convolutionEmit)(const T*, const T*, T*, T*, std::size_t, T) noexcept = nullptr;
    void (*cubicDirect)(const T*, T*, std::size_t, const double*, double, double, double, double,
                        bool) noexcept = nullptr;
    void (*eqDetectPair)(const T*, const T*, double*, double*, std::size_t, const double*, double*,
                         double, double) noexcept = nullptr;
    void (*foldDirect)(const T*, T*, std::size_t, const double*, double, double, double, double,
                       bool) noexcept = nullptr;
};
struct CompressorCurve {
    double thresholdDb, ratio, kneeDb, lower, upper, threshold, slope;
    bool limiter;
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
    void (*compressorGainPrepared)(const double*, double*, std::size_t,
                                   const CompressorCurve&) noexcept = nullptr;
    // Cancellation-free mean of (2/pi)atan between consecutive driven samples.
    void (*adaaAtan)(const double*, double*, std::size_t, double, bool, double,
                     double) noexcept = nullptr;
    // Memoryless (shape(x)-dc)*gain on driven doubles; in-place is allowed.
    void (*shapeTanh)(const double*, double*, std::size_t, double, double) noexcept = nullptr;
    void (*shapeAtan)(const double*, double*, std::size_t, double, double) noexcept = nullptr;
    // Tile transfer laws. Envelopes are finite in [0,1e100]; outputs are disjoint.
    // expander: (envelope, gains, n, thresholdDb, ratio).
    void (*expanderGain)(const double*, double*, std::size_t, double, double) noexcept = nullptr;
    // dynamic EQ weight: (envelope, weights, n, thresholdDb, staticGainDb, rangeDb).
    void (*dynamicEqGain)(const double*, double*, std::size_t, double, double,
                          double) noexcept = nullptr;
    // transient: (fast, slow, gains, n, {attack, sustain, maximumGainDb}).
    void (*transientGain)(const double*, const double*, double*, std::size_t,
                          const double*) noexcept = nullptr;
    double (*minimum)(const double*, std::size_t) noexcept = nullptr;
    void (*adaaCubic)(const double*, double*, std::size_t, double, bool, double,
                      double) noexcept = nullptr;
    void (*shapeCubic)(const double*, double*, std::size_t, double, double) noexcept = nullptr;
    void (*eqApplyPair)(double*, double*, const double*, const double*, std::size_t, const double*,
                        double*) noexcept = nullptr;
    void (*transientExponent)(const double*, const double*, double*, std::size_t,
                              const double*) noexcept = nullptr;
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
