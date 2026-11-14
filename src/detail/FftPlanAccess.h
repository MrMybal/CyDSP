#pragma once
#include "cydsp/processors/RealFft.h"
#include "detail/FftRuntime.h"
namespace cydsp::detail {
// Private immutable plan access; the incremental convolver owns its RealFft workspace.
struct FftPlanAccess {
    // Input already follows this plan's bit-reversed order. Fusing a domain-
    // specific pack/permutation avoids another full buffer traversal in STFT.
    template <Sample T>
    static void transformPermutedUnscaled(const ComplexFft<T>& plan,
                                          std::span<std::complex<T>> data, bool inverse,
                                          std::size_t firstHalf = 1) noexcept {
        const auto weights = std::span<const std::complex<T>>(plan.twiddles_);
        const auto& table = kernels(plan);
        fftRuntimeStages<T>(data, weights, table, inverse, firstHalf);
    }
    // Permutation and the first two stages in one pass when the backend has the
    // fused kernel; returns the first stage still to run (4, or 1 if not taken).
    template <Sample T>
    static std::size_t permuteHead(const ComplexFft<T>& plan, const T* natural, T* permuted,
                                   std::size_t count, bool inverse) noexcept {
        const auto head = kernels(plan).fftHead;
        if (!head || count < 16)
            return 1;
        head(natural, permuted, plan.reverse_.data(),
             reinterpret_cast<const T*>(plan.twiddles_.data()), count, inverse);
        return 4;
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
