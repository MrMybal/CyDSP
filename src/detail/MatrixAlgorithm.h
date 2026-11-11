#pragma once
#include "cydsp/core/AudioBlock.h"
#include "detail/LinearRamp.h"
#include "detail/OrderedAlias.h"
#include <span>
namespace cydsp::detail {
template <Sample T> constexpr bool validMatrixCoefficient(double value) noexcept {
    const auto limit = static_cast<double>(std::numeric_limits<T>::max());
    return isFinite(value) && value >= -limit && value <= limit;
}
// Shapes and finite differences are validated before calling this helper.
constexpr void advanceMatrixRamp(std::span<double> current, std::span<const double> target,
                                 std::size_t& remaining) noexcept {
    if (!remaining)
        return;
    for (std::size_t i = 0; i < current.size(); ++i) {
        auto steps = remaining;
        current[i] = advanceLinearRamp(current[i], target[i], steps);
    }
    --remaining;
}
template <Sample T>
constexpr void matrixFrame(AudioBlock<const T> input, AudioBlock<T> output,
                           std::span<const double> coefficients, std::size_t frame) noexcept {
    for (std::size_t out = 0; out < output.channels(); ++out) {
        T sum{};
        for (std::size_t in = 0; in < input.channels(); ++in) {
            const auto gain = static_cast<T>(coefficients[out * input.channels() + in]);
            if (gain != T{})
                sum += input.channel(in)[frame] * gain;
        }
        output.channel(out)[frame] = sum;
    }
}
// Exact input/output aliases are safe only because every tile snapshots ALL inputs.
template <class InputAddress, class OutputAddress>
constexpr Status validateMatrixAlias(std::size_t inputs, std::size_t outputs, std::size_t bytes,
                                     InputAddress input, OutputAddress output) noexcept {
    if (!bytes)
        return Status::ok;
    for (std::size_t out = 0; out < outputs; ++out) {
        const auto b = output(out);
        for (std::size_t other = 0; other < out; ++other)
            if (equalLengthRangesOverlap(b, output(other), bytes))
                return Status::partial_overlap;
        for (std::size_t in = 0; in < inputs; ++in) {
            const auto a = input(in);
            if (a != b && equalLengthRangesOverlap(a, b, bytes))
                return Status::partial_overlap;
        }
    }
    return Status::ok;
}
} // namespace cydsp::detail
