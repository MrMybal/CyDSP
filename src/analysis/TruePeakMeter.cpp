#include "cydsp/analysis/TruePeakMeter.h"
#include "cydsp/math/VectorOps.h"
#include "detail/Analysis.h"
#include "detail/VectorTable.h"
#include <algorithm>
#include <numbers>
#include <vector>
namespace cydsp {
template <Sample T> struct TruePeakMeter<T>::State {
    ProcessSpec spec;
    unsigned factor = 4;
    std::size_t taps = 64, head = 0;
    VectorOps ops;
    std::vector<double> coefficients, history, sample, peak;
    void push(std::size_t ch, double value) noexcept {
        auto* data = history.data() + ch * taps * 2;
        data[head] = data[head + taps] = value;
        const auto* first = data + (head + 1) % taps;
        const auto& kernels = detail::VectorAccess::kernels<double>(ops);
        // Chronological history, reconstruction centered taps/2 input frames ago.
        peak[ch] = std::max(peak[ch], std::abs(first[taps - 1 - taps / 2]));
        for (unsigned phase = 1; phase < factor; ++phase)
            peak[ch] = std::max(
                peak[ch],
                std::abs(kernels.dot(first, coefficients.data() + (phase - 1) * taps, taps)));
    }
};
template <Sample T> TruePeakMeter<T>::TruePeakMeter() = default;
template <Sample T> TruePeakMeter<T>::~TruePeakMeter() = default;
template <Sample T> TruePeakMeter<T>::TruePeakMeter(TruePeakMeter&&) noexcept = default;
template <Sample T>
TruePeakMeter<T>& TruePeakMeter<T>::operator=(TruePeakMeter&&) noexcept = default;
template <Sample T>
Status TruePeakMeter<T>::prepare(const ProcessSpec& spec, unsigned factor, std::size_t taps,
                                 CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || (factor != 4 && factor != 8) || taps < 16 ||
        taps > 128 || !isPowerOfTwo(taps))
        return Status::invalid_spec;
    try {
        auto n = std::make_unique<State>();
        n->spec = spec;
        n->factor = factor;
        n->taps = taps;
        auto status = VectorOps::create(backend, n->ops);
        if (status != Status::ok)
            return status;
        n->history.resize(spec.channels * taps * 2);
        n->sample.resize(spec.channels);
        n->peak.resize(spec.channels);
        n->coefficients.resize((factor - 1) * taps);
        for (unsigned phase = 1; phase < factor; ++phase) {
            double sum = 0;
            for (std::size_t i = 0; i < taps; ++i) {
                // i is chronological: k=taps-1-i is the age of this sample.
                const double distance =
                    double(taps - 1 - i) - double(taps / 2) + double(phase) / factor;
                const double x = distance / (double(taps) / 2);
                const double window = std::abs(x) < 1 ? .42 + .5 * std::cos(std::numbers::pi * x) +
                                                            .08 * std::cos(2 * std::numbers::pi * x)
                                                      : 0;
                const double sinc =
                    std::abs(distance) < 1e-15
                        ? 1
                        : std::sin(std::numbers::pi * distance) / (std::numbers::pi * distance);
                const auto coefficient = sinc * window;
                n->coefficients[(phase - 1) * taps + i] = coefficient;
                sum += coefficient;
            }
            for (std::size_t i = 0; i < taps; ++i)
                n->coefficients[(phase - 1) * taps + i] /= sum;
        }
        state_.swap(n);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status TruePeakMeter<T>::process(AudioBlock<const T> in) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    for (std::size_t i = 0; i < in.frames(); ++i) {
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            const double x = double(in.channel(ch)[i]);
            s.sample[ch] = std::max(s.sample[ch], std::abs(x));
            s.peak[ch] = std::max(s.peak[ch], std::abs(x));
            s.push(ch, x);
        }
        s.head = (s.head + 1) % s.taps;
    }
    return Status::ok;
}
template <Sample T> void TruePeakMeter<T>::flush() noexcept {
    if (!state_)
        return;
    auto& s = *state_;
    for (std::size_t i = 0; i < s.taps; ++i) {
        for (std::size_t ch = 0; ch < s.spec.channels; ++ch)
            s.push(ch, 0);
        s.head = (s.head + 1) % s.taps;
    }
}
template <Sample T> void TruePeakMeter<T>::reset() noexcept {
    if (!state_)
        return;
    auto& s = *state_;
    s.head = 0;
    for (auto* data : {&s.history, &s.sample, &s.peak})
        std::fill(data->begin(), data->end(), 0.);
}
template <Sample T> double TruePeakMeter<T>::samplePeak(std::size_t ch) const noexcept {
    return state_ && ch < state_->spec.channels ? state_->sample[ch] : 0;
}
template <Sample T> double TruePeakMeter<T>::truePeak(std::size_t ch) const noexcept {
    return state_ && ch < state_->spec.channels ? state_->peak[ch] : 0;
}
template <Sample T> double TruePeakMeter<T>::truePeakDbtp(std::size_t ch) const noexcept {
    return detail::amplitudeDb(truePeak(ch));
}
template <Sample T> std::size_t TruePeakMeter<T>::latencyFrames() const noexcept {
    return state_ ? state_->taps / 2 : 0;
}
template <Sample T> CpuBackend TruePeakMeter<T>::backend() const noexcept {
    return state_ ? state_->ops.backend() : CpuBackend::scalar;
}
template class TruePeakMeter<float>;
template class TruePeakMeter<double>;
} // namespace cydsp
