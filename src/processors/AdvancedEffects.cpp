#include "cydsp/processors/AdvancedEffects.h"
#include "cydsp/core/CircularHistory.h"
#include "detail/Analysis.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>
namespace cydsp {
namespace {
template <Sample T>
Status checked(const ProcessSpec& s, AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    auto status = validateProcessBlocks(s, in, out);
    return status == Status::ok ? detail::validateAnalysis(s, in) : status;
}
struct DelayCell {
    std::vector<double> ring;
    std::size_t head = 0;
    double delay(double x) noexcept {
        double y = ring[head];
        ring[head] = x;
        if (++head == ring.size())
            head = 0;
        return y;
    }
    double allpass(double x, double g) noexcept {
        const double z = ring[head], y = z - g * x;
        ring[head] = x + g * y;
        if (++head == ring.size())
            head = 0;
        return y;
    }
    void reset() noexcept {
        std::fill(ring.begin(), ring.end(), 0);
        head = 0;
    }
};
} // namespace
template <Sample T> struct DelayEffect<T>::State {
    ProcessSpec spec;
    DelayEffectParameters p;
    CircularHistory<T> history;
    std::array<EchoTap, 16> taps{};
    std::size_t count = 0;
    double maxMs = 0, phase = 0, flutter = 0, duck = 0, pole = 0;
    std::vector<double> low;
};
template <Sample T> DelayEffect<T>::DelayEffect() = default;
template <Sample T> DelayEffect<T>::~DelayEffect() = default;
template <Sample T> DelayEffect<T>::DelayEffect(DelayEffect&&) noexcept = default;
template <Sample T> DelayEffect<T>& DelayEffect<T>::operator=(DelayEffect&&) noexcept = default;
template <Sample T>
Status DelayEffect<T>::prepare(const ProcessSpec& spec, double maximum) noexcept {
    if (!detail::validAnalysisSpec(spec) || !isFinite(maximum) || maximum < 1 || maximum > 10000)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->maxMs = maximum;
        s->low.resize(spec.channels);
        s->p.delayMs = std::min(250., maximum);
        s->p.lowPassHz = std::min(10000., spec.sampleRate * .4);
        s->pole = std::exp(-2 * std::numbers::pi * s->p.lowPassHz / spec.sampleRate);
        auto status = s->history.prepare(
            spec.channels, std::size_t(std::ceil(maximum * spec.sampleRate / 1000)) + 8);
        if (status != Status::ok)
            return status;
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status DelayEffect<T>::setParameters(const DelayEffectParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (unsigned(p.mode) > unsigned(DelayEffectMode::wowFlutter) || !isFinite(p.delayMs) ||
        !isFinite(p.depthMs) || p.depthMs < 0 || p.delayMs - p.depthMs < 2000 / s.spec.sampleRate ||
        p.delayMs + p.depthMs > s.maxMs || !isFinite(p.rateHz) || p.rateHz < 0 || p.rateHz > 100 ||
        !isFinite(p.feedback) || std::abs(p.feedback) >= .999 || !isFinite(p.lowPassHz) ||
        p.lowPassHz < 10 || p.lowPassHz >= s.spec.sampleRate * .5 || !isFinite(p.feedbackDrive) ||
        p.feedbackDrive < .01 || p.feedbackDrive > 64 || !isFinite(p.duckDb) || p.duckDb < 0 ||
        p.duckDb > 60 || !isFinite(p.wet) || std::abs(p.wet) > 4 || !isFinite(p.dry) ||
        std::abs(p.dry) > 4 || (p.pingPong && s.spec.channels != 2))
        return Status::invalid_argument;
    s.p = p;
    s.pole = std::exp(-2 * std::numbers::pi * p.lowPassHz / s.spec.sampleRate);
    return Status::ok;
}
template <Sample T> Status DelayEffect<T>::setTaps(std::span<const EchoTap> taps) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (taps.size() > 16)
        return Status::invalid_argument;
    for (auto t : taps)
        if (!isFinite(t.delayMs) || t.delayMs < 2000 / s.spec.sampleRate || t.delayMs > s.maxMs ||
            !isFinite(t.gain) || std::abs(t.gain) > 16)
            return Status::invalid_argument;
    std::copy(taps.begin(), taps.end(), s.taps.begin());
    s.count = taps.size();
    return Status::ok;
}
template <Sample T>
Status DelayEffect<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = checked(s.spec, in, out);
    if (status != Status::ok)
        return status;
    std::array<T, 64> frame{};
    std::array<double, 64> wet{}, feedback{}, dry{};
    const auto& p = s.p;
    for (std::size_t i = 0; i < in.frames(); ++i) {
        double level = 0;
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            dry[ch] = in.channel(ch)[i];
            level = std::max(level, std::abs(dry[ch]));
        }
        const double envPole = level > s.duck ? .01 : .0002;
        s.duck += (level - s.duck) * envPole;
        const double duck = std::pow(10., -p.duckDb * std::clamp(s.duck, 0., 1.) / 20);
        const double modulation = p.mode == DelayEffectMode::wowFlutter
                                      ? .8 * std::sin(2 * std::numbers::pi * s.phase) +
                                            .2 * std::sin(2 * std::numbers::pi * s.flutter)
                                      : std::sin(2 * std::numbers::pi * s.phase);
        const double delay = (p.delayMs + p.depthMs * modulation) * s.spec.sampleRate / 1000;
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            const auto source = p.pingPong ? 1 - ch : ch;
            feedback[ch] = s.history.read(source, delay - 1, HistoryInterpolation::lagrange3);
            s.low[ch] = (1 - s.pole) * feedback[ch] + s.pole * s.low[ch];
            feedback[ch] = std::tanh(s.low[ch] * p.feedbackDrive) / p.feedbackDrive;
            wet[ch] = s.history.read(ch, delay - 1, HistoryInterpolation::lagrange3);
            if (s.count) {
                wet[ch] = 0;
                for (std::size_t t = 0; t < s.count; ++t)
                    wet[ch] += s.taps[t].gain *
                               s.history.read(ch, s.taps[t].delayMs * s.spec.sampleRate / 1000 - 1,
                                              HistoryInterpolation::lagrange3);
            }
            frame[ch] = T(dry[ch] + p.feedback * feedback[ch]);
        }
        (void)s.history.pushFrame({frame.data(), in.channels()});
        for (std::size_t ch = 0; ch < in.channels(); ++ch)
            out.channel(ch)[i] = T(p.dry * dry[ch] + p.wet * duck * wet[ch]);
        s.phase += p.rateHz / s.spec.sampleRate;
        s.phase -= std::floor(s.phase);
        s.flutter += 8.13 / s.spec.sampleRate;
        s.flutter -= std::floor(s.flutter);
    }
    return Status::ok;
}
template <Sample T> void DelayEffect<T>::reset() noexcept {
    if (state_) {
        auto& s = *state_;
        s.history.reset();
        s.phase = s.flutter = s.duck = 0;
        std::fill(s.low.begin(), s.low.end(), 0);
    }
}
template <Sample T> Status AmplitudeModulator<T>::prepare(const ProcessSpec& spec) noexcept {
    if (!detail::validAnalysisSpec(spec))
        return Status::invalid_spec;
    spec_ = spec;
    prepared_ = true;
    phase_ = 0;
    p_ = {};
    return Status::ok;
}
template <Sample T>
Status AmplitudeModulator<T>::setParameters(const AmplitudeModulationParameters& p) noexcept {
    if (!prepared_)
        return Status::not_prepared;
    if (!isFinite(p.rateHz) || p.rateHz < 0 || p.rateHz > 100 || !isFinite(p.depth) ||
        p.depth < 0 || p.depth > 1 || (p.autopan && spec_.channels != 2))
        return Status::invalid_argument;
    p_ = p;
    return Status::ok;
}
template <Sample T>
Status AmplitudeModulator<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!prepared_)
        return Status::not_prepared;
    auto status = checked(spec_, in, out);
    if (status != Status::ok)
        return status;
    for (std::size_t i = 0; i < in.frames(); ++i) {
        const double lfo = std::sin(2 * std::numbers::pi * phase_);
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            const double gain =
                p_.autopan ? std::sqrt(std::max(0., 1 + (ch == 0 ? -1 : 1) * p_.depth * lfo))
                           : 1 - p_.depth * .5 * (1 + lfo);
            out.channel(ch)[i] = T(in.channel(ch)[i] * gain);
        }
        phase_ += p_.rateHz / spec_.sampleRate;
        phase_ -= std::floor(phase_);
    }
    return Status::ok;
}

template <Sample T> struct PlateReverb<T>::State {
    ProcessSpec spec;
    PlateParameters p;
    std::array<DelayCell, 4> input;
    struct Tank {
        DelayCell a, b, c, d;
        double low = 0, feedback = 0;
    };
    std::array<Tank, 2> tank;
    CircularHistory<double> shimmer;
    double phase = 0, pole = 0;
};
template <Sample T> PlateReverb<T>::PlateReverb() = default;
template <Sample T> PlateReverb<T>::~PlateReverb() = default;
template <Sample T> PlateReverb<T>::PlateReverb(PlateReverb&&) noexcept = default;
template <Sample T> PlateReverb<T>& PlateReverb<T>::operator=(PlateReverb&&) noexcept = default;
template <Sample T> Status PlateReverb<T>::prepare(const ProcessSpec& spec, double size) noexcept {
    if (!detail::validAnalysisSpec(spec) || spec.channels > 2 || !isFinite(size) || size < .25 ||
        size > 4)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        const double scale = spec.sampleRate * size;
        const std::array<double, 4> diff{.0047, .0036, .0127, .0093};
        for (std::size_t i = 0; i < 4; ++i)
            s->input[i].ring.resize(std::max<std::size_t>(1, std::size_t(diff[i] * scale)));
        for (std::size_t i = 0; i < 2; ++i) {
            auto& t = s->tank[i];
            t.a.ring.resize(std::size_t((.0227 + .007 * i) * scale) + 1);
            t.b.ring.resize(std::size_t((.1493 - .021 * i) * scale) + 1);
            t.c.ring.resize(std::size_t((.0605 + .019 * i) * scale) + 1);
            t.d.ring.resize(std::size_t((.1241 - .014 * i) * scale) + 1);
        }
        s->p.dampingHz = std::min(6000., spec.sampleRate * .4);
        s->pole = std::exp(-2 * std::numbers::pi * s->p.dampingHz / spec.sampleRate);
        auto status = s->shimmer.prepare(2, std::size_t(spec.sampleRate * .1) + 8);
        if (status != Status::ok)
            return status;
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status PlateReverb<T>::setParameters(const PlateParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (!isFinite(p.decay) || p.decay < 0 || p.decay >= .9999 || !isFinite(p.diffusion) ||
        p.diffusion < 0 || p.diffusion > .95 || !isFinite(p.dampingHz) || p.dampingHz < 20 ||
        p.dampingHz >= s.spec.sampleRate * .5 || !isFinite(p.wet) || std::abs(p.wet) > 4 ||
        !isFinite(p.dry) || std::abs(p.dry) > 4 || !isFinite(p.shimmer) || p.shimmer < 0 ||
        p.shimmer > 1)
        return Status::invalid_argument;
    s.p = p;
    s.pole = std::exp(-2 * std::numbers::pi * p.dampingHz / s.spec.sampleRate);
    return Status::ok;
}
template <Sample T>
Status PlateReverb<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = checked(s.spec, in, out);
    if (status != Status::ok)
        return status;
    const auto& p = s.p;
    for (std::size_t i = 0; i < in.frames(); ++i) {
        std::array<double, 2> dry{};
        double mono = 0;
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            dry[ch] = in.channel(ch)[i];
            mono += dry[ch] / double(in.channels());
        }
        double x = p.freeze ? 0 : mono;
        for (auto& d : s.input)
            x = d.allpass(x, p.diffusion);
        const std::array<double, 2> old{s.tank[0].feedback, s.tank[1].feedback};
        std::array<double, 2> next{};
        for (std::size_t ch = 0; ch < 2; ++ch) {
            auto& t = s.tank[ch];
            const double phase = std::fmod(s.phase + ch * .5, 1.);
            const double other = std::fmod(phase + .5, 1.);
            const double a = 2 + (.04 * (1 - phase)) * s.spec.sampleRate,
                         b = 2 + (.04 * (1 - other)) * s.spec.sampleRate;
            const double w = .5 - .5 * std::cos(2 * std::numbers::pi * phase);
            const double shifted = w * s.shimmer.read(ch, a, HistoryInterpolation::linear) +
                                   (1 - w) * s.shimmer.read(ch, b, HistoryInterpolation::linear);
            double y =
                x + (p.freeze ? 1 : p.decay) * ((p.freeze ? 0 : p.shimmer) * shifted +
                                                (p.freeze ? 1 : 1 - p.shimmer) * old[1 - ch]);
            y = t.a.allpass(y, -p.diffusion);
            y = t.b.delay(y);
            t.low = (1 - s.pole) * y + s.pole * t.low;
            if (!p.freeze)
                y = t.low;
            y = t.c.allpass(y, p.diffusion * .7);
            next[ch] = t.d.delay(y);
            t.feedback = next[ch];
        }
        (void)s.shimmer.pushFrame(next);
        s.phase += 1 / (.04 * s.spec.sampleRate);
        s.phase -= std::floor(s.phase);
        for (std::size_t ch = 0; ch < out.channels(); ++ch)
            out.channel(ch)[i] =
                T(p.dry * dry[ch] +
                  p.wet * (out.channels() == 1 ? .5 * (next[0] + next[1]) : next[ch]));
    }
    return Status::ok;
}
template <Sample T> void PlateReverb<T>::reset() noexcept {
    if (state_) {
        auto& s = *state_;
        for (auto& d : s.input)
            d.reset();
        for (auto& t : s.tank) {
            t.a.reset();
            t.b.reset();
            t.c.reset();
            t.d.reset();
            t.low = t.feedback = 0;
        }
        s.shimmer.reset();
        s.phase = 0;
    }
}

template <Sample T> struct SpringReverb<T>::State {
    struct Spring {
        DelayCell travel;
        std::array<double, 12> memory{};
        double low = 0;
    };
    ProcessSpec spec;
    SpringParameters p;
    std::vector<Spring> springs;
    double length = 0, pole = 0, feedback = 0;
};
template <Sample T> SpringReverb<T>::SpringReverb() = default;
template <Sample T> SpringReverb<T>::~SpringReverb() = default;
template <Sample T> SpringReverb<T>::SpringReverb(SpringReverb&&) noexcept = default;
template <Sample T> SpringReverb<T>& SpringReverb<T>::operator=(SpringReverb&&) noexcept = default;
template <Sample T>
Status SpringReverb<T>::prepare(const ProcessSpec& spec, double length) noexcept {
    if (!detail::validAnalysisSpec(spec) || spec.channels > 8 || !isFinite(length) ||
        length < .002 || length > .2)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->length = length;
        s->springs.resize(spec.channels);
        for (std::size_t ch = 0; ch < spec.channels; ++ch)
            s->springs[ch].travel.ring.resize(
                std::size_t(length * spec.sampleRate * (1 + .013 * ch)) + 1);
        s->pole = std::exp(-2 * std::numbers::pi * std::min(5000., spec.sampleRate * .4) /
                           spec.sampleRate);
        s->p.dampingHz = std::min(5000., spec.sampleRate * .4);
        s->feedback = std::pow(.001, length / s->p.decaySeconds);
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status SpringReverb<T>::setParameters(const SpringParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (!isFinite(p.decaySeconds) || p.decaySeconds < .01 || p.decaySeconds > 120 ||
        !isFinite(p.dispersion) || p.dispersion < 0 || p.dispersion > .95 ||
        !isFinite(p.dampingHz) || p.dampingHz < 20 || p.dampingHz >= s.spec.sampleRate * .5 ||
        !isFinite(p.wet) || std::abs(p.wet) > 4 || !isFinite(p.dry) || std::abs(p.dry) > 4)
        return Status::invalid_argument;
    s.p = p;
    s.pole = std::exp(-2 * std::numbers::pi * p.dampingHz / s.spec.sampleRate);
    s.feedback = std::pow(.001, s.length / p.decaySeconds);
    return Status::ok;
}
template <Sample T>
Status SpringReverb<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = checked(s.spec, in, out);
    if (status != Status::ok)
        return status;
    for (std::size_t ch = 0; ch < in.channels(); ++ch) {
        auto& spring = s.springs[ch];
        for (std::size_t i = 0; i < in.frames(); ++i) {
            const double x = in.channel(ch)[i];
            double y = spring.travel.delay(x + s.feedback * spring.low);
            for (std::size_t k = 0; k < spring.memory.size(); ++k) {
                const double g = s.p.dispersion * (.6 + .03 * k), z = spring.memory[k];
                const double v = z - g * y;
                spring.memory[k] = y + g * v;
                y = v;
            }
            spring.low = (1 - s.pole) * y + s.pole * spring.low;
            out.channel(ch)[i] = T(s.p.dry * x + s.p.wet * y);
        }
    }
    return Status::ok;
}
template <Sample T> void SpringReverb<T>::reset() noexcept {
    if (state_)
        for (auto& s : state_->springs) {
            s.travel.reset();
            s.memory = {};
            s.low = 0;
        }
}
template class DelayEffect<float>;
template class DelayEffect<double>;
template class AmplitudeModulator<float>;
template class AmplitudeModulator<double>;
template class PlateReverb<float>;
template class PlateReverb<double>;
template class SpringReverb<float>;
template class SpringReverb<double>;
} // namespace cydsp
