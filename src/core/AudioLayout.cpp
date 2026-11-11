#include "cydsp/core/AudioLayout.h"
#include "detail/Alias.h"
#include "detail/AudioLayout.h"
#include "detail/CheckedSize.h"
namespace cydsp {
namespace {
template <class T> Status validateLayout(AudioBlock<T> planar, std::size_t flatSize) noexcept {
    std::size_t count{};
    if (!planar.isValid() || (!planar.channels() && planar.frames()) ||
        !detail::checkedMultiply(planar.channels(), planar.frames(), count) || count != flatSize ||
        count > std::numeric_limits<std::size_t>::max() / sizeof(T))
        return Status::invalid_argument;
    return Status::ok;
}
template <class T> Status pack(AudioBlock<const T> input, std::span<T> output) noexcept {
    const auto status = validateLayout(input, output.size());
    if (status != Status::ok)
        return status;
    for (std::size_t ch = 0; ch < input.channels(); ++ch)
        if (detail::overlaps(input.channel(ch).data(), input.frames() * sizeof(T), output.data(),
                             output.size_bytes()))
            return Status::partial_overlap;
    detail::interleaveUnchecked<T>(input, output);
    return Status::ok;
}
template <class T> Status unpack(std::span<const T> input, AudioBlock<T> output) noexcept {
    const auto status = validateLayout(output, input.size());
    if (status != Status::ok)
        return status;
    for (std::size_t ch = 0; ch < output.channels(); ++ch) {
        if (detail::overlaps(input.data(), input.size_bytes(), output.channel(ch).data(),
                             output.frames() * sizeof(T)))
            return Status::partial_overlap;
        for (std::size_t other = 0; other < ch; ++other)
            if (detail::overlaps(output.channel(ch).data(), output.frames() * sizeof(T),
                                 output.channel(other).data(), output.frames() * sizeof(T)))
                return Status::partial_overlap;
    }
    detail::deinterleaveUnchecked<T>(input, output);
    return Status::ok;
}
} // namespace
Status interleave(AudioBlock<const float> in, std::span<float> out) noexcept {
    return pack(in, out);
}
Status interleave(AudioBlock<const double> in, std::span<double> out) noexcept {
    return pack(in, out);
}
Status deinterleave(std::span<const float> in, AudioBlock<float> out) noexcept {
    return unpack(in, out);
}
Status deinterleave(std::span<const double> in, AudioBlock<double> out) noexcept {
    return unpack(in, out);
}
} // namespace cydsp
