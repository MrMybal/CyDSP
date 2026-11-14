#pragma once
#include "detail/VectorTable.h"
#include <cmath>
namespace cydsp::detail {
inline CompressorCurve makeCompressorCurve(double threshold, double ratio, double knee,
                                           bool limiter) noexcept {
    return {threshold,
            ratio,
            knee,
            std::pow(10., (threshold - knee * .5) / 20),
            std::pow(10., (threshold + knee * .5) / 20),
            std::pow(10., threshold / 20),
            limiter ? -1 : 1 / ratio - 1,
            limiter};
}
} // namespace cydsp::detail
