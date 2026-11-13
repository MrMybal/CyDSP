#include "cydsp/processors/BandGain.h"
#include "detail/Analysis.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>
namespace cydsp {
template <Sample T> struct BandGain<T>::State {
    ProcessSpec spec;
    double low = 0, high = 0;
    std::array<double, 3> gain{1, 1, 1}, target{1, 1, 1}, step{};
    std::size_t remaining = 0;
    std::vector<double> lows, highs;
};
template <Sample T> BandGain<T>::BandGain() = default;
template <Sample T> BandGain<T>::~BandGain() = default;
template <Sample T> BandGain<T>::BandGain(BandGain&&) noexcept = default;
template <Sample T> BandGain<T>& BandGain<T>::operator=(BandGain&&) noexcept = default;
template <Sample T>
Status BandGain<T>::prepare(const ProcessSpec& spec, double low, double high) noexcept {
    if (!detail::validAnalysisSpec(spec) || !isFinite(low) || !isFinite(high) || low <= 0 ||
        high <= low || high >= spec.sampleRate * .5)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->low = std::exp(-2 * std::numbers::pi * low / spec.sampleRate);
        s->high = std::exp(-2 * std::numbers::pi * high / spec.sampleRate);
        s->lows.resize(spec.channels);
        s->highs.resize(spec.channels);
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status BandGain<T>::setGains(const std::array<double, 3>& gains, std::size_t frames) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (frames > (1u << 30))
        return Status::invalid_argument;
    for (double gain : gains)
        if (!isFinite(gain) || std::abs(gain) > 64)
            return Status::invalid_argument;
    auto& s = *state_;
    s.target = gains;
    s.remaining = frames;
    for (std::size_t b = 0; b < 3; ++b) {
        if (frames)
            s.step[b] = (gains[b] - s.gain[b]) / double(frames);
        else
            s.gain[b] = gains[b];
    }
    return Status::ok;
}
template <Sample T>
Status BandGain<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, input, output);
    if (status != Status::ok)
        return status;
    status = detail::validateAnalysis(s.spec, input);
    if (status != Status::ok)
        return status;
    for (std::size_t i = 0; i < input.frames(); ++i) {
        if (s.remaining) {
            for (std::size_t b = 0; b < 3; ++b)
                s.gain[b] += s.step[b];
            if (!--s.remaining)
                s.gain = s.target;
        }
        for (std::size_t ch = 0; ch < input.channels(); ++ch) {
            const double x = double(input.channel(ch)[i]);
            s.lows[ch] = x + s.low * (s.lows[ch] - x);
            s.highs[ch] = x + s.high * (s.highs[ch] - x);
            output.channel(ch)[i] = T(s.gain[2] * x + (s.gain[1] - s.gain[2]) * s.highs[ch] +
                                      (s.gain[0] - s.gain[1]) * s.lows[ch]);
        }
    }
    return Status::ok;
}
template <Sample T> void BandGain<T>::reset() noexcept {
    if (state_) {
        auto& s = *state_;
        std::fill(s.lows.begin(), s.lows.end(), 0.);
        std::fill(s.highs.begin(), s.highs.end(), 0.);
        s.gain = s.target;
        s.remaining = 0;
    }
}
template class BandGain<float>;
template class BandGain<double>;
} // namespace cydsp
