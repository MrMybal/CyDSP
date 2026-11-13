#include "cydsp/synthesis/LiveGranulator.h"
#include "cydsp/core/CircularHistory.h"
#include "cydsp/math/VectorOps.h"
#include "detail/Analysis.h"
#include "detail/HistorySinc.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>
namespace cydsp {
template <Sample T> struct LiveGranulator<T>::State {
    struct Voice {
        LiveGrainParameters p;
        double age = 0;
        std::size_t duration = 0, elapsed = 0, pending = 0;
        bool active = false;
    };
    ProcessSpec spec;
    std::size_t outputs = 0;
    CircularHistory<T> history;
    detail::HistorySinc sinc;
    std::vector<Voice> voices;
    LiveCloudParameters cloud;
    bool frozen = false;
    double since = 0;
    std::uint64_t random = 1, dropped = 0;
    double uniform() noexcept {
        random ^= random >> 12;
        random ^= random << 25;
        random ^= random >> 27;
        return double((random * 2685821657736338717ULL) >> 11) * 0x1.0p-53;
    }
    bool valid(const LiveGrainParameters& p) const noexcept {
        if (!isFinite(p.ageSeconds) || p.ageSeconds < 0 ||
            p.ageSeconds * spec.sampleRate + 32 >= double(history.capacity()) ||
            !isFinite(p.durationMs) || p.durationMs < .1 || p.durationMs > 10000 ||
            !isFinite(p.pitchRatio) || std::abs(p.pitchRatio) < .125 ||
            std::abs(p.pitchRatio) > 4 || !isFinite(p.gain) || std::abs(p.gain) > 16 ||
            p.inputChannel >= spec.channels)
            return false;
        for (double g : p.channelGains)
            if (!isFinite(g) || std::abs(g) > 4)
                return false;
        return true;
    }
    Status trigger(const LiveGrainParameters& p, std::size_t offset) noexcept {
        for (auto& v : voices)
            if (!v.active) {
                v = {};
                v.p = p;
                v.age = p.ageSeconds * spec.sampleRate + 16;
                v.duration = std::max<std::size_t>(
                    1, std::size_t(std::ceil(p.durationMs * spec.sampleRate / 1000)));
                v.pending = offset;
                v.active = true;
                return Status::ok;
            }
        ++dropped;
        return Status::update_queue_full;
    }
    double read(const Voice& v) noexcept {
        return sinc.read(history, v.p.inputChannel, v.age, v.p.pitchRatio);
    }
};
template <Sample T> LiveGranulator<T>::LiveGranulator() = default;
template <Sample T> LiveGranulator<T>::~LiveGranulator() = default;
template <Sample T> LiveGranulator<T>::LiveGranulator(LiveGranulator&&) noexcept = default;
template <Sample T>
LiveGranulator<T>& LiveGranulator<T>::operator=(LiveGranulator&&) noexcept = default;
template <Sample T>
Status LiveGranulator<T>::prepare(const ProcessSpec& spec, std::size_t outputs, double seconds,
                                  std::size_t maximum, CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || !outputs || outputs > 8 || !isFinite(seconds) ||
        seconds < .05 || seconds > 20 || !maximum || maximum > 1024 ||
        seconds * spec.sampleRate > (1u << 24) - 64)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->outputs = outputs;
        s->voices.resize(maximum);
        auto status = s->history.prepare(spec.channels,
                                         std::size_t(std::ceil(seconds * spec.sampleRate)) + 32);
        if (status != Status::ok)
            return status;
        status = s->sinc.prepare(backend);
        if (status != Status::ok)
            return status;
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status LiveGranulator<T>::trigger(const LiveGrainParameters& p, std::size_t offset) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!state_->valid(p) || offset > (1u << 30))
        return Status::invalid_argument;
    return state_->trigger(p, offset);
}
template <Sample T> Status LiveGranulator<T>::setCloud(const LiveCloudParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (!s.valid(p.grain) || !isFinite(p.grainsPerSecond) || p.grainsPerSecond < 0 ||
        p.grainsPerSecond > 1000 || !isFinite(p.ageSpreadSeconds) || p.ageSpreadSeconds < 0 ||
        p.ageSpreadSeconds > s.history.capacity() / s.spec.sampleRate ||
        !isFinite(p.pitchSpreadSemitones) || p.pitchSpreadSemitones < 0 ||
        p.pitchSpreadSemitones > 24)
        return Status::invalid_argument;
    s.cloud = p;
    return Status::ok;
}
template <Sample T> void LiveGranulator<T>::setFrozen(bool frozen) noexcept {
    if (state_)
        state_->frozen = frozen;
}
template <Sample T>
Status LiveGranulator<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    if (!out.isValid() || out.frames() != in.frames())
        return Status::invalid_argument;
    if (out.channels() != s.outputs)
        return Status::channel_mismatch;
    status = validateDisjointBlocks(in, out);
    if (status != Status::ok)
        return status;
    std::array<T, 64> frame{};
    for (std::size_t i = 0; i < in.frames(); ++i) {
        if (!s.frozen) {
            for (std::size_t ch = 0; ch < in.channels(); ++ch)
                frame[ch] = in.channel(ch)[i];
            (void)s.history.pushFrame({frame.data(), in.channels()});
        }
        for (std::size_t ch = 0; ch < out.channels(); ++ch)
            out.channel(ch)[i] = T{};
        s.since += s.cloud.grainsPerSecond / s.spec.sampleRate;
        if (s.since >= 1) {
            s.since -= 1;
            auto p = s.cloud.grain;
            p.ageSeconds =
                std::clamp(p.ageSeconds + (2 * s.uniform() - 1) * s.cloud.ageSpreadSeconds, 0.,
                           (double(s.history.capacity()) - 33) / s.spec.sampleRate);
            p.pitchRatio = std::copysign(
                std::clamp(std::abs(p.pitchRatio) *
                               std::exp2((2 * s.uniform() - 1) * s.cloud.pitchSpreadSemitones / 12),
                           .125, 4.),
                p.pitchRatio);
            (void)s.trigger(p, 0);
        }
        for (auto& v : s.voices)
            if (v.active) {
                if (v.pending) {
                    --v.pending;
                    continue;
                }
                if (v.age < 15 || v.age + 17 >= double(s.history.availableFrames())) {
                    v.active = false;
                    continue;
                }
                const double phase =
                                 v.duration == 1 ? .5 : double(v.elapsed) / double(v.duration - 1),
                             window = .5 - .5 * std::cos(2 * std::numbers::pi * phase);
                const double sample = s.read(v) * window * v.p.gain;
                for (std::size_t ch = 0; ch < out.channels(); ++ch)
                    out.channel(ch)[i] += T(sample * v.p.channelGains[ch]);
                v.age += (s.frozen ? 0 : 1) - v.p.pitchRatio;
                if (++v.elapsed >= v.duration)
                    v.active = false;
            }
    }
    return Status::ok;
}
template <Sample T> void LiveGranulator<T>::reset(std::uint64_t seed) noexcept {
    if (state_) {
        auto& s = *state_;
        s.history.reset();
        for (auto& v : s.voices)
            v.active = false;
        s.random = seed ? seed : 1;
        s.since = 0;
        s.dropped = 0;
        s.frozen = false;
    }
}
template <Sample T> std::size_t LiveGranulator<T>::activeGrains() const noexcept {
    if (!state_)
        return 0;
    return std::count_if(state_->voices.begin(), state_->voices.end(),
                         [](const auto& v) { return v.active; });
}
template <Sample T> std::uint64_t LiveGranulator<T>::droppedGrains() const noexcept {
    return state_ ? state_->dropped : 0;
}
template class LiveGranulator<float>;
template class LiveGranulator<double>;
} // namespace cydsp
