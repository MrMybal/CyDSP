#pragma once
#include "cydsp/processors/ComplexFft.h"
#include <utility>
namespace cydsp {
// N real samples <-> N/2+1 complex bins (DC through Nyquist).
// N>=2 uses a half-size packed complex FFT and reusable workspace.
// Concurrent use of one instance is unsupported.
template <Sample T>
    requires(!std::is_const_v<T>)
class RealFft {
  public:
    RealFft() = default;
    RealFft(const RealFft&) = delete;
    RealFft& operator=(const RealFft&) = delete;
    RealFft(RealFft&& other) noexcept {
        swap(other);
    }
    RealFft& operator=(RealFft&& other) noexcept {
        if (this != &other) {
            RealFft next(std::move(other));
            swap(next);
        }
        return *this;
    }
    [[nodiscard]] Status prepare(std::size_t size, CpuBackend = CpuBackend::automatic) noexcept;
    void reset() noexcept {}
    [[nodiscard]] Status forward(std::span<const T>, std::span<std::complex<T>>) noexcept;
    [[nodiscard]] Status inverse(std::span<const std::complex<T>>, std::span<T>) noexcept;
    [[nodiscard]] std::size_t size() const noexcept {
        return size_;
    }
    [[nodiscard]] std::size_t bins() const noexcept {
        return size() ? size() / 2 + 1 : 0;
    }
    [[nodiscard]] CpuBackend backend() const noexcept {
        return fft_.backend();
    }

  private:
    friend struct detail::FftPlanAccess;
    void swap(RealFft& other) noexcept {
        std::swap(fft_, other.fft_);
        scratch_.swap(other.scratch_);
        natural_.swap(other.natural_);
        twiddles_.swap(other.twiddles_);
        std::swap(size_, other.size_);
    }
    ComplexFft<T> fft_;
    std::vector<std::complex<T>> scratch_, natural_, twiddles_;
    std::size_t size_ = 0;
};
extern template class RealFft<float>;
extern template class RealFft<double>;
} // namespace cydsp
