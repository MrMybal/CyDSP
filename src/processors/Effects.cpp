#include "cydsp/processors/Effects.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <vector>
namespace cydsp {
namespace {
bool effectSpec(const ProcessSpec& spec) {
    return spec.isValid() && spec.sampleRate >= 8000 && spec.sampleRate <= 768000 &&
           spec.channels <= 256;
}
bool range(double x, double lo, double hi) {
    return isFinite(x) && x >= lo && x <= hi;
}
struct Ring {
    std::vector<double> data;
    std::size_t position = 0;
    double low = 0, gain = 0;
    void reset() {
        std::fill(data.begin(), data.end(), 0);
        position = 0;
        low = 0;
    }
    double comb(double x, double damping) {
        const double y = data[position];
        low = (1 - damping) * y + damping * low;
        data[position] = x + gain * low;
        if (++position == data.size())
            position = 0;
        return y;
    }
    double allpass(double x) {
        const double delayed = data[position], y = delayed - .5 * x;
        data[position] = x + .5 * y;
        if (++position == data.size())
            position = 0;
        return y;
    }
};
} // namespace
#define CYDSP_EFFECT_LIFETIME(Name)                                                                \
    template <Sample T> Name<T>::Name() = default;                                                 \
    template <Sample T> Name<T>::~Name() = default;                                                \
    template <Sample T> Name<T>::Name(Name&&) noexcept = default;                                  \
    template <Sample T> Name<T>& Name<T>::operator=(Name&&) noexcept = default;
template <Sample T> struct Chorus<T>::State {
    ProcessSpec spec;
    ChorusParameters p;
    std::vector<T> history;
    std::vector<double> last;
    std::size_t ring = 0, position = 0;
    double phase = 0, maxMs = 0;
};
CYDSP_EFFECT_LIFETIME(Chorus)
template <Sample T> Status Chorus<T>::prepare(const ProcessSpec& spec, double maxMs) noexcept {
    if (!effectSpec(spec) || !range(maxMs, 1, 2000))
        return Status::invalid_spec;
    try {
        auto next = std::make_unique<State>();
        next->spec = spec;
        next->maxMs = maxMs;
        next->p.centreDelayMs = std::min(7.0, maxMs / 2);
        next->ring = static_cast<std::size_t>(std::ceil(spec.sampleRate * maxMs / 1000)) + 2;
        next->history.resize(spec.channels * next->ring);
        next->last.resize(spec.channels);
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_spec;
    }
}
template <Sample T> Status Chorus<T>::setParameters(const ChorusParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!range(p.rateHz, 0, 20) || !range(p.depth, 0, 1) ||
        !range(p.centreDelayMs, 0, state_->maxMs) ||
        p.centreDelayMs * (1 + .95 * p.depth) > state_->maxMs || !range(p.feedback, -.99, .99) ||
        !range(p.wet, 0, 1))
        return Status::invalid_argument;
    state_->p = p;
    return Status::ok;
}
template <Sample T> Status Chorus<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, in, out);
    if (status != Status::ok)
        return status;
    for (std::size_t i = 0; i < in.frames(); ++i) {
        const double delay = std::clamp(s.p.centreDelayMs * s.spec.sampleRate / 1000 *
                                            (1 + .95 * s.p.depth * std::sin(s.phase)),
                                        1.0, static_cast<double>(s.ring - 2));
        const auto whole = static_cast<std::size_t>(delay);
        const auto fraction = delay - static_cast<double>(whole);
        const auto a = (s.position + s.ring - whole) % s.ring, b = (a + s.ring - 1) % s.ring;
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            auto* h = s.history.data() + ch * s.ring;
            const double x = in.channel(ch)[i],
                         y = h[a] + (static_cast<double>(h[b]) - h[a]) * fraction;
            h[s.position] = static_cast<T>(x + s.p.feedback * s.last[ch]);
            s.last[ch] = y;
            out.channel(ch)[i] = static_cast<T>((1 - s.p.wet) * x + s.p.wet * y);
        }
        if (++s.position == s.ring)
            s.position = 0;
        s.phase += 2 * std::numbers::pi * s.p.rateHz / s.spec.sampleRate;
        if (s.phase >= 2 * std::numbers::pi)
            s.phase -= 2 * std::numbers::pi;
    }
    return Status::ok;
}
template <Sample T> void Chorus<T>::reset() noexcept {
    if (state_) {
        std::fill(state_->history.begin(), state_->history.end(), T{});
        std::fill(state_->last.begin(), state_->last.end(), 0);
        state_->position = 0;
        state_->phase = 0;
    }
}
template <Sample T> struct Phaser<T>::State {
    ProcessSpec spec;
    PhaserParameters p;
    std::vector<std::array<double, 6>> memories;
    std::vector<double> last;
    double phase = 0;
};
CYDSP_EFFECT_LIFETIME(Phaser)
template <Sample T> Status Phaser<T>::prepare(const ProcessSpec& spec) noexcept {
    if (!effectSpec(spec))
        return Status::invalid_spec;
    try {
        auto next = std::make_unique<State>();
        next->spec = spec;
        next->memories.resize(spec.channels);
        next->last.resize(spec.channels);
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_spec;
    }
}
template <Sample T> Status Phaser<T>::setParameters(const PhaserParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!range(p.rateHz, 0, 20) || !range(p.depth, 0, 1) ||
        !range(p.centreHz, 10, state_->spec.sampleRate * .45) || !range(p.feedback, -.99, .99) ||
        !range(p.wet, 0, 1))
        return Status::invalid_argument;
    state_->p = p;
    return Status::ok;
}
template <Sample T> Status Phaser<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, in, out);
    if (status != Status::ok)
        return status;
    for (std::size_t i = 0; i < in.frames(); ++i) {
        const auto hz = std::clamp(s.p.centreHz * std::exp2(3 * s.p.depth * std::sin(s.phase)),
                                   10.0, s.spec.sampleRate * .45);
        const auto g = std::tan(std::numbers::pi * hz / s.spec.sampleRate), a = (g - 1) / (g + 1);
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            const double x = in.channel(ch)[i];
            double y = x + s.p.feedback * s.last[ch];
            for (auto& memory : s.memories[ch]) {
                const auto output = a * y + memory;
                memory = y - a * output;
                y = output;
            }
            s.last[ch] = y;
            out.channel(ch)[i] = static_cast<T>((1 - s.p.wet) * x + s.p.wet * y);
        }
        s.phase += 2 * std::numbers::pi * s.p.rateHz / s.spec.sampleRate;
        if (s.phase >= 2 * std::numbers::pi)
            s.phase -= 2 * std::numbers::pi;
    }
    return Status::ok;
}
template <Sample T> void Phaser<T>::reset() noexcept {
    if (state_) {
        std::fill(state_->memories.begin(), state_->memories.end(), std::array<double, 6>{});
        std::fill(state_->last.begin(), state_->last.end(), 0);
        state_->phase = 0;
    }
}
template <Sample T> struct Reverb<T>::State {
    ProcessSpec spec;
    ReverbParameters p;
    struct Channel {
        std::array<Ring, 8> combs;
        std::array<Ring, 4> allpasses;
    };
    std::vector<Channel> channels;
    std::vector<double> wet, dry;
};
CYDSP_EFFECT_LIFETIME(Reverb)
template <Sample T> Status Reverb<T>::prepare(const ProcessSpec& spec) noexcept {
    if (!effectSpec(spec))
        return Status::invalid_spec;
    try {
        auto next = std::make_unique<State>();
        next->spec = spec;
        next->channels.resize(spec.channels);
        next->wet.resize(spec.channels);
        next->dry.resize(spec.channels);
        // Independent prime-like delay lengths at 48 kHz, decorrelated across channels.
        constexpr std::array<unsigned, 8> comb{1429, 1601, 1789, 1999, 2203, 2411, 2671, 2953};
        constexpr std::array<unsigned, 4> ap{149, 211, 263, 331};
        for (std::size_t ch = 0; ch < spec.channels; ++ch) {
            auto& channel = next->channels[ch];
            for (std::size_t n = 0; n < comb.size(); ++n) {
                auto& r = channel.combs[n];
                r.data.resize(static_cast<std::size_t>(
                    std::ceil((comb[n] + ch * 17) * spec.sampleRate / 48000)));
                r.gain = std::pow(.001, static_cast<double>(r.data.size()) /
                                            (spec.sampleRate * next->p.decaySeconds));
            }
            for (std::size_t n = 0; n < ap.size(); ++n)
                channel.allpasses[n].data.resize(static_cast<std::size_t>(
                    std::ceil((ap[n] + ch * 7) * spec.sampleRate / 48000)));
        }
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_spec;
    }
}
template <Sample T> Status Reverb<T>::setParameters(const ReverbParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!range(p.decaySeconds, .05, 60) || !range(p.damping, 0, .99) || !range(p.wet, 0, 1) ||
        !range(p.width, 0, 1))
        return Status::invalid_argument;
    state_->p = p;
    for (auto& ch : state_->channels)
        for (auto& comb : ch.combs)
            comb.gain = std::pow(.001, static_cast<double>(comb.data.size()) /
                                           (state_->spec.sampleRate * p.decaySeconds));
    return Status::ok;
}
template <Sample T> Status Reverb<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, in, out);
    if (status != Status::ok)
        return status;
    for (std::size_t i = 0; i < in.frames(); ++i) {
        double mean = 0;
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            const double x = in.channel(ch)[i];
            s.dry[ch] = x;
            double y = 0;
            for (auto& comb : s.channels[ch].combs)
                y += comb.comb(x, s.p.damping) / 8;
            for (auto& ap : s.channels[ch].allpasses)
                y = ap.allpass(y);
            s.wet[ch] = y;
            mean += y;
        }
        mean /= static_cast<double>(in.channels());
        for (std::size_t ch = 0; ch < out.channels(); ++ch)
            out.channel(ch)[i] =
                static_cast<T>((1 - s.p.wet) * s.dry[ch] +
                               s.p.wet * (s.p.width * s.wet[ch] + (1 - s.p.width) * mean));
    }
    return Status::ok;
}
template <Sample T> void Reverb<T>::reset() noexcept {
    if (state_)
        for (auto& ch : state_->channels) {
            for (auto& r : ch.combs)
                r.reset();
            for (auto& r : ch.allpasses)
                r.reset();
        }
}
#undef CYDSP_EFFECT_LIFETIME
template class Chorus<float>;
template class Chorus<double>;
template class Phaser<float>;
template class Phaser<double>;
template class Reverb<float>;
template class Reverb<double>;
} // namespace cydsp
