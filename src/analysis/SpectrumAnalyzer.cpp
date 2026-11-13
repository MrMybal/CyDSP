#include "cydsp/analysis/SpectrumAnalyzer.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/math/VectorOps.h"
#include "cydsp/processors/RealFft.h"
#include "detail/Alias.h"
#include "detail/Analysis.h"
#include "detail/VectorTable.h"
#include <algorithm>
#include <numeric>
#include <vector>
namespace cydsp {
template <Sample T> struct SpectrumAnalyzer<T>::State {
    ProcessSpec spec;
    SpectrumConfig config;
    RealFft<double> fft;
    VectorOps ops;
    AudioBuffer<double> history;
    std::vector<double> window, scratch, magnitude, power, previous;
    std::vector<std::complex<double>> spectrum;
    std::vector<SpectralFeatures> features;
    std::size_t head = 0, filled = 0, since = 0;
    std::uint64_t frames = 0;
    double windowSum = 0, windowSquares = 0;
    void analyze() noexcept {
        const auto n = config.fftFrames, bins = fft.bins();
        for (std::size_t ch = 0; ch < spec.channels; ++ch) {
            const auto source = history.block().channel(ch);
            (void)ops.copy(source.subspan(head), std::span<double>(scratch).first(n - head));
            if (head)
                (void)ops.copy(source.first(head), std::span<double>(scratch).subspan(n - head));
            const double energy =
                detail::VectorAccess::kernels<double>(ops).dot(scratch.data(), scratch.data(), n);
            (void)ops.multiply(scratch, window, scratch);
            auto complex = std::span<std::complex<double>>(spectrum).subspan(ch * bins, bins);
            (void)fft.forward(scratch, complex);
            auto magnitudes = std::span<double>(magnitude).subspan(ch * bins, bins);
            auto powers = std::span<double>(power).subspan(ch * bins, bins);
            double sum = 0, magnitudeSum = 0, weighted = 0, maxPower = 0, logSum = 0, noDcPower = 0;
            std::size_t dominant = 0;
            for (std::size_t k = 0; k < bins; ++k) {
                const auto norm = std::norm(complex[k]);
                const bool edge = k == 0 || k == bins - 1;
                magnitudes[k] = std::sqrt(norm) * (edge ? 1 : 2) / windowSum;
                powers[k] = norm * (edge ? 1 : 2) / (spec.sampleRate * windowSquares);
                sum += powers[k];
                magnitudeSum += magnitudes[k];
                weighted += powers[k] * double(k) * spec.sampleRate / double(n);
                if (powers[k] > maxPower) {
                    maxPower = powers[k];
                    dominant = k;
                }
                if (k) {
                    noDcPower += powers[k];
                    logSum += std::log(std::max(1e-300, powers[k]));
                }
            }
            SpectralFeatures f;
            f.rms = std::sqrt(energy / double(n));
            if (sum > 0) {
                f.centroidHz = weighted / sum;
                f.dominantHz = double(dominant) * spec.sampleRate / double(n);
                double cumulative = 0;
                for (std::size_t k = 0; k < bins; ++k) {
                    cumulative += powers[k];
                    if (cumulative >= sum * .85) {
                        f.rolloffHz = double(k) * spec.sampleRate / double(n);
                        break;
                    }
                }
                if (noDcPower > 0)
                    f.flatness = std::clamp(std::exp(logSum / double(bins - 1)) /
                                                (noDcPower / double(bins - 1)),
                                            0., 1.);
            }
            for (std::size_t k = 0; k < bins; ++k) {
                const double value = magnitudeSum > 0 ? magnitudes[k] / magnitudeSum : 0;
                if (frames) {
                    const double delta = std::max(0., value - previous[ch * bins + k]);
                    f.positiveFlux += delta * delta;
                }
                previous[ch * bins + k] = value;
            }
            f.positiveFlux = std::sqrt(f.positiveFlux);
            features[ch] = f;
        }
        ++frames;
        since = 0;
    }
};
template <Sample T> SpectrumAnalyzer<T>::SpectrumAnalyzer() = default;
template <Sample T> SpectrumAnalyzer<T>::~SpectrumAnalyzer() = default;
template <Sample T> SpectrumAnalyzer<T>::SpectrumAnalyzer(SpectrumAnalyzer&&) noexcept = default;
template <Sample T>
SpectrumAnalyzer<T>& SpectrumAnalyzer<T>::operator=(SpectrumAnalyzer&&) noexcept = default;
template <Sample T>
Status SpectrumAnalyzer<T>::prepare(const ProcessSpec& spec, const SpectrumConfig& config,
                                    CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || !isPowerOfTwo(config.fftFrames) ||
        config.fftFrames < 16 || config.fftFrames > 65536 || !config.hopFrames ||
        config.hopFrames > config.fftFrames ||
        unsigned(config.window) > unsigned(WindowType::kaiser))
        return Status::invalid_spec;
    try {
        auto n = std::make_unique<State>();
        n->spec = spec;
        n->config = config;
        auto status = VectorOps::create(backend, n->ops);
        if (status != Status::ok)
            return status;
        status = n->fft.prepare(config.fftFrames, backend);
        if (status != Status::ok)
            return status;
        status = n->history.resize(spec.channels, config.fftFrames);
        if (status != Status::ok)
            return status;
        n->window.resize(config.fftFrames);
        status = makeWindow(config.window, std::span<double>(n->window), WindowSymmetry::periodic);
        if (status != Status::ok)
            return status;
        n->windowSum = std::accumulate(n->window.begin(), n->window.end(), 0.);
        n->windowSquares =
            std::inner_product(n->window.begin(), n->window.end(), n->window.begin(), 0.);
        if (n->windowSum <= 0 || n->windowSquares <= 0)
            return Status::invalid_argument;
        n->scratch.resize(config.fftFrames);
        const auto total = spec.channels * n->fft.bins();
        n->spectrum.resize(total);
        n->magnitude.resize(total);
        n->power.resize(total);
        n->previous.resize(total);
        n->features.resize(spec.channels);
        state_.swap(n);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status SpectrumAnalyzer<T>::process(AudioBlock<const T> in) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    for (std::size_t i = 0; i < in.frames(); ++i) {
        for (std::size_t ch = 0; ch < in.channels(); ++ch)
            s.history.block().channel(ch)[s.head] = double(in.channel(ch)[i]);
        s.head = (s.head + 1) % s.config.fftFrames;
        s.filled = std::min(s.filled + 1, s.config.fftFrames);
        ++s.since;
        if (s.filled == s.config.fftFrames && (!s.frames || s.since >= s.config.hopFrames))
            s.analyze();
    }
    return Status::ok;
}
template <Sample T> void SpectrumAnalyzer<T>::reset() noexcept {
    if (!state_)
        return;
    auto& s = *state_;
    s.head = s.filled = s.since = 0;
    s.frames = 0;
    for (std::size_t ch = 0; ch < s.spec.channels; ++ch)
        std::fill(s.history.block().channel(ch).begin(), s.history.block().channel(ch).end(), 0.);
    for (auto* values : {&s.magnitude, &s.power, &s.previous})
        std::fill(values->begin(), values->end(), 0.);
    std::fill(s.spectrum.begin(), s.spectrum.end(), std::complex<double>{});
    std::fill(s.features.begin(), s.features.end(), SpectralFeatures{});
}
template <Sample T> bool SpectrumAnalyzer<T>::hasFrame() const noexcept {
    return state_ && state_->frames > 0;
}
template <Sample T> std::uint64_t SpectrumAnalyzer<T>::frameCount() const noexcept {
    return state_ ? state_->frames : 0;
}
template <Sample T> std::size_t SpectrumAnalyzer<T>::bins() const noexcept {
    return state_ ? state_->fft.bins() : 0;
}
template <Sample T> double SpectrumAnalyzer<T>::binFrequency(std::size_t k) const noexcept {
    return state_ && k < bins()
               ? double(k) * state_->spec.sampleRate / double(state_->config.fftFrames)
               : 0;
}
template <Sample T>
std::span<const double> SpectrumAnalyzer<T>::magnitudes(std::size_t ch) const noexcept {
    return state_ && ch < state_->spec.channels
               ? std::span<const double>(state_->magnitude).subspan(ch * bins(), bins())
               : std::span<const double>{};
}
template <Sample T>
std::span<const double> SpectrumAnalyzer<T>::powerDensity(std::size_t ch) const noexcept {
    return state_ && ch < state_->spec.channels
               ? std::span<const double>(state_->power).subspan(ch * bins(), bins())
               : std::span<const double>{};
}
template <Sample T>
std::span<const std::complex<double>>
SpectrumAnalyzer<T>::complexSpectrum(std::size_t ch) const noexcept {
    return state_ && ch < state_->spec.channels
               ? std::span<const std::complex<double>>(state_->spectrum)
                     .subspan(ch * bins(), bins())
               : std::span<const std::complex<double>>{};
}
template <Sample T> SpectralFeatures SpectrumAnalyzer<T>::features(std::size_t ch) const noexcept {
    return state_ && ch < state_->features.size() ? state_->features[ch] : SpectralFeatures{};
}
template <Sample T> CpuBackend SpectrumAnalyzer<T>::backend() const noexcept {
    return state_ ? state_->ops.backend() : CpuBackend::scalar;
}
struct SpectralBands::State {
    VectorOps ops;
    std::size_t bins = 0;
    std::vector<double> centers, weights;
    std::vector<std::size_t> first, offset, length;
};
SpectralBands::SpectralBands() = default;
SpectralBands::~SpectralBands() = default;
SpectralBands::SpectralBands(SpectralBands&&) noexcept = default;
SpectralBands& SpectralBands::operator=(SpectralBands&&) noexcept = default;
Status SpectralBands::prepare(double rate, std::size_t frames, const SpectralBandConfig& config,
                              CpuBackend backend) noexcept {
    const double maximum = config.maximumHz == 0 ? rate * .5 : config.maximumHz;
    if (!isFinite(rate) || rate < 8000 || rate > 768000 || !isPowerOfTwo(frames) || frames < 16 ||
        frames > 65536 || !config.bands || config.bands > 512 || !isFinite(config.minimumHz) ||
        config.minimumHz < 0 || !isFinite(maximum) || maximum <= config.minimumHz ||
        maximum > rate * .5 || unsigned(config.scale) > unsigned(SpectralBandScale::logarithmic) ||
        (config.scale == SpectralBandScale::logarithmic && config.minimumHz <= 0))
        return Status::invalid_argument;
    try {
        auto n = std::make_unique<State>();
        n->bins = frames / 2 + 1;
        auto status = VectorOps::create(backend, n->ops);
        if (status != Status::ok)
            return status;
        auto map = [&](double f) {
            return config.scale == SpectralBandScale::mel ? 2595 * std::log10(1 + f / 700)
                                                          : std::log(f);
        };
        auto inverse = [&](double x) {
            return config.scale == SpectralBandScale::mel ? 700 * (std::pow(10., x / 2595) - 1)
                                                          : std::exp(x);
        };
        std::vector<double> points(config.bands + 2);
        const auto first = map(config.minimumHz), last = map(maximum);
        for (std::size_t i = 0; i < points.size(); ++i)
            points[i] = inverse(first + (last - first) * double(i) / double(points.size() - 1));
        const double spacing = rate / double(frames);
        for (std::size_t band = 0; band < config.bands; ++band) {
            const double left = points[band], center = points[band + 1], right = points[band + 2];
            if (!isFinite(left) || !isFinite(center) || !isFinite(right) || left >= center ||
                center >= right)
                return Status::invalid_argument;
            const auto begin =
                std::min(std::size_t(std::max(0., std::floor(left / spacing))), n->bins - 1);
            const auto end = std::min(std::size_t(std::ceil(right / spacing)) + 1, n->bins);
            const auto offset = n->weights.size();
            double sum = 0;
            for (std::size_t k = begin; k < end; ++k) {
                const double f = double(k) * spacing;
                const double weight = std::max(
                    0., std::min((f - left) / (center - left), (right - f) / (right - center)));
                n->weights.push_back(weight);
                sum += weight;
            }
            if (sum <= 0)
                return Status::invalid_argument; // Increase FFT resolution or reduce band count.
            for (std::size_t i = offset; i < n->weights.size(); ++i)
                n->weights[i] /= sum;
            n->centers.push_back(center);
            n->first.push_back(begin);
            n->offset.push_back(offset);
            n->length.push_back(end - begin);
        }
        state_.swap(n);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status SpectralBands::process(std::span<const double> in, std::span<double> out) const noexcept {
    if (!state_)
        return Status::not_prepared;
    const auto& s = *state_;
    if (in.size() != s.bins || out.size() != s.centers.size())
        return Status::invalid_argument;
    if (detail::overlaps(in.data(), in.size_bytes(), out.data(), out.size_bytes()))
        return Status::partial_overlap;
    for (double value : in)
        if (!isFinite(value) || value < 0 || value > 1e200)
            return Status::invalid_argument;
    const auto& kernels = detail::VectorAccess::kernels<double>(s.ops);
    for (std::size_t band = 0; band < out.size(); ++band)
        out[band] = kernels.dot(in.data() + s.first[band], s.weights.data() + s.offset[band],
                                s.length[band]);
    return Status::ok;
}
std::span<const double> SpectralBands::centerFrequencies() const noexcept {
    return state_ ? std::span<const double>(state_->centers) : std::span<const double>{};
}
CpuBackend SpectralBands::backend() const noexcept {
    return state_ ? state_->ops.backend() : CpuBackend::scalar;
}
template class SpectrumAnalyzer<float>;
template class SpectrumAnalyzer<double>;
} // namespace cydsp
