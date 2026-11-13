#include "cydsp/spectral/SpectralPitch.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/math/VectorOps.h"
#include "detail/Analysis.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>
namespace cydsp {
namespace {
double wrapPhase(double x) noexcept {
    return std::remainder(x, 2 * std::numbers::pi);
}
} // namespace
template <Sample T> struct SpectralPitchShifter<T>::State {
    ProcessSpec spec;
    StftConfig config;
    SpectralPitchParameters p;
    Stft<T> stft;
    VectorOps ops;
    AudioBuffer<T> packed, processed;
    std::vector<std::complex<T>> input, output;
    std::vector<double> magnitude, previousMagnitude, phase, previousPhase, frequency, synthPhase,
        envelope, work;
    std::vector<std::size_t> guide, peaks;
    std::size_t bins = 0;
    bool first = true;
    double interpolate(const std::vector<double>& x, double position) const noexcept {
        const auto a = std::min(bins - 1, std::size_t(position)), b = std::min(bins - 1, a + 1);
        return x[a] + (x[b] - x[a]) * (position - double(a));
    }
    double interpolatedPhase(double position) const noexcept {
        const auto low = std::min(bins - 1, std::size_t(position)),
                   high = std::min(bins - 1, low + 1);
        return phase[low] + wrapPhase(phase[high] - phase[low]) * (position - double(low));
    }
    void transform() noexcept {
        const auto b = bins, c = spec.channels;
        double flux = 0, total = 0;
        for (std::size_t k = 0; k < b; ++k) {
            std::size_t selected = guide[k];
            double power = std::norm(std::complex<double>(input[selected * b + k]));
            // Retain the guide until another channel is substantially stronger.
            for (std::size_t ch = 0; ch < c; ++ch) {
                const double candidate = std::norm(std::complex<double>(input[ch * b + k]));
                if (candidate > power * 4) {
                    selected = ch;
                    power = candidate;
                }
            }
            const double currentPhase = std::arg(input[selected * b + k]);
            if (selected != guide[k] && !first)
                previousPhase[k] += wrapPhase(currentPhase - std::arg(input[guide[k] * b + k]));
            guide[k] = selected;
            phase[k] = currentPhase;
            magnitude[k] = std::sqrt(power);
            const double expected = 2 * std::numbers::pi * double(k) * double(config.hopFrames) /
                                    double(config.fftFrames);
            frequency[k] = (expected + wrapPhase(currentPhase - previousPhase[k] - expected)) /
                           double(config.hopFrames);
            if (first)
                frequency[k] = 2 * std::numbers::pi * double(k) / double(config.fftFrames);
            previousPhase[k] = currentPhase;
            flux += std::max(0., magnitude[k] - previousMagnitude[k]);
            total += previousMagnitude[k];
            previousMagnitude[k] = magnitude[k];
            envelope[k] = magnitude[k];
        }
        // Two symmetric box averages estimate the amplitude envelope without
        // log-domain notches from empty bins between harmonics. Gains use logs.
        const auto radius = std::min<std::size_t>(
            b - 1,
            std::max<std::size_t>(1, std::size_t(p.envelopeWidthHz * double(config.fftFrames) /
                                                 spec.sampleRate * .5)));
        for (int pass = 0; pass < 2; ++pass) {
            double sum = 0;
            std::size_t end = 0, start = 0;
            for (std::size_t k = 0; k < b; ++k) {
                const auto wantedEnd = std::min(b, k + radius + 1),
                           wantedStart = k > radius ? k - radius : 0;
                while (end < wantedEnd)
                    sum += envelope[end++];
                while (start < wantedStart)
                    sum -= envelope[start++];
                work[k] = sum / double(end - start);
            }
            envelope.swap(work);
        }
        for (auto& value : envelope)
            value = std::log(std::max(1e-20, value));
        const bool transient = first || (p.protectTransients && total > 1e-12 &&
                                         flux / total > p.transientFluxThreshold);
        for (std::size_t k = 0; k < b; ++k) {
            const double source = double(k) / p.ratio;
            if (source > double(b - 1)) {
                synthPhase[k] = 0;
                continue;
            }
            if (transient)
                synthPhase[k] = interpolatedPhase(source);
            else
                synthPhase[k] = wrapPhase(synthPhase[k] + interpolate(frequency, source) * p.ratio *
                                                              double(config.hopFrames));
        }
        std::size_t peakCount = 0;
        const double peakFloor = *std::max_element(magnitude.begin(), magnitude.end()) * 1e-5;
        for (std::size_t k = 1; k + 1 < b; ++k)
            if (magnitude[k] > peakFloor && magnitude[k] > magnitude[k - 1] &&
                magnitude[k] >= magnitude[k + 1])
                peaks[peakCount++] = k;
        std::size_t nearest = 0;
        for (std::size_t k = 0; k < b; ++k) {
            const double source = double(k) / p.ratio;
            if (source > double(b - 1)) {
                for (std::size_t ch = 0; ch < c; ++ch)
                    output[ch * b + k] = {};
                continue;
            }
            const auto low = std::min(b - 1, std::size_t(source)), high = std::min(b - 1, low + 1);
            const double mix = source - double(low);
            double commonPhase = synthPhase[k];
            if (p.lockSpectralPeaks && peakCount && !transient) {
                while (nearest + 1 < peakCount && std::abs(double(peaks[nearest + 1]) - source) <
                                                      std::abs(double(peaks[nearest]) - source))
                    ++nearest;
                const auto peak = peaks[nearest],
                           mapped =
                               std::min(b - 1, std::size_t(std::round(double(peak) * p.ratio)));
                commonPhase =
                    synthPhase[mapped] + wrapPhase(interpolatedPhase(source) - phase[peak]);
            }
            const double formant =
                p.preserveFormants
                    ? std::exp(std::clamp(envelope[k] - interpolate(envelope, source), -3., 3.))
                    : 1;
            const auto g = guide[low];
            for (std::size_t ch = 0; ch < c; ++ch) {
                const auto a = input[ch * b + low], z = input[ch * b + high];
                const double mag = (std::abs(a) + (std::abs(z) - std::abs(a)) * mix) * formant;
                const double relative = wrapPhase(std::arg(a) - std::arg(input[g * b + low]));
                output[ch * b + k] = std::polar(T(mag), T(commonPhase + relative));
            }
        }
        if (p.ratio == 1)
            std::copy(input.begin(), input.end(), output.begin());
        first = false;
    }
    static void callback(void* ptr, std::size_t ch, std::span<std::complex<T>> x) noexcept {
        auto& s = *static_cast<State*>(ptr);
        if (ch < s.spec.channels) {
            std::copy(x.begin(), x.end(), s.input.begin() + ch * s.bins);
            std::fill(x.begin(), x.end(), std::complex<T>{});
        } else {
            if (ch == s.spec.channels)
                s.transform();
            std::copy_n(s.output.begin() + (ch - s.spec.channels) * s.bins, s.bins, x.begin());
        }
    }
};
template <Sample T> SpectralPitchShifter<T>::SpectralPitchShifter() = default;
template <Sample T> SpectralPitchShifter<T>::~SpectralPitchShifter() = default;
template <Sample T>
SpectralPitchShifter<T>::SpectralPitchShifter(SpectralPitchShifter&&) noexcept = default;
template <Sample T>
SpectralPitchShifter<T>&
SpectralPitchShifter<T>::operator=(SpectralPitchShifter&&) noexcept = default;
template <Sample T>
Status SpectralPitchShifter<T>::prepare(const ProcessSpec& spec, const StftConfig& config,
                                        CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || spec.channels > 8 || config.fftFrames > 8192 ||
        config.hopFrames > config.fftFrames / 4)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->config = config;
        auto expanded = spec;
        expanded.channels *= 2;
        auto status = s->stft.prepare(expanded, config, backend);
        if (status != Status::ok)
            return status;
        status = VectorOps::create(backend, s->ops);
        if (status != Status::ok)
            return status;
        for (auto* buffer : {&s->packed, &s->processed}) {
            status = buffer->resize(expanded.channels, spec.maxBlockFrames);
            if (status != Status::ok)
                return status;
        }
        s->bins = s->stft.bins();
        s->input.resize(spec.channels * s->bins);
        s->output.resize(s->input.size());
        for (auto* v : {&s->magnitude, &s->previousMagnitude, &s->phase, &s->previousPhase,
                        &s->frequency, &s->synthPhase, &s->envelope, &s->work})
            v->resize(s->bins);
        s->guide.resize(s->bins);
        s->peaks.resize(s->bins);
        s->stft.setProcessor(State::callback, s.get());
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status SpectralPitchShifter<T>::setParameters(const SpectralPitchParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!isFinite(p.ratio) || p.ratio < .25 || p.ratio > 4 || !isFinite(p.envelopeWidthHz) ||
        p.envelopeWidthHz < 50 || p.envelopeWidthHz > 4000 || !isFinite(p.transientFluxThreshold) ||
        p.transientFluxThreshold < .01 || p.transientFluxThreshold > 4)
        return Status::invalid_argument;
    state_->p = p;
    return Status::ok;
}
template <Sample T>
Status SpectralPitchShifter<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, in, out);
    if (status != Status::ok)
        return status;
    status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    auto packed = s.packed.block(in.frames()), processed = s.processed.block(in.frames());
    for (std::size_t ch = 0; ch < in.channels(); ++ch) {
        (void)s.ops.copy(in.channel(ch), packed.channel(ch));
        s.ops.fill(packed.channel(ch + in.channels()), T{});
    }
    status = s.stft.process(packed, processed);
    if (status != Status::ok)
        return status;
    for (std::size_t ch = 0; ch < out.channels(); ++ch)
        (void)s.ops.copy(processed.channel(ch + in.channels()), out.channel(ch));
    return Status::ok;
}
template <Sample T> void SpectralPitchShifter<T>::reset() noexcept {
    if (state_) {
        auto& s = *state_;
        s.stft.reset();
        s.first = true;
        std::fill(s.previousMagnitude.begin(), s.previousMagnitude.end(), 0);
        std::fill(s.guide.begin(), s.guide.end(), 0);
    }
}
template <Sample T> std::size_t SpectralPitchShifter<T>::latencyFrames() const noexcept {
    return state_ ? state_->config.fftFrames : 0;
}
template class SpectralPitchShifter<float>;
template class SpectralPitchShifter<double>;
} // namespace cydsp
