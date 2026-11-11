#pragma once
#include "cydsp/core/Types.h"
#include "cydsp/math/VectorOps.h"
#include <complex>
#include <span>
#include <utility>
#include <vector>
namespace cydsp {
namespace detail {
struct FftPlanAccess;
}
enum class FftDirection { forward, inverse };
// Radix-2, contiguous per-stage twiddles, prepared SIMD butterfly selection.
// Forward unscaled, inverse divided by N. Exact in-place is supported.
template <Sample T>
    requires(!std::is_const_v<T>)
class ComplexFft {
  public:
    ComplexFft() = default;
    ComplexFft(const ComplexFft&) = delete;
    ComplexFft& operator=(const ComplexFft&) = delete;
    ComplexFft(ComplexFft&& other) noexcept {
        swap(other);
    }
    ComplexFft& operator=(ComplexFft&& other) noexcept {
        if (this != &other) {
            ComplexFft next(std::move(other));
            swap(next);
        }
        return *this;
    }
    [[nodiscard]] Status prepare(std::size_t size, CpuBackend = CpuBackend::automatic) noexcept;
    void reset() noexcept {}
    [[nodiscard]] Status process(std::span<const std::complex<T>>, std::span<std::complex<T>>,
                                 FftDirection) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept {
        return reverse_.size();
    }
    [[nodiscard]] CpuBackend backend() const noexcept {
        return ops_.backend();
    }

  private:
    friend struct detail::FftPlanAccess;
    void swap(ComplexFft& other) noexcept {
        reverse_.swap(other.reverse_);
        twiddles_.swap(other.twiddles_);
        std::swap(ops_, other.ops_);
    }
    std::vector<std::size_t> reverse_;
    std::vector<std::complex<T>> twiddles_;
    VectorOps ops_;
};
extern template class ComplexFft<float>;
extern template class ComplexFft<double>;
} // namespace cydsp
