#include "detail/Scalar.h"
#include "detail/VectorTable.h"
namespace cydsp::detail {
const VectorTable& scalarTable() noexcept {
    static constexpr VectorTable table{
        CpuBackend::scalar,
        {scalarMultiply<float>, scalarComplexMac<float>, scalarButterfly<float>, scalarCopy<float>,
         scalarScale<float>, scalarAddScaled<float>, scalarFill<float>, scalarDot<float>,
         scalarBiquadBank<float>, scalarGainChain<float>, scalarFftStage<float>},
        {scalarMultiply<double>, scalarComplexMac<double>, scalarButterfly<double>,
         scalarCopy<double>, scalarScale<double>, scalarAddScaled<double>, scalarFill<double>,
         scalarDot<double>, scalarBiquadBank<double>, scalarGainChain<double>,
         scalarFftStage<double>},
        scalarDotMixed};
    return table;
}
} // namespace cydsp::detail
