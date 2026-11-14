#pragma once
#include "SaturationSimd.h"
#include "detail/CompressorCurve.h"
#include <cmath>
#include <limits>
// Original width-generic gain-curve kernels for dynamics processors. Envelope
// recurrences stay scalar in the processors; these kernels evaluate the
// memoryless transfer laws of whole tiles. Double arithmetic, no FMA, vector
// libm or floating-point state change.
namespace cydsp::detail {
namespace {
// log(x) for positive normal x. Relative error near 2e-16.
template <class V> CYDSP_SIMD_INLINE typename V::Reg simdLog(typename V::Reg x) noexcept {
    const auto one = V::set(1);
    typename V::Reg mantissa, exponent;
    V::split(x, mantissa, exponent);
    const auto high = V::gt(mantissa, V::set(std::numbers::sqrt2));
    mantissa = V::select(high, V::mul(mantissa, V::set(.5)), mantissa);
    exponent = V::add(exponent, V::select(high, one, V::set(0)));
    const auto z = V::div(V::sub(mantissa, one), V::add(mantissa, one));
    const auto series = simdPolynomial<V>(simdAtanhRatio, V::mul(z, z));
    return V::add(V::mul(exponent, V::set(std::numbers::ln2)),
                  V::add(V::mul(V::mul(V::set(2), z), series),
                         V::mul(exponent, V::set(2.3190468138462996e-17))));
}
template <class V>
CYDSP_SIMD_INLINE typename V::Reg simdClamp(typename V::Reg x, double low, double high) noexcept {
    return V::min(V::max(x, V::set(low)), V::set(high));
}
// out[i] = f(a[i], b[i]) over n values. Whole groups read the caller's arrays;
// the last partial group is padded by repeating its final element.
template <class V, class F>
CYDSP_SIMD_INLINE void simdMap2(const double* a, const double* b, double* out, std::size_t n,
                                F&& f) noexcept {
    alignas(64) double first[V::maxWidth], second[V::maxWidth], result[V::maxWidth];
    for (std::size_t i = 0; i < n; i += V::lanes()) {
        const bool whole = n - i >= V::lanes();
        if (!whole) {
            const auto count = n - i;
            for (std::size_t j = 0; j < V::lanes(); ++j) {
                first[j] = a[i + std::min(j, count - 1)];
                second[j] = b[i + std::min(j, count - 1)];
            }
        }
        const auto value = f(V::load(whole ? a + i : first), V::load(whole ? b + i : second));
        if (whole)
            V::store(out + i, value);
        else {
            V::store(result, value);
            std::copy_n(result, n - i, out + i);
        }
    }
}
// 20*log10(max(1e-12, envelope)); envelope is finite in [0,1e100].
template <class V>
CYDSP_SIMD_INLINE void simdLevelDb(const double* envelope, double* decibels,
                                   std::size_t n) noexcept {
    simdMap2<V>(envelope, envelope, decibels, n, [](typename V::Reg e, typename V::Reg) noexcept {
        return V::mul(simdLog<V>(V::max(e, V::set(1e-12))), V::set(20 / std::numbers::ln10));
    });
}
// The logarithm and the exponential are separate passes over a tile: each is a
// short dependency chain, so consecutive vector groups overlap.
template <class V, class Curve>
CYDSP_SIMD_INLINE void simdLevelCurve(const double* envelope, double* gains, std::size_t n,
                                      Curve&& curve) noexcept {
    alignas(64) double decibels[simdTile];
    for (std::size_t offset = 0; offset < n;) {
        const auto count = std::min(simdTile, n - offset);
        simdLevelDb<V>(envelope + offset, decibels, count);
        simdMap2<V>(decibels, decibels, gains + offset, count, curve);
        offset += count;
    }
}
// Downward expander: gain = 10^(max(-160, (ratio-1)*over)/20) below threshold.
template <class V>
CYDSP_SIMD_INLINE void simdExpanderGain(const double* envelope, double* gains, std::size_t n,
                                        double thresholdDb, double ratio) noexcept {
    simdLevelCurve<V>(
        envelope, gains, n, [thresholdDb, ratio](typename V::Reg level, typename V::Reg) noexcept {
            const auto over = V::sub(level, V::set(thresholdDb));
            const auto decibels =
                V::select(V::lt(over, V::set(0)),
                          V::max(V::set(-160), V::mul(V::set(ratio - 1), over)), V::set(0));
            return simdExp<V>(V::mul(decibels, V::set(std::numbers::ln10 / 20)));
        });
}
// Dynamic EQ band weight: 10^((static + range*clamp(over/24,0,1))/20) - 1.
template <class V>
CYDSP_SIMD_INLINE void simdDynamicEqGain(const double* envelope, double* gains, std::size_t n,
                                         double thresholdDb, double staticGainDb,
                                         double rangeDb) noexcept {
    simdLevelCurve<V>(
        envelope, gains, n,
        [thresholdDb, staticGainDb, rangeDb](typename V::Reg level, typename V::Reg) noexcept {
            const auto amount =
                simdClamp<V>(V::div(V::sub(level, V::set(thresholdDb)), V::set(24)), 0, 1);
            const auto decibels = V::add(V::set(staticGainDb), V::mul(V::set(rangeDb), amount));
            return V::sub(simdExp<V>(V::mul(decibels, V::set(std::numbers::ln10 / 20))), V::set(1));
        });
}
// Transient shaper gain from its fast/slow envelopes.
// parameters = {attack, sustain, maximumGainDb}.
template <class V>
CYDSP_SIMD_INLINE void simdTransientGain(const double* fast, const double* slow, double* gains,
                                         std::size_t n, const double* parameters) noexcept {
    const double attackAmount = parameters[0], sustainAmount = parameters[1],
                 maximumDb = parameters[2];
    simdMap2<V>(
        fast, slow, gains, n,
        [attackAmount, sustainAmount, maximumDb](typename V::Reg f, typename V::Reg s) noexcept {
            const auto floor = V::max(V::set(1e-12), f);
            const auto attack = simdClamp<V>(V::div(V::sub(f, s), floor), 0, 1);
            const auto sustain =
                V::mul(simdClamp<V>(V::div(s, floor), 0, 1), V::sub(V::set(1), attack));
            const auto decibels = V::mul(
                V::set(maximumDb), simdClamp<V>(V::add(V::mul(V::set(attackAmount), attack),
                                                       V::mul(V::set(sustainAmount), sustain)),
                                                -1, 1));
            return simdExp<V>(V::mul(decibels, V::set(std::numbers::ln10 / 20)));
        });
}
// Compressor/limiter transfer curve. Envelope is finite in [0,1e100], output is
// disjoint; gain is always normal. Groups entirely below the knee are unity and
// groups entirely above it use square roots for the common ratios; the others
// take the decibel curve as a logarithm pass followed by an exponential pass.
template <class V>
CYDSP_SIMD_INLINE void simdCompressorGain(const double* envelope, double* gains, std::size_t n,
                                          const CompressorCurve& curve) noexcept {
    const auto w = V::lanes();
    const double half = curve.kneeDb * .5;
    const bool rooted = curve.limiter || curve.ratio == 2 || curve.ratio == 4;
    const auto below = V::set(curve.lower * (1 - 1e-12)), above = V::set(curve.upper * (1 + 1e-12));
    alignas(64) double level[simdSlots<V>], padded[V::maxWidth], result[V::maxWidth];
    bool general[simdTile / 2];
    for (std::size_t offset = 0; offset < n;) {
        const auto count = std::min(simdTile, n - offset);
        const auto emit = [&](std::size_t i, typename V::Reg value) noexcept {
            if (count - i >= w)
                V::store(gains + offset + i, value);
            else {
                V::store(result, value);
                std::copy_n(result, count - i, gains + offset + i);
            }
        };
        for (std::size_t i = 0; i < count; i += w) {
            const double* source = envelope + offset + i;
            if (count - i < w) {
                std::fill(std::copy_n(source, count - i, padded), padded + w,
                          source[count - i - 1]);
                source = padded;
            }
            const auto env = V::max(V::load(source), V::set(1e-12));
            general[i / w] = false;
            if (V::all(V::lt(env, below)))
                emit(i, V::set(1));
            else if (rooted && V::all(V::gt(env, above))) {
                auto value = V::div(V::set(curve.threshold), env);
                if (!curve.limiter) {
                    value = V::sqrt(value);
                    if (curve.ratio == 4)
                        value = V::mul(value, V::sqrt(value));
                }
                emit(i, value);
            } else {
                general[i / w] = true;
                V::store(level + i, V::mul(simdLog<V>(env), V::set(20 / std::numbers::ln10)));
            }
        }
        for (std::size_t i = 0; i < count; i += w) {
            if (!general[i / w])
                continue;
            const auto over = V::sub(V::load(level + i), V::set(curve.thresholdDb));
            auto decibels = V::mul(V::max(over, V::set(0)), V::set(curve.slope));
            if (curve.kneeDb > 0) {
                const auto t = V::max(V::add(over, V::set(half)), V::set(0));
                decibels =
                    V::select(V::le(V::set(half), over), decibels,
                              V::mul(V::mul(t, t), V::set(curve.slope / (2 * curve.kneeDb))));
            }
            emit(i, simdExp<V>(V::mul(decibels, V::set(std::numbers::ln10 / 20))));
        }
        offset += count;
    }
}
template <class V> CYDSP_SIMD_INLINE double simdMinimum(const double* x, std::size_t n) noexcept {
    double result = std::numeric_limits<double>::infinity();
    std::size_t i = 0;
    if (n >= V::lanes()) {
        auto low = V::load(x);
        for (i = V::lanes(); n - i >= V::lanes(); i += V::lanes())
            low = V::min(low, V::load(x + i));
        alignas(64) double lanes[V::maxWidth];
        V::store(lanes, low);
        for (std::size_t j = 0; j < V::lanes(); ++j)
            result = std::min(result, lanes[j]);
    }
    for (; i < n; ++i)
        result = std::min(result, x[i]);
    return result;
}
// out = |in| as double, or max(out, |in|) when accumulating across channels.
template <class V, class T>
CYDSP_SIMD_INLINE void simdMagnitude(const T* in, double* out, std::size_t n,
                                     bool accumulate) noexcept {
    std::size_t i = 0;
    for (; n - i >= V::lanes(); i += V::lanes()) {
        auto value = V::abs(V::fromSamples(in + i));
        if (accumulate)
            value = V::max(V::load(out + i), value);
        V::store(out + i, value);
    }
    for (; i < n; ++i) {
        const double value = std::abs(double(in[i]));
        out[i] = accumulate ? std::max(out[i], value) : value;
    }
}
// value = clamp(in*gain*scale, -ceiling, ceiling); out = T(value), or the
// dry/wet mix dryGain*dry + wetGain*T(value). in/dry may alias out exactly.
template <class V, class T>
CYDSP_SIMD_INLINE void simdApplyGains(const T* in, const double* gains, T* out, std::size_t n,
                                      double scale, double ceiling, const T* dry, double dryGain,
                                      double wetGain, bool mixed) noexcept {
    const auto upper = V::set(ceiling), lower = V::set(-ceiling), makeup = V::set(scale);
    std::size_t i = 0;
    for (; n - i >= V::lanes(); i += V::lanes()) {
        auto value = V::min(
            V::max(V::mul(V::mul(V::fromSamples(in + i), V::load(gains + i)), makeup), lower),
            upper);
        if (mixed)
            value = V::add(V::mul(V::set(dryGain), V::fromSamples(dry + i)),
                           V::mul(V::set(wetGain), V::rounded(value, out)));
        V::toSamples(out + i, value);
    }
    for (; i < n; ++i) {
        const double value = std::clamp(double(in[i]) * gains[i] * scale, -ceiling, ceiling);
        out[i] = mixed ? T(dryGain * double(dry[i]) + wetGain * double(T(value))) : T(value);
    }
}
} // namespace
} // namespace cydsp::detail
