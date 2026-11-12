// Private MIT backend: third_party/licenses/signalsmith-stretch.txt and signalsmith-linear.txt
#include "cydsp/processors/TimeStretch.h"
#include "cydsp/core/AudioBuffer.h"
#include "detail/Alias.h"
#include "detail/StretchAlgorithm.h"
#include <cmath>
namespace cydsp {
namespace {
template <Sample T> struct Channels {
    AudioBlock<T> block;
    T* operator[](int ch) const {
        return block.channel(static_cast<std::size_t>(ch)).data();
    }
};
} // namespace
template <Sample T> struct TimeStretch<T>::State {
    detail::StretchAlgorithm<T> engine{1};
    ProcessSpec spec;
    std::size_t maximumOutputFrames = 0;
    bool ending = false;
    AudioBuffer<T> tail;
    std::size_t tailPosition = 0;
};
template <Sample T> TimeStretch<T>::TimeStretch() = default;
template <Sample T> TimeStretch<T>::~TimeStretch() = default;
template <Sample T> TimeStretch<T>::TimeStretch(TimeStretch&&) noexcept = default;
template <Sample T> TimeStretch<T>& TimeStretch<T>::operator=(TimeStretch&&) noexcept = default;
template <Sample T>
Status TimeStretch<T>::prepare(const ProcessSpec& spec, std::size_t max, bool cheaper, bool split,
                               CpuBackend backend) noexcept {
    if (!spec.isValid() || spec.sampleRate < 8000 || spec.sampleRate > 384000 ||
        spec.channels > 64 || spec.maxBlockFrames > 1048576 || !max || max > 1048576)
        return Status::invalid_spec;
    try {
        auto next = std::make_unique<State>();
        next->engine.select(backend);
        next->spec = spec;
        next->maximumOutputFrames = max;
        if (cheaper)
            next->engine.presetCheaper(static_cast<int>(spec.channels),
                                       static_cast<T>(spec.sampleRate), split);
        else
            next->engine.presetDefault(static_cast<int>(spec.channels),
                                       static_cast<T>(spec.sampleRate), split);
        const auto tailStatus = next->tail.resize(
            spec.channels, static_cast<std::size_t>(next->engine.outputLatency()));
        if (tailStatus != Status::ok)
            return tailStatus;
        state_.swap(next);
        return Status::ok;
    } catch (const detail::StretchPreparationError& error) {
        return error.status;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::invalid_spec;
    }
}
template <Sample T>
Status TimeStretch<T>::setPitchSemitones(double semitones, double limit) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!isFinite(semitones) || semitones < -48 || semitones > 48 || !isFinite(limit) ||
        limit < 0 || limit > state_->spec.sampleRate / 2)
        return Status::invalid_argument;
    state_->engine.setTransposeSemitones(static_cast<T>(semitones),
                                         static_cast<T>(limit / state_->spec.sampleRate));
    return Status::ok;
}
template <Sample T>
Status TimeStretch<T>::setFormant(double multiplier, bool compensate, double base) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!isFinite(multiplier) || multiplier < 0.25 || multiplier > 4 || !isFinite(base) ||
        base < 0 || base > state_->spec.sampleRate / 2)
        return Status::invalid_argument;
    state_->engine.setFormantFactor(static_cast<T>(multiplier), compensate);
    state_->engine.setFormantBase(static_cast<T>(base / state_->spec.sampleRate));
    return Status::ok;
}
template <Sample T>
Status TimeStretch<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (s.ending)
        return Status::end_of_stream;
    if (!in.isValid() || !out.isValid() || (!out.frames() && in.frames()))
        return Status::invalid_argument;
    if (in.channels() != s.spec.channels || out.channels() != s.spec.channels)
        return Status::channel_mismatch;
    if (in.frames() > s.spec.maxBlockFrames || out.frames() > s.maximumOutputFrames)
        return Status::block_too_large;
    const auto status = validateProcessBlocks<T>(
        {s.spec.sampleRate, s.maximumOutputFrames, s.spec.channels}, out, out);
    if (status != Status::ok)
        return status;
    for (std::size_t ch = 0; ch < out.channels(); ++ch)
        for (std::size_t i = 0; i < in.channels(); ++i)
            if (detail::overlaps(in.channel(i).data(), in.frames() * sizeof(T),
                                 out.channel(ch).data(), out.frames() * sizeof(T)))
                return Status::partial_overlap;
    if (!out.frames())
        return Status::ok;
    s.engine.process(Channels<const T>{in}, static_cast<int>(in.frames()), Channels<T>{out},
                     static_cast<int>(out.frames()));
    return Status::ok;
}
template <Sample T> Status TimeStretch<T>::prime(AudioBlock<const T> history) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (!history.isValid() || history.channels() != s.spec.channels ||
        history.frames() <= inputLatency())
        return Status::invalid_argument;
    if (history.frames() > s.spec.maxBlockFrames)
        return Status::block_too_large;
    s.engine.outputSeek(Channels<const T>{history}, static_cast<int>(history.frames()));
    s.ending = false;
    s.tailPosition = 0;
    return Status::ok;
}
template <Sample T> Status TimeStretch<T>::flush(AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    const auto status = validateProcessBlocks<T>(
        {s.spec.sampleRate, s.maximumOutputFrames, s.spec.channels}, out, out);
    if (status != Status::ok)
        return status;
    if (!out.frames())
        return Status::ok;
    if (!s.ending) {
        s.engine.flush(Channels<T>{s.tail.block()}, s.engine.outputLatency());
        s.ending = true;
    }
    const auto tail = s.tail.block();
    const auto n = std::min(out.frames(), tail.frames() - s.tailPosition);
    for (std::size_t ch = 0; ch < out.channels(); ++ch) {
        std::copy_n(tail.channel(ch).begin() + s.tailPosition, n, out.channel(ch).begin());
        std::fill(out.channel(ch).begin() + n, out.channel(ch).end(), T{});
    }
    s.tailPosition += n;
    return n == out.frames() ? Status::ok : Status::end_of_stream;
}
template <Sample T> void TimeStretch<T>::reset() noexcept {
    if (state_) {
        state_->engine.reset();
        state_->ending = false;
        state_->tailPosition = 0;
    }
}
template <Sample T> std::size_t TimeStretch<T>::inputLatency() const noexcept {
    return state_ ? static_cast<std::size_t>(state_->engine.inputLatency()) : 0;
}
template <Sample T> std::size_t TimeStretch<T>::outputLatency() const noexcept {
    return state_ ? static_cast<std::size_t>(state_->engine.outputLatency()) : 0;
}
template <Sample T> CpuBackend TimeStretch<T>::backend() const noexcept {
    return state_ ? state_->engine.backend() : CpuBackend::scalar;
}

template class TimeStretch<float>;
template class TimeStretch<double>;
} // namespace cydsp
