#include "cydsp/synthesis/PhysicalModels.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/core/CircularHistory.h"
#include "detail/Analysis.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>
namespace cydsp {
template <Sample T> struct ModalResonator<T>::State {
    struct Mode {
        double a = 0, b = 0, gain = 0, pan = 0, y1 = 0, y2 = 0;
    };
    ProcessSpec spec;
    std::size_t outputs = 0, count = 0;
    std::vector<Mode> modes;
};
template <Sample T> ModalResonator<T>::ModalResonator() = default;
template <Sample T> ModalResonator<T>::~ModalResonator() = default;
template <Sample T> ModalResonator<T>::ModalResonator(ModalResonator&&) noexcept = default;
template <Sample T>
ModalResonator<T>& ModalResonator<T>::operator=(ModalResonator&&) noexcept = default;
template <Sample T>
Status ModalResonator<T>::prepare(const ProcessSpec& spec, std::size_t outputs,
                                  std::size_t maximum) noexcept {
    if (!detail::validAnalysisSpec(spec) || spec.channels != 1 || !outputs || outputs > 2 ||
        !maximum || maximum > 512)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->outputs = outputs;
        s->modes.resize(maximum);
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status ModalResonator<T>::setModes(std::span<const ResonantMode> modes) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (modes.size() > s.modes.size())
        return Status::invalid_argument;
    for (auto m : modes)
        if (!isFinite(m.frequencyHz) || m.frequencyHz < 1 ||
            m.frequencyHz >= s.spec.sampleRate * .49 || !isFinite(m.decaySeconds) ||
            m.decaySeconds < .001 || m.decaySeconds > 120 || !isFinite(m.gain) ||
            std::abs(m.gain) > 16 || !isFinite(m.pan) || std::abs(m.pan) > 1)
            return Status::invalid_argument;
    for (std::size_t i = 0; i < modes.size(); ++i) {
        const auto& m = modes[i];
        const double radius = std::pow(.001, 1 / (m.decaySeconds * s.spec.sampleRate));
        auto& v = s.modes[i];
        v.a = 2 * radius * std::cos(2 * std::numbers::pi * m.frequencyHz / s.spec.sampleRate);
        v.b = -radius * radius;
        v.gain = m.gain * std::sin(2 * std::numbers::pi * m.frequencyHz / s.spec.sampleRate);
        v.pan = m.pan;
    }
    s.count = modes.size();
    return Status::ok;
}
template <Sample T>
Status ModalResonator<T>::setSurface(ModalSurface type, double fundamental, double aspect,
                                     double decay, double x, double y) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (unsigned(type) > unsigned(ModalSurface::rectangularPlate) || !isFinite(fundamental) ||
        fundamental < 1 || !isFinite(aspect) || aspect < .2 || aspect > 5 || !isFinite(decay) ||
        decay < .001 || decay > 120 || !isFinite(x) || x < 0 || x > 1 || !isFinite(y) || y < 0 ||
        y > 1)
        return Status::invalid_argument;
    std::array<ResonantMode, 512> modes{};
    std::size_t count = 0;
    const double base = 1 + 1 / (aspect * aspect);
    for (unsigned m = 1; m <= 16; ++m)
        for (unsigned n = 1; n <= 16; ++n) {
            const double relative = (double(m * m) + double(n * n) / (aspect * aspect)) / base,
                         f = type == ModalSurface::rectangularMembrane
                                 ? fundamental * std::sqrt(relative)
                                 : fundamental * relative;
            if (f >= state_->spec.sampleRate * .49 || count == state_->modes.size())
                continue;
            const double gain = std::sin(m * std::numbers::pi * x) *
                                std::sin(n * std::numbers::pi * y) / (double(m * n));
            modes[count++] = {f, decay / std::sqrt(relative), gain, 0};
        }
    if (!count)
        return Status::invalid_argument;
    return setModes({modes.data(), count});
}
template <Sample T>
Status ModalResonator<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
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
    for (std::size_t i = 0; i < in.frames(); ++i) {
        std::array<double, 2> sum{};
        for (std::size_t k = 0; k < s.count; ++k) {
            auto& m = s.modes[k];
            const double v = m.gain * in.channel(0)[i] + m.a * m.y1 + m.b * m.y2;
            m.y2 = m.y1;
            m.y1 = v;
            sum[0] += v * (s.outputs == 1 ? 1 : std::sqrt((1 - m.pan) * .5));
            sum[1] += v * std::sqrt((1 + m.pan) * .5);
        }
        for (std::size_t ch = 0; ch < out.channels(); ++ch)
            out.channel(ch)[i] = T(sum[ch]);
    }
    return Status::ok;
}
template <Sample T> void ModalResonator<T>::reset() noexcept {
    if (state_)
        for (auto& m : state_->modes)
            m.y1 = m.y2 = 0;
}

template <Sample T> struct Waveguide<T>::State {
    ProcessSpec spec;
    WaveguideParameters p;
    CircularHistory<double> history;
    AudioBuffer<T> zeros;
    std::vector<double> previous, low, allpass;
    double minimum = 20, delay = 0, gain = 0, excitation = 0;
    std::size_t burst = 0;
    std::uint64_t random = 1;
};
template <Sample T> Waveguide<T>::Waveguide() = default;
template <Sample T> Waveguide<T>::~Waveguide() = default;
template <Sample T> Waveguide<T>::Waveguide(Waveguide&&) noexcept = default;
template <Sample T> Waveguide<T>& Waveguide<T>::operator=(Waveguide&&) noexcept = default;
template <Sample T> Status Waveguide<T>::prepare(const ProcessSpec& spec, double minimum) noexcept {
    if (!detail::validAnalysisSpec(spec) || spec.channels > 8 || !isFinite(minimum) ||
        minimum < 5 || minimum > 1000)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->minimum = minimum;
        s->p.frequencyHz = std::max(220., minimum);
        s->delay = std::max(2., spec.sampleRate / s->p.frequencyHz - .5);
        s->gain = std::pow(.001, (s->delay + .5) / (s->p.decaySeconds * spec.sampleRate));
        s->previous.resize(spec.channels);
        s->low.resize(spec.channels);
        s->allpass.resize(spec.channels);
        auto status = s->history.prepare(spec.channels,
                                         std::size_t(std::ceil(spec.sampleRate / minimum)) + 8);
        if (status != Status::ok)
            return status;
        status = s->zeros.resize(spec.channels, spec.maxBlockFrames);
        if (status != Status::ok)
            return status;
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status Waveguide<T>::setParameters(const WaveguideParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (unsigned(p.model) > unsigned(WaveguideModel::tube) || !isFinite(p.frequencyHz) ||
        p.frequencyHz < s.minimum || p.frequencyHz > s.spec.sampleRate * .2 ||
        !isFinite(p.decaySeconds) || p.decaySeconds < .01 || p.decaySeconds > 120 ||
        !isFinite(p.brightness) || p.brightness < 0 || p.brightness > 1 ||
        !isFinite(p.dispersion) || p.dispersion < 0 || p.dispersion > .95)
        return Status::invalid_argument;
    double delay = s.spec.sampleRate / p.frequencyHz;
    if (p.model == WaveguideModel::tube && p.closedTube)
        delay *= .5;
    if (p.model == WaveguideModel::karplusStrong)
        delay -= .5;
    if (delay < 2 || delay + 2 > double(s.history.capacity()))
        return Status::invalid_argument;
    s.p = p;
    s.delay = delay;
    s.gain = std::min(.99999, std::pow(.001, delay / (p.decaySeconds * s.spec.sampleRate)));
    return Status::ok;
}
template <Sample T> void Waveguide<T>::pluck(double gain, std::uint64_t seed) noexcept {
    if (state_ && isFinite(gain) && std::abs(gain) <= 16) {
        state_->excitation = gain;
        state_->burst = std::size_t(std::ceil(state_->delay));
        state_->random = seed ? seed : 1;
    }
}
template <Sample T>
Status Waveguide<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, in, out);
    if (status != Status::ok)
        return status;
    status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    std::array<double, 8> frame{};
    for (std::size_t i = 0; i < in.frames(); ++i) {
        double noise = 0;
        if (s.burst) {
            s.random ^= s.random >> 12;
            s.random ^= s.random << 25;
            s.random ^= s.random >> 27;
            noise =
                (double((s.random * 2685821657736338717ULL) >> 11) * 0x1.0p-52 - 1) * s.excitation;
            --s.burst;
        }
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            const double x = in.channel(ch)[i],
                         delayed = s.history.read(ch, s.delay - 1, HistoryInterpolation::linear);
            double y = delayed;
            if (s.p.model == WaveguideModel::karplusStrong) {
                y = .5 * (delayed + s.previous[ch]);
                s.previous[ch] = delayed;
            } else {
                const double pole = .05 + .9 * (1 - s.p.brightness);
                s.low[ch] = (1 - pole) * y + pole * s.low[ch];
                y = s.low[ch];
                if (s.p.model == WaveguideModel::string) {
                    const double a = s.p.dispersion, v = s.allpass[ch] - a * y;
                    s.allpass[ch] = y + a * v;
                    y = v;
                }
            }
            const double polarity = s.p.model == WaveguideModel::tube && s.p.closedTube ? -1. : 1.;
            frame[ch] = x + noise + polarity * s.gain * y;
            out.channel(ch)[i] = T(delayed);
        }
        (void)s.history.pushFrame({frame.data(), in.channels()});
    }
    return Status::ok;
}
template <Sample T> Status Waveguide<T>::render(AudioBlock<T> out) noexcept {
    return state_ ? process(state_->zeros.block(out.frames()), out) : Status::not_prepared;
}
template <Sample T> void Waveguide<T>::reset() noexcept {
    if (state_) {
        auto& s = *state_;
        s.history.reset();
        std::fill(s.previous.begin(), s.previous.end(), 0);
        std::fill(s.low.begin(), s.low.end(), 0);
        std::fill(s.allpass.begin(), s.allpass.end(), 0);
        s.burst = 0;
        s.random = 1;
    }
}
template class ModalResonator<float>;
template class ModalResonator<double>;
template class Waveguide<float>;
template class Waveguide<double>;
} // namespace cydsp
