#include "cydsp/processors/SmoothedBiquadCascade.h"
#include "detail/BiquadAlgorithm.h"
#include "detail/Crossfade.h"
#include <algorithm>
namespace cydsp {
namespace {
bool equalSections(std::span<const BiquadCoefficients> a,
                   std::span<const BiquadCoefficients> b) noexcept {
    if (a.size() != b.size())
        return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const auto& x = a[i];
        const auto& y = b[i];
        if (x.b0 != y.b0 || x.b1 != y.b1 || x.b2 != y.b2 || x.a1 != y.a1 || x.a2 != y.a2)
            return false;
    }
    return true;
}
} // namespace
template <Sample T>
    requires(!std::is_const_v<T>)
void SmoothedBiquadCascade<T>::swap(SmoothedBiquadCascade& other) noexcept {
    std::swap(spec_, other.spec_);
    banks_.swap(other.banks_);
    scratch_.swap(other.scratch_);
    std::swap(bankCoefficients_, other.bankCoefficients_);
    std::swap(target_, other.target_);
    std::swap(counts_, other.counts_);
    std::swap(ops_, other.ops_);
    std::swap(active_, other.active_);
    std::swap(maximum_, other.maximum_);
    std::swap(tile_, other.tile_);
    std::swap(targetCount_, other.targetCount_);
    std::swap(remaining_, other.remaining_);
    std::swap(queuedFrames_, other.queuedFrames_);
    std::swap(blend_, other.blend_);
    std::swap(queued_, other.queued_);
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status SmoothedBiquadCascade<T>::prepare(const ProcessSpec& spec, std::size_t maximum,
                                         CpuBackend backend) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    if (!maximum || maximum > 64)
        return Status::invalid_argument;
    SmoothedBiquadCascade next;
    auto status = VectorOps::create(backend, next.ops_);
    if (status != Status::ok)
        return status;
    next.tile_ = std::min(spec.maxBlockFrames, std::size_t{64});
    const ProcessSpec internal{spec.sampleRate, next.tile_, spec.channels};
    try {
        for (auto& bank : next.banks_)
            bank.resize(maximum);
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
    for (std::size_t b = 0; b < 2; ++b) {
        for (auto& section : next.banks_[b]) {
            status = section.prepare(internal, next.ops_.backend());
            if (status != Status::ok)
                return status;
        }
        status = next.scratch_[b].resize(spec.channels, next.tile_);
        if (status != Status::ok)
            return status;
    }
    next.spec_ = spec;
    next.maximum_ = maximum;
    swap(next);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void SmoothedBiquadCascade<T>::configure(std::size_t bank, bool clear) noexcept {
    for (std::size_t i = 0; i < targetCount_; ++i) {
        (void)banks_[bank][i].setAllCoefficients(target_[i]);
        if (clear)
            banks_[bank][i].reset();
        bankCoefficients_[bank][i] = target_[i];
    }
    counts_[bank] = targetCount_;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status SmoothedBiquadCascade<T>::setCoefficients(std::span<const BiquadCoefficients> coefficients,
                                                 std::size_t frames) noexcept {
    if (!maximum_)
        return Status::not_prepared;
    if (coefficients.size() > maximum_)
        return Status::invalid_argument;
    std::array<BiquadCoefficients, 64> rounded;
    for (std::size_t i = 0; i < coefficients.size(); ++i)
        if (!detail::roundBiquad<T>(coefficients[i], rounded[i]))
            return Status::invalid_argument;
    const auto wanted = std::span<const BiquadCoefficients>{rounded}.first(coefficients.size());
    if (frames &&
        equalSections(wanted, std::span<const BiquadCoefficients>{target_}.first(targetCount_)))
        return Status::ok;
    std::copy(wanted.begin(), wanted.end(), target_.begin());
    targetCount_ = wanted.size();
    if (!frames) {
        configure(active_, true);
        remaining_ = 0;
        queued_ = false;
        queuedFrames_ = 0;
        blend_ = 0;
    } else if (remaining_) {
        queued_ = !equalSections(
            wanted, std::span<const BiquadCoefficients>{bankCoefficients_[active_ ^ 1u]}.first(
                        counts_[active_ ^ 1u]));
        queuedFrames_ = queued_ ? frames : 0;
    } else {
        queued_ = false;
        queuedFrames_ = 0;
        if (!equalSections(wanted,
                           std::span<const BiquadCoefficients>{bankCoefficients_[active_]}.first(
                               counts_[active_]))) {
            configure(active_ ^ 1u, true);
            remaining_ = frames;
            blend_ = 0;
        }
    }
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void SmoothedBiquadCascade<T>::reset() noexcept {
    if (!maximum_)
        return;
    configure(0, true);
    configure(1, true);
    active_ = 0;
    remaining_ = 0;
    queued_ = false;
    queuedFrames_ = 0;
    blend_ = 0;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void SmoothedBiquadCascade<T>::run(std::size_t bank, AudioBlock<const T> input,
                                   AudioBlock<T> output) noexcept {
    if (!counts_[bank]) {
        for (std::size_t ch = 0; ch < spec_.channels; ++ch)
            (void)ops_.copy(input.channel(ch), output.channel(ch));
        return;
    }
    (void)banks_[bank][0].process(input, output);
    for (std::size_t section = 1; section < counts_[bank]; ++section)
        (void)banks_[bank][section].process(output, output);
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status SmoothedBiquadCascade<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    if (!maximum_)
        return Status::not_prepared;
    const auto status = validateProcessBlocks(spec_, input, output);
    if (status != Status::ok)
        return status;
    std::size_t offset = 0;
    while (offset < input.frames()) {
        if (!remaining_ && queued_) {
            configure(active_ ^ 1u, true);
            remaining_ = queuedFrames_;
            blend_ = 0;
            queued_ = false;
            queuedFrames_ = 0;
        }
        auto count = std::min(tile_, input.frames() - offset);
        if (remaining_)
            count = std::min(count, remaining_);
        const auto source = input.subBlock(offset, count);
        auto destination = output.subBlock(offset, count);
        if (!remaining_)
            run(active_, source, destination);
        else {
            auto from = scratch_[0].block(count), to = scratch_[1].block(count);
            run(active_, source, from);
            run(active_ ^ 1u, source, to);
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
template class SmoothedBiquadCascade<float>;
template class SmoothedBiquadCascade<double>;
} // namespace cydsp
