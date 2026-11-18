#include "cydsp/processors/Restoration.h"
#include "cydsp/core/CircularHistory.h"
#include "cydsp/processors/BiquadCascade.h"
#include "detail/Analysis.h"
#include "detail/BiquadAlgorithm.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
namespace cydsp {
template <Sample T> struct SpectralDenoiser<T>::State {
    ProcessSpec spec;
    Stft<T> stft;
    DenoiseParameters p;
    std::vector<double> noise, persistent, gain;
    std::vector<std::size_t> dwell;
    std::size_t bins = 0, hop = 0;
    double smooth = 0, persist = 0;
    bool learningFirst = true;
    static void callback(void* raw, std::size_t ch, std::span<std::complex<T>> bins) noexcept {
        auto& s = *static_cast<State*>(raw);
        const auto base = ch * s.bins;
        for (std::size_t k = 0; k < s.bins; ++k) {
            const double power = std::norm(std::complex<double>(bins[k]));
            auto& average = s.persistent[base + k];
            average = power + s.persist * (average - power);
            if (s.p.learnNoise)
                s.noise[base + k] = s.learningFirst ? power : .95 * s.noise[base + k] + .05 * power;
        }
        for (std::size_t k = 0; k < s.bins; ++k) {
            const auto index = base + k;
            const double power = std::norm(std::complex<double>(bins[k]));
            double target = 1;
            if (!s.p.learnNoise && s.noise[index] > 0)
                target = std::max(std::pow(10., -s.p.reductionDb / 20),
                                  std::sqrt(std::max(0., 1 - s.p.strength * s.noise[index] /
                                                                 std::max(1e-30, power))));
            if (s.p.resonanceReductionDb > 0) {
                double neighbours = 0;
                std::size_t count = 0;
                for (std::size_t j = k > 8 ? k - 8 : 0; j <= std::min(s.bins - 1, k + 8); ++j)
                    if (j + 1 < k || j > k + 1) {
                        neighbours += s.persistent[base + j];
                        ++count;
                    }
                const double ratio =
                    s.persistent[index] /
                    std::max(1e-30, neighbours / double(std::max<std::size_t>(1, count)));
                const auto required =
                    std::size_t(std::ceil(s.p.persistenceMs * s.spec.sampleRate / 1000));
                s.dwell[index] = ratio > std::pow(10., s.p.resonanceThresholdDb / 10)
                                     ? std::min(required, s.dwell[index] + s.hop)
                                     : 0;
                if (s.dwell[index] >= required)
                    target *= std::pow(10., -s.p.resonanceReductionDb / 20);
            }
            auto& g = s.gain[index];
            g = target + s.smooth * (g - target);
            bins[k] *= T(g);
        }
        if (ch + 1 == s.spec.channels && s.p.learnNoise)
            s.learningFirst = false;
    }
};
template <Sample T> SpectralDenoiser<T>::SpectralDenoiser() = default;
template <Sample T> SpectralDenoiser<T>::~SpectralDenoiser() = default;
template <Sample T> SpectralDenoiser<T>::SpectralDenoiser(SpectralDenoiser&&) noexcept = default;
template <Sample T>
SpectralDenoiser<T>& SpectralDenoiser<T>::operator=(SpectralDenoiser&&) noexcept = default;
template <Sample T>
Status SpectralDenoiser<T>::prepare(const ProcessSpec& spec, const StftConfig& config,
                                    CpuBackend backend) noexcept {
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->hop = config.hopFrames;
        auto status = s->stft.prepare(spec, config, backend);
        if (status != Status::ok)
            return status;
        s->bins = s->stft.bins();
        s->noise.resize(spec.channels * s->bins);
        s->persistent.resize(spec.channels * s->bins);
        s->gain.resize(spec.channels * s->bins, 1);
        s->dwell.resize(spec.channels * s->bins);
        s->smooth = std::exp(-double(s->hop) / (.03 * spec.sampleRate));
        s->persist = std::exp(-double(s->hop) / (.5 * spec.sampleRate));
        s->stft.setProcessor(State::callback, s.get());
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status SpectralDenoiser<T>::setParameters(const DenoiseParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!isFinite(p.reductionDb) || p.reductionDb < 0 || p.reductionDb > 96 ||
        !isFinite(p.strength) || p.strength < 0 || p.strength > 10 || !isFinite(p.smoothingMs) ||
        p.smoothingMs < 0 || p.smoothingMs > 10000 || !isFinite(p.resonanceThresholdDb) ||
        p.resonanceThresholdDb < 0 || p.resonanceThresholdDb > 96 ||
        !isFinite(p.resonanceReductionDb) || p.resonanceReductionDb < 0 ||
        p.resonanceReductionDb > 60 || !isFinite(p.persistenceMs) || p.persistenceMs < 1 ||
        p.persistenceMs > 10000)
        return Status::invalid_argument;
    auto& s = *state_;
    s.p = p;
    s.smooth = p.smoothingMs == 0
                   ? 0
                   : std::exp(-1000 * double(s.hop) / (p.smoothingMs * s.spec.sampleRate));
    s.persist = std::exp(-1000 * double(s.hop) / (p.persistenceMs * s.spec.sampleRate));
    return Status::ok;
}
template <Sample T>
Status SpectralDenoiser<T>::setNoiseProfile(std::span<const double> p) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (p.size() != state_->noise.size())
        return Status::invalid_argument;
    for (auto v : p)
        if (!isFinite(v) || v < 0 || v > 1e200)
            return Status::invalid_argument;
    std::copy(p.begin(), p.end(), state_->noise.begin());
    state_->learningFirst = false;
    return Status::ok;
}
template <Sample T> std::span<const double> SpectralDenoiser<T>::noiseProfile() const noexcept {
    return state_ ? std::span<const double>(state_->noise) : std::span<const double>{};
}
template <Sample T>
Status SpectralDenoiser<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    return state_ ? state_->stft.process(in, out) : Status::not_prepared;
}
template <Sample T> void SpectralDenoiser<T>::reset() noexcept {
    if (state_) {
        state_->stft.reset();
        std::fill(state_->persistent.begin(), state_->persistent.end(), 0);
        std::fill(state_->gain.begin(), state_->gain.end(), 1);
        std::fill(state_->dwell.begin(), state_->dwell.end(), 0);
    }
}
template <Sample T> std::size_t SpectralDenoiser<T>::latencyFrames() const noexcept {
    return state_ ? state_->stft.latencyFrames() : 0;
}

template <Sample T> struct Dehum<T>::State {
    ProcessSpec spec;
    BiquadCascade<T> cascade;
    std::size_t maximum = 0;
};
template <Sample T> Dehum<T>::Dehum() = default;
template <Sample T> Dehum<T>::~Dehum() = default;
template <Sample T> Dehum<T>::Dehum(Dehum&&) noexcept = default;
template <Sample T> Dehum<T>& Dehum<T>::operator=(Dehum&&) noexcept = default;
template <Sample T>
Status Dehum<T>::prepare(const ProcessSpec& spec, std::size_t maximum) noexcept {
    if (!detail::validAnalysisSpec(spec) || !maximum || maximum > 64)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->maximum = maximum;
        std::array<BiquadCoefficients, 64> identity{};
        auto status = s->cascade.prepare(spec, {identity.data(), maximum});
        if (status != Status::ok)
            return status;
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status Dehum<T>::setParameters(const DehumParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (!isFinite(p.fundamentalHz) || p.fundamentalHz < 10 ||
        p.fundamentalHz > s.spec.sampleRate * .25 || !isFinite(p.q) || p.q < .5 || p.q > 1000 ||
        !isFinite(p.reductionDb) || p.reductionDb < 0 || p.reductionDb > 120 ||
        p.harmonics > s.maximum)
        return Status::invalid_argument;
    std::array<BiquadCoefficients, 64> coefficients{};
    const double wet = 1 - std::pow(10., -p.reductionDb / 20);
    for (std::size_t i = 0; i < p.harmonics; ++i) {
        const double frequency = p.fundamentalHz * double(i + 1);
        if (frequency >= s.spec.sampleRate * .49)
            break;
        auto& c = coefficients[i];
        auto status = designBiquad(BiquadType::notch, s.spec.sampleRate, frequency, p.q, c);
        if (status != Status::ok)
            return status;
        c.b0 = (1 - wet) + wet * c.b0;
        c.b1 = (1 - wet) * c.a1 + wet * c.b1;
        c.b2 = (1 - wet) * c.a2 + wet * c.b2;
        if (!detail::roundBiquad<T>(c, c))
            return Status::invalid_argument;
    }
    for (std::size_t i = 0; i < s.maximum; ++i)
        (void)s.cascade.setSection(i, coefficients[i]);
    return Status::ok;
}
template <Sample T> Status Dehum<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto status = detail::validateAnalysis(state_->spec, in);
    return status == Status::ok ? state_->cascade.process(in, out) : status;
}
template <Sample T> void Dehum<T>::reset() noexcept {
    if (state_)
        state_->cascade.reset();
}

template <Sample T> struct Declicker<T>::State {
    ProcessSpec spec;
    DeclickParameters p;
    CircularHistory<T> history;
    std::size_t context = 0;
    std::uint64_t repaired = 0;
};
template <Sample T> Declicker<T>::Declicker() = default;
template <Sample T> Declicker<T>::~Declicker() = default;
template <Sample T> Declicker<T>::Declicker(Declicker&&) noexcept = default;
template <Sample T> Declicker<T>& Declicker<T>::operator=(Declicker&&) noexcept = default;
template <Sample T>
Status Declicker<T>::prepare(const ProcessSpec& spec, std::size_t context) noexcept {
    if (!detail::validAnalysisSpec(spec) || !context || context > 32)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->context = context;
        auto status = s->history.prepare(spec.channels, 2 * context + 1);
        if (status != Status::ok)
            return status;
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status Declicker<T>::setParameters(const DeclickParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!isFinite(p.absoluteThreshold) || p.absoluteThreshold < 0 || p.absoluteThreshold > 64 ||
        !isFinite(p.deviationRatio) || p.deviationRatio < 1 || p.deviationRatio > 100)
        return Status::invalid_argument;
    state_->p = p;
    return Status::ok;
}
template <Sample T>
Status Declicker<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, in, out);
    if (status != Status::ok)
        return status;
    status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    std::array<T, 64> frame{};
    std::array<double, 65> window{}, deviation{};
    const auto count = 2 * s.context + 1;
    for (std::size_t i = 0; i < in.frames(); ++i) {
        for (std::size_t ch = 0; ch < in.channels(); ++ch)
            frame[ch] = in.channel(ch)[i];
        (void)s.history.pushFrame({frame.data(), in.channels()});
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            (void)s.history.readWindow(ch, 0, std::span<double>(window).first(count));
            std::copy_n(window.begin(), count, deviation.begin());
            std::sort(deviation.begin(), deviation.begin() + count);
            const double median = deviation[s.context], center = window[s.context];
            for (std::size_t k = 0; k < count; ++k)
                deviation[k] = std::abs(window[k] - median);
            std::sort(deviation.begin(), deviation.begin() + count);
            const double threshold =
                s.p.absoluteThreshold + s.p.deviationRatio * deviation[s.context];
            double value = center;
            if (std::abs(center - median) > threshold) {
                std::size_t left = 0, right = 0;
                for (std::size_t k = 1; k <= s.context; ++k) {
                    if (!left && std::abs(window[s.context + k] - median) <= threshold)
                        left = k;
                    if (!right && std::abs(window[s.context - k] - median) <= threshold)
                        right = k;
                }
                value = left && right ? (window[s.context + left] * double(right) +
                                         window[s.context - right] * double(left)) /
                                            double(left + right)
                                      : median;
                ++s.repaired;
            }
            out.channel(ch)[i] = T(value);
        }
    }
    return Status::ok;
}
template <Sample T> void Declicker<T>::reset() noexcept {
    if (state_) {
        state_->history.reset();
        state_->repaired = 0;
    }
}
template <Sample T> std::size_t Declicker<T>::latencyFrames() const noexcept {
    return state_ ? state_->context : 0;
}
template <Sample T> std::uint64_t Declicker<T>::repairedSamples() const noexcept {
    return state_ ? state_->repaired : 0;
}
template class SpectralDenoiser<float>;
template class SpectralDenoiser<double>;
template class Dehum<float>;
template class Dehum<double>;
template class Declicker<float>;
template class Declicker<double>;
} // namespace cydsp
