#pragma once
#include <cstddef>
namespace cydsp::detail {
// Callers validate finite endpoints/differences. Advance once for each audio frame.
constexpr double advanceLinearRamp(double current, double target, std::size_t& remaining) noexcept {
    if (remaining) {
        current += (target - current) / static_cast<double>(remaining);
        if (--remaining == 0)
            current = target;
    }
    return current;
}
} // namespace cydsp::detail
