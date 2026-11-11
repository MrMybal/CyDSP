#include "cydsp/processors/NonuniformConvolver.h"
#include <algorithm>
namespace cydsp {
template <Sample T>
    requires(!std::is_const_v<T>)
void NonuniformConvolver<T>::swap(NonuniformConvolver& other) noexcept {
    std::swap(spec_, other.spec_);
    std::swap(early_, other.early_);
    std::swap(late_, other.late_);
    scratch_.swap(other.scratch_);
    std::swap(ops_, other.ops_);
    std::swap(impulse_, other.impulse_);
    std::swap(latePartition_, other.latePartition_);
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status NonuniformConvolver<T>::prepare(const ProcessSpec& spec, std::span<const T> ir,
                                       std::size_t early, std::size_t late,
                                       CpuBackend backend) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    if (ir.empty() || !isPowerOfTwo(early) || early > 65536 || !isPowerOfTwo(late) ||
        late < early || late > (1u << 23))
        return Status::invalid_argument;
    for (auto value : ir)
        if (!isFinite(value))
            return Status::invalid_argument;
    NonuniformConvolver next;
    auto status = next.early_.prepare(spec, ir.first(std::min(ir.size(), late)), early, backend);
    if (status != Status::ok)
        return status;
    status = VectorOps::create(next.early_.backend(), next.ops_);
    if (status != Status::ok)
        return status;
    if (ir.size() > late) {
        status = next.late_.prepare(spec, ir.subspan(late), late, next.ops_.backend());
        if (status != Status::ok)
            return status;
        status = next.scratch_.resize(spec.channels, spec.maxBlockFrames);
        if (status != Status::ok)
            return status;
    }
    next.spec_ = spec;
    next.impulse_ = ir.size();
    next.latePartition_ = late;
    swap(next);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void NonuniformConvolver<T>::reset() noexcept {
    early_.reset();
    late_.reset();
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status NonuniformConvolver<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    if (!impulse_)
        return Status::not_prepared;
    const auto status = validateProcessBlocks(spec_, input, output);
    if (status != Status::ok)
        return status;
    if (impulse_ <= latePartition_)
        return early_.process(input, output);
    auto scratch = scratch_.block(input.frames());
    (void)late_.process(input, scratch);
    (void)early_.process(input, output);
    for (std::size_t ch = 0; ch < spec_.channels; ++ch)
        (void)ops_.addScaled(scratch.channel(ch), output.channel(ch), T{1});
    return Status::ok;
}
template class NonuniformConvolver<float>;
template class NonuniformConvolver<double>;
} // namespace cydsp
