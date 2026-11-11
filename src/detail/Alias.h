#pragma once
#include <cstddef>
#include <cstdint>
namespace cydsp::detail {
inline bool overlaps(const void* a, std::size_t bytesA, const void* b,
                     std::size_t bytesB) noexcept {
    if (!bytesA || !bytesB)
        return false;
    const auto x = reinterpret_cast<std::uintptr_t>(a), y = reinterpret_cast<std::uintptr_t>(b);
    return x <= y ? y - x < bytesA : x - y < bytesB;
}
} // namespace cydsp::detail
