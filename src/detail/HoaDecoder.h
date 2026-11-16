#pragma once
#include "cydsp/spatial/SpatialRendering.h"
#include <vector>
namespace cydsp::detail {
Status designHoaFilters(unsigned order, double rate, std::span<const HrirPoint<double>> points,
                        const HoaDecoderDesign& config, std::vector<std::vector<double>>& filters,
                        HoaDecoderReport& report, CpuBackend backend);
}
