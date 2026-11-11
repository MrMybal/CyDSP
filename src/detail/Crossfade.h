#pragma once
#include "cydsp/core/AudioBlock.h"
#include "detail/LinearRamp.h"
namespace cydsp::detail {
// The caller supplies a mix in [0,1]. Endpoints avoid an unnecessary 0*signal.
template <class T> constexpr T linearCrossfade(T from, T to, double mix) noexcept {
    const auto alpha = static_cast<T>(mix);
    if (alpha == T{0})
        return from;
    if (alpha == T{1})
        return to;
    return from * (T{1} - alpha) + to * alpha;
}
// Matching disjoint scratch views, output shape, and frames <= remaining are prevalidated.
template <Sample T>
constexpr void crossfadeBlocks(AudioBlock<const T> from, AudioBlock<const T> to,
                               AudioBlock<T> output, double& blend,
                               std::size_t& remaining) noexcept {
    for (std::size_t frame = 0; frame < output.frames(); ++frame) {
        blend = advanceLinearRamp(blend, 1, remaining);
        for (std::size_t ch = 0; ch < output.channels(); ++ch)
            output.channel(ch)[frame] =
                linearCrossfade(from.channel(ch)[frame], to.channel(ch)[frame], blend);
    }
}
} // namespace cydsp::detail
