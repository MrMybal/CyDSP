#include "cydsp/analysis/Diagnostics.h"
#include "cydsp/processors/RealFft.h"
#include "detail/Analysis.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>
namespace cydsp {
namespace {
bool responseFrequency(double fs, double f) noexcept {
    return isFinite(fs) && fs > 0 && isFinite(f) && f >= 0 && f <= fs * .5;
}
} // namespace
Status firResponse(std::span<const double> coefficients, double fs, double f,
                   FilterResponse& out) noexcept {
    if (coefficients.empty() || coefficients.size() > 65536 || !responseFrequency(fs, f))
        return Status::invalid_argument;
    std::complex<double> value{}, weighted{};
    const double omega = 2 * std::numbers::pi * f / fs;
    for (std::size_t k = 0; k < coefficients.size(); ++k) {
        if (!isFinite(coefficients[k]) || std::abs(coefficients[k]) > 1e100)
            return Status::invalid_argument;
        const auto v = coefficients[k] * std::polar(1., -omega * double(k));
        value += v;
        weighted += double(k) * v;
    }
    if (std::abs(value) < 1e-20)
        return Status::ambiguous;
    out = {value, std::arg(value), std::real(weighted / value)};
    return Status::ok;
}
Status iirResponse(std::span<const BiquadCoefficients> sections, double fs, double f,
                   FilterResponse& out) noexcept {
    if (sections.size() > 64 || !responseFrequency(fs, f))
        return Status::invalid_argument;
    const auto z = std::polar(1., -2 * std::numbers::pi * f / fs);
    std::complex<double> value{1, 0}, derivative{};
    for (const auto& c : sections) {
        if (!c.isStable())
            return Status::invalid_argument;
        const auto b = c.b0 + c.b1 * z + c.b2 * z * z, a = 1. + c.a1 * z + c.a2 * z * z;
        if (std::abs(b) < 1e-20 || std::abs(a) < 1e-20)
            return Status::ambiguous;
        value *= b / a;
        derivative += std::complex<double>(0, -1) *
                      ((c.b1 * z + 2 * c.b2 * z * z) / b - (c.a1 * z + 2 * c.a2 * z * z) / a);
    }
    if (!isFinite(value.real()) || !isFinite(value.imag()))
        return Status::invalid_argument;
    out = {value, std::arg(value), -derivative.imag()};
    return Status::ok;
}

template <Sample T> struct TransferAnalyzer<T>::State {
    ProcessSpec spec;
    std::size_t n = 0, hop = 0, averages = 0, head = 0, filled = 0, since = 0;
    std::uint64_t frames = 0;
    RealFft<double> fft;
    std::vector<double> x, y, time, window, xx, yy;
    std::vector<std::complex<double>> a, b, xy;
    std::vector<TransferBin> result;
    void analyze() noexcept {
        const auto bins = fft.bins();
        const double alpha = 1 / double(std::min<std::uint64_t>(frames + 1, averages));
        for (std::size_t ch = 0; ch < spec.channels; ++ch) {
            for (std::size_t i = 0; i < n; ++i)
                time[i] = x[ch * n + (head + i) % n] * window[i];
            (void)fft.forward(time, a);
            for (std::size_t i = 0; i < n; ++i)
                time[i] = y[ch * n + (head + i) % n] * window[i];
            (void)fft.forward(time, b);
            double previous = 0, unwrapped = 0;
            for (std::size_t k = 0; k < bins; ++k) {
                const auto index = ch * bins + k;
                xx[index] += (std::norm(a[k]) - xx[index]) * alpha;
                yy[index] += (std::norm(b[k]) - yy[index]) * alpha;
                xy[index] += (std::conj(a[k]) * b[k] - xy[index]) * alpha;
                auto& r = result[index];
                r.gain = xx[index] > 1e-30 ? xy[index] / xx[index] : std::complex<double>{};
                r.coherence =
                    xx[index] > 1e-30 && yy[index] > 1e-30
                        ? std::clamp(std::norm(xy[index]) / (xx[index] * yy[index]), 0., 1.)
                        : 0;
                const double arg = std::arg(r.gain);
                if (k == 0)
                    unwrapped = arg;
                else
                    unwrapped += std::remainder(arg - previous, 2 * std::numbers::pi);
                previous = arg;
                r.phaseRadians = unwrapped;
            }
            for (std::size_t k = 0; k < bins; ++k) {
                const auto first = k ? k - 1 : 0, last = std::min(k + 1, bins - 1);
                result[ch * bins + k].groupDelayFrames =
                    -(result[ch * bins + last].phaseRadians -
                      result[ch * bins + first].phaseRadians) *
                    double(n) / (2 * std::numbers::pi * double(last - first));
            }
        }
        ++frames;
    }
};
template <Sample T> TransferAnalyzer<T>::TransferAnalyzer() = default;
template <Sample T> TransferAnalyzer<T>::~TransferAnalyzer() = default;
template <Sample T> TransferAnalyzer<T>::TransferAnalyzer(TransferAnalyzer&&) noexcept = default;
template <Sample T>
TransferAnalyzer<T>& TransferAnalyzer<T>::operator=(TransferAnalyzer&&) noexcept = default;
template <Sample T>
Status TransferAnalyzer<T>::prepare(const ProcessSpec& spec, std::size_t n, std::size_t hop,
                                    std::size_t averages, CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || spec.channels > 16 || !isPowerOfTwo(n) || n < 64 ||
        n > 65536 || !hop || hop > n || !averages || averages > 65536)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->n = n;
        s->hop = hop;
        s->averages = averages;
        auto status = s->fft.prepare(n, backend);
        if (status != Status::ok)
            return status;
        s->x.resize(n * spec.channels);
        s->y.resize(n * spec.channels);
        s->time.resize(n);
        s->window.resize(n);
        const auto bins = s->fft.bins();
        s->a.resize(bins);
        s->b.resize(bins);
        s->xx.resize(bins * spec.channels);
        s->yy.resize(bins * spec.channels);
        s->xy.resize(bins * spec.channels);
        s->result.resize(bins * spec.channels);
        for (std::size_t i = 0; i < n; ++i)
            s->window[i] = .5 - .5 * std::cos(2 * std::numbers::pi * double(i) / double(n));
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status TransferAnalyzer<T>::process(AudioBlock<const T> reference,
                                    AudioBlock<const T> measured) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = detail::validateAnalysis(s.spec, reference);
    if (status != Status::ok)
        return status;
    status = detail::validateAnalysis(s.spec, measured);
    if (status != Status::ok)
        return status;
    if (reference.frames() != measured.frames())
        return Status::invalid_argument;
    for (std::size_t i = 0; i < reference.frames(); ++i) {
        for (std::size_t ch = 0; ch < s.spec.channels; ++ch) {
            s.x[ch * s.n + s.head] = reference.channel(ch)[i];
            s.y[ch * s.n + s.head] = measured.channel(ch)[i];
        }
        s.head = (s.head + 1) % s.n;
        s.filled = std::min(s.n, s.filled + 1);
        if (++s.since >= s.hop && s.filled == s.n) {
            s.since = 0;
            s.analyze();
        }
    }
    return Status::ok;
}
template <Sample T>
TransferBin TransferAnalyzer<T>::bin(std::size_t ch, std::size_t k) const noexcept {
    return state_ && ch < state_->spec.channels && k < state_->fft.bins()
               ? state_->result[ch * state_->fft.bins() + k]
               : TransferBin{};
}
template <Sample T> std::size_t TransferAnalyzer<T>::bins() const noexcept {
    return state_ ? state_->fft.bins() : 0;
}
template <Sample T> std::uint64_t TransferAnalyzer<T>::analyzedFrames() const noexcept {
    return state_ ? state_->frames : 0;
}
template <Sample T> void TransferAnalyzer<T>::reset() noexcept {
    if (state_) {
        auto& s = *state_;
        for (auto* v : {&s.x, &s.y, &s.xx, &s.yy})
            std::fill(v->begin(), v->end(), 0);
        std::fill(s.xy.begin(), s.xy.end(), std::complex<double>{});
        std::fill(s.result.begin(), s.result.end(), TransferBin{});
        s.head = s.filled = s.since = 0;
        s.frames = 0;
    }
}

template <Sample T>
Status measureDistortion(std::span<const T> samples, double fs, double f, DistortionReport& out,
                         unsigned harmonics) noexcept {
    if (!responseFrequency(fs, f) || f <= 0 || !harmonics || harmonics > 20 ||
        samples.size() < 128 || samples.size() > (1u << 24))
        return Status::invalid_argument;
    const auto count = std::min(harmonics, unsigned(std::floor(fs * .499 / f)));
    if (!count)
        return Status::invalid_argument;
    const std::size_t n = 1 + 2 * count;
    std::array<double, 41 * 42> system{};
    std::array<double, 41> basis{}, solution{};
    for (std::size_t i = 0; i < samples.size(); ++i) {
        const double x = samples[i];
        if (!isFinite(x) || std::abs(x) > 1e100)
            return Status::invalid_argument;
        basis[0] = 1;
        for (unsigned h = 1; h <= count; ++h) {
            const double phase = 2 * std::numbers::pi * f * double(h) * double(i) / fs;
            basis[2 * h - 1] = std::cos(phase);
            basis[2 * h] = std::sin(phase);
        }
        for (std::size_t r = 0; r < n; ++r) {
            for (std::size_t c = 0; c < n; ++c)
                system[r * (n + 1) + c] += basis[r] * basis[c];
            system[r * (n + 1) + n] += basis[r] * x;
        }
    }
    for (std::size_t p = 0; p < n; ++p) {
        std::size_t pivot = p;
        for (std::size_t r = p + 1; r < n; ++r)
            if (std::abs(system[r * (n + 1) + p]) > std::abs(system[pivot * (n + 1) + p]))
                pivot = r;
        if (std::abs(system[pivot * (n + 1) + p]) < 1e-10)
            return Status::ambiguous;
        for (std::size_t c = 0; c <= n; ++c)
            std::swap(system[p * (n + 1) + c], system[pivot * (n + 1) + c]);
        const double d = system[p * (n + 1) + p];
        for (std::size_t c = p; c <= n; ++c)
            system[p * (n + 1) + c] /= d;
        for (std::size_t r = 0; r < n; ++r)
            if (r != p) {
                const double v = system[r * (n + 1) + p];
                for (std::size_t c = p; c <= n; ++c)
                    system[r * (n + 1) + c] -= v * system[p * (n + 1) + c];
            }
    }
    for (std::size_t r = 0; r < n; ++r)
        solution[r] = system[r * (n + 1) + n];
    DistortionReport result;
    result.dc = solution[0];
    result.measuredHarmonics = count;
    result.fundamentalRms = std::hypot(solution[1], solution[2]) / std::sqrt(2.);
    double harmonicPower = 0;
    for (unsigned h = 2; h <= count; ++h)
        harmonicPower +=
            (solution[2 * h - 1] * solution[2 * h - 1] + solution[2 * h] * solution[2 * h]) * .5;
    result.harmonicsRms = std::sqrt(harmonicPower);
    double residual = 0;
    for (std::size_t i = 0; i < samples.size(); ++i) {
        double fitted = solution[0];
        for (unsigned h = 1; h <= count; ++h) {
            const double phase = 2 * std::numbers::pi * f * double(h) * double(i) / fs;
            fitted += solution[2 * h - 1] * std::cos(phase) + solution[2 * h] * std::sin(phase);
        }
        const double e = double(samples[i]) - fitted;
        residual += e * e;
    }
    result.noiseRms = std::sqrt(residual / double(samples.size()));
    if (result.fundamentalRms < 1e-20)
        return Status::ambiguous;
    result.thd = result.harmonicsRms / result.fundamentalRms;
    result.thdPlusNoise = std::hypot(result.harmonicsRms, result.noiseRms) / result.fundamentalRms;
    out = result;
    return Status::ok;
}

template <Sample T> Status MeasurementGenerator<T>::prepare(const ProcessSpec& spec) noexcept {
    if (!detail::validAnalysisSpec(spec))
        return Status::invalid_spec;
    spec_ = spec;
    p_ = {};
    p_.endHz = std::min(20000., spec.sampleRate * .45);
    prepared_ = true;
    reset();
    return Status::ok;
}
template <Sample T>
Status MeasurementGenerator<T>::setParameters(const MeasurementSignalParameters& p) noexcept {
    if (!prepared_)
        return Status::not_prepared;
    if (unsigned(p.kind) > unsigned(MeasurementSignal::logSweep) || !isFinite(p.startHz) ||
        p.startHz <= 0 || p.startHz >= spec_.sampleRate * .5 || !isFinite(p.endHz) ||
        p.endHz <= 0 || p.endHz >= spec_.sampleRate * .5 || !isFinite(p.durationSeconds) ||
        p.durationSeconds <= 0 || p.durationSeconds > 3600 || !isFinite(p.amplitude) ||
        std::abs(p.amplitude) > 16 || !isFinite(p.fadeSeconds) || p.fadeSeconds < 0 ||
        p.fadeSeconds > p.durationSeconds * .5)
        return Status::invalid_argument;
    p_ = p;
    reset();
    return Status::ok;
}
template <Sample T> Status MeasurementGenerator<T>::process(AudioBlock<T> out) noexcept {
    if (!prepared_)
        return Status::not_prepared;
    if (!out.isValid() || out.channels() != spec_.channels)
        return Status::channel_mismatch;
    if (out.frames() > spec_.maxBlockFrames)
        return Status::block_too_large;
    const double total = p_.durationSeconds * spec_.sampleRate;
    for (std::size_t i = 0; i < out.frames(); ++i) {
        double value = 0;
        if (double(position_) < total) {
            const double t = double(position_) / spec_.sampleRate;
            switch (p_.kind) {
            case MeasurementSignal::impulse:
                value = position_ == 0 ? 1. : 0.;
                break;
            case MeasurementSignal::whiteNoise:
                random_ ^= random_ >> 12;
                random_ ^= random_ << 25;
                random_ ^= random_ >> 27;
                value = double((random_ * 2685821657736338717ULL) >> 11) * 0x1.0p-52 - 1;
                break;
            default: {
                double phase = 2 * std::numbers::pi * p_.startHz * t;
                if (p_.kind == MeasurementSignal::linearSweep)
                    phase +=
                        std::numbers::pi * (p_.endHz - p_.startHz) * t * t / p_.durationSeconds;
                else if (p_.kind == MeasurementSignal::logSweep) {
                    const double k = std::log(p_.endHz / p_.startHz) / p_.durationSeconds;
                    if (std::abs(k) > 1e-12)
                        phase = 2 * std::numbers::pi * p_.startHz * std::expm1(k * t) / k;
                }
                value = std::sin(phase);
                break;
            }
            }
            if (p_.fadeSeconds > 0 && p_.kind != MeasurementSignal::impulse) {
                const double fade =
                    std::clamp(std::min(t, p_.durationSeconds - t) / p_.fadeSeconds, 0., 1.);
                value *= .5 - .5 * std::cos(std::numbers::pi * fade);
            }
            ++position_;
        }
        for (std::size_t ch = 0; ch < out.channels(); ++ch)
            out.channel(ch)[i] = T(value * p_.amplitude);
    }
    return Status::ok;
}
template <Sample T> void MeasurementGenerator<T>::reset(std::uint64_t seed) noexcept {
    position_ = 0;
    random_ = seed ? seed : 1;
}
template <Sample T>
Status inverseLogSweep(std::span<const T> sweep, double start, double end,
                       std::span<T> inverse) noexcept {
    if (sweep.empty() || sweep.size() > (1u << 24) || inverse.size() != sweep.size() ||
        !isFinite(start) || start <= 0 || !isFinite(end) || end <= 0)
        return Status::invalid_argument;
    for (T x : sweep)
        if (!isFinite(x) || std::abs(double(x)) > 1e100)
            return Status::invalid_argument;
    try {
        std::vector<T> result(sweep.size());
        const double k = std::log(end / start);
        double peak = 0;
        for (std::size_t i = 0; i < sweep.size(); ++i) {
            const double weight = std::exp(-k * double(i) / double(sweep.size()));
            const double x = sweep[sweep.size() - 1 - i];
            result[i] = T(x * weight);
            peak += x * double(result[i]);
        }
        if (!isFinite(peak) || peak < 1e-30)
            return Status::ambiguous;
        for (auto& x : result)
            x = T(double(x) / peak);
        std::copy(result.begin(), result.end(), inverse.begin());
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status deconvolveSweep(std::span<const T> recording, std::span<const T> excitation,
                       std::span<T> impulse, double regularization, CpuBackend backend) noexcept {
    if (recording.empty() || excitation.empty() || recording.size() > (1u << 22) ||
        excitation.size() > (1u << 22) || impulse.empty() || impulse.size() > recording.size() ||
        !isFinite(regularization) || regularization < -240 || regularization > 0)
        return Status::invalid_argument;
    for (auto data : {recording, excitation})
        for (T x : data)
            if (!isFinite(x) || std::abs(double(x)) > 1e100)
                return Status::invalid_argument;
    try {
        std::size_t n = 2;
        while (n < recording.size() + excitation.size() - 1)
            n *= 2;
        RealFft<double> fft;
        auto status = fft.prepare(n, backend);
        if (status != Status::ok)
            return status;
        std::vector<double> time(n);
        std::vector<std::complex<double>> x(fft.bins()), y(fft.bins());
        std::copy(excitation.begin(), excitation.end(), time.begin());
        (void)fft.forward(time, x);
        std::fill(time.begin(), time.end(), 0);
        std::copy(recording.begin(), recording.end(), time.begin());
        (void)fft.forward(time, y);
        double maximum = 0;
        for (auto v : x)
            maximum = std::max(maximum, std::norm(v));
        if (maximum < 1e-30)
            return Status::ambiguous;
        const double floor = maximum * std::pow(10., regularization / 10);
        for (std::size_t k = 0; k < x.size(); ++k)
            y[k] = y[k] * std::conj(x[k]) / (std::norm(x[k]) + floor);
        (void)fft.inverse(y, time);
        for (std::size_t i = 0; i < impulse.size(); ++i)
            impulse[i] = T(time[i]);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template class TransferAnalyzer<float>;
template class TransferAnalyzer<double>;
template class MeasurementGenerator<float>;
template class MeasurementGenerator<double>;
template Status measureDistortion<float>(std::span<const float>, double, double, DistortionReport&,
                                         unsigned) noexcept;
template Status measureDistortion<double>(std::span<const double>, double, double,
                                          DistortionReport&, unsigned) noexcept;
template Status inverseLogSweep<float>(std::span<const float>, double, double,
                                       std::span<float>) noexcept;
template Status inverseLogSweep<double>(std::span<const double>, double, double,
                                        std::span<double>) noexcept;
template Status deconvolveSweep<float>(std::span<const float>, std::span<const float>,
                                       std::span<float>, double, CpuBackend) noexcept;
template Status deconvolveSweep<double>(std::span<const double>, std::span<const double>,
                                        std::span<double>, double, CpuBackend) noexcept;
} // namespace cydsp
