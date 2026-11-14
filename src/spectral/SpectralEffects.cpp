#include "cydsp/spectral/SpectralEffects.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/math/VectorOps.h"
#include "cydsp/processors/Hilbert.h"
#include "detail/Analysis.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>
namespace cydsp {
template <Sample T> struct SpectralEffects<T>::State {
    ProcessSpec spec;
    SpectralEffectParameters p;
    Stft<T> stft;
    AudioBuffer<T> packed, result;
    VectorOps ops;
    std::size_t bins = 0;
    std::vector<double> side, previous, magnitude, increment, phase, gains, work;
    std::vector<unsigned char> captured;
    static void callback(void* raw, std::size_t ch, std::span<std::complex<T>> spectrum) noexcept {
        auto& s = *static_cast<State*>(raw);
        if (ch < s.spec.channels) {
            for (std::size_t k = 0; k < s.bins; ++k)
                s.side[ch * s.bins + k] = std::abs(spectrum[k]);
            return;
        }
        ch -= s.spec.channels;
        const auto base = ch * s.bins;
        const auto mode = s.p.mode;
        for (std::size_t k = 0; k < s.bins; ++k)
            s.work[k] = std::abs(spectrum[k]);
        const bool capture = mode == SpectralEffectMode::freeze && !s.captured[ch];
        for (std::size_t k = 0; k < s.bins; ++k) {
            const auto index = base + k;
            const double arg = std::arg(spectrum[k]), mag = s.work[k];
            auto processed = spectrum[k];
            if (mode == SpectralEffectMode::freeze) {
                if (capture) {
                    s.magnitude[index] = mag;
                    s.increment[index] =
                        std::remainder(arg - s.previous[index], 2 * std::numbers::pi);
                    s.phase[index] = arg;
                } else
                    s.phase[index] =
                        std::remainder(s.phase[index] + s.increment[index], 2 * std::numbers::pi);
                processed = std::polar(T(s.magnitude[index]), T(s.phase[index]));
            } else if (mode == SpectralEffectMode::filter)
                processed *= T(s.gains[k]);
            else if (mode == SpectralEffectMode::morph)
                processed = std::polar(T(s.side[index]), T(arg));
            else if (mode == SpectralEffectMode::vocoder) {
                const auto first = k > s.p.envelopeRadiusBins ? k - s.p.envelopeRadiusBins : 0,
                           last = std::min(s.bins - 1, k + s.p.envelopeRadiusBins);
                double carrier = 0, modulator = 0;
                for (std::size_t j = first; j <= last; ++j) {
                    carrier += s.work[j];
                    modulator += s.side[base + j];
                }
                processed *= T(std::clamp(modulator / std::max(1e-12, carrier), 0., 32.));
            }
            s.previous[index] = arg;
            spectrum[k] += T(s.p.mix) * (processed - spectrum[k]);
        }
        if (capture)
            s.captured[ch] = 1;
    }
};
template <Sample T> SpectralEffects<T>::SpectralEffects() = default;
template <Sample T> SpectralEffects<T>::~SpectralEffects() = default;
template <Sample T> SpectralEffects<T>::SpectralEffects(SpectralEffects&&) noexcept = default;
template <Sample T>
SpectralEffects<T>& SpectralEffects<T>::operator=(SpectralEffects&&) noexcept = default;
template <Sample T>
Status SpectralEffects<T>::prepare(const ProcessSpec& spec, const StftConfig& config,
                                   CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || spec.channels > 32)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        auto packedSpec = spec;
        packedSpec.channels *= 2;
        auto status = s->stft.prepare(packedSpec, config, backend);
        if (status != Status::ok)
            return status;
        status = VectorOps::create(backend, s->ops);
        if (status != Status::ok)
            return status;
        for (auto* b : {&s->packed, &s->result}) {
            status = b->resize(packedSpec.channels, spec.maxBlockFrames);
            if (status != Status::ok)
                return status;
        }
        s->bins = s->stft.bins();
        for (auto* v : {&s->side, &s->previous, &s->magnitude, &s->increment, &s->phase})
            v->resize(s->bins * spec.channels);
        s->gains.resize(s->bins, 1);
        s->work.resize(s->bins);
        s->captured.resize(spec.channels);
        s->stft.setProcessor(State::callback, s.get());
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status SpectralEffects<T>::setParameters(const SpectralEffectParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (unsigned(p.mode) > unsigned(SpectralEffectMode::filter) || !isFinite(p.mix) || p.mix < 0 ||
        p.mix > 1 || p.envelopeRadiusBins > 256)
        return Status::invalid_argument;
    if (state_->p.mode != p.mode)
        std::fill(state_->captured.begin(), state_->captured.end(), static_cast<unsigned char>(0));
    state_->p = p;
    return Status::ok;
}
template <Sample T> Status SpectralEffects<T>::setBinGains(std::span<const double> g) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (g.size() != state_->bins)
        return Status::invalid_argument;
    for (double v : g)
        if (!isFinite(v) || std::abs(v) > 64)
            return Status::invalid_argument;
    std::copy(g.begin(), g.end(), state_->gains.begin());
    return Status::ok;
}
template <Sample T>
Status SpectralEffects<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    return process(in, in, out);
}
template <Sample T>
Status SpectralEffects<T>::process(AudioBlock<const T> in, AudioBlock<const T> side,
                                   AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, in, out);
    if (status != Status::ok)
        return status;
    status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    status = detail::validateAnalysis(s.spec, side);
    if (status != Status::ok)
        return status;
    if (side.frames() != in.frames())
        return Status::invalid_argument;
    auto packed = s.packed.block(in.frames()), result = s.result.block(in.frames());
    for (std::size_t ch = 0; ch < in.channels(); ++ch) {
        (void)s.ops.copy(side.channel(ch), packed.channel(ch));
        (void)s.ops.copy(in.channel(ch), packed.channel(ch + in.channels()));
    }
    status = s.stft.process(packed, result);
    if (status != Status::ok)
        return status;
    for (std::size_t ch = 0; ch < out.channels(); ++ch)
        (void)s.ops.copy(result.channel(ch + out.channels()), out.channel(ch));
    return Status::ok;
}
template <Sample T> void SpectralEffects<T>::reset() noexcept {
    if (state_) {
        auto& s = *state_;
        s.stft.reset();
        for (auto* v : {&s.side, &s.previous, &s.magnitude, &s.increment, &s.phase})
            std::fill(v->begin(), v->end(), 0);
        std::fill(s.captured.begin(), s.captured.end(), static_cast<unsigned char>(0));
    }
}
template <Sample T> std::size_t SpectralEffects<T>::latencyFrames() const noexcept {
    return state_ ? state_->stft.latencyFrames() : 0;
}

template <Sample T> struct FrequencyShifter<T>::State {
    ProcessSpec spec;
    HilbertTransform<T> hilbert;
    AudioBuffer<T> i, q;
    double frequency = 0, phase = 0;
};
template <Sample T> FrequencyShifter<T>::FrequencyShifter() = default;
template <Sample T> FrequencyShifter<T>::~FrequencyShifter() = default;
template <Sample T> FrequencyShifter<T>::FrequencyShifter(FrequencyShifter&&) noexcept = default;
template <Sample T>
FrequencyShifter<T>& FrequencyShifter<T>::operator=(FrequencyShifter&&) noexcept = default;
template <Sample T>
Status FrequencyShifter<T>::prepare(const ProcessSpec& spec, std::size_t taps,
                                    CpuBackend backend) noexcept {
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        auto status = s->hilbert.prepare(spec, taps, backend);
        if (status != Status::ok)
            return status;
        for (auto* b : {&s->i, &s->q}) {
            status = b->resize(spec.channels, spec.maxBlockFrames);
            if (status != Status::ok)
                return status;
        }
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status FrequencyShifter<T>::setFrequency(double hz) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!isFinite(hz) || std::abs(hz) > state_->spec.sampleRate * .25)
        return Status::invalid_argument;
    state_->frequency = hz;
    return Status::ok;
}
template <Sample T>
Status FrequencyShifter<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, in, out);
    if (status != Status::ok)
        return status;
    auto a = s.i.block(in.frames()), b = s.q.block(in.frames());
    status = s.hilbert.process(in, a, b);
    if (status != Status::ok)
        return status;
    for (std::size_t i = 0; i < in.frames(); ++i) {
        const double c = std::cos(2 * std::numbers::pi * s.phase),
                     v = std::sin(2 * std::numbers::pi * s.phase);
        for (std::size_t ch = 0; ch < in.channels(); ++ch)
            out.channel(ch)[i] = T(double(a.channel(ch)[i]) * c - double(b.channel(ch)[i]) * v);
        s.phase += s.frequency / s.spec.sampleRate;
        s.phase -= std::floor(s.phase);
    }
    return Status::ok;
}
template <Sample T> void FrequencyShifter<T>::reset() noexcept {
    if (state_) {
        state_->hilbert.reset();
        state_->phase = 0;
    }
}
template <Sample T> std::size_t FrequencyShifter<T>::latencyFrames() const noexcept {
    return state_ ? state_->hilbert.latencyFrames() : 0;
}
template class SpectralEffects<float>;
template class SpectralEffects<double>;
template class FrequencyShifter<float>;
template class FrequencyShifter<double>;
} // namespace cydsp
