#pragma once
#include "detail/VectorTable.h"
#include <algorithm>
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
} // namespace cydsp::detail
