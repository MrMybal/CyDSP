#pragma once
#include "cydsp/core/AudioBlock.h"
namespace cydsp {
// Frame-major interleaving: sample[frame*channels+channel]. All storage is borrowed.
// The flat span must have exactly frames*channels elements. Overlap is rejected.
[[nodiscard]] Status interleave(AudioBlock<const float>, std::span<float>) noexcept;
[[nodiscard]] Status interleave(AudioBlock<const double>, std::span<double>) noexcept;
[[nodiscard]] Status deinterleave(std::span<const float>, AudioBlock<float>) noexcept;
[[nodiscard]] Status deinterleave(std::span<const double>, AudioBlock<double>) noexcept;
} // namespace cydsp
