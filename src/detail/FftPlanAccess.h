#pragma once
#include "cydsp/processors/RealFft.h"
#include "detail/VectorTable.h"
namespace cydsp::detail {
// Private immutable plan access; the incremental convolver owns its RealFft workspace.
struct FftPlanAccess {
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
