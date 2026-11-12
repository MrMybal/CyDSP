#include "cydsp/sources/AudioTransport.h"
#include "cydsp/core/SpscQueue.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <stdexcept>
namespace cydsp {
Status TempoMap::prepare(double rate, std::span<const TempoPoint> points) noexcept {
    if (!isFinite(rate) || rate < 1 || rate > 768000 || points.empty() || points[0].beat != 0)
        return Status::invalid_argument;
    try {
        std::vector<Point> next;
        next.reserve(points.size());
        double frame = 0;
        std::int64_t bar = 0;
        double barBeat = 0;
        for (std::size_t i = 0; i < points.size(); ++i) {
            const auto& p = points[i];
            if (!isFinite(p.beat) || p.beat < 0 || !isFinite(p.bpm) || p.bpm < 1 || p.bpm > 1000 ||
                !p.numerator || p.numerator > 64 || !isPowerOfTwo(p.denominator) ||
                p.denominator > 64 || (i && p.beat <= points[i - 1].beat))
                return Status::invalid_argument;
            if (i) {
                const auto& previous = points[i - 1];
                frame += (p.beat - previous.beat) * 60 * rate / previous.bpm;
                const auto length = previous.numerator * 4.0 / previous.denominator;
                const auto total = barBeat + p.beat - previous.beat,
                           bars = std::floor(total / length);
                if (bars > static_cast<double>(std::numeric_limits<std::int64_t>::max() - bar))
                    return Status::invalid_argument;
                bar += static_cast<std::int64_t>(bars);
                barBeat = total - bars * length;
                if (p.numerator != previous.numerator || p.denominator != previous.denominator) {
                    if (barBeat > 1e-10)
                        ++bar;
                    barBeat = 0;
                }
            }
            if (!isFinite(frame) || frame > 9007199254740991.0)
                return Status::invalid_argument;
            next.push_back({p, frame, bar, barBeat});
        }
        points_.swap(next);
        rate_ = rate;
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
}
MusicalPosition TempoMap::position(std::uint64_t frame) const noexcept {
    if (points_.empty() || frame > 9007199254740991ull)
        return {};
    const auto where = std::upper_bound(points_.begin(), points_.end(), static_cast<double>(frame),
                                        [](double v, const Point& p) { return v < p.frame; });
    const auto& p = where == points_.begin() ? points_.front() : *(where - 1);
    const auto delta = std::max(0.0, (static_cast<double>(frame) - p.frame) * p.tempo.bpm /
                                         (60 * rate_)),
               barLength = p.tempo.numerator * 4.0 / p.tempo.denominator;
    const auto barDelta = delta + p.barBeat;
    const auto bars = std::floor(barDelta / barLength);
    return {p.tempo.beat + delta,        p.tempo.bpm,       p.bar + static_cast<std::int64_t>(bars),
            barDelta - bars * barLength, p.tempo.numerator, p.tempo.denominator};
}
Status TempoMap::frameAtBeat(double beat, std::uint64_t& frame) const noexcept {
    if (points_.empty())
        return Status::not_prepared;
    if (!isFinite(beat) || beat < 0)
        return Status::invalid_argument;
    const auto where = std::upper_bound(points_.begin(), points_.end(), beat,
                                        [](double v, const Point& p) { return v < p.tempo.beat; });
    const auto& p = where == points_.begin() ? points_.front() : *(where - 1);
    const auto value = p.frame + (beat - p.tempo.beat) * 60 * rate_ / p.tempo.bpm;
    if (!isFinite(value) || value > 9007199254740991.0)
        return Status::invalid_argument;
    frame = static_cast<std::uint64_t>(std::llround(value));
    return Status::ok;
}
template <Sample T> struct AudioTransport<T>::State {
    ProcessSpec spec;
    AudioSource<T>* source = nullptr;
    VectorOps ops;
    SpscQueue<TransportCommand> commands;
    std::uint64_t frame = 0, loopFirst = 0, loopEnd = 0;
    bool playing = false, loop = false;
    std::uint32_t underruns = 0;
    std::atomic<std::uint32_t> revision{0}, low{0}, high{0}, flags{0}, misses{0};
    void publish() noexcept {
        revision.fetch_add(1, std::memory_order_acq_rel);
        low.store(static_cast<std::uint32_t>(frame), std::memory_order_relaxed);
        high.store(static_cast<std::uint32_t>(frame >> 32), std::memory_order_relaxed);
        flags.store((playing ? 1u : 0u) | (loop ? 2u : 0u), std::memory_order_relaxed);
        misses.store(underruns, std::memory_order_relaxed);
        revision.fetch_add(1, std::memory_order_release);
    }
};
template <Sample T> AudioTransport<T>::AudioTransport() = default;
template <Sample T> AudioTransport<T>::~AudioTransport() = default;
template <Sample T> AudioTransport<T>::AudioTransport(AudioTransport&&) noexcept = default;
template <Sample T>
AudioTransport<T>& AudioTransport<T>::operator=(AudioTransport&&) noexcept = default;
template <Sample T>
Status AudioTransport<T>::prepare(const ProcessSpec& spec, AudioSource<T>& source,
                                  std::size_t capacity, CpuBackend backend) noexcept {
    const auto info = source.info();
    if (!spec.isValid() || info.sampleRate != spec.sampleRate || info.channels != spec.channels)
        return Status::invalid_spec;
    try {
        auto next = std::make_unique<State>();
        next->spec = spec;
        next->source = &source;
        auto status = VectorOps::create(backend, next->ops);
        if (status != Status::ok)
            return status;
        status = next->commands.prepare(capacity);
        if (status != Status::ok)
            return status;
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status AudioTransport<T>::enqueue(const TransportCommand& command) noexcept {
    if (!state_)
        return Status::not_prepared;
    const auto total = state_->source->info().frames;
    if (static_cast<unsigned>(command.action) > static_cast<unsigned>(TransportAction::setLoop) ||
        (command.action == TransportAction::seek && command.first > total) ||
        (command.action == TransportAction::setLoop && command.enabled &&
         (command.first >= command.end || command.end > total)))
        return Status::invalid_argument;
    return state_->commands.push(command) ? Status::ok : Status::update_queue_full;
}
template <Sample T>
Status AudioTransport<T>::process(AudioBlock<T> out, UnderrunPolicy policy) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    const auto valid = validateProcessBlocks<T>(s.spec, out, out);
    if (valid != Status::ok)
        return valid;
    if (static_cast<unsigned>(policy) > 1)
        return Status::invalid_argument;
    TransportCommand command;
    for (std::size_t i = 0; i < s.commands.capacity() && s.commands.pop(command); ++i)
        switch (command.action) {
        case TransportAction::play:
            s.playing = true;
            break;
        case TransportAction::pause:
            s.playing = false;
            break;
        case TransportAction::stop:
            s.playing = false;
            s.frame = 0;
            break;
        case TransportAction::seek:
            s.frame = command.first;
            break;
        case TransportAction::setLoop:
            s.loop = command.enabled;
            s.loopFirst = command.first;
            s.loopEnd = command.end;
            break;
        }
    for (std::size_t ch = 0; ch < out.channels(); ++ch)
        s.ops.fill(out.channel(ch), T{});
    Status result = Status::ok;
    std::size_t offset = 0;
    const auto total = s.source->info().frames;
    while (s.playing && offset < out.frames()) {
        if (s.loop && (s.frame >= s.loopEnd || s.frame < s.loopFirst))
            s.frame = s.loopFirst;
        const auto end = s.loop ? s.loopEnd : total;
        if (s.frame >= end) {
            s.playing = false;
            break;
        }
        const auto n =
            static_cast<std::size_t>(std::min<std::uint64_t>(out.frames() - offset, end - s.frame));
        std::size_t got = 0;
        const auto status = s.source->readAt(s.frame, out.subBlock(offset, n), got);
        if (got > n) {
            result = Status::invalid_argument;
            break;
        }
        s.frame += got;
        offset += got;
        if (status != Status::ok && status != Status::end_of_stream &&
            status != Status::would_block) {
            result = status;
            break;
        }
        if (got < n) {
            if (status == Status::would_block) {
                ++s.underruns;
                if (policy == UnderrunPolicy::silenceAndAdvance) {
                    s.frame += n - got;
                    offset += n - got;
                } else
                    break;
            } else {
                s.playing = false;
                break;
            }
        }
        if (s.loop && s.frame == s.loopEnd)
            s.frame = s.loopFirst;
        if (!s.loop && s.frame >= total)
            s.playing = false;
    }
    s.publish();
    return result;
}
template <Sample T> Status AudioTransport<T>::position(TransportPosition& result) const noexcept {
    if (!state_)
        return Status::not_prepared;
    const auto& s = *state_;
    for (unsigned i = 0; i < 4; ++i) {
        const auto a = s.revision.load(std::memory_order_acquire);
        if (a & 1)
            continue;
        TransportPosition next;
        next.frame = s.low.load(std::memory_order_relaxed) |
                     (std::uint64_t(s.high.load(std::memory_order_relaxed)) << 32);
        const auto flags = s.flags.load(std::memory_order_relaxed);
        next.playing = (flags & 1) != 0;
        next.looping = (flags & 2) != 0;
        next.underruns = s.misses.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        if (s.revision.load(std::memory_order_relaxed) == a) {
            result = next;
            return Status::ok;
        }
    }
    return Status::transition_in_progress;
}
template <Sample T> void AudioTransport<T>::reset() noexcept {
    if (state_) {
        TransportCommand c;
        for (std::size_t n = 0; n < state_->commands.capacity() && state_->commands.pop(c); ++n) {
        }
        state_->frame = 0;
        state_->playing = false;
        state_->underruns = 0;
        state_->publish();
    }
}
template class AudioTransport<float>;
template class AudioTransport<double>;
} // namespace cydsp
