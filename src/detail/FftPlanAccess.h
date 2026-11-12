#pragma once
#include "cydsp/processors/RealFft.h"
#include "detail/VectorTable.h"
namespace cydsp::detail {
// Private immutable plan access; the incremental convolver owns its RealFft workspace.
struct FftPlanAccess {
    // Input already follows this plan's bit-reversed order. Fusing a domain-
    // specific pack/permutation avoids another full buffer traversal in STFT.
    template <Sample T>
    static void transformPermutedUnscaled(const ComplexFft<T>& plan,
                                          std::span<std::complex<T>> data, bool inverse) noexcept {
        const auto weights = std::span<const std::complex<T>>(plan.twiddles_);
        const auto& table = kernels(plan);
        for (std::size_t half = 1; half < data.size(); half *= 2)
            table.fftStage(reinterpret_cast<T*>(data.data()),
                           reinterpret_cast<const T*>(weights.data() + half - 1), data.size(), half,
                           inverse);
    }
    template <Sample T>
    static std::span<const std::size_t> reverse(const ComplexFft<T>& plan) noexcept {
        return plan.reverse_;
    }
    template <Sample T>
    static std::span<const std::complex<T>> complexTwiddles(const ComplexFft<T>& plan) noexcept {
        return plan.twiddles_;
    }
    template <Sample T> static const Kernels<T>& kernels(const ComplexFft<T>& plan) noexcept {
        return VectorAccess::kernels<T>(plan.ops_);
    }
    template <Sample T>
    static std::span<const std::size_t> reverse(const RealFft<T>& plan) noexcept {
        return plan.fft_.reverse_;
    }
    template <Sample T>
    static std::span<const std::complex<T>> complexTwiddles(const RealFft<T>& plan) noexcept {
        return plan.fft_.twiddles_;
    }
    template <Sample T>
    static std::span<const std::complex<T>> realTwiddles(const RealFft<T>& plan) noexcept {
        return plan.twiddles_;
    }
    template <Sample T> static std::span<std::complex<T>> scratch(RealFft<T>& plan) noexcept {
        return plan.scratch_;
    }
    template <Sample T> static const Kernels<T>& kernels(const RealFft<T>& plan) noexcept {
        return VectorAccess::kernels<T>(plan.fft_.ops_);
    }
};
} // namespace cydsp::detail
