#pragma once
#include <cstddef>
#include <cstdint>
namespace cydsp::detail {
// No product of the potentially large job count and the frame count.
constexpr std::size_t duePartitionJobs(std::size_t jobs, std::size_t frame,
                                       std::size_t partition) noexcept {
    return (jobs / partition) * frame +
           static_cast<std::size_t>((static_cast<std::uint64_t>(jobs % partition) * frame) /
                                    partition);
}
constexpr std::size_t advancePartitionJobs(std::size_t base, std::size_t extra,
                                           std::size_t partition, std::size_t& debt) noexcept {
    debt += extra;
    if (debt >= partition) {
        debt -= partition;
        return base + 1;
    }
    return base;
}
} // namespace cydsp::detail
