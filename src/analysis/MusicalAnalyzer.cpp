#include "cydsp/analysis/MusicalAnalyzer.h"
#include "cydsp/processors/RealFft.h"
#include "detail/Analysis.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>
namespace cydsp {
template <Sample T> struct MusicalAnalyzer<T>::State {
    ProcessSpec spec;
    MusicalAnalysisConfig config;
    RealFft<double> fft;
    std::vector<double> history, time, reference, correlation, energy, difference, window, previous,
        melWeights, melLog;
    std::vector<std::complex<double>> spectrum, refSpectrum;
    std::size_t head = 0, filled = 0, since = 0, minLag = 0, maxLag = 0, onsetCount = 0;
    std::uint64_t clock = 0, lastOnset = 0;
    double fluxMean = 0, tempo = 0;
    std::array<double, 8> intervals{};
    MusicalFeatures result;
    Callback callback = nullptr;
    void* context = nullptr;
    void analyze() noexcept {
        const auto n = config.fftFrames, half = n / 2, bins = fft.bins();
        double mean = 0;
        for (std::size_t i = 0; i < n; ++i) {
            time[i] = history[(head + i) % n];
            mean += time[i] / double(n);
        }
        double power = 0;
        for (auto& x : time) {
            x -= mean;
            power += x * x;
        }
        std::fill(reference.begin(), reference.end(), 0);
        std::copy_n(time.begin(), half, reference.begin());
        (void)fft.forward(time, spectrum);
        (void)fft.forward(reference, refSpectrum);
        for (std::size_t k = 0; k < bins; ++k)
            refSpectrum[k] = spectrum[k] * std::conj(refSpectrum[k]);
        (void)fft.inverse(refSpectrum, correlation);
        energy[0] = 0;
        for (std::size_t i = 0; i < n; ++i)
            energy[i + 1] = energy[i] + time[i] * time[i];
        difference[0] = 1;
        double sum = 0;
        for (std::size_t tau = 1; tau <= maxLag + 1; ++tau) {
            const double d = std::max(0., energy[half] + energy[half + tau] - energy[tau] -
                                              2 * correlation[tau]);
            sum += d;
            difference[tau] = sum > 1e-25 ? d * double(tau) / sum : 1;
        }
        result.fundamentalHz = result.pitchConfidence = 0;
        if (power / double(n) > 1e-12) {
            std::size_t best = 0;
            for (std::size_t tau = minLag; tau <= maxLag; ++tau)
                if (difference[tau] < .15) {
                    while (tau < maxLag && difference[tau + 1] < difference[tau])
                        ++tau;
                    best = tau;
                    break;
                }
            if (!best) {
                best = minLag;
                for (std::size_t tau = minLag + 1; tau <= maxLag; ++tau)
                    if (difference[tau] < difference[best])
                        best = tau;
            }
            const double confidence = std::clamp(1 - difference[best], 0., 1.);
            if (confidence > .65) {
                const double a = difference[best - 1], b = difference[best],
                             c = difference[best + 1], den = a - 2 * b + c;
                const double offset =
                    std::abs(den) > 1e-12 ? std::clamp(.5 * (a - c) / den, -.5, .5) : 0;
                result.fundamentalHz = spec.sampleRate / (double(best) + offset);
                result.pitchConfidence = confidence;
            }
        }
        for (std::size_t i = 0; i < n; ++i)
            time[i] *= window[i];
        (void)fft.forward(time, spectrum);
        result.chroma = {};
        double flux = 0, total = 0;
        for (std::size_t k = 0; k < bins; ++k) {
            const double magnitude = std::abs(spectrum[k]) / double(n);
            correlation[k] = magnitude * magnitude;
            flux += std::max(0., magnitude - previous[k]);
            total += magnitude;
            previous[k] = magnitude;
            const double frequency = double(k) * spec.sampleRate / double(n);
            if (frequency >= 30 && frequency <= 5000) {
                const double midi = 69 + 12 * std::log2(frequency / 440);
                const auto base = int(std::floor(midi));
                const double fraction = midi - base;
                const unsigned p = unsigned((base % 12 + 12) % 12);
                result.chroma[p] += correlation[k] * (1 - fraction);
                result.chroma[(p + 1) % 12] += correlation[k] * fraction;
            }
        }
        const double strength = total > 1e-10 ? flux / total : 0;
        const double average = fluxMean;
        fluxMean += .1 * (strength - fluxMean);
        result.onsetStrength = strength;
        result.onset = strength > std::max(.08, average * config.onsetThreshold) && total > 1e-6 &&
                       (lastOnset == 0 || double(clock - lastOnset) > spec.sampleRate * .07);
        if (result.onset) {
            if (lastOnset) {
                double interval = double(clock - lastOnset) / spec.sampleRate;
                while (60 / interval > config.maximumBpm)
                    interval *= 2;
                while (60 / interval < config.minimumBpm)
                    interval *= .5;
                if (60 / interval >= config.minimumBpm && 60 / interval <= config.maximumBpm) {
                    intervals[onsetCount % 8] = interval;
                    ++onsetCount;
                    const auto count = std::min<std::size_t>(onsetCount, 8);
                    std::array<double, 8> sorted = intervals;
                    std::sort(sorted.begin(), sorted.begin() + count);
                    const double median = sorted[count / 2];
                    double value = 0;
                    std::size_t used = 0;
                    for (std::size_t i = 0; i < count; ++i)
                        if (std::abs(intervals[i] - median) < median * .12) {
                            value += intervals[i];
                            ++used;
                        }
                    tempo = used ? 60 / (value / double(used)) : 0;
                }
            }
            lastOnset = clock;
        }
        result.tempoBpm = tempo;
        result.beatConfidence = std::min(1., double(onsetCount) / 6);
        result.beatPhase =
            tempo > 0 ? std::fmod(double(clock - lastOnset) * tempo / (60 * spec.sampleRate), 1.)
                      : 0;
        double chromaSum = 0;
        for (auto x : result.chroma)
            chromaSum += x;
        for (auto& x : result.chroma)
            x = chromaSum > 1e-20 ? x / chromaSum : 0;
        double bestScore = -1e10, second = -1e10;
        for (unsigned minor = 0; minor < 2; ++minor)
            for (unsigned root = 0; root < 12; ++root) {
                const std::array<int, 7> scale = minor ? std::array<int, 7>{0, 2, 3, 5, 7, 8, 10}
                                                       : std::array<int, 7>{0, 2, 4, 5, 7, 9, 11};
                double score = 0;
                for (int pc = 0; pc < 12; ++pc) {
                    double weight = -.5;
                    for (auto note : scale)
                        if (note == pc)
                            weight = .5;
                    if (pc == 0)
                        weight = 1.5;
                    if (pc == (minor ? 3 : 4) || pc == 7)
                        weight = 1;
                    score += result.chroma[(root + unsigned(pc)) % 12] * weight;
                }
                if (score > bestScore) {
                    second = bestScore;
                    bestScore = score;
                    result.keyPitchClass = root;
                    result.keyMinor = minor != 0;
                } else
                    second = std::max(second, score);
            }
        result.keyConfidence = chromaSum > 1e-12 ? std::clamp((bestScore - second) * 4, 0., 1.) : 0;
        for (std::size_t m = 0; m < config.melBands; ++m) {
            double p = 0;
            for (std::size_t k = 0; k < bins; ++k)
                p += melWeights[m * bins + k] * correlation[k];
            melLog[m] = std::log(std::max(1e-20, p));
        }
        result.mfcc = {};
        for (std::size_t c = 0; c < config.mfccCount; ++c) {
            double value = 0;
            for (std::size_t m = 0; m < config.melBands; ++m)
                value += melLog[m] * std::cos(std::numbers::pi * double(c) * (double(m) + .5) /
                                              double(config.melBands));
            result.mfcc[c] = value * std::sqrt((c ? 2. : 1.) / double(config.melBands));
        }
        result.frameEnd = clock;
        if (callback)
            callback(context, result);
    }
};
template <Sample T> MusicalAnalyzer<T>::MusicalAnalyzer() = default;
template <Sample T> MusicalAnalyzer<T>::~MusicalAnalyzer() = default;
template <Sample T> MusicalAnalyzer<T>::MusicalAnalyzer(MusicalAnalyzer&&) noexcept = default;
template <Sample T>
MusicalAnalyzer<T>& MusicalAnalyzer<T>::operator=(MusicalAnalyzer&&) noexcept = default;
template <Sample T>
Status MusicalAnalyzer<T>::prepare(const ProcessSpec& spec, const MusicalAnalysisConfig& c,
                                   CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || !isPowerOfTwo(c.fftFrames) || c.fftFrames < 256 ||
        c.fftFrames > 32768 || !c.hopFrames || c.hopFrames > c.fftFrames || c.melBands < 8 ||
        c.melBands > 80 || !c.mfccCount || c.mfccCount > 32 || c.mfccCount > c.melBands ||
        !isFinite(c.minimumPitchHz) || !isFinite(c.maximumPitchHz) ||
        c.minimumPitchHz < spec.sampleRate / double(c.fftFrames / 2 - 2) ||
        c.maximumPitchHz <= c.minimumPitchHz || c.maximumPitchHz > spec.sampleRate * .25 ||
        !isFinite(c.onsetThreshold) || c.onsetThreshold < 1 || c.onsetThreshold > 10 ||
        !isFinite(c.minimumBpm) || !isFinite(c.maximumBpm) || c.minimumBpm < 20 ||
        c.maximumBpm < c.minimumBpm || c.maximumBpm > 400)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->config = c;
        s->minLag =
            std::max<std::size_t>(2, std::size_t(std::floor(spec.sampleRate / c.maximumPitchHz)));
        s->maxLag = std::min(c.fftFrames / 2 - 1,
                             std::size_t(std::ceil(spec.sampleRate / c.minimumPitchHz)));
        auto status = s->fft.prepare(c.fftFrames, backend);
        if (status != Status::ok)
            return status;
        const auto n = c.fftFrames, bins = s->fft.bins();
        for (auto* v : {&s->history, &s->time, &s->reference, &s->correlation, &s->window})
            v->resize(n);
        s->energy.resize(n + 1);
        s->difference.resize(s->maxLag + 2);
        s->previous.resize(bins);
        s->spectrum.resize(bins);
        s->refSpectrum.resize(bins);
        s->melWeights.resize(c.melBands * bins);
        s->melLog.resize(c.melBands);
        for (std::size_t i = 0; i < n; ++i)
            s->window[i] = .5 - .5 * std::cos(2 * std::numbers::pi * double(i) / double(n));
        const double melMax = 2595 * std::log10(1 + spec.sampleRate * .5 / 700);
        for (std::size_t m = 0; m < c.melBands; ++m) {
            const double
                a = 700 * (std::pow(10., melMax * double(m) / double(c.melBands + 1) / 2595) - 1),
                b = 700 *
                    (std::pow(10., melMax * double(m + 1) / double(c.melBands + 1) / 2595) - 1),
                d = 700 *
                    (std::pow(10., melMax * double(m + 2) / double(c.melBands + 1) / 2595) - 1);
            for (std::size_t k = 0; k < bins; ++k) {
                const double f = double(k) * spec.sampleRate / double(n);
                s->melWeights[m * bins + k] =
                    std::max(0., std::min((f - a) / (b - a), (d - f) / (d - b)));
            }
        }
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
void MusicalAnalyzer<T>::setCallback(Callback callback, void* context) noexcept {
    if (state_) {
        state_->callback = callback;
        state_->context = context;
    }
}
template <Sample T> Status MusicalAnalyzer<T>::process(AudioBlock<const T> in) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    for (std::size_t i = 0; i < in.frames(); ++i) {
        double x = 0;
        for (std::size_t ch = 0; ch < in.channels(); ++ch)
            x += double(in.channel(ch)[i]) / double(in.channels());
        s.history[s.head] = x;
        s.head = (s.head + 1) % s.config.fftFrames;
        s.filled = std::min(s.filled + 1, s.config.fftFrames);
        ++s.clock;
        ++s.since;
        if (s.filled == s.config.fftFrames && s.since >= s.config.hopFrames) {
            s.since = 0;
            s.analyze();
        }
    }
    return Status::ok;
}
template <Sample T> MusicalFeatures MusicalAnalyzer<T>::features() const noexcept {
    return state_ ? state_->result : MusicalFeatures{};
}
template <Sample T> void MusicalAnalyzer<T>::reset() noexcept {
    if (state_) {
        auto& s = *state_;
        std::fill(s.history.begin(), s.history.end(), 0);
        std::fill(s.previous.begin(), s.previous.end(), 0);
        s.head = s.filled = s.since = s.onsetCount = 0;
        s.clock = s.lastOnset = 0;
        s.fluxMean = s.tempo = 0;
        s.intervals = {};
        s.result = {};
    }
}
template class MusicalAnalyzer<float>;
template class MusicalAnalyzer<double>;
} // namespace cydsp
