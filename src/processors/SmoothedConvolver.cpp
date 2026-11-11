#include "cydsp/processors/SmoothedConvolver.h"
#include "detail/Crossfade.h"
#include <algorithm>
namespace cydsp {
template <Sample T>
    requires(!std::is_const_v<T>)
void SmoothedConvolver<T>::swap(SmoothedConvolver& other) noexcept {
    std::swap(spec_, other.spec_);
    std::swap(config_, other.config_);
    for (std::size_t i = 0; i < 2; ++i) {
        std::swap(banks_[i], other.banks_[i]);
        scratch_[i].swap(other.scratch_[i]);
    }
    std::swap(active_, other.active_);
    std::swap(tileFrames_, other.tileFrames_);
    std::swap(remaining_, other.remaining_);
    std::swap(warming_, other.warming_);
    std::swap(blend_, other.blend_);
    std::swap(staged_, other.staged_);
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status SmoothedConvolver<T>::prepare(const ProcessSpec& spec, std::span<const T> impulse,
                                     std::size_t partition, CpuBackend backend) noexcept {
    return prepare(spec, impulse, {ConvolutionMode::hybrid, partition, 1024}, backend);
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status SmoothedConvolver<T>::prepare(const ProcessSpec& spec, std::span<const T> impulse,
                                     const ConvolutionConfig& config, CpuBackend backend) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    SmoothedConvolver next;
    const auto tile = std::min(spec.maxBlockFrames, std::size_t{64});
    const ProcessSpec internal{spec.sampleRate, tile, spec.channels};
    auto status = next.banks_[0].prepare(internal, impulse, config, backend);
    if (status != Status::ok)
        return status;
    for (auto& scratch : next.scratch_) {
        status = scratch.resize(spec.channels, tile);
        if (status != Status::ok)
            return status;
    }
    next.spec_ = spec;
    next.config_ = config;
    next.tileFrames_ = tile;
    swap(next);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status SmoothedConvolver<T>::stageImpulse(std::span<const T> impulse) noexcept {
    if (!tileFrames_)
        return Status::not_prepared;
    if (isTransitioning())
        return Status::transition_in_progress;
    const ProcessSpec internal{spec_.sampleRate, tileFrames_, spec_.channels};
    // Bank preparation commits transactionally, retaining an earlier stage on failure.
    const auto status = banks_[active_ ^ 1u].prepare(internal, impulse, config_, backend());
    if (status == Status::ok)
        staged_ = true;
    return status;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status SmoothedConvolver<T>::startTransition(std::size_t frames, bool prewarm) noexcept {
    if (!tileFrames_)
        return Status::not_prepared;
    if (isTransitioning())
        return Status::transition_in_progress;
    if (!staged_)
        return Status::invalid_argument;
    staged_ = false;
    remaining_ = frames;
    warming_ = prewarm ? banks_[active_ ^ 1u].tailFrames() : 0;
    blend_ = 0;
    if (!frames && !warming_)
        active_ ^= 1u;
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
std::size_t SmoothedConvolver<T>::tailFrames() const noexcept {
    const auto activeTail = banks_[active_].tailFrames();
    return isTransitioning() ? std::max(activeTail, banks_[active_ ^ 1u].tailFrames()) : activeTail;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void SmoothedConvolver<T>::reset() noexcept {
    if (isTransitioning())
        active_ ^= 1u;
    remaining_ = 0;
    warming_ = 0;
    blend_ = 0;
    for (auto& bank : banks_)
        bank.reset();
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status SmoothedConvolver<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    if (!tileFrames_)
        return Status::not_prepared;
    const auto status = validateProcessBlocks(spec_, input, output);
    if (status != Status::ok)
        return status;
    std::size_t offset = 0;
    while (offset < input.frames()) {
        auto count = std::min(tileFrames_, input.frames() - offset);
        if (warming_)
            count = std::min(count, warming_);
        else if (remaining_)
            count = std::min(count, remaining_);
        const auto source = input.subBlock(offset, count);
        auto destination = output.subBlock(offset, count);
        if (warming_) {
            // Read the destination first: the active bank may overwrite exact in-place input.
            (void)banks_[active_ ^ 1u].process(source, scratch_[1].block(count));
            (void)banks_[active_].process(source, destination);
            warming_ -= count;
            if (!warming_ && !remaining_)
                active_ ^= 1u;
        } else if (!remaining_) {
            (void)banks_[active_].process(source, destination);
        } else {
            auto from = scratch_[0].block(count), to = scratch_[1].block(count);
            (void)banks_[active_].process(source, from);
            (void)banks_[active_ ^ 1u].process(source, to);
            detail::crossfadeBlocks<T>(from, to, destination, blend_, remaining_);
            if (!remaining_) {
                active_ ^= 1u;
                blend_ = 0;
            }
        }
        offset += count;
    }
    return Status::ok;
}
template class SmoothedConvolver<float>;
template class SmoothedConvolver<double>;
} // namespace cydsp
