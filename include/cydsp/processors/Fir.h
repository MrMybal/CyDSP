#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/math/VectorOps.h"
#include <utility>
#include <vector>
namespace cydsp {
// Causal direct FIR, h[0] multiplies the current sample. No added block buffering.
// Up to 65536 coefficients, shared by independently stateful channels.
template <Sample T>
    requires(!std::is_const_v<T>)
class Fir {
  public:
    Fir() = default;
    Fir(const Fir&) = delete;
    Fir& operator=(const Fir&) = delete;
    Fir(Fir&& other) noexcept {
        swap(other);
    }
    Fir& operator=(Fir&& other) noexcept {
        if (this != &other) {
            Fir next(std::move(other));
            swap(next);
        }
        return *this;
    }
    [[nodiscard]] Status prepare(const ProcessSpec&, std::span<const T> coefficients,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    void reset() noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    [[nodiscard]] std::size_t taps() const noexcept {
        return coefficients_.size();
    }
    [[nodiscard]] CpuBackend backend() const noexcept {
        return ops_.backend();
    }

  private:
    void swap(Fir&) noexcept;
    ProcessSpec spec_{};
    VectorOps ops_;
    std::vector<T> coefficients_, history_;
    std::size_t write_ = 0;
};
extern template class Fir<float>;
extern template class Fir<double>;
} // namespace cydsp
