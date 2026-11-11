#pragma once
#include "cydsp/core/Cpu.h"
#include "cydsp/core/Types.h"
#include <complex>
#include <span>
namespace cydsp {
namespace detail {
struct VectorTable;
struct VectorAccess;
} // namespace detail
class VectorOps {
  public:
    VectorOps() noexcept;
    // Reduction order depends on the selected backend; result is unchanged on error.
    [[nodiscard]] Status dot(std::span<const float>, std::span<const float>,
                             float& result) const noexcept;
    [[nodiscard]] Status dot(std::span<const double>, std::span<const double>,
                             double& result) const noexcept;
    // Float input with double coefficients; conversion precedes multiplication.
    [[nodiscard]] Status dot(std::span<const float>, std::span<const double>,
                             double& result) const noexcept;
    [[nodiscard]] Status multiply(std::span<const float>, std::span<const float>,
                                  std::span<float>) const noexcept;
    [[nodiscard]] Status multiply(std::span<const double>, std::span<const double>,
                                  std::span<double>) const noexcept;
    [[nodiscard]] Status complexMultiplyAccumulate(std::span<const std::complex<float>>,
                                                   std::span<const std::complex<float>>,
                                                   std::span<std::complex<float>>) const noexcept;
    [[nodiscard]] Status complexMultiplyAccumulate(std::span<const std::complex<double>>,
                                                   std::span<const std::complex<double>>,
                                                   std::span<std::complex<double>>) const noexcept;
    [[nodiscard]] Status complexButterfly(std::span<std::complex<float>>,
                                          std::span<std::complex<float>>,
                                          std::span<const std::complex<float>>,
                                          bool conjugateTwiddles = false) const noexcept;
    [[nodiscard]] Status complexButterfly(std::span<std::complex<double>>,
                                          std::span<std::complex<double>>,
                                          std::span<const std::complex<double>>,
                                          bool conjugateTwiddles = false) const noexcept;

    [[nodiscard]] static Status create(CpuBackend requested, VectorOps& result) noexcept;
    [[nodiscard]] CpuBackend backend() const noexcept;
    [[nodiscard]] Status copy(std::span<const float>, std::span<float>) const noexcept;
    [[nodiscard]] Status copy(std::span<const double>, std::span<double>) const noexcept;
    [[nodiscard]] Status scale(std::span<const float>, std::span<float>, float) const noexcept;
    [[nodiscard]] Status scale(std::span<const double>, std::span<double>, double) const noexcept;
    [[nodiscard]] Status addScaled(std::span<const float>, std::span<float>, float) const noexcept;
    [[nodiscard]] Status addScaled(std::span<const double>, std::span<double>,
                                   double) const noexcept;
    void fill(std::span<float>, float) const noexcept;
    void fill(std::span<double>, double) const noexcept;

  private:
    friend struct detail::VectorAccess;
    const detail::VectorTable* table_;
};
} // namespace cydsp
