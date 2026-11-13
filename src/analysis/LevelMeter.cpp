#include "cydsp/analysis/LevelMeter.h"
#include "detail/Analysis.h"
#include <algorithm>
#include <cstdint>
#include <vector>
namespace cydsp {
double ChannelLevels::peakDbfs() const noexcept {
    return detail::amplitudeDb(peak);
}
double ChannelLevels::rmsDbfs() const noexcept {
    return detail::amplitudeDb(rms);
}
double ChannelLevels::crestFactorDb() const noexcept {
    return rms > 0 ? 20 * std::log10(peak / rms) : 0;
}
template <Sample T> struct LevelMeter<T>::State {
    ProcessSpec spec;
    std::size_t window = 0, head = 0, filled = 0;
    std::uint64_t clock = 0;
    std::vector<double> history, held, peaks;
    std::vector<detail::CompensatedSum> sum, squares;
    std::vector<std::uint64_t> times;
    std::vector<std::size_t> read, count;
    detail::CompensatedSum cross;
};
template <Sample T> LevelMeter<T>::LevelMeter() = default;
template <Sample T> LevelMeter<T>::~LevelMeter() = default;
template <Sample T> LevelMeter<T>::LevelMeter(LevelMeter&&) noexcept = default;
template <Sample T> LevelMeter<T>& LevelMeter<T>::operator=(LevelMeter&&) noexcept = default;
template <Sample T>
Status LevelMeter<T>::prepare(const ProcessSpec& spec, std::size_t window) noexcept {
    if (!detail::validAnalysisSpec(spec) || !window || window > (1u << 20))
        return Status::invalid_spec;
    try {
        auto n = std::make_unique<State>();
        n->spec = spec;
        n->window = window;
        n->history.resize(window * spec.channels);
        n->peaks.resize(window * spec.channels);
        n->times.resize(window * spec.channels);
        n->sum.resize(spec.channels);
        n->squares.resize(spec.channels);
        n->held.resize(spec.channels);
        n->read.resize(spec.channels);
        n->count.resize(spec.channels);
        state_.swap(n);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status LevelMeter<T>::process(AudioBlock<const T> in) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    for (std::size_t i = 0; i < in.frames(); ++i) {
        if (in.channels() >= 2) {
            s.cross.add(double(in.channel(0)[i]) * double(in.channel(1)[i]));
            s.cross.add(-s.history[s.head] * s.history[s.window + s.head]);
        }
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            const auto base = ch * s.window, index = base + s.head;
            const double x = double(in.channel(ch)[i]), old = s.history[index];
            s.sum[ch].add(x);
            s.sum[ch].add(-old);
            s.squares[ch].add(x * x);
            s.squares[ch].add(-old * old);
            s.history[index] = x;
            const double magnitude = std::abs(x);
            s.held[ch] = std::max(s.held[ch], magnitude);
            auto& read = s.read[ch];
            auto& count = s.count[ch];
            while (count && s.clock - s.times[base + read] >= s.window) {
                read = (read + 1) % s.window;
                --count;
            }
            while (count && s.peaks[base + (read + count - 1) % s.window] <= magnitude)
                --count;
            const auto write = (read + count) % s.window;
            s.peaks[base + write] = magnitude;
            s.times[base + write] = s.clock;
            ++count;
        }
        s.head = (s.head + 1) % s.window;
        s.filled = std::min(s.filled + 1, s.window);
        ++s.clock;
    }
    return Status::ok;
}
template <Sample T> void LevelMeter<T>::reset() noexcept {
    if (!state_)
        return;
    auto& s = *state_;
    s.head = s.filled = 0;
    s.clock = 0;
    s.cross = {};
    for (auto* data : {&s.history, &s.held})
        std::fill(data->begin(), data->end(), 0.);
    std::fill(s.sum.begin(), s.sum.end(), detail::CompensatedSum{});
    std::fill(s.squares.begin(), s.squares.end(), detail::CompensatedSum{});
    std::fill(s.read.begin(), s.read.end(), 0);
    std::fill(s.count.begin(), s.count.end(), 0);
}
template <Sample T> ChannelLevels LevelMeter<T>::levels(std::size_t ch) const noexcept {
    if (!state_ || ch >= state_->spec.channels || !state_->filled)
        return {};
    const auto& s = *state_;
    return {s.peaks[ch * s.window + s.read[ch]],
            std::sqrt(std::max(0., s.squares[ch].value()) / double(s.filled)),
            s.sum[ch].value() / double(s.filled), s.held[ch]};
}
template <Sample T> double LevelMeter<T>::stereoCorrelation() const noexcept {
    if (!state_ || state_->spec.channels < 2)
        return 0;
    const auto& s = *state_;
    const double denominator = std::sqrt(std::max(0., s.squares[0].value())) *
                               std::sqrt(std::max(0., s.squares[1].value()));
    return denominator > 0 ? std::clamp(s.cross.value() / denominator, -1., 1.) : 0;
}
template <Sample T> std::size_t LevelMeter<T>::windowFrames() const noexcept {
    return state_ ? state_->window : 0;
}
template class LevelMeter<float>;
template class LevelMeter<double>;
} // namespace cydsp
