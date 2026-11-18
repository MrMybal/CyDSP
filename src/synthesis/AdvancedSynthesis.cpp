#include "cydsp/synthesis/AdvancedSynthesis.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/processors/Oversampler.h"
#include "detail/Analysis.h"
#include "detail/OscillatorMath.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
namespace cydsp {
Status MultiSegmentEnvelope::prepare(double rate, std::size_t maximum) noexcept {
    if (!isFinite(rate) || rate < 8000 || rate > 768000 || !maximum || maximum > 256)
        return Status::invalid_spec;
    try {
        std::vector<Segment> next(maximum);
        segments_.swap(next);
        rate_ = rate;
        count_ = 0;
        reset();
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status MultiSegmentEnvelope::setSegments(std::span<const EnvelopeSegment> segments,
                                         std::size_t release, std::size_t begin,
                                         std::size_t end) noexcept {
    if (rate_ == 0)
        return Status::not_prepared;
    if (segments.empty() || segments.size() > segments_.size() || release >= segments.size() ||
        begin > end || end > release || (end && begin == end))
        return Status::invalid_argument;
    for (auto s : segments)
        if (!isFinite(s.seconds) || s.seconds < 0 || s.seconds > 3600 || !isFinite(s.target) ||
            std::abs(s.target) > 64 || !isFinite(s.curve) || std::abs(s.curve) > 8)
            return Status::invalid_argument;
    for (std::size_t i = 0; i < segments.size(); ++i)
        segments_[i] = {
            std::max<std::size_t>(1, std::size_t(std::ceil(segments[i].seconds * rate_))),
            segments[i].target, segments[i].curve};
    count_ = segments.size();
    release_ = release;
    loopBegin_ = begin;
    loopEnd_ = end;
    reset();
    return Status::ok;
}
void MultiSegmentEnvelope::trigger() noexcept {
    if (count_) {
        active_ = true;
        released_ = false;
        index_ = elapsed_ = 0;
        start_ = value_;
    }
}
void MultiSegmentEnvelope::release() noexcept {
    if (active_) {
        released_ = true;
        index_ = release_;
        elapsed_ = 0;
        start_ = value_;
    }
}
void MultiSegmentEnvelope::reset() noexcept {
    value_ = start_ = 0;
    index_ = elapsed_ = 0;
    active_ = released_ = false;
}
double MultiSegmentEnvelope::next() noexcept {
    if (!active_)
        return value_;
    const auto& s = segments_[index_];
    const double t = double(++elapsed_) / double(s.frames),
                 shaped =
                     std::abs(s.curve) < 1e-8 ? t : std::expm1(s.curve * t) / std::expm1(s.curve);
    value_ = start_ + (s.target - start_) * shaped;
    if (elapsed_ == s.frames) {
        value_ = s.target;
        start_ = value_;
        elapsed_ = 0;
        ++index_;
        if (!released_ && loopEnd_ && index_ == loopEnd_)
            index_ = loopBegin_;
        if (index_ >= count_)
            active_ = false;
    }
    return value_;
}
double ModulationLfo::randomValue() noexcept {
    random_ ^= random_ >> 12;
    random_ ^= random_ << 25;
    random_ ^= random_ >> 27;
    return double((random_ * 2685821657736338717ULL) >> 11) * 0x1.0p-52 - 1;
}
Status ModulationLfo::prepare(double rate) noexcept {
    if (!isFinite(rate) || rate < 8000 || rate > 768000)
        return Status::invalid_spec;
    rate_ = rate;
    reset();
    return Status::ok;
}
Status ModulationLfo::setParameters(double frequency, LfoShape shape) noexcept {
    if (rate_ == 0)
        return Status::not_prepared;
    if (!isFinite(frequency) || frequency < 0 || frequency > rate_ * .5 ||
        unsigned(shape) > unsigned(LfoShape::smoothRandom))
        return Status::invalid_argument;
    frequency_ = frequency;
    shape_ = shape;
    return Status::ok;
}
void ModulationLfo::reset(std::uint64_t seed) noexcept {
    random_ = seed ? seed : 1;
    phase_ = 0;
    held_ = randomValue();
    next_ = randomValue();
}
double ModulationLfo::next() noexcept {
    if (rate_ == 0)
        return 0;
    double value = 0;
    switch (shape_) {
    case LfoShape::sine:
        value = std::sin(2 * std::numbers::pi * phase_);
        break;
    case LfoShape::triangle:
        value = 1 - 4 * std::abs(phase_ - .5);
        break;
    case LfoShape::sampleAndHold:
        value = held_;
        break;
    case LfoShape::smoothRandom: {
        const double t = phase_ * phase_ * (3 - 2 * phase_);
        value = held_ + (next_ - held_) * t;
        break;
    }
    }
    phase_ += frequency_ / rate_;
    if (phase_ >= 1 - 8 * std::numeric_limits<double>::epsilon()) {
        phase_ = std::max(0., phase_ - 1);
        held_ = next_;
        next_ = randomValue();
    }
    return value;
}

template <Sample T> struct ModulatedOscillator<T>::State {
    ProcessSpec spec;
    ModulatedOscillatorParameters p;
    Oversampler<T> over;
    AudioBuffer<T> zeros;
    std::array<double, 16> phase{}, tuning{}, left{}, right{};
    std::vector<const Wavetable<T>*> tables;
    double master = 0;
    std::uint64_t random = 1;
};
template <Sample T> ModulatedOscillator<T>::ModulatedOscillator() = default;
template <Sample T> ModulatedOscillator<T>::~ModulatedOscillator() = default;
template <Sample T>
ModulatedOscillator<T>::ModulatedOscillator(ModulatedOscillator&&) noexcept = default;
template <Sample T>
ModulatedOscillator<T>& ModulatedOscillator<T>::operator=(ModulatedOscillator&&) noexcept = default;
template <Sample T>
Status ModulatedOscillator<T>::prepare(const ProcessSpec& spec,
                                       std::span<const Wavetable<T>* const> tables,
                                       CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || spec.channels > 2 || tables.size() > 32)
        return Status::invalid_spec;
    for (auto* table : tables)
        if (!table || !table->size())
            return Status::invalid_argument;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->tables.assign(tables.begin(), tables.end());
        s->tuning.fill(1);
        s->left.fill(std::sqrt(.5));
        s->right.fill(std::sqrt(.5));
        auto status = s->over.prepare(spec, 4, backend);
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
template <Sample T>
Status ModulatedOscillator<T>::setParameters(const ModulatedOscillatorParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    const auto rate = state_->spec.sampleRate;
    if (unsigned(p.waveform) > unsigned(Waveform::whiteNoise) || !isFinite(p.frequencyHz) ||
        p.frequencyHz < 0 || p.frequencyHz > rate * .45 || !isFinite(p.fmDepth) ||
        std::abs(p.fmDepth) > 64 || !isFinite(p.pmDepthCycles) || std::abs(p.pmDepthCycles) > 64 ||
        !isFinite(p.syncHz) || p.syncHz < 0 || p.syncHz > rate * .45 || !isFinite(p.morph) ||
        p.morph < 0 || p.morph > 1 || !isFinite(p.detuneCents) || p.detuneCents < 0 ||
        p.detuneCents > 1200 || !isFinite(p.spread) || p.spread < 0 || p.spread > 1 ||
        !isFinite(p.gain) || std::abs(p.gain) > 16 || !p.unison || p.unison > 16)
        return Status::invalid_argument;
    for (std::size_t v = 0; v < p.unison; ++v) {
        const double position = p.unison == 1 ? 0 : 2 * double(v) / double(p.unison - 1) - 1;
        state_->tuning[v] = std::exp2(position * p.detuneCents / 1200);
        state_->left[v] = std::sqrt((1 - position * p.spread) * .5);
        state_->right[v] = std::sqrt((1 + position * p.spread) * .5);
    }
    state_->p = p;
    return Status::ok;
}
template <Sample T> Status ModulatedOscillator<T>::process(AudioBlock<T> out) noexcept {
    return process({}, {}, out);
}
template <Sample T>
Status ModulatedOscillator<T>::process(AudioBlock<const T> fm, AudioBlock<const T> pm,
                                       AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (!out.isValid() || out.channels() != s.spec.channels)
        return Status::channel_mismatch;
    if (out.frames() > s.spec.maxBlockFrames)
        return Status::block_too_large;
    // With no modulation input, the disjoint-block checks below never see the outputs.
    if (!fm.channels() && !pm.channels()) {
        const auto status = validateDisjointBlocks<T>({}, out);
        if (status != Status::ok)
            return status;
    }
    for (auto block : {fm, pm})
        if (block.channels()) {
            auto spec = s.spec;
            spec.channels = 1;
            auto status = detail::validateAnalysis(spec, block);
            if (status != Status::ok)
                return status;
            if (block.frames() != out.frames())
                return Status::invalid_argument;
            status = validateDisjointBlocks(block, out);
            if (status != Status::ok)
                return status;
        }
    AudioBlock<T> high;
    auto status = s.over.processUp(s.zeros.block(out.frames()), high);
    if (status != Status::ok)
        return status;
    const auto& p = s.p;
    const double rate = s.spec.sampleRate * 4;
    for (std::size_t i = 0; i < high.frames(); ++i) {
        const auto base = i / 4;
        const double frequency = std::clamp(
                         p.frequencyHz +
                             (fm.channels() ? double(fm.channel(0)[base]) * p.fmDepth : 0),
                         0., s.spec.sampleRate * .45),
                     phaseMod = pm.channels() ? double(pm.channel(0)[base]) * p.pmDepthCycles : 0;
        std::array<double, 2> sum{};
        s.master += p.syncHz / rate;
        if (s.master >= 1) {
            s.master -= 1;
            for (auto& phase : s.phase)
                phase = 0;
        }
        for (std::size_t v = 0; v < p.unison; ++v) {
            const double hz = frequency * s.tuning[v], step = std::min(.45, hz / rate);
            const double raw = s.phase[v] + phaseMod, phase = raw - std::floor(raw);
            double value = 0;
            if (!s.tables.empty()) {
                const double at = p.morph * double(s.tables.size() - 1);
                const auto first = std::size_t(at),
                           second = std::min(first + 1, s.tables.size() - 1);
                value = double(s.tables[first]->value(phase, hz, rate)) * (1 - (at - first)) +
                        double(s.tables[second]->value(phase, hz, rate)) * (at - first);
            } else
                switch (p.waveform) {
                case Waveform::sine:
                    value = std::sin(2 * std::numbers::pi * phase);
                    break;
                case Waveform::triangle:
                    value = 1 - 4 * std::abs(phase - .5);
                    break;
                case Waveform::saw:
                    value = 2 * phase - 1 - detail::polyBlep(phase, step);
                    break;
                case Waveform::square:
                    value = (phase < .5 ? 1. : -1.) + detail::polyBlep(phase, step) -
                            detail::polyBlep(std::fmod(phase + .5, 1.), step);
                    break;
                case Waveform::whiteNoise:
                    s.random ^= s.random >> 12;
                    s.random ^= s.random << 25;
                    s.random ^= s.random >> 27;
                    value = double((s.random * 2685821657736338717ULL) >> 11) * 0x1.0p-52 - 1;
                    break;
                }
            if (s.spec.channels == 1)
                sum[0] += value;
            else {
                sum[0] += value * s.left[v];
                sum[1] += value * s.right[v];
            }
            s.phase[v] += step;
            s.phase[v] -= std::floor(s.phase[v]);
        }
        for (std::size_t ch = 0; ch < high.channels(); ++ch)
            high.channel(ch)[i] = T(sum[ch] * p.gain / double(p.unison));
    }
    return s.over.processDown(out);
}
template <Sample T> void ModulatedOscillator<T>::reset() noexcept {
    if (state_) {
        state_->over.reset();
        state_->phase = {};
        state_->master = 0;
        state_->random = 1;
    }
}
template <Sample T> std::size_t ModulatedOscillator<T>::latencyFrames() const noexcept {
    return state_ ? state_->over.latencyFrames() / 2 : 0;
}

template <Sample T> Status ProceduralSource<T>::prepare(const ProcessSpec& spec) noexcept {
    if (!detail::validAnalysisSpec(spec))
        return Status::invalid_spec;
    spec_ = spec;
    prepared_ = true;
    p_ = {};
    reset();
    return setParameters(p_);
}
template <Sample T>
Status ProceduralSource<T>::setParameters(const ProceduralParameters& p) noexcept {
    if (!prepared_)
        return Status::not_prepared;
    if (unsigned(p.kind) > unsigned(ProceduralKind::impact) || !isFinite(p.frequencyHz) ||
        p.frequencyHz < 1 || p.frequencyHz > spec_.sampleRate * .25 || !isFinite(p.intensity) ||
        p.intensity < 0 || p.intensity > 4 || !isFinite(p.roughness) || p.roughness < 0 ||
        p.roughness > 1 || !isFinite(p.decaySeconds) || p.decaySeconds < .001 ||
        p.decaySeconds > 60)
        return Status::invalid_argument;
    p_ = p;
    pole_ = std::exp(-2 * std::numbers::pi *
                     std::min(spec_.sampleRate * .4, 100 + p.roughness * 5000) / spec_.sampleRate);
    decay_ = std::exp(std::log(.001) / (p.decaySeconds * spec_.sampleRate));
    return Status::ok;
}
template <Sample T> void ProceduralSource<T>::trigger(double strength) noexcept {
    if (isFinite(strength) && strength >= 0 && strength <= 4)
        energy_ = strength;
}
template <Sample T> Status ProceduralSource<T>::process(AudioBlock<T> out) noexcept {
    if (!prepared_)
        return Status::not_prepared;
    if (!out.isValid() || out.channels() != spec_.channels)
        return Status::channel_mismatch;
    if (out.frames() > spec_.maxBlockFrames)
        return Status::block_too_large;
    for (std::size_t i = 0; i < out.frames(); ++i) {
        random_ ^= random_ >> 12;
        random_ ^= random_ << 25;
        random_ ^= random_ >> 27;
        const double noise = double((random_ * 2685821657736338717ULL) >> 11) * 0x1.0p-52 - 1;
        low_ = (1 - pole_) * noise + pole_ * low_;
        const double harmonic = std::sin(2 * std::numbers::pi * phase_) +
                                .35 * std::sin(4 * std::numbers::pi * phase_) +
                                .15 * std::sin(6 * std::numbers::pi * phase_);
        double value = 0;
        switch (p_.kind) {
        case ProceduralKind::wind:
            value = low_ * (.8 + .2 * std::sin(phase_ * 2 * std::numbers::pi));
            break;
        case ProceduralKind::engine:
            value = (1 - p_.roughness) * harmonic * .6 + p_.roughness * low_;
            break;
        case ProceduralKind::impact:
            value = energy_ * (p_.roughness * noise + (1 - p_.roughness) * harmonic * .6);
            break;
        }
        for (std::size_t ch = 0; ch < out.channels(); ++ch)
            out.channel(ch)[i] = T(value * p_.intensity);
        energy_ *= decay_;
        phase_ += p_.frequencyHz / spec_.sampleRate;
        phase_ -= std::floor(phase_);
    }
    return Status::ok;
}
template <Sample T> void ProceduralSource<T>::reset(std::uint64_t seed) noexcept {
    phase_ = low_ = energy_ = 0;
    random_ = seed ? seed : 1;
}
template class ModulatedOscillator<float>;
template class ModulatedOscillator<double>;
template class ProceduralSource<float>;
template class ProceduralSource<double>;
} // namespace cydsp
