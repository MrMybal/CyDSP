#include "cydsp/processors/Resampler.h"
#include "detail/Alias.h"
#include "detail/CheckedSize.h"
#include "detail/RateClock.h"
#include "detail/WindowedSinc.h"
#include <algorithm>
#include <cmath>
#include <new>
#include <stdexcept>
namespace cydsp {
template <Sample T>
    requires(!std::is_const_v<T>)
void Resampler<T>::swap(Resampler& other) noexcept {
    std::swap(spec_, other.spec_);
    std::swap(ops_, other.ops_);
    history_.swap(other.history_);
    zeros_.swap(other.zeros_);
    std::swap(zerosRemaining_, other.zerosRemaining_);
    std::swap(hasInput_, other.hasInput_);
    std::swap(ending_, other.ending_);
    std::swap(finished_, other.finished_);
    table_.swap(other.table_);
    weights_.swap(other.weights_);
    std::swap(write_, other.write_);
    std::swap(pending_, other.pending_);
    std::swap(phase_, other.phase_);
    std::swap(variablePhase_, other.variablePhase_);
    std::swap(ratio_, other.ratio_);
    std::swap(targetRatio_, other.targetRatio_);
    std::swap(ratioRemaining_, other.ratioRemaining_);
    std::swap(prepared_, other.prepared_);
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status Resampler<T>::prepare(const ResampleSpec& spec, CpuBackend backend) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    std::size_t historySize{}, tableSize{};
    if (!detail::checkedMultiply(2 * spec.taps, spec.channels, historySize) ||
        !detail::checkedMultiply(spec.phases + 1, spec.taps, tableSize))
        return Status::invalid_spec;
    Resampler next;
    const auto status = VectorOps::create(backend, next.ops_);
    if (status != Status::ok)
        return status;
    try {
        next.history_.resize(historySize);
        next.table_.resize(tableSize);
        next.weights_.resize(spec.taps);
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_spec;
    }
    const auto zeroStatus =
        next.zeros_.resize(spec.channels, std::min(spec.maxInputFrames, spec.taps));
    if (zeroStatus != Status::ok)
        return zeroStatus;
    const auto worstRatio = spec.maximumRatio
                                ? spec.maximumRatio
                                : static_cast<double>(spec.inputRate) / spec.outputRate;
    const auto cutoff = 0.94 * std::min(1.0, 1.0 / worstRatio);
    const auto delay = static_cast<double>(spec.taps - 1) * 0.5;
    for (std::size_t phase = 0; phase <= spec.phases; ++phase) {
        const auto fraction = static_cast<double>(phase) / static_cast<double>(spec.phases);
        double sum = 0;
        for (std::size_t i = 0; i < spec.taps; ++i) {
            const auto value = detail::windowedSinc(delay - static_cast<double>(i) + fraction,
                                                    cutoff, static_cast<double>(spec.taps) * 0.5);
            next.table_[phase * spec.taps + i] = value;
            sum += value;
        }
        if (!isFinite(sum) || std::abs(sum) < 1e-30)
            return Status::invalid_spec;
        for (std::size_t i = 0; i < spec.taps; ++i)
            next.table_[phase * spec.taps + i] /= sum;
    }
    next.spec_ = spec;
    next.targetRatio_ = static_cast<double>(spec.inputRate) / spec.outputRate;
    next.prepared_ = true;
    next.reset();
    swap(next);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void Resampler<T>::reset() noexcept {
    std::fill(history_.begin(), history_.end(), T{});
    write_ = 0;
    pending_ = 1;
    phase_ = 0;
    variablePhase_ = 0;
    ratio_ = targetRatio_;
    ratioRemaining_ = 0;
    zerosRemaining_ = 0;
    hasInput_ = false;
    ending_ = false;
    finished_ = false;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status Resampler<T>::validateBlocks(AudioBlock<const T> input,
                                    AudioBlock<T> output) const noexcept {
    if (!prepared_)
        return Status::not_prepared;
    if (input.channels() != spec_.channels || output.channels() != spec_.channels)
        return Status::channel_mismatch;
    if (input.frames() > spec_.maxInputFrames || output.frames() > spec_.maxOutputFrames)
        return Status::block_too_large;
    if (!input.isValid() || !output.isValid() ||
        input.frames() > std::numeric_limits<std::size_t>::max() / sizeof(T) ||
        output.frames() > std::numeric_limits<std::size_t>::max() / sizeof(T))
        return Status::invalid_argument;
    for (std::size_t ch = 0; ch < spec_.channels; ++ch) {
        for (std::size_t other = 0; other < ch; ++other)
            if (detail::overlaps(output.channel(ch).data(), output.frames() * sizeof(T),
                                 output.channel(other).data(), output.frames() * sizeof(T)))
                return Status::partial_overlap;
        for (std::size_t in = 0; in < spec_.channels; ++in)
            if (detail::overlaps(input.channel(in).data(), input.frames() * sizeof(T),
                                 output.channel(ch).data(), output.frames() * sizeof(T)))
                return Status::partial_overlap;
    }
    return Status::ok;
}

template <Sample T>
    requires(!std::is_const_v<T>)
ResampleResult Resampler<T>::processInternal(AudioBlock<const T> input,
                                             AudioBlock<T> output) noexcept {
    const auto validation = validateBlocks(input, output);
    if (validation != Status::ok)
        return {validation};
    ResampleResult result{};
    while (result.outputProduced < output.frames()) {
        while (pending_) {
            if (result.inputConsumed == input.frames())
                return result;
            for (std::size_t ch = 0; ch < spec_.channels; ++ch) {
                const auto sample = input.channel(ch)[result.inputConsumed];
                const auto base = ch * 2 * spec_.taps;
                history_[base + write_] = sample;
                history_[base + write_ + spec_.taps] = sample;
            }
            if (++write_ == spec_.taps)
                write_ = 0;
            ++result.inputConsumed;
            --pending_;
        }
        const auto scaled = phase_ * spec_.phases;
        const auto variableScaled = variablePhase_ * static_cast<double>(spec_.phases);
        const auto row = spec_.maximumRatio ? static_cast<std::size_t>(variableScaled)
                                            : static_cast<std::size_t>(scaled / spec_.outputRate);
        const auto blend = spec_.maximumRatio
                               ? variableScaled - static_cast<double>(row)
                               : static_cast<double>(scaled % spec_.outputRate) / spec_.outputRate;
        for (std::size_t i = 0; i < spec_.taps; ++i) {
            const auto a = table_[row * spec_.taps + i], b = table_[(row + 1) * spec_.taps + i];
            weights_[i] = a + (b - a) * blend;
        }
        for (std::size_t ch = 0; ch < spec_.channels; ++ch) {
            const auto* source = history_.data() + ch * 2 * spec_.taps + write_;
            double value = 0;
            (void)ops_.dot(std::span<const T>{source, spec_.taps}, weights_, value);
            output.channel(ch)[result.outputProduced] = static_cast<T>(value);
        }
        ++result.outputProduced;
        if (spec_.maximumRatio) {
            if (ratioRemaining_) {
                ratio_ += (targetRatio_ - ratio_) / static_cast<double>(ratioRemaining_);
                if (!--ratioRemaining_)
                    ratio_ = targetRatio_;
            }
            const auto next = variablePhase_ + ratio_;
            pending_ = static_cast<std::size_t>(next);
            variablePhase_ = next - static_cast<double>(pending_);
        } else {
            detail::RateClock clock{phase_, pending_};
            clock.advance(spec_.inputRate, spec_.outputRate);
            phase_ = clock.phase;
            pending_ = clock.pending;
        }
    }
    return result;
}
template <Sample T>
    requires(!std::is_const_v<T>)
ResampleResult Resampler<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    if (ending_)
        return {Status::end_of_stream};
    const auto result = processInternal(input, output);
    if (result.inputConsumed)
        hasInput_ = true;
    return result;
}
template <Sample T>
    requires(!std::is_const_v<T>)
ResampleResult Resampler<T>::finish(AudioBlock<T> output) noexcept {
    if (!prepared_)
        return {Status::not_prepared};
    const auto status = validateBlocks(zeros_.block(0), output);
    if (status != Status::ok)
        return {status};
    if (!output.frames() || finished_)
        return {};
    if (!ending_) {
        ending_ = true;
        zerosRemaining_ = hasInput_ ? spec_.taps : 0;
        if (!hasInput_) {
            finished_ = true;
            return {};
        }
    }
    ResampleResult result;
    while (result.outputProduced < output.frames() && !finished_) {
        const auto count = std::min(zerosRemaining_, zeros_.block().frames());
        const auto step = processInternal(
            zeros_.block(count),
            output.subBlock(result.outputProduced, output.frames() - result.outputProduced));
        zerosRemaining_ -= step.inputConsumed;
        result.outputProduced += step.outputProduced;
        if (!zerosRemaining_ && pending_)
            finished_ = true;
    }
    return result;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status Resampler<T>::setRates(std::uint32_t inputRate, std::uint32_t outputRate) noexcept {
    if (!prepared_)
        return Status::not_prepared;
    if (!inputRate || !outputRate)
        return Status::invalid_argument;
    if (ending_)
        return Status::end_of_stream;
    if (inputRate == spec_.inputRate && outputRate == spec_.outputRate)
        return Status::ok;
    auto spec = spec_;
    spec.inputRate = inputRate;
    spec.outputRate = outputRate;
    Resampler next;
    const auto status = next.prepare(spec, backend());
    if (status != Status::ok)
        return status;
    std::copy(history_.begin(), history_.end(), next.history_.begin());
    next.write_ = write_;
    next.pending_ = pending_;
    next.phase_ = (phase_ * outputRate) / spec_.outputRate;
    next.hasInput_ = hasInput_;
    next.variablePhase_ = variablePhase_;
    swap(next);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status Resampler<T>::setRatio(double value, std::size_t frames) noexcept {
    if (!prepared_)
        return Status::not_prepared;
    if (ending_)
        return Status::end_of_stream;
    if (!isFinite(value) || !spec_.maximumRatio || value < spec_.minimumRatio ||
        value > spec_.maximumRatio)
        return Status::invalid_argument;
    targetRatio_ = value;
    ratioRemaining_ = frames;
    if (!frames)
        ratio_ = value;
    return Status::ok;
}
template class Resampler<float>;
template class Resampler<double>;
} // namespace cydsp
