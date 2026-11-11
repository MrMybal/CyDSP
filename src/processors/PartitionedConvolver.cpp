// Uniform partitioned overlap-add adapted from FFTConvolver.
// Copyright (c) 2017 HiFi-LoFi. MIT: third_party/licenses/FFTConvolver.txt.
// CyDSP redesign: shared FFT/math, float/double, independent channels, fixed latency,
// transactional prepare, no deletion in reset, and no quiet-IR truncation.
#include "cydsp/processors/PartitionedConvolver.h"
#include "detail/CheckedSize.h"
#include "detail/PartitionSchedule.h"
#include "detail/RateClock.h"
#include "detail/VectorTable.h"
#include <algorithm>
#include <new>
#include <stdexcept>
namespace cydsp {
template <Sample T>
    requires(!std::is_const_v<T>)
void PartitionedConvolver<T>::swap(PartitionedConvolver& other) noexcept {
    std::swap(spec_, other.spec_);
    std::swap(partition_, other.partition_);
    std::swap(partitions_, other.partitions_);
    std::swap(position_, other.position_);
    std::swap(head_, other.head_);
    std::swap(tail_, other.tail_);
    std::swap(jobsDone_, other.jobsDone_);
    std::swap(jobChannel_, other.jobChannel_);
    std::swap(jobPart_, other.jobPart_);
    std::swap(jobsPerFrame_, other.jobsPerFrame_);
    std::swap(extraJobs_, other.extraJobs_);
    std::swap(workDebt_, other.workDebt_);
    std::swap(fft_, other.fft_);
    std::swap(ops_, other.ops_);
    input_.swap(other.input_);
    ready_.swap(other.ready_);
    overlap_.swap(other.overlap_);
    time_.swap(other.time_);
    impulses_.swap(other.impulses_);
    history_.swap(other.history_);
    sum_.swap(other.sum_);
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status PartitionedConvolver<T>::prepare(const ProcessSpec& spec, std::span<const T> impulse,
                                        std::size_t partition, CpuBackend backend) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    if (impulse.empty() || !isPowerOfTwo(partition) || partition > (std::size_t{1} << 23))
        return Status::invalid_argument;
    for (auto sample : impulse)
        if (!isFinite(sample))
            return Status::invalid_argument;
    const auto parts = 1 + (impulse.size() - 1) / partition, bins = partition + 1;
    std::size_t spectra{}, history{}, channelSamples{}, tail{};
    if (!detail::checkedMultiply(parts, bins, spectra) ||
        !detail::checkedMultiply(spectra, spec.channels, history) ||
        !detail::checkedMultiply(partition, spec.channels, channelSamples) ||
        !detail::checkedAdd(partition, impulse.size() - 1, tail))
        return Status::invalid_argument;
    PartitionedConvolver next;
    auto status = next.fft_.prepare(2 * partition, backend);
    if (status != Status::ok)
        return status;
    status = VectorOps::create(next.fft_.backend(), next.ops_);
    if (status != Status::ok)
        return status;
    try {
        next.input_.resize(channelSamples);
        next.ready_.resize(channelSamples);
        next.overlap_.resize(channelSamples);
        next.time_.resize(2 * partition);
        next.impulses_.resize(spectra);
        next.history_.resize(history);
        next.sum_.resize(spec.channels * bins);
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
    next.spec_ = spec;
    next.partition_ = partition;
    next.partitions_ = parts;
    const auto jobs = (parts - 1) * spec.channels;
    next.jobsPerFrame_ = jobs / partition;
    next.extraJobs_ = jobs % partition;
    next.tail_ = tail;
    for (std::size_t part = 0; part < parts; ++part) {
        std::fill(next.time_.begin(), next.time_.end(), T{});
        const auto offset = part * partition, count = std::min(partition, impulse.size() - offset);
        std::copy_n(impulse.data() + offset, count, next.time_.begin());
        (void)next.fft_.forward(next.time_, std::span{next.impulses_}.subspan(part * bins, bins));
    }
    next.reset();
    swap(next);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void PartitionedConvolver<T>::reset() noexcept {
    for (auto* buffer : {&input_, &ready_, &overlap_, &time_})
        std::fill(buffer->begin(), buffer->end(), T{});
    std::fill(history_.begin(), history_.end(), std::complex<T>{});
    std::fill(sum_.begin(), sum_.end(), std::complex<T>{});
    position_ = 0;
    head_ = 0;
    jobsDone_ = 0;
    jobChannel_ = workDebt_ = 0;
    jobPart_ = 1;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void PartitionedConvolver<T>::accumulateUntil(std::size_t due) noexcept {
    const auto bins = partition_ + 1;
    while (jobsDone_ < due) {
        const auto ch = jobChannel_, part = jobPart_;
        const auto ring = detail::convolutionHistoryIndex(head_, part, partitions_);
        const auto* audio = history_.data() + (ch * partitions_ + ring) * bins;
        const auto* filter = impulses_.data() + part * bins;
        detail::VectorAccess::kernels<T>(ops_).complexMac(
            reinterpret_cast<const T*>(audio), reinterpret_cast<const T*>(filter),
            reinterpret_cast<T*>(sum_.data() + ch * bins), bins);
        if (++jobPart_ == partitions_) {
            jobPart_ = 1;
            ++jobChannel_;
        }
        ++jobsDone_;
    }
}
template <Sample T>
    requires(!std::is_const_v<T>)
void PartitionedConvolver<T>::processPartition() noexcept {
    const auto bins = partition_ + 1;
    for (std::size_t ch = 0; ch < spec_.channels; ++ch) {
        std::copy_n(input_.data() + ch * partition_, partition_, time_.data());
        std::fill(time_.begin() + static_cast<std::ptrdiff_t>(partition_), time_.end(), T{});
        auto current = std::span{history_}.subspan((ch * partitions_ + head_) * bins, bins);
        auto sum = std::span{sum_}.subspan(ch * bins, bins);
        (void)fft_.forward(time_, current);
        detail::VectorAccess::kernels<T>(ops_).complexMac(
            reinterpret_cast<const T*>(current.data()),
            reinterpret_cast<const T*>(impulses_.data()), reinterpret_cast<T*>(sum.data()), bins);
        sum.front().imag(T{});
        sum.back().imag(T{});
        (void)fft_.inverse(sum, time_);
        for (std::size_t i = 0; i < partition_; ++i) {
            ready_[ch * partition_ + i] = time_[i] + overlap_[ch * partition_ + i];
            overlap_[ch * partition_ + i] = time_[partition_ + i];
        }
    }
    std::fill(sum_.begin(), sum_.end(), std::complex<T>{});
    jobsDone_ = 0;
    jobChannel_ = workDebt_ = 0;
    jobPart_ = 1;
    if (++head_ == partitions_)
        head_ = 0;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status PartitionedConvolver<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    if (!partition_)
        return Status::not_prepared;
    const auto status = validateProcessBlocks(spec_, input, output);
    if (status != Status::ok)
        return status;
    for (std::size_t frame = 0; frame < input.frames(); ++frame) {
        for (std::size_t ch = 0; ch < spec_.channels; ++ch) {
            const auto sample = input.channel(ch)[frame];
            input_[ch * partition_ + position_] = sample;
            output.channel(ch)[frame] = ready_[ch * partition_ + position_];
        }
        ++position_;
        accumulateUntil(jobsDone_ + detail::advancePartitionJobs(jobsPerFrame_, extraJobs_,
                                                                 partition_, workDebt_));
        if (position_ == partition_) {
            processPartition();
            position_ = 0;
        }
    }
    return Status::ok;
}
template class PartitionedConvolver<float>;
template class PartitionedConvolver<double>;
} // namespace cydsp
