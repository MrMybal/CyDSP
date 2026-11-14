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
