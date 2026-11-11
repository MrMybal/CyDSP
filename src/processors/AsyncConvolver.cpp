#include "cydsp/processors/AsyncConvolver.h"
#include "detail/Crossfade.h"
#include <algorithm>
namespace cydsp {
template <Sample T>
    requires(!std::is_const_v<T>)
Status AsyncConvolver<T>::prepare(const ProcessSpec& spec, std::span<const T> ir,
                                  std::size_t partition, CpuBackend backend) noexcept {
    return prepare(spec, ir, {ConvolutionMode::hybrid, partition, 1024}, backend);
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status AsyncConvolver<T>::prepare(const ProcessSpec& spec, std::span<const T> ir,
                                  const ConvolutionConfig& config, CpuBackend backend) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    const auto tile = std::min(spec.maxBlockFrames, std::size_t{64});
    std::array<Convolver<T>, 3> banks;
    std::array<AudioBuffer<T>, 2> scratch;
    auto status = banks[0].prepare({spec.sampleRate, tile, spec.channels}, ir, config, backend);
    if (status != Status::ok)
        return status;
    for (auto& buffer : scratch) {
        status = buffer.resize(spec.channels, tile);
        if (status != Status::ok)
            return status;
    }
    banks_.swap(banks);
    scratch_.swap(scratch);
    spec_ = spec;
    tile_ = tile;
    config_ = config;
    backend_ = banks_[0].backend();
    active_ = 0;
    destination_ = none;
    remaining_ = 0;
    warming_ = 0;
    blend_ = 0;
    pending_.store(none, std::memory_order_relaxed);
    for (unsigned i = 0; i < 3; ++i)
        occupied_[i].store(i == 0 ? 1u : 0u, std::memory_order_relaxed);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status AsyncConvolver<T>::stageImpulse(std::span<const T> ir, std::size_t frames,
                                       bool prewarm) noexcept {
    if (!tile_)
        return Status::not_prepared;
    unsigned slot = none;
    for (unsigned i = 0; i < 3; ++i) {
        unsigned free = 0;
        if (occupied_[i].compare_exchange_strong(free, 1, std::memory_order_acquire)) {
            slot = i;
            break;
        }
    }
    if (slot == none)
        return Status::update_queue_full;
    const auto status =
        banks_[slot].prepare({spec_.sampleRate, tile_, spec_.channels}, ir, config_, backend_);
    if (status != Status::ok) {
        occupied_[slot].store(0, std::memory_order_release);
        return status;
    }
    durations_[slot] = frames;
    warmups_[slot] = prewarm ? ir.size() - 1 : 0;
    const auto replaced = pending_.exchange(slot, std::memory_order_acq_rel);
    if (replaced != none)
        occupied_[replaced].store(0, std::memory_order_release);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void AsyncConvolver<T>::adopt(unsigned slot) noexcept {
    const auto retired = active_;
    active_ = slot;
    destination_ = none;
    remaining_ = 0;
    warming_ = 0;
    blend_ = 0;
    // No access to the retired bank after this release; its next owner is the producer.
    occupied_[retired].store(0, std::memory_order_release);
}
template <Sample T>
    requires(!std::is_const_v<T>)
void AsyncConvolver<T>::reset() noexcept {
    if (isTransitioning())
        adopt(destination_);
    for (auto& bank : banks_)
        bank.reset();
}
template <Sample T>
    requires(!std::is_const_v<T>)
std::size_t AsyncConvolver<T>::tailFrames() const noexcept {
    const auto tail = banks_[active_].tailFrames();
    return isTransitioning() ? std::max(tail, banks_[destination_].tailFrames()) : tail;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status AsyncConvolver<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    if (!tile_)
        return Status::not_prepared;
    const auto status = validateProcessBlocks(spec_, input, output);
    if (status != Status::ok)
        return status;
    if (!input.frames())
        return Status::ok;
    // Accept at most one publication per host callback; empty/invalid calls consume nothing.
    if (!isTransitioning()) {
        const auto ready = pending_.exchange(none, std::memory_order_acq_rel);
        if (ready != none) {
            destination_ = ready;
            remaining_ = durations_[ready];
            warming_ = warmups_[ready];
            blend_ = 0;
            if (!isTransitioning())
                adopt(ready);
        }
    }
    std::size_t offset = 0;
    while (offset < input.frames()) {
        auto count = std::min(tile_, input.frames() - offset);
        if (warming_)
            count = std::min(count, warming_);
        else if (remaining_)
            count = std::min(count, remaining_);
        const auto source = input.subBlock(offset, count);
        auto outputPart = output.subBlock(offset, count);
        if (warming_) {
            (void)banks_[destination_].process(source, scratch_[1].block(count));
            (void)banks_[active_].process(source, outputPart);
            warming_ -= count;
            if (!warming_ && !remaining_)
                adopt(destination_);
        } else if (!remaining_)
            (void)banks_[active_].process(source, outputPart);
        else {
            auto from = scratch_[0].block(count), to = scratch_[1].block(count);
            (void)banks_[active_].process(source, from);
            (void)banks_[destination_].process(source, to);
            detail::crossfadeBlocks<T>(from, to, outputPart, blend_, remaining_);
            if (!isTransitioning())
                adopt(destination_);
        }
        offset += count;
    }
    return Status::ok;
}
template class AsyncConvolver<float>;
template class AsyncConvolver<double>;
} // namespace cydsp
