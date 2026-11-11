#pragma once
#include <cstddef>
#include <limits>
namespace cydsp::detail {
constexpr bool checkedMultiply(std::size_t a, std::size_t b, std::size_t& result) noexcept {
    if (b != 0 && a > std::numeric_limits<std::size_t>::max() / b)
        return false;
    result = a * b;
    return true;
}
constexpr bool checkedAdd(std::size_t a, std::size_t b, std::size_t& result) noexcept {
    if (a > std::numeric_limits<std::size_t>::max() - b)
        return false;
    result = a + b;
    return true;
}
} // namespace cydsp::detail
