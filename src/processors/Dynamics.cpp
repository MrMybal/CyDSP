#include "cydsp/processors/Dynamics.h"
#include "detail/Analysis.h"
#include "detail/AudioMath.h"
#include "detail/CompressorCurve.h"
#include "detail/VectorTable.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <vector>
namespace cydsp {
namespace {
bool validTime(double value) noexcept {
    return isFinite(value) && value >= 0 && value <= 86400000;
}
double coefficient(double ms, double rate) noexcept {
    return ms <= 0 ? 0 : std::exp(-1000.0 / ms / rate);
}
double advance(double value, double old, double attack, double release) noexcept {
    const double c = value > old ? attack : release;
    return value + c * (old - value);
}
} // namespace
template <Sample T> struct BallisticsFilter<T>::State {
    ProcessSpec spec;
    DetectorMode mode;
    std::vector<double> values;
    double attack = 0, release = 0;
};
template <Sample T> BallisticsFilter<T>::BallisticsFilter() = default;
template <Sample T> BallisticsFilter<T>::~BallisticsFilter() = default;
template <Sample T> BallisticsFilter<T>::BallisticsFilter(BallisticsFilter&&) noexcept = default;
template <Sample T>
BallisticsFilter<T>& BallisticsFilter<T>::operator=(BallisticsFilter&&) noexcept = default;
template <Sample T>
Status BallisticsFilter<T>::prepare(const ProcessSpec& spec, DetectorMode mode) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    try {
        auto next = std::make_unique<State>();
        next->spec = spec;
        next->mode = mode;
        next->values.resize(spec.channels);
        next->release = coefficient(100, spec.sampleRate);
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
}
template <Sample T> Status BallisticsFilter<T>::setTimes(double attack, double release) noexcept {
    if (!validTime(attack) || !validTime(release))
        return Status::invalid_argument;
    if (!state_)
        return Status::not_prepared;
    state_->attack = coefficient(attack, state_->spec.sampleRate);
    state_->release = coefficient(release, state_->spec.sampleRate);
    return Status::ok;
}
template <Sample T> void BallisticsFilter<T>::reset() noexcept {
    if (state_)
        std::fill(state_->values.begin(), state_->values.end(), 0);
}
template <Sample T>
Status BallisticsFilter<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, in, out);
    if (status != Status::ok)
        return status;
    for (std::size_t ch = 0; ch < in.channels(); ++ch)
        for (std::size_t i = 0; i < in.frames(); ++i) {
            const double x = in.channel(ch)[i];
            const double detector = s.mode == DetectorMode::rms ? x * x : std::abs(x);
            s.values[ch] = advance(detector, s.values[ch], s.attack, s.release);
            out.channel(ch)[i] =
                static_cast<T>(s.mode == DetectorMode::rms ? std::sqrt(std::max(0.0, s.values[ch]))
                                                           : s.values[ch]);
        }
    return Status::ok;
}
template <Sample T> struct Dynamics<T>::State {
    ProcessSpec spec;
    DynamicsMode mode;
    DynamicsParameters p;
    VectorOps ops;
    detail::CompressorCurve curve{};
    std::array<double, 64> curveInput{}, curveOutput{};
    std::vector<double> values, gateGain;
    std::vector<std::uint64_t> hold;
    std::vector<std::uint8_t> gateOpen;
    std::vector<T> delay;
    std::vector<double> peaks;
    std::vector<std::uint64_t> peakTimes;
    std::vector<std::size_t> peakRead, peakCount;
    std::uint64_t clock = 0;
    std::size_t delayCapacity = 0, delayFrames = 0, head = 0;
    double maximumLookAheadMs = 0;
    double attack = 0, release = 0, makeup = 1, reduction = 0, minimumGain = 1;
};
template <Sample T> Dynamics<T>::Dynamics() = default;
template <Sample T> Dynamics<T>::~Dynamics() = default;
template <Sample T> Dynamics<T>::Dynamics(Dynamics&&) noexcept = default;
template <Sample T> Dynamics<T>& Dynamics<T>::operator=(Dynamics&&) noexcept = default;
template <Sample T>
Status Dynamics<T>::prepare(const ProcessSpec& spec, DynamicsMode mode,
                            double maximumLookAheadMs) noexcept {
    if (!spec.isValid() || !validTime(maximumLookAheadMs) || maximumLookAheadMs > 1000 ||
        static_cast<unsigned>(mode) > static_cast<unsigned>(DynamicsMode::limiter) ||
        std::ceil(spec.sampleRate * maximumLookAheadMs / 1000) + 1 >=
            double(std::numeric_limits<std::size_t>::max() / spec.channels))
        return Status::invalid_spec;
    try {
        auto next = std::make_unique<State>();
        next->spec = spec;
        next->mode = mode;
        const auto vectorStatus = VectorOps::create(CpuBackend::automatic, next->ops);
        if (vectorStatus != Status::ok)
            return vectorStatus;
        next->values.resize(spec.channels);
        next->gateGain.resize(spec.channels);
        next->gateOpen.resize(spec.channels);
        next->hold.resize(spec.channels);
        next->maximumLookAheadMs = maximumLookAheadMs;
        next->delayCapacity =
            std::size_t(std::ceil(spec.sampleRate * maximumLookAheadMs / 1000)) + 1;
        next->delay.resize(spec.channels * next->delayCapacity);
        next->peaks.resize(spec.channels * next->delayCapacity);
        next->peakTimes.resize(spec.channels * next->delayCapacity);
        next->peakRead.resize(spec.channels);
        next->peakCount.resize(spec.channels);
        next->curve = detail::makeCompressorCurve(next->p.thresholdDb, next->p.ratio,
                                                  next->p.kneeDb, mode == DynamicsMode::limiter);
        next->attack = coefficient(next->p.attackMs, spec.sampleRate);
        next->release = coefficient(next->p.releaseMs, spec.sampleRate);
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
}
template <Sample T> Status Dynamics<T>::setParameters(const DynamicsParameters& p) noexcept {
    if (!isFinite(p.thresholdDb) || p.thresholdDb < -160 || p.thresholdDb > 24 ||
        !isFinite(p.ratio) || p.ratio < 1 || p.ratio > 1000 || !isFinite(p.kneeDb) ||
        p.kneeDb < 0 || p.kneeDb > 60 || !validTime(p.attackMs) || !validTime(p.releaseMs) ||
        !isFinite(p.makeupDb) || p.makeupDb < -60 || p.makeupDb > 60 || !validTime(p.holdMs) ||
        !validTime(p.lookAheadMs) || !isFinite(p.hysteresisDb) || p.hysteresisDb < 0 ||
        p.hysteresisDb > 60 ||
        static_cast<unsigned>(p.detector) > static_cast<unsigned>(DetectorMode::rms))
        return Status::invalid_argument;
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (p.lookAheadMs > s.maximumLookAheadMs)
        return Status::invalid_argument;
    const auto frames = std::size_t(std::ceil(p.lookAheadMs * s.spec.sampleRate / 1000));
    if (frames != s.delayFrames) {
        s.delayFrames = frames;
        s.head = 0;
        std::fill(s.delay.begin(), s.delay.end(), T{});
        std::fill(s.peakRead.begin(), s.peakRead.end(), 0);
        std::fill(s.peakCount.begin(), s.peakCount.end(), 0);
        s.clock = 0;
    }
    if (s.p.thresholdDb != p.thresholdDb || s.p.ratio != p.ratio || s.p.kneeDb != p.kneeDb)
        s.curve = detail::makeCompressorCurve(p.thresholdDb, p.ratio, p.kneeDb,
                                              s.mode == DynamicsMode::limiter);
    if (s.p.attackMs != p.attackMs)
        s.attack = coefficient(p.attackMs, s.spec.sampleRate);
    if (s.p.releaseMs != p.releaseMs)
        s.release = coefficient(p.releaseMs, s.spec.sampleRate);
    if (s.p.makeupDb != p.makeupDb)
        s.makeup = std::pow(10.0, p.makeupDb / 20);
    s.p = p;
    return Status::ok;
}
template <Sample T> void Dynamics<T>::reset() noexcept {
    if (state_) {
        std::fill(state_->values.begin(), state_->values.end(), 0);
        std::fill(state_->delay.begin(), state_->delay.end(), T{});
        std::fill(state_->hold.begin(), state_->hold.end(), 0);
        std::fill(state_->gateOpen.begin(), state_->gateOpen.end(), std::uint8_t{});
        std::fill(state_->gateGain.begin(), state_->gateGain.end(), 0);
        std::fill(state_->peakRead.begin(), state_->peakRead.end(), 0);
        std::fill(state_->peakCount.begin(), state_->peakCount.end(), 0);
        state_->clock = 0;
        state_->head = 0;
        state_->reduction = 0;
        state_->minimumGain = 1;
    }
}
template <Sample T> double Dynamics<T>::gainReductionDb() const noexcept {
    return state_ ? state_->reduction : 0;
}
template <Sample T> double Dynamics<T>::minimumAppliedGain() const noexcept {
    return state_ ? state_->minimumGain : 1;
}
template <Sample T> std::size_t Dynamics<T>::latencyFrames() const noexcept {
    return state_ ? state_->delayFrames : 0;
}
template <Sample T>
Status Dynamics<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    return process(in, in, out);
}
template <Sample T>
Status Dynamics<T>::process(AudioBlock<const T> in, AudioBlock<const T> side,
                            AudioBlock<T> out) noexcept {
    return processImpl<false>(in, side, out, 0, 1);
}
template <Sample T>
Status Dynamics<T>::processMixed(AudioBlock<const T> in, AudioBlock<T> out, double dryGain,
                                 double wetGain) noexcept {
    return processMixed(in, in, out, dryGain, wetGain);
}
template <Sample T>
Status Dynamics<T>::processMixed(AudioBlock<const T> in, AudioBlock<const T> side,
                                 AudioBlock<T> out, double dryGain, double wetGain) noexcept {
    if (!isFinite(dryGain) || !isFinite(wetGain) || std::abs(dryGain) > 16 ||
        std::abs(wetGain) > 16)
        return Status::invalid_argument;
    return processImpl<true>(in, side, out, dryGain, wetGain);
}
template <Sample T>
template <bool Mixed>
Status Dynamics<T>::processImpl(AudioBlock<const T> in, AudioBlock<const T> side, AudioBlock<T> out,
                                double dryGain, double wetGain) noexcept {
    const auto publish = [=](double dry, double value) noexcept {
        if constexpr (Mixed)
            return T(dryGain * dry + wetGain * double(T(value)));
        else
            return T(value);
    };
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, in, out);
    if (status != Status::ok)
        return status;
    status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    if (side.channels() != 1 && side.channels() != s.spec.channels)
        return Status::channel_mismatch;
    auto detectorSpec = s.spec;
    detectorSpec.channels = side.channels();
    if (side.frames() != in.frames())
        return Status::invalid_argument;
    const bool same = side.channelData() == in.channelData() &&
                      side.frameOffset() == in.frameOffset() && side.channels() == in.channels();
    if (!same) {
        status = detail::validateAnalysis(detectorSpec, side);
        if (status != Status::ok)
            return status;
        status = validateDisjointBlocks(side, out);
        if (status != Status::ok)
            return status;
    }
    if (!in.frames())
        return Status::ok;
    double minimumGain = std::numeric_limits<double>::infinity();
    const auto holdFrames = s.mode == DynamicsMode::noiseGate
                                ? std::uint64_t(std::ceil(s.p.holdMs * s.spec.sampleRate / 1000))
                                : 0;
    const double halfKnee = s.p.kneeDb * 0.5;
    const double slope = s.curve.slope;
    const double ceiling = s.mode == DynamicsMode::limiter ? s.curve.threshold : 1;
    // With linked channels and equal retained envelopes, a zero-lookahead
    // compressor/limiter has one detector and one transfer curve per frame.
    // After unlinked operation the envelopes may differ: retain the general
    // path until reset (no state discontinuity when linking is automated).
    const bool linkedCurve =
        s.p.linkedChannels && !s.delayFrames &&
        (s.mode == DynamicsMode::compressor || s.mode == DynamicsMode::limiter) &&
        std::all_of(s.values.begin(), s.values.end(),
                    [&](double v) { return v == s.values.front(); });
    if (linkedCurve) {
        double envelopeState = s.values.front(), db = s.reduction, lastEnvelope = 0;
        const double lowerKnee = s.curve.lower, upperKnee = s.curve.upper,
                     threshold = s.curve.threshold;
        const auto& table = detail::VectorAccess::table(s.ops);
        const auto curve = table.compressorGain;
        const auto preparedCurve = table.compressorGainPrepared;
        if ((curve || preparedCurve) && in.frames() >= 4) {
            for (std::size_t offset = 0; offset < in.frames();) {
                const auto count = std::min(s.curveInput.size(), in.frames() - offset);
                const auto* first = side.channel(0).data() + offset;
                const auto* second =
                    side.channels() == 2 ? side.channel(1).data() + offset : nullptr;
                for (std::size_t i = 0; i < count; ++i) {
                    double magnitude = std::abs(double(first[i]));
                    if (second)
                        magnitude = std::max(magnitude, std::abs(double(second[i])));
                    else if (side.channels() > 2)
                        for (std::size_t ch = 1; ch < side.channels(); ++ch)
                            magnitude =
                                std::max(magnitude, std::abs(double(side.channel(ch)[offset + i])));
                    const double detector =
                        s.p.detector == DetectorMode::rms ? magnitude * magnitude : magnitude;
                    envelopeState = advance(detector, envelopeState, s.attack, s.release);
                    s.curveInput[i] = s.p.detector == DetectorMode::rms
                                          ? std::sqrt(std::max(0., envelopeState))
                                          : envelopeState;
                }
                lastEnvelope = s.curveInput[count - 1];
                if (preparedCurve)
                    preparedCurve(s.curveInput.data(), s.curveOutput.data(), count, s.curve);
                else
                    curve(s.curveInput.data(), s.curveOutput.data(), count, s.p.thresholdDb,
                          s.p.ratio, s.p.kneeDb, s.mode == DynamicsMode::limiter);
                for (std::size_t i = 0; i < count; ++i)
                    minimumGain = std::min(minimumGain, s.curveOutput[i] * s.makeup);
                for (std::size_t ch = 0; ch < in.channels(); ++ch) {
                    const auto* source = in.channel(ch).data() + offset;
                    auto* destination = out.channel(ch).data() + offset;
                    for (std::size_t i = 0; i < count; ++i) {
                        double value = double(source[i]) * s.curveOutput[i] * s.makeup;
                        if (s.mode == DynamicsMode::limiter)
                            value = std::clamp(value, -ceiling, ceiling);
                        destination[i] = publish(double(source[i]), value);
                    }
                }
                offset += count;
            }
        } else
            for (std::size_t i = 0; i < in.frames(); ++i) {
                double magnitude = 0;
                for (std::size_t ch = 0; ch < side.channels(); ++ch)
                    magnitude = std::max(magnitude, std::abs(double(side.channel(ch)[i])));
                const double detector =
                    s.p.detector == DetectorMode::rms ? magnitude * magnitude : magnitude;
                envelopeState = advance(detector, envelopeState, s.attack, s.release);
                const double envelope = s.p.detector == DetectorMode::rms
                                            ? std::sqrt(std::max(0.0, envelopeState))
                                            : envelopeState;
                lastEnvelope = envelope;
                db = 0;
                double gain = 1;
                // Above the soft knee the curve is a power of the linear ratio.
                // Common ratios use square roots; the final dB meter is evaluated
                // once below. This is the same curve, with ordinary FP rounding.
                if (envelope > upperKnee * (1 + 1e-12)) {
                    const double t = threshold / envelope;
                    if (s.mode == DynamicsMode::limiter)
                        gain = t;
                    else if (s.p.ratio == 2)
                        gain = std::sqrt(t);
                    else if (s.p.ratio == 4) {
                        const double root = std::sqrt(t);
                        gain = root * std::sqrt(root);
                    } else
                        gain = detail::audioExp(detail::audioLog(t) * (1 - 1 / s.p.ratio));
                } else
                    // This conservative guard skips only levels strictly below the knee;
                    // the original log curve still decides all boundary cases.
                    if (std::max(1e-12, envelope) >= lowerKnee * (1 - 1e-12)) {
                        const double over = (20 / std::numbers::ln10) *
                                                detail::audioLog(std::max(1e-12, envelope)) -
                                            s.p.thresholdDb;
                        if (s.p.kneeDb > 0 && std::abs(over) < halfKnee) {
                            const double t = over + halfKnee;
                            db = slope * t * t / (2 * s.p.kneeDb);
                        } else if (over >= halfKnee)
                            db = slope * over;
                    }
                if (db != 0)
                    gain = detail::audioExp(db * (std::numbers::ln10 / 20));
                minimumGain = std::min(minimumGain, gain * s.makeup);
                for (std::size_t ch = 0; ch < in.channels(); ++ch) {
                    double value = double(in.channel(ch)[i]) * gain * s.makeup;
                    if (s.mode == DynamicsMode::limiter)
                        value = std::clamp(value, -ceiling, ceiling);
                    out.channel(ch)[i] = publish(double(in.channel(ch)[i]), value);
                }
            }
        std::fill(s.values.begin(), s.values.end(), envelopeState);
        const double over = 20 * std::log10(std::max(1e-12, lastEnvelope)) - s.p.thresholdDb;
        db = 0;
        if (s.p.kneeDb > 0 && std::abs(over) < halfKnee) {
            const double t = over + halfKnee;
            db = slope * t * t / (2 * s.p.kneeDb);
        } else if (over >= halfKnee)
            db = slope * over;
        s.reduction = db;
        s.minimumGain = minimumGain;
        return Status::ok;
    }
    for (std::size_t i = 0; i < in.frames(); ++i) {
        const auto readHead =
            s.delayFrames ? (s.head + s.delayCapacity - s.delayFrames) % s.delayCapacity : 0;
        double linked = 0;
        if (s.p.linkedChannels)
            for (std::size_t ch = 0; ch < side.channels(); ++ch)
                linked = std::max(linked, std::abs(static_cast<double>(side.channel(ch)[i])));
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            const double x = in.channel(ch)[i],
                         magnitude =
                             s.p.linkedChannels
                                 ? linked
                                 : std::abs(double(side.channel(side.channels() == 1 ? 0 : ch)[i]));
            const double detector =
                s.p.detector == DetectorMode::rms ? magnitude * magnitude : magnitude;
            double ahead = detector;
            if (s.delayFrames) {
                const auto base = ch * s.delayCapacity;
                auto& first = s.peakRead[ch];
                auto& count = s.peakCount[ch];
                while (count && s.clock - s.peakTimes[base + first] > s.delayFrames) {
                    first = (first + 1) % s.delayCapacity;
                    --count;
                }
                while (count) {
                    const auto back = (first + count - 1) % s.delayCapacity;
                    if (s.peaks[base + back] > detector)
                        break;
                    --count;
                }
                const auto write = (first + count) % s.delayCapacity;
                s.peaks[base + write] = detector;
                s.peakTimes[base + write] = s.clock;
                ++count;
                ahead = s.peaks[base + first];
            }
            s.values[ch] = advance(ahead, s.values[ch], s.attack, s.release);
            const double envelope = s.p.detector == DetectorMode::rms
                                        ? std::sqrt(std::max(0.0, s.values[ch]))
                                        : s.values[ch];
            const double level = 20 * std::log10(std::max(1e-12, envelope));
            double over = level - s.p.thresholdDb, db = 0;
            if (s.mode == DynamicsMode::compressor || s.mode == DynamicsMode::limiter) {
                if (s.p.kneeDb > 0 && std::abs(over) < halfKnee) {
                    const double t = over + halfKnee;
                    db = slope * t * t / (2 * s.p.kneeDb);
                } else if (over >= halfKnee)
                    db = slope * over;
            } else if (s.mode == DynamicsMode::noiseGate) {
                if (level >= s.p.thresholdDb) {
                    s.gateOpen[ch] = 1;
                    s.hold[ch] = holdFrames;
                } else if (s.gateOpen[ch] && level < s.p.thresholdDb - s.p.hysteresisDb) {
                    if (s.hold[ch])
                        --s.hold[ch];
                    else
                        s.gateOpen[ch] = 0;
                } else if (s.gateOpen[ch])
                    s.hold[ch] = holdFrames;
                s.gateGain[ch] =
                    advance(s.gateOpen[ch] ? 1.0 : 0.0, s.gateGain[ch], s.attack, s.release);
            } else if (over < 0)
                db = std::max(-160.0, (s.p.ratio - 1) * over);
            s.reduction = db;
            const auto index = ch * s.delayCapacity + s.head;
            const double delayed = s.delayFrames ? s.delay[ch * s.delayCapacity + readHead] : x;
            s.delay[index] = static_cast<T>(x);
            // The gate envelope already is a linear gain. Retain the -160 dB floor,
            // avoiding the log10/pow round trip for every sample.
            const double gain = s.mode == DynamicsMode::noiseGate ? std::max(1e-8, s.gateGain[ch])
                                                                  : std::pow(10.0, db / 20);
            minimumGain = std::min(minimumGain, gain * s.makeup);
            double value = delayed * gain * s.makeup;
            if (s.mode == DynamicsMode::limiter)
                value = std::clamp(value, -ceiling, ceiling);
            out.channel(ch)[i] = publish(delayed, value);
        }
        s.head = (s.head + 1) % s.delayCapacity;
        ++s.clock;
    }
    s.minimumGain = minimumGain;
    // Observers are owner-serialized; only the final channel/frame is observable.
    // Empty blocks must preserve the previous meter value.
    if (s.mode == DynamicsMode::noiseGate && in.frames())
        s.reduction = 20 * std::log10(std::max(1e-8, s.gateGain[in.channels() - 1]));
    return Status::ok;
}
template class BallisticsFilter<float>;
template class BallisticsFilter<double>;
template class Dynamics<float>;
template class Dynamics<double>;
} // namespace cydsp
