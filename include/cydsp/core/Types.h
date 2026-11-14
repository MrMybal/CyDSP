#pragma once
#include <concepts>
#include <cstddef>
#include <limits>
#include <type_traits>
namespace cydsp {
template <class T>
concept Sample =
    std::same_as<std::remove_const_t<T>, float> || std::same_as<std::remove_const_t<T>, double>;
template <Sample T> constexpr bool isFinite(T value) noexcept {
    return value == value && value >= -std::numeric_limits<T>::max() &&
           value <= std::numeric_limits<T>::max();
}
// Per-processor arithmetic policy; precise retains its reference algorithm.
// Fast approximations require explicit opt-in and never change global FP flags.
enum class MathAccuracy { precise, fast };

enum class Status {
    ok,
    invalid_spec,
    invalid_argument,
    not_prepared,
    channel_mismatch,
    block_too_large,
    unsupported_backend,
    partial_overlap,
    allocation_failed,
    transition_in_progress,
    end_of_stream,
    update_queue_full,
    io_error,
    unsupported_format,
    device_unavailable,
    would_block,
    timed_out,
    ambiguous,
    incompatible_version,
    disconnected
};
struct ProcessSpec {
    double sampleRate = 48000.0;
    std::size_t maxBlockFrames = 512, channels = 2;
    [[nodiscard]] constexpr bool isValid() const noexcept {
        return isFinite(sampleRate) && sampleRate > 0 && maxBlockFrames > 0 && channels > 0;
    }
};
[[nodiscard]] constexpr bool isPowerOfTwo(std::size_t value) noexcept {
    return value && (value & (value - 1)) == 0;
}
} // namespace cydsp
