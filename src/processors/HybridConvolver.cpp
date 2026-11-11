#include "cydsp/processors/HybridConvolver.h"
#include <algorithm>
namespace cydsp {
template <Sample T>
    requires(!std::is_const_v<T>)
void HybridConvolver<T>::swap(HybridConvolver& other) noexcept {
    std::swap(spec_, other.spec_);
    std::swap(head_, other.head_);
    std::swap(tail_, other.tail_);
    tailBuffer_.swap(other.tailBuffer_);
    std::swap(ops_, other.ops_);
    std::swap(partition_, other.partition_);
    std::swap(impulseFrames_, other.impulseFrames_);
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status HybridConvolver<T>::prepare(const ProcessSpec& spec, std::span<const T> impulse,
                                   std::size_t partition, CpuBackend backend) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    if (impulse.empty() || !isPowerOfTwo(partition) || partition > 65536)
        return Status::invalid_argument;
    for (const auto sample : impulse)
        if (!isFinite(sample))
            return Status::invalid_argument;
    HybridConvolver next;
    auto status =
        next.head_.prepare(spec, impulse.first(std::min(partition, impulse.size())), backend);
    if (status != Status::ok)
        return status;
    status = VectorOps::create(next.head_.backend(), next.ops_);
    if (status != Status::ok)
        return status;
    if (impulse.size() > partition) {
        // The tail's P-frame buffering is exactly the original IR's P-frame offset.
        status =
            next.tail_.prepare(spec, impulse.subspan(partition), partition, next.ops_.backend());
        if (status != Status::ok)
            return status;
        status = next.tailBuffer_.resize(spec.channels, spec.maxBlockFrames);
        if (status != Status::ok)
            return status;
    }
    next.spec_ = spec;
    next.partition_ = partition;
    next.impulseFrames_ = impulse.size();
    swap(next);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void HybridConvolver<T>::reset() noexcept {
    head_.reset();
    tail_.reset();
    auto scratch = tailBuffer_.block();
    for (std::size_t ch = 0; ch < scratch.channels(); ++ch)
        ops_.fill(scratch.channel(ch), T{});
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status HybridConvolver<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    if (!impulseFrames_)
        return Status::not_prepared;
    const auto status = validateProcessBlocks(spec_, input, output);
    if (status != Status::ok)
        return status;
    if (impulseFrames_ <= partition_)
        return head_.process(input, output);
    auto scratch = tailBuffer_.block(input.frames());
    // Preserve exact in-place input: the FFT tail must read it before the FIR writes.
    (void)tail_.process(input, scratch);
    (void)head_.process(input, output);
    for (std::size_t ch = 0; ch < spec_.channels; ++ch)
        (void)ops_.addScaled(scratch.channel(ch), output.channel(ch), T{1});
    return Status::ok;
}
template class HybridConvolver<float>;
template class HybridConvolver<double>;
} // namespace cydsp
