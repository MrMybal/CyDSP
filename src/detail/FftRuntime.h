#pragma once
#include "detail/VectorTable.h"
#include <complex>
#include <span>
namespace cydsp::detail {
template <class T>
void fftRuntimeStages(std::span<std::complex<T>> data, std::span<const std::complex<T>> weights,
                      const Kernels<T>& table, bool inverse,
                      std::size_t firstHalf = 1) noexcept {
    for (std::size_t half = firstHalf; half < data.size();) {
        if (table.fftPair && half >= 4 && half <= data.size() / 4) {
            table.fftPair(reinterpret_cast<T*>(data.data()),
                          reinterpret_cast<const T*>(weights.data() + half - 1),
                          reinterpret_cast<const T*>(weights.data() + 2 * half - 1), data.size(),
                          half, inverse);
            half *= 4;
        } else {
            table.fftStage(reinterpret_cast<T*>(data.data()),
                           reinterpret_cast<const T*>(weights.data() + half - 1), data.size(), half,
                           inverse);
            half *= 2;
        }
    }
}
} // namespace cydsp::detail
