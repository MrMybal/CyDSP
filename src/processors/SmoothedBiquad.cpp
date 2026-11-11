#include "cydsp/processors/SmoothedBiquad.h"
#include "detail/BiquadAlgorithm.h"
#include "detail/Crossfade.h"
#include "detail/LinearRamp.h"
#include <algorithm>
namespace cydsp {
namespace {
bool sameCoefficients(const BiquadCoefficients& a, const BiquadCoefficients& b) noexcept {
    return a.b0 == b.b0 && a.b1 == b.b1 && a.b2 == b.b2 && a.a1 == b.a1 && a.a2 == b.a2;
}
} // namespace
template <Sample T>
    requires(!std::is_const_v<T>)
void SmoothedBiquad<T>::swap(SmoothedBiquad& other) noexcept {
    std::swap(spec_, other.spec_);
    for (std::size_t i = 0; i < 2; ++i) {
        std::swap(banks_[i], other.banks_[i]);
        scratch_[i].swap(other.scratch_[i]);
    }
    std::swap(target_, other.target_);
    std::swap(bankCoefficients_, other.bankCoefficients_);
    std::swap(active_, other.active_);
    std::swap(tileFrames_, other.tileFrames_);
    std::swap(remaining_, other.remaining_);
    std::swap(queuedFrames_, other.queuedFrames_);
    std::swap(blend_, other.blend_);
    std::swap(queued_, other.queued_);
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status SmoothedBiquad<T>::prepare(const ProcessSpec& spec, CpuBackend backend) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    SmoothedBiquad next;
    next.target_ = target_;
    const auto tile = std::min(spec.maxBlockFrames, std::size_t{64});
    for (std::size_t i = 0; i < 2; ++i) {
        auto status = next.banks_[i].prepare(spec, backend);
        if (status != Status::ok)
            return status;
        status = next.banks_[i].setAllCoefficients(target_);
        if (status != Status::ok)
            return status;
        next.bankCoefficients_[i] = target_;
        status = next.scratch_[i].resize(spec.channels, tile);
        if (status != Status::ok)
            return status;
    }
    next.spec_ = spec;
    next.tileFrames_ = tile;
    swap(next);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void SmoothedBiquad<T>::beginTransition(const BiquadCoefficients& coefficients,
                                        std::size_t frames) noexcept {
    const auto pending = active_ ^ 1u;
    (void)banks_[pending].setAllCoefficients(coefficients);
    bankCoefficients_[pending] = coefficients;
    banks_[pending].reset();
    remaining_ = frames;
    blend_ = 0;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status SmoothedBiquad<T>::setCoefficients(const BiquadCoefficients& coefficients,
                                          std::size_t frames) noexcept {
    if (!tileFrames_)
        return Status::not_prepared;
    BiquadCoefficients rounded;
    if (!detail::roundBiquad<T>(coefficients, rounded))
        return Status::invalid_argument;
    if (frames && sameCoefficients(rounded, target_))
        return Status::ok;
    target_ = rounded;
    if (!frames) {
        (void)banks_[active_].setAllCoefficients(target_);
        bankCoefficients_[active_] = target_;
        remaining_ = 0;
        queued_ = false;
        queuedFrames_ = 0;
        blend_ = 0;
    } else if (remaining_) {
        queued_ = !sameCoefficients(target_, bankCoefficients_[active_ ^ 1u]);
        queuedFrames_ = queued_ ? frames : 0;
    } else {
        queued_ = false;
        queuedFrames_ = 0;
        if (!sameCoefficients(target_, bankCoefficients_[active_]))
            beginTransition(target_, frames);
    }
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void SmoothedBiquad<T>::reset() noexcept {
    if (!tileFrames_)
        return;
    for (std::size_t i = 0; i < 2; ++i) {
        (void)banks_[i].setAllCoefficients(target_);
        bankCoefficients_[i] = target_;
        banks_[i].reset();
        auto scratch = scratch_[i].block();
        for (std::size_t ch = 0; ch < scratch.channels(); ++ch)
            std::fill(scratch.channel(ch).begin(), scratch.channel(ch).end(), T{});
    }
    active_ = 0;
    remaining_ = 0;
    queued_ = false;
    queuedFrames_ = 0;
    blend_ = 0;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status SmoothedBiquad<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    if (!tileFrames_)
        return Status::not_prepared;
    const auto status = validateProcessBlocks(spec_, input, output);
    if (status != Status::ok)
        return status;
    std::size_t offset = 0;
    while (offset < input.frames()) {
        if (!remaining_ && queued_) {
            const auto frames = queuedFrames_;
            queued_ = false;
            queuedFrames_ = 0;
            beginTransition(target_, frames);
        }
        if (!remaining_)
            return banks_[active_].process(input.subBlock(offset, input.frames() - offset),
                                           output.subBlock(offset, output.frames() - offset));
        const auto count = std::min({tileFrames_, remaining_, input.frames() - offset});
        const auto source = input.subBlock(offset, count);
        auto destination = output.subBlock(offset, count);
        auto from = scratch_[0].block(count), to = scratch_[1].block(count);
        (void)banks_[active_].process(source, from);
        (void)banks_[active_ ^ 1u].process(source, to);
        detail::crossfadeBlocks<T>(from, to, destination, blend_, remaining_);
        offset += count;
        if (!remaining_) {
            active_ ^= 1u;
            blend_ = 0;
        }
    }
    return Status::ok;
}
template class SmoothedBiquad<float>;
template class SmoothedBiquad<double>;
} // namespace cydsp
