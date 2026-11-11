#pragma once
#include "cydsp/core/Types.h"
#include <cstdint>
namespace cydsp::detail {
struct OrderedAliasResult {
    bool handled = false;
    Status status = Status::ok;
};
constexpr bool equalLengthRangesOverlap(std::uintptr_t a, std::uintptr_t b,
                                        std::size_t bytes) noexcept {
    return bytes && (a <= b ? b - a < bytes : a - b < bytes);
}
// Linear sweep for ordered, internally disjoint inputs/outputs. Arbitrary layouts
// fall back to the complete validator. Getter parameters keep the actual policy constexpr.
template <class InputAddress, class OutputAddress>
constexpr OrderedAliasResult validateOrderedAlias(std::size_t channels, std::size_t bytes,
                                                  InputAddress input,
                                                  OutputAddress output) noexcept {
    if (!bytes || !channels)
        return {true, Status::ok};
    auto previousInput = input(0), previousOutput = output(0);
    for (std::size_t ch = 1; ch < channels; ++ch) {
        const auto a = input(ch), b = output(ch);
        if (a < previousInput || a - previousInput < bytes || b < previousOutput ||
            b - previousOutput < bytes)
            return {};
        previousInput = a;
        previousOutput = b;
    }
    std::size_t in = 0, out = 0;
    while (in < channels && out < channels) {
        const auto a = input(in), b = output(out);
        if (equalLengthRangesOverlap(a, b, bytes)) {
            if (in != out || a != b)
                return {true, Status::partial_overlap};
            ++in;
            ++out;
        } else if (a < b)
            ++in;
        else
            ++out;
    }
    return {true, Status::ok};
}
} // namespace cydsp::detail
