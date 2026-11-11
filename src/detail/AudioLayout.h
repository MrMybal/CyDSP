#pragma once
#include "cydsp/core/AudioBlock.h"
namespace cydsp::detail {
template <class T>
constexpr void interleaveUnchecked(AudioBlock<const T> input, std::span<T> output) noexcept {
    for (std::size_t frame = 0; frame < input.frames(); ++frame)
        for (std::size_t ch = 0; ch < input.channels(); ++ch)
            output[frame * input.channels() + ch] = input.channel(ch)[frame];
}
template <class T>
constexpr void deinterleaveUnchecked(std::span<const T> input, AudioBlock<T> output) noexcept {
    for (std::size_t ch = 0; ch < output.channels(); ++ch)
        for (std::size_t frame = 0; frame < output.frames(); ++frame)
            output.channel(ch)[frame] = input[frame * output.channels() + ch];
}
} // namespace cydsp::detail
