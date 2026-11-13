#pragma once
namespace cydsp::detail {
inline double polyBlep(double phase, double step) noexcept {
    if (step <= 0)
        return 0;
    if (phase < step) {
        const double x = phase / step;
        return x + x - x * x - 1;
    }
    if (phase > 1 - step) {
        const double x = (phase - 1) / step;
        return x * x + x + x + 1;
    }
    return 0;
}
} // namespace cydsp::detail
