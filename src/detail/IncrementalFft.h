#pragma once
#include "detail/RealFftAlgorithm.h"
#include "detail/VectorTable.h"
#include <algorithm>
#include <array>
#include <complex>
#include <span>
namespace cydsp::detail {
inline constexpr std::size_t fftWorkQuantum = 64;
// One bounded group of butterflies in a packed FFT (power-of-two work.size() >= 2).
// Callers retain stage/cursor state and have already prepared all plan/storage data.
template <class T>
bool advanceFftChunk(const Kernels<T>& kernels, std::span<std::complex<T>> work,
                     const std::complex<T>* weights, std::size_t& half, std::size_t& cursor,
                     bool inverse) noexcept {
    // Two adjacent stages may share loads/stores. A job still performs at most
    // 64 butterflies in total, even when the packed transform is very small.
    // The scheduler's original stage count stays a conservative upper bound.
    if (kernels.fftPair && half >= 4 && half <= fftWorkQuantum / 4 && half <= work.size() / 4) {
        const auto elements = std::min(fftWorkQuantum, work.size() - 2 * cursor);
        kernels.fftPair(reinterpret_cast<T*>(work.data() + 2 * cursor),
                        reinterpret_cast<const T*>(weights + half - 1),
                        reinterpret_cast<const T*>(weights + 2 * half - 1), elements, half,
                        inverse);
        cursor += elements / 2;
        if (cursor == work.size() / 2) {
            cursor = 0;
            half *= 4;
        }
        return half == work.size();
    }
    const auto count = std::min(fftWorkQuantum, work.size() / 2 - cursor);
    if (half <= fftWorkQuantum) {
        kernels.fftStage(reinterpret_cast<T*>(work.data() + 2 * cursor),
                         reinterpret_cast<const T*>(weights + half - 1), 2 * count, half, inverse);
    } else {
        const auto within = cursor & (half - 1), base = 2 * (cursor & ~(half - 1));
        kernels.butterfly(reinterpret_cast<T*>(work.data() + base + within),
                          reinterpret_cast<T*>(work.data() + base + within + half),
                          reinterpret_cast<const T*>(weights + half - 1 + within), count, inverse);
    }
    cursor += count;
    if (cursor == work.size() / 2) {
        cursor = 0;
        half *= 2;
    }
    return half == work.size();
}
// Pack a bounded part of a zero-padded real partition. Split at the input
// boundary once, avoiding two per-bin bounds checks inside the scatter loop.
template <class T>
void packRealChunk(const T* input, std::complex<T>* work, const std::size_t* reverse, std::size_t p,
                   std::size_t first, std::size_t end) noexcept {
    const auto middle = std::min(end, p / 2);
    for (auto i = first; i < middle; ++i)
        work[reverse[i]] = {input[2 * i], input[2 * i + 1]};
    if (p == 1 && first == 0) {
        work[0] = {input[0], T{}};
        return;
    }
    for (auto i = std::max(first, middle); i < end; ++i)
        work[reverse[i]] = {};
}
template <class T>
void emitRealChunk(const Kernels<T>& kernels, const std::complex<T>* work, T* overlap, T* ready,
                   std::size_t p, std::size_t first, std::size_t end) noexcept {
    const auto scale = T{1} / static_cast<T>(p);
    if (p == 1) {
        ready[0] = work[0].real() + overlap[0];
        overlap[0] = work[0].imag();
        return;
    }
    const auto* values = reinterpret_cast<const T*>(work);
    if (kernels.convolutionEmit)
        kernels.convolutionEmit(values + first, values + p + first, overlap + first, ready + first,
                                end - first, scale);
    else
        for (auto i = first; i < end; ++i) {
            ready[i] = values[i] * scale + overlap[i];
            overlap[i] = values[p + i] * scale;
        }
}
// Bins [first,end), 1 <= first, of the real-spectrum split of a packed transform.
template <class T>
void splitRealRange(const Kernels<T>& kernels, const std::complex<T>* packed,
                    const std::complex<T>* realWeights, std::complex<T>* spectrum, std::size_t p,
                    std::size_t first, std::size_t end) noexcept {
    if (first >= end)
        return;
    if (kernels.realSplitRange)
        kernels.realSplitRange(reinterpret_cast<const T*>(packed),
                               reinterpret_cast<const T*>(realWeights),
                               reinterpret_cast<T*>(spectrum + first), p, first, end);
    else
        for (auto i = first; i < end; ++i)
            spectrum[i] = splitRealBin<T>({packed, p}, {realWeights, p - 1}, i);
}
// At most fftWorkQuantum bins [first,end), 1 <= first, merged and written to
// their bit-reversed positions of the packed workspace.
template <class T>
void mergeRealRange(const Kernels<T>& kernels, const std::complex<T>* spectrum,
                    const std::complex<T>* realWeights, std::complex<T>* packed,
                    const std::size_t* reverse, std::size_t p, std::size_t first,
                    std::size_t end) noexcept {
    if (first >= end)
        return;
    if (kernels.realMergeRange && end - first <= fftWorkQuantum) {
        std::array<std::complex<T>, fftWorkQuantum> merged;
        kernels.realMergeRange(reinterpret_cast<const T*>(spectrum),
                               reinterpret_cast<const T*>(realWeights),
                               reinterpret_cast<T*>(merged.data()), p, first, end);
        for (auto i = first; i < end; ++i)
            packed[reverse[i]] = merged[i - first];
    } else
        for (auto i = first; i < end; ++i)
            packed[reverse[i]] = mergeRealBin<T>({spectrum, p + 1}, {realWeights, p - 1}, i);
}
} // namespace cydsp::detail
