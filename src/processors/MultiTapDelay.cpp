#include "cydsp/processors/MultiTapDelay.h"
#include "cydsp/math/VectorOps.h"
#include "detail/Analysis.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
namespace cydsp {
template <Sample T> struct MultiTapDelay<T>::State {
    ProcessSpec spec;
    std::size_t outputs = 0, maximum = 0, total = 0, remaining = 0, oldCount = 0, count = 0;
    CircularHistory<T> history;
    VectorOps ops;
    std::vector<DelayTap> from, to;
};
template <Sample T> MultiTapDelay<T>::MultiTapDelay() = default;
template <Sample T> MultiTapDelay<T>::~MultiTapDelay() = default;
template <Sample T> MultiTapDelay<T>::MultiTapDelay(MultiTapDelay&&) noexcept = default;
template <Sample T>
MultiTapDelay<T>& MultiTapDelay<T>::operator=(MultiTapDelay&&) noexcept = default;
template <Sample T>
Status MultiTapDelay<T>::prepare(const ProcessSpec& spec, std::size_t outputs, std::size_t maximum,
                                 std::size_t taps, CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || !outputs || outputs > 64 || maximum > (1u << 24) - 4 ||
        !taps || taps > 4096)
        return Status::invalid_spec;
    try {
        auto n = std::make_unique<State>();
        n->spec = spec;
        n->outputs = outputs;
        n->maximum = maximum;
        n->from.resize(taps);
        n->to.resize(taps);
        auto s = VectorOps::create(backend, n->ops);
        if (s != Status::ok)
            return s;
        s = n->history.prepare(spec.channels, maximum + 4);
        if (s != Status::ok)
            return s;
        state_.swap(n);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status MultiTapDelay<T>::setTaps(std::span<const DelayTap> taps, std::size_t transition) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (taps.size() > s.to.size() || transition > (1u << 30))
        return Status::invalid_argument;
    for (const auto& t : taps)
        if (t.inputChannel >= s.spec.channels || t.outputChannel >= s.outputs ||
            !isFinite(t.delayFrames) || t.delayFrames < 0 || t.delayFrames > double(s.maximum) ||
            !isFinite(t.gain) || std::abs(t.gain) > 64 ||
            unsigned(t.interpolation) > unsigned(HistoryInterpolation::catmullRom))
            return Status::invalid_argument;
    if (s.remaining)
        return Status::transition_in_progress;
    std::copy_n(s.to.begin(), s.count, s.from.begin());
    s.oldCount = s.count;
    std::copy(taps.begin(), taps.end(), s.to.begin());
    s.count = taps.size();
    s.total = s.remaining = transition;
    return Status::ok;
}
template <Sample T>
Status MultiTapDelay<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    if (!out.isValid() || out.frames() != in.frames())
        return Status::invalid_argument;
    if (out.channels() != s.outputs)
        return Status::channel_mismatch;
    status = validateDisjointBlocks(in, out);
    if (status != Status::ok)
        return status;
    for (std::size_t ch = 0; ch < out.channels(); ++ch)
        s.ops.fill(out.channel(ch), T{});
    std::array<T, 64> frame{};
    for (std::size_t i = 0; i < in.frames(); ++i) {
        for (std::size_t ch = 0; ch < in.channels(); ++ch)
            frame[ch] = in.channel(ch)[i];
        (void)s.history.pushFrame(std::span<const T>(frame).first(in.channels()));
        const double mix = s.remaining ? double(s.total - s.remaining + 1) / double(s.total) : 1;
        if (s.remaining)
            for (std::size_t j = 0; j < s.oldCount; ++j) {
                const auto& t = s.from[j];
                out.channel(t.outputChannel)[i] +=
                    T(double(s.history.read(t.inputChannel, t.delayFrames, t.interpolation)) *
                      t.gain * (1 - mix));
            }
        for (std::size_t j = 0; j < s.count; ++j) {
            const auto& t = s.to[j];
            out.channel(t.outputChannel)[i] +=
                T(double(s.history.read(t.inputChannel, t.delayFrames, t.interpolation)) * t.gain *
                  mix);
        }
        if (s.remaining)
            --s.remaining;
    }
    return Status::ok;
}
template <Sample T> void MultiTapDelay<T>::reset() noexcept {
    if (state_) {
        state_->history.reset();
        state_->remaining = 0;
    }
}
template <Sample T> std::size_t MultiTapDelay<T>::transitionRemainingFrames() const noexcept {
    return state_ ? state_->remaining : 0;
}
template <Sample T> std::size_t MultiTapDelay<T>::tailFrames() const noexcept {
    return state_ ? state_->maximum + 2 : 0;
}
template class MultiTapDelay<float>;
template class MultiTapDelay<double>;
} // namespace cydsp
