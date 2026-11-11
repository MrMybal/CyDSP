#include "cydsp/processors/ChannelMatrix.h"
#include "detail/MatrixAlgorithm.h"
#include <algorithm>
namespace cydsp {
template <Sample T>
    requires(!std::is_const_v<T>)
void ChannelMatrix<T>::swap(ChannelMatrix& other) noexcept {
    std::swap(spec_, other.spec_);
    inputScratch_.swap(other.inputScratch_);
    current_.swap(other.current_);
    target_.swap(other.target_);
    std::swap(ops_, other.ops_);
    std::swap(outputs_, other.outputs_);
    std::swap(tileFrames_, other.tileFrames_);
    std::swap(remaining_, other.remaining_);
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status ChannelMatrix<T>::prepare(const ProcessSpec& spec, std::size_t outputs,
                                 std::span<const double> coefficients,
                                 CpuBackend backend) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    if (!outputs || outputs > std::numeric_limits<std::size_t>::max() / spec.channels ||
        coefficients.size() != outputs * spec.channels)
        return Status::invalid_argument;
    for (const auto value : coefficients)
        if (!detail::validMatrixCoefficient<T>(value))
            return Status::invalid_argument;
    ChannelMatrix next;
    auto status = VectorOps::create(backend, next.ops_);
    if (status != Status::ok)
        return status;
    const auto tile = std::min(spec.maxBlockFrames, std::size_t{64});
    status = next.inputScratch_.resize(spec.channels, tile);
    if (status != Status::ok)
        return status;
    try {
        next.current_.resize(coefficients.size());
        next.target_.resize(coefficients.size());
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
    next.spec_ = spec;
    next.outputs_ = outputs;
    next.tileFrames_ = tile;
    (void)next.setMatrix(coefficients);
    swap(next);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status ChannelMatrix<T>::setMatrix(std::span<const double> coefficients,
                                   std::size_t frames) noexcept {
    if (!tileFrames_)
        return Status::not_prepared;
    if (coefficients.size() != current_.size())
        return Status::invalid_argument;
    bool identical = true;
    for (std::size_t i = 0; i < coefficients.size(); ++i) {
        if (!detail::validMatrixCoefficient<T>(coefficients[i]))
            return Status::invalid_argument;
        const auto rounded = static_cast<double>(static_cast<T>(coefficients[i]));
        if (frames && !isFinite(rounded - current_[i]))
            return Status::invalid_argument;
        identical = identical && rounded == target_[i];
    }
    if (frames && identical)
        return Status::ok;
    for (std::size_t i = 0; i < coefficients.size(); ++i) {
        target_[i] = static_cast<double>(static_cast<T>(coefficients[i]));
        if (!frames)
            current_[i] = target_[i];
    }
    remaining_ = frames;
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void ChannelMatrix<T>::reset() noexcept {
    std::copy(target_.begin(), target_.end(), current_.begin());
    remaining_ = 0;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status ChannelMatrix<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    if (!tileFrames_)
        return Status::not_prepared;
    if (!input.isValid() || !output.isValid())
        return Status::invalid_argument;
    if (input.channels() != spec_.channels || output.channels() != outputs_)
        return Status::channel_mismatch;
    if (input.frames() != output.frames())
        return Status::invalid_argument;
    if (input.frames() > spec_.maxBlockFrames)
        return Status::block_too_large;
    if (input.frames() > std::numeric_limits<std::size_t>::max() / sizeof(T))
        return Status::invalid_argument;
    const auto status = detail::validateMatrixAlias(
        spec_.channels, outputs_, input.frames() * sizeof(T),
        [input](auto ch) noexcept {
            return reinterpret_cast<std::uintptr_t>(input.channel(ch).data());
        },
        [output](auto ch) noexcept {
            return reinterpret_cast<std::uintptr_t>(output.channel(ch).data());
        });
    if (status != Status::ok)
        return status;
    std::size_t offset = 0;
    while (offset < input.frames()) {
        const auto count = std::min(tileFrames_, input.frames() - offset);
        auto saved = inputScratch_.block(count);
        const auto source = input.subBlock(offset, count);
        auto destination = output.subBlock(offset, count);
        for (std::size_t ch = 0; ch < spec_.channels; ++ch)
            (void)ops_.copy(source.channel(ch), saved.channel(ch));
        std::size_t frame = 0;
        for (; frame < count && remaining_; ++frame) {
            detail::advanceMatrixRamp(current_, target_, remaining_);
            detail::matrixFrame<T>(saved, destination, current_, frame);
        }
        if (frame < count) {
            for (std::size_t out = 0; out < outputs_; ++out) {
                auto samples = destination.channel(out).subspan(frame);
                ops_.fill(samples, T{});
                for (std::size_t in = 0; in < spec_.channels; ++in) {
                    const auto gain = static_cast<T>(current_[out * spec_.channels + in]);
                    if (gain != T{})
                        (void)ops_.addScaled(saved.channel(in).subspan(frame), samples, gain);
                }
            }
        }
        offset += count;
    }
    return Status::ok;
}
template class ChannelMatrix<float>;
template class ChannelMatrix<double>;
} // namespace cydsp
