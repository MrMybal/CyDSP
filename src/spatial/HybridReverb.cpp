#include "cydsp/spatial/HybridReverb.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/processors/Gain.h"
#include "cydsp/processors/SmoothedConvolver.h"
#include "detail/Analysis.h"
#include <algorithm>
#include <cmath>
namespace cydsp {
template <Sample T> struct HybridReverb<T>::State {
    ProcessSpec spec;
    SmoothedConvolver<T> impulse;
    FdnReverb<T> late;
    Gain<T> impulseGain, lateGain;
    AudioBuffer<T> a, b;
    VectorOps ops;
};
template <Sample T> HybridReverb<T>::HybridReverb() = default;
template <Sample T> HybridReverb<T>::~HybridReverb() = default;
template <Sample T> HybridReverb<T>::HybridReverb(HybridReverb&&) noexcept = default;
template <Sample T> HybridReverb<T>& HybridReverb<T>::operator=(HybridReverb&&) noexcept = default;
template <Sample T>
Status HybridReverb<T>::prepare(const ProcessSpec& spec, std::span<const T> impulse,
                                const FdnConfig& room, const ConvolutionConfig& convolution,
                                CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || impulse.empty())
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        auto status = s->impulse.prepare(spec, impulse, convolution, backend);
        if (status != Status::ok)
            return status;
        status = s->late.prepare(spec, room, backend);
        if (status != Status::ok)
            return status;
        status = VectorOps::create(backend, s->ops);
        if (status != Status::ok)
            return status;
        for (auto* gain : {&s->impulseGain, &s->lateGain}) {
            status = gain->prepare(spec, backend);
            if (status != Status::ok)
                return status;
        }
        for (auto* block : {&s->a, &s->b}) {
            status = block->resize(spec.channels, spec.maxBlockFrames);
            if (status != Status::ok)
                return status;
        }
        (void)s->lateGain.setGain(T(.25));
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status HybridReverb<T>::setMix(double a, double b, std::size_t frames) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!isFinite(a) || !isFinite(b) || std::abs(a) > 16 || std::abs(b) > 16 || frames > (1u << 30))
        return Status::invalid_argument;
    (void)state_->impulseGain.setGain(T(a), frames);
    (void)state_->lateGain.setGain(T(b), frames);
    return Status::ok;
}
template <Sample T>
Status HybridReverb<T>::setLateParameters(const FdnParameters& p, std::size_t frames) noexcept {
    return state_ ? state_->late.setParameters(p, frames) : Status::not_prepared;
}
template <Sample T> Status HybridReverb<T>::stageImpulse(std::span<const T> impulse) noexcept {
    return state_ ? state_->impulse.stageImpulse(impulse) : Status::not_prepared;
}
template <Sample T> Status HybridReverb<T>::startImpulseTransition(std::size_t frames) noexcept {
    return state_ ? state_->impulse.startTransition(frames) : Status::not_prepared;
}
template <Sample T>
Status HybridReverb<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, input, output);
    if (status != Status::ok)
        return status;
    status = detail::validateAnalysis(s.spec, input);
    if (status != Status::ok)
        return status;
    auto a = s.a.block(input.frames()), b = s.b.block(input.frames());
    status = s.impulse.process(input, a);
    if (status != Status::ok)
        return status;
    status = s.late.process(input, b);
    if (status != Status::ok)
        return status;
    (void)s.impulseGain.process(a, a);
    (void)s.lateGain.process(b, b);
    for (std::size_t ch = 0; ch < output.channels(); ++ch) {
        (void)s.ops.copy(a.channel(ch), output.channel(ch));
        (void)s.ops.addScaled(b.channel(ch), output.channel(ch), T{1});
    }
    return Status::ok;
}
template <Sample T> void HybridReverb<T>::reset() noexcept {
    if (state_) {
        state_->impulse.reset();
        state_->late.reset();
        state_->impulseGain.reset();
        state_->lateGain.reset();
    }
}
template <Sample T> std::size_t HybridReverb<T>::nominalTailFrames() const noexcept {
    return state_ ? std::max(state_->impulse.tailFrames(), state_->late.nominalTailFrames()) : 0;
}
template class HybridReverb<float>;
template class HybridReverb<double>;
} // namespace cydsp
