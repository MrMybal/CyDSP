#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/math/VectorOps.h"
#include <array>
namespace cydsp {
enum class PolynomialInterpolation { lagrange, catmullRom };
// Random-access centered interpolation. Caller supplies source guard samples:
// floor(position)>=order/2 and floor(last)+order-order/2 < source.frames().
template <Sample T> class PolynomialInterpolator {
  public:
    Status prepare(const ProcessSpec&, double inputRate,
                   PolynomialInterpolation = PolynomialInterpolation::lagrange, unsigned order = 3,
                   CpuBackend = CpuBackend::automatic) noexcept;
    Status process(AudioBlock<const T> source, double firstFrame, AudioBlock<T> output) noexcept;

  private:
    ProcessSpec spec_{};
    VectorOps ops_;
    double ratio_ = 1;
    unsigned order_ = 3;
    PolynomialInterpolation type_ = PolynomialInterpolation::lagrange;
    bool ready_ = false;
};
extern template class PolynomialInterpolator<float>;
extern template class PolynomialInterpolator<double>;
} // namespace cydsp
