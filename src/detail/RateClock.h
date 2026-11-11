#pragma once
#include <cstddef>
#include <cstdint>
namespace cydsp::detail {
// Phase remainder never exceeds outputRate-1; no unbounded absolute sample index.
struct RateClock {
    std::uint64_t phase = 0;
    std::size_t pending = 1;
    constexpr void advance(std::uint32_t inputRate, std::uint32_t outputRate) noexcept {
        const auto next = phase + inputRate;
        pending = static_cast<std::size_t>(next / outputRate);
        phase = next % outputRate;
    }
};
constexpr std::size_t convolutionHistoryIndex(std::size_t head, std::size_t part,
                                              std::size_t count) noexcept {
    return head >= part ? head - part : count - (part - head);
}
} // namespace cydsp::detail
