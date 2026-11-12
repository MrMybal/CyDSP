#include "cydsp/core/AudioBlock.h"
#include "detail/Alias.h"
#include "detail/OrderedAlias.h"
namespace cydsp {
template <Sample T>
Status validateDisjointBlocks(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!in.isValid() || !out.isValid() ||
        in.frames() > std::numeric_limits<std::size_t>::max() / sizeof(T) ||
        out.frames() > std::numeric_limits<std::size_t>::max() / sizeof(T))
        return Status::invalid_argument;
    for (std::size_t a = 0; a < out.channels(); ++a) {
        for (std::size_t b = 0; b < a; ++b)
            if (detail::overlaps(out.channel(a).data(), out.frames() * sizeof(T),
                                 out.channel(b).data(), out.frames() * sizeof(T)))
                return Status::partial_overlap;
        for (std::size_t b = 0; b < in.channels(); ++b)
            if (detail::overlaps(out.channel(a).data(), out.frames() * sizeof(T),
                                 in.channel(b).data(), in.frames() * sizeof(T)))
                return Status::partial_overlap;
    }
    return Status::ok;
}
template Status validateDisjointBlocks<float>(AudioBlock<const float>, AudioBlock<float>) noexcept;
template Status validateDisjointBlocks<double>(AudioBlock<const double>,
                                               AudioBlock<double>) noexcept;

template <Sample T>
    requires(!std::is_const_v<T>)
Status validateProcessBlocks(const ProcessSpec& spec, AudioBlock<const T> input,
                             AudioBlock<T> output) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    if (!input.isValid() || !output.isValid())
        return Status::invalid_argument;
    if (input.channels() != spec.channels || output.channels() != spec.channels)
        return Status::channel_mismatch;
    if (input.frames() != output.frames())
        return Status::invalid_argument;
    if (input.frames() > spec.maxBlockFrames)
        return Status::block_too_large;
    if (input.frames() > std::numeric_limits<std::size_t>::max() / sizeof(T))
        return Status::invalid_argument;
    const auto bytes = input.frames() * sizeof(T);
    const auto ordered = detail::validateOrderedAlias(
        spec.channels, bytes,
        [input](std::size_t ch) noexcept {
            return reinterpret_cast<std::uintptr_t>(input.channel(ch).data());
        },
        [output](std::size_t ch) noexcept {
            return reinterpret_cast<std::uintptr_t>(output.channel(ch).data());
        });
    if (ordered.handled)
        return ordered.status;

    for (std::size_t out = 0; out < output.channels(); ++out) {
        for (std::size_t other = 0; other < out; ++other)
            if (detail::overlaps(output.channel(out).data(), bytes, output.channel(other).data(),
                                 bytes))
                return Status::partial_overlap;
        for (std::size_t in = 0; in < input.channels(); ++in) {
            const auto* a = input.channel(in).data();
            const auto* b = output.channel(out).data();
            if (in == out && a == b)
                continue;
            if (detail::overlaps(a, bytes, b, bytes))
                return Status::partial_overlap;
        }
    }
    return Status::ok;
}
template Status validateProcessBlocks<float>(const ProcessSpec&, AudioBlock<const float>,
                                             AudioBlock<float>) noexcept;
template Status validateProcessBlocks<double>(const ProcessSpec&, AudioBlock<const double>,
                                              AudioBlock<double>) noexcept;
} // namespace cydsp
