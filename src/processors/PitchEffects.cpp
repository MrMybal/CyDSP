#include "cydsp/processors/PitchEffects.h"
#include "detail/Analysis.h"
#include "detail/HistorySinc.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
namespace cydsp {
template <Sample T> struct PitchEffects<T>::State {
    struct Voice {
        HarmonyVoice p;
        double phase = 0;
    };
    ProcessSpec spec;
    MusicalAnalyzer<T> analyzer;
    detail::HistorySinc sinc;
    CircularHistory<T> history;
    std::vector<Voice> voices;
    std::vector<SpectralPitchShifter<T>> spectral;
    CircularHistory<T> shiftedHistory;
    std::size_t spectralLatency = 0;
    std::size_t count = 1;
    PitchCorrectionParameters correction;
    double ratio = 1, window = 0;
};
template <Sample T> PitchEffects<T>::PitchEffects() = default;
template <Sample T> PitchEffects<T>::~PitchEffects() = default;
template <Sample T> PitchEffects<T>::PitchEffects(PitchEffects&&) noexcept = default;
template <Sample T> PitchEffects<T>& PitchEffects<T>::operator=(PitchEffects&&) noexcept = default;
template <Sample T>
Status PitchEffects<T>::prepare(const ProcessSpec& spec, std::size_t maximum,
                                const MusicalAnalysisConfig& config, CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || spec.channels > 2 || !maximum || maximum > 8)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->window = spec.sampleRate * .04;
        s->voices.resize(maximum);
        auto status = s->analyzer.prepare(spec, config, backend);
        if (status != Status::ok)
            return status;
        status = s->history.prepare(spec.channels, std::size_t(spec.sampleRate * .3) + 64);
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
Status PitchEffects<T>::prepareSpectral(const ProcessSpec& spec, std::size_t maximum,
                                        const StftConfig& stft,
                                        const MusicalAnalysisConfig& analysis,
                                        CpuBackend backend) noexcept {
    try {
        PitchEffects<T> next;
        auto status = next.prepare(spec, maximum, analysis, backend);
        if (status != Status::ok)
            return status;
        auto& s = *next.state_;
        s.spectral.resize(maximum);
        for (auto& processor : s.spectral) {
            status = processor.prepare(spec, stft, backend);
            if (status != Status::ok)
                return status;
        }
        status = s.shiftedHistory.prepare(spec.channels * maximum,
                                          std::size_t(spec.sampleRate * .201) + 8);
        if (status != Status::ok)
            return status;
        s.spectralLatency = stft.fftFrames;
        state_.swap(next.state_);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> std::size_t PitchEffects<T>::latencyFrames() const noexcept {
    return state_ ? state_->spectralLatency : 0;
}

template <Sample T>
Status PitchEffects<T>::setVoices(std::span<const HarmonyVoice> voices) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (voices.size() > state_->voices.size())
        return Status::invalid_argument;
    for (auto& p : voices)
        if (!isFinite(p.semitones) || std::abs(p.semitones) > 24 || !isFinite(p.gain) ||
            std::abs(p.gain) > 8 || !isFinite(p.pan) || std::abs(p.pan) > 1 ||
            !isFinite(p.delayMs) || p.delayMs < 0 || p.delayMs > 200)
            return Status::invalid_argument;
    for (std::size_t i = 0; i < voices.size(); ++i)
        state_->voices[i].p = voices[i];
    state_->count = voices.size();
    return Status::ok;
}
template <Sample T>
Status PitchEffects<T>::setCorrection(const PitchCorrectionParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!isFinite(p.targetHz) || p.targetHz < 0 || p.targetHz > state_->spec.sampleRate * .25 ||
        !isFinite(p.referenceA4Hz) || p.referenceA4Hz < 300 || p.referenceA4Hz > 600 ||
        !isFinite(p.retuneMs) || p.retuneMs < 0 || p.retuneMs > 5000 || !p.scalePitchClasses ||
        p.scalePitchClasses > 0xfff)
        return Status::invalid_argument;
    state_->correction = p;
    return Status::ok;
}
template <Sample T>
Status PitchEffects<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, in, out);
    if (status != Status::ok)
        return status;
    status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    // Analyze one frame at a time so block segmentation cannot anticipate pitch.
    std::array<T, 2> frame{}, shifted{};
    std::array<T, 16> shiftedFrame{};
    std::array<T*, 2> shiftedPointers{shifted.data(), shifted.data() + 1};
    std::array<const T*, 2> ptr{frame.data(), frame.data() + 1};
    const double smoothing = s.correction.retuneMs == 0
                                 ? 0
                                 : std::exp(-1000 / (s.correction.retuneMs * s.spec.sampleRate));
    for (std::size_t i = 0; i < in.frames(); ++i) {
        for (std::size_t ch = 0; ch < in.channels(); ++ch)
            frame[ch] = in.channel(ch)[i];
        (void)s.analyzer.process({ptr.data(), in.channels(), 1});
        (void)s.history.pushFrame({frame.data(), in.channels()});
        const auto pitch = s.analyzer.features();
        double targetRatio = 1;
        if (s.correction.enabled && pitch.pitchConfidence > .7 && pitch.fundamentalHz > 0) {
            double target = s.correction.targetHz;
            if (target == 0) {
                const double midi =
                    69 + 12 * std::log2(pitch.fundamentalHz / s.correction.referenceA4Hz);
                double nearest = 0, distance = 1e10;
                const int rounded = int(std::round(midi));
                for (int note = rounded - 12; note <= rounded + 12; ++note)
                    if (s.correction.scalePitchClasses & (1u << unsigned((note % 12 + 12) % 12)))
                        if (std::abs(note - midi) < distance) {
                            distance = std::abs(note - midi);
                            nearest = double(note);
                        }
                target = s.correction.referenceA4Hz * std::exp2((nearest - 69) / 12);
            }
            targetRatio = std::clamp(target / pitch.fundamentalHz, .25, 4.);
        }
        s.ratio = targetRatio + smoothing * (s.ratio - targetRatio);
        std::array<double, 2> sum{};
        for (std::size_t v = 0; v < s.count; ++v) {
            auto& voice = s.voices[v];
            const double voiceRatio =
                std::clamp(s.ratio * std::exp2(voice.p.semitones / 12), .25, 4.);
            if (!s.spectral.empty()) {
                SpectralPitchParameters parameters;
                parameters.ratio = voiceRatio;
                (void)s.spectral[v].setParameters(parameters);
                (void)s.spectral[v].process({ptr.data(), in.channels(), 1},
                                            {shiftedPointers.data(), in.channels(), 1});
                for (std::size_t ch = 0; ch < in.channels(); ++ch)
                    shiftedFrame[v * in.channels() + ch] = shifted[ch];
            } else {
                const double ratio = voiceRatio, other = std::fmod(voice.phase + .5, 1.),
                             weight = .5 - .5 * std::cos(2 * std::numbers::pi * voice.phase),
                             base = 16 + voice.p.delayMs * s.spec.sampleRate / 1000;
                for (std::size_t ch = 0; ch < in.channels(); ++ch) {
                    const double a = s.sinc.read(s.history, ch, base + voice.phase * s.window,
                                                 ratio),
                                 b = s.sinc.read(s.history, ch, base + other * s.window, ratio),
                                 pan = in.channels() == 1
                                           ? 1
                                           : std::sqrt(1 + (ch == 0 ? -1 : 1) * voice.p.pan);
                    sum[ch] += voice.p.gain * pan * (weight * a + (1 - weight) * b);
                }
                voice.phase += (1 - ratio) / s.window;
                voice.phase -= std::floor(voice.phase);
            }
        }
        if (!s.spectral.empty()) {
            (void)s.shiftedHistory.pushFrame(
                {shiftedFrame.data(), s.spectral.size() * in.channels()});
            for (std::size_t v = 0; v < s.count; ++v)
                for (std::size_t ch = 0; ch < in.channels(); ++ch) {
                    const auto& p = s.voices[v].p;
                    const double pan =
                        in.channels() == 1 ? 1 : std::sqrt(1 + (ch == 0 ? -1 : 1) * p.pan);
                    sum[ch] += p.gain * pan *
                               s.shiftedHistory.read(v * in.channels() + ch,
                                                     p.delayMs * s.spec.sampleRate / 1000);
                }
        }
        for (std::size_t ch = 0; ch < out.channels(); ++ch)
            out.channel(ch)[i] = T(sum[ch]);
    }
    return Status::ok;
}
template <Sample T> void PitchEffects<T>::reset() noexcept {
    if (state_) {
        state_->analyzer.reset();
        state_->history.reset();
        state_->shiftedHistory.reset();
        for (auto& processor : state_->spectral)
            processor.reset();
        state_->ratio = 1;
        for (auto& v : state_->voices)
            v.phase = 0;
    }
}
template <Sample T> double PitchEffects<T>::detectedPitchHz() const noexcept {
    return state_ ? state_->analyzer.features().fundamentalHz : 0;
}
template class PitchEffects<float>;
template class PitchEffects<double>;
} // namespace cydsp
