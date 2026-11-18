#include "cydsp/spatial/AcousticPaths.h"
#include "cydsp/core/CircularHistory.h"
#include "detail/Analysis.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>
namespace cydsp {
template <Sample T> struct AcousticPathRenderer<T>::State {
    struct Slot {
        AcousticPath path;
        bool active = false, queued = false;
        double delay = 0, nextDelay = 0, queuedDelay = 0, low = 0, high = 0;
        std::array<double, 3> gain{}, target{};
        std::size_t gainRemaining = 0, delayRemaining = 0, delayTotal = 0, queuedTotal = 0;
    };
    ProcessSpec spec;
    std::vector<Slot> slots;
    CircularHistory<T> history;
    double maximumDelay = 0, lowPole = 0, highPole = 0;
    double readDelay(double frames) const noexcept {
        // The centered cubic stencil needs one sample ahead of the integer delay.
        const auto interpolation =
            frames < 1 ? HistoryInterpolation::linear : HistoryInterpolation::lagrange3;
        return double(history.read(0, frames, interpolation));
    }
};
template <Sample T> AcousticPathRenderer<T>::AcousticPathRenderer() = default;
template <Sample T> AcousticPathRenderer<T>::~AcousticPathRenderer() = default;
template <Sample T>
AcousticPathRenderer<T>::AcousticPathRenderer(AcousticPathRenderer&&) noexcept = default;
template <Sample T>
AcousticPathRenderer<T>&
AcousticPathRenderer<T>::operator=(AcousticPathRenderer&&) noexcept = default;
template <Sample T>
Status AcousticPathRenderer<T>::prepare(const ProcessSpec& spec, std::size_t maximum,
                                        double delay) noexcept {
    if (!detail::validAnalysisSpec(spec) || !maximum || maximum > 64 || !isFinite(delay) ||
        delay < 0 || delay > 10)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->maximumDelay = delay;
        s->slots.resize(maximum);
        auto status = s->history.prepare(1, std::size_t(std::ceil(delay * spec.sampleRate)) + 4);
        if (status != Status::ok)
            return status;
        s->lowPole = std::exp(-2 * std::numbers::pi * std::min(400., spec.sampleRate * .1) /
                              spec.sampleRate);
        s->highPole = std::exp(-2 * std::numbers::pi * std::min(4000., spec.sampleRate * .4) /
                               spec.sampleRate);
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status AcousticPathRenderer<T>::setPaths(std::span<const AcousticPath> paths,
                                         std::size_t transition) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (paths.size() > s.slots.size() || transition > (1u << 30))
        return Status::invalid_argument;
    std::array<std::size_t, 64> mapping{};
    std::array<bool, 64> assigned{};
    for (std::size_t i = 0; i < paths.size(); ++i) {
        const auto& p = paths[i];
        if (!p.id || !isFinite(p.delaySeconds) || p.delaySeconds < 0 ||
            p.delaySeconds > s.maximumDelay || !isFinite(p.arrivalDirection.x) ||
            !isFinite(p.arrivalDirection.y) || !isFinite(p.arrivalDirection.z) ||
            length(p.arrivalDirection) < 1e-9 || length(p.arrivalDirection) > 1e6)
            return Status::invalid_argument;
        for (double gain : p.bandGains)
            if (!isFinite(gain) || std::abs(gain) > 64)
                return Status::invalid_argument;
        for (std::size_t j = 0; j < i; ++j)
            if (paths[j].id == p.id)
                return Status::invalid_argument;
        mapping[i] = s.slots.size();
        for (std::size_t slot = 0; slot < s.slots.size(); ++slot)
            if (s.slots[slot].active && s.slots[slot].path.id == p.id) {
                mapping[i] = slot;
                assigned[slot] = true;
                break;
            }
    }
    for (std::size_t i = 0; i < paths.size(); ++i)
        if (mapping[i] == s.slots.size()) {
            for (std::size_t slot = 0; slot < s.slots.size(); ++slot)
                if (!assigned[slot] && !s.slots[slot].active) {
                    mapping[i] = slot;
                    assigned[slot] = true;
                    break;
                }
            if (mapping[i] == s.slots.size())
                return Status::update_queue_full;
        }
    for (std::size_t slot = 0; slot < s.slots.size(); ++slot)
        if (s.slots[slot].active && !assigned[slot]) {
            auto& p = s.slots[slot];
            p.target = {};
            p.gainRemaining = transition;
            if (!transition) {
                p.gain = {};
                p.active = false;
            }
        }
    for (std::size_t i = 0; i < paths.size(); ++i) {
        auto& slot = s.slots[mapping[i]];
        const auto& p = paths[i];
        const double delay = p.delaySeconds * s.spec.sampleRate;
        if (!slot.active) {
            slot = {};
            slot.active = true;
            slot.delay = slot.nextDelay = delay;
        }
        slot.path = p;
        slot.path.arrivalDirection = p.arrivalDirection * (1 / length(p.arrivalDirection));
        slot.target = p.bandGains;
        slot.gainRemaining = transition;
        if (!transition) {
            slot.gain = slot.target;
            slot.delay = slot.nextDelay = delay;
            slot.delayRemaining = 0;
            slot.queued = false;
        } else if (slot.delayRemaining) {
            slot.queuedDelay = delay;
            slot.queuedTotal = transition;
            slot.queued = delay != slot.nextDelay;
        } else if (delay != slot.delay) {
            slot.nextDelay = delay;
            slot.delayRemaining = slot.delayTotal = transition;
        }
    }
    return Status::ok;
}
template <Sample T>
Status AcousticPathRenderer<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = detail::validateAnalysis(s.spec, input);
    if (status != Status::ok)
        return status;
    if (!output.isValid() || output.frames() != input.frames())
        return Status::invalid_argument;
    if (output.channels() != s.slots.size())
        return Status::channel_mismatch;
    status = validateDisjointBlocks(input, output);
    if (status != Status::ok)
        return status;
    for (std::size_t ch = 0; ch < output.channels(); ++ch)
        std::fill(output.channel(ch).begin(), output.channel(ch).end(), T{});
    for (std::size_t i = 0; i < input.frames(); ++i) {
        double mono = 0;
        for (std::size_t ch = 0; ch < input.channels(); ++ch)
            mono += double(input.channel(ch)[i]);
        const T sample = T(mono / double(input.channels()));
        (void)s.history.pushFrame({&sample, 1});
        for (std::size_t index = 0; index < s.slots.size(); ++index) {
            auto& p = s.slots[index];
            if (!p.active)
                continue;
            if (p.gainRemaining) {
                const double step = 1 / double(p.gainRemaining);
                for (std::size_t b = 0; b < 3; ++b)
                    p.gain[b] += (p.target[b] - p.gain[b]) * step;
                --p.gainRemaining;
            }
            double value = s.readDelay(p.delay);
            if (p.delayRemaining) {
                const double mix =
                    double(p.delayTotal - p.delayRemaining + 1) / double(p.delayTotal);
                value = value * (1 - mix) + s.readDelay(p.nextDelay) * mix;
                if (!--p.delayRemaining) {
                    p.delay = p.nextDelay;
                    if (p.queued) {
                        p.nextDelay = p.queuedDelay;
                        p.delayRemaining = p.delayTotal = p.queuedTotal;
                        p.queued = false;
                    }
                }
            }
            p.low = value + s.lowPole * (p.low - value);
            p.high = value + s.highPole * (p.high - value);
            output.channel(index)[i] = T(p.gain[2] * value + (p.gain[1] - p.gain[2]) * p.high +
                                         (p.gain[0] - p.gain[1]) * p.low);
            if (!p.gainRemaining && p.target == std::array<double, 3>{})
                p.active = false;
        }
    }
    return Status::ok;
}
template <Sample T> void AcousticPathRenderer<T>::reset() noexcept {
    if (state_) {
        auto& s = *state_;
        s.history.reset();
        for (auto& p : s.slots) {
            p.low = p.high = 0;
            p.gain = p.target;
            p.delay = p.nextDelay =
                p.queued ? p.queuedDelay : p.path.delaySeconds * s.spec.sampleRate;
            p.queued = false;
            p.gainRemaining = p.delayRemaining = 0;
            if (p.target == std::array<double, 3>{})
                p.active = false;
        }
    }
}
template <Sample T> std::size_t AcousticPathRenderer<T>::maximumPaths() const noexcept {
    return state_ ? state_->slots.size() : 0;
}
template <Sample T> std::size_t AcousticPathRenderer<T>::activePaths() const noexcept {
    return state_ ? std::size_t(std::count_if(state_->slots.begin(), state_->slots.end(),
                                              [](const auto& s) { return s.active; }))
                  : 0;
}
template <Sample T> AcousticPath AcousticPathRenderer<T>::path(std::size_t slot) const noexcept {
    if (!state_ || slot >= state_->slots.size() || !state_->slots[slot].active)
        return {0};
    auto p = state_->slots[slot].path;
    p.bandGains = state_->slots[slot].gain;
    return p;
}
template class AcousticPathRenderer<float>;
template class AcousticPathRenderer<double>;
} // namespace cydsp
