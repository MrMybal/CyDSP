#include "cydsp/control/Quality.h"
#include "detail/Analysis.h"
#include <algorithm>
namespace cydsp {
namespace {
bool valid(QualityProfile p) noexcept {
    return unsigned(p) <= unsigned(QualityProfile::offline);
}
} // namespace
Status qualitySettings(QualityProfile profile, double rate, QualitySettings& output) noexcept {
    if (!valid(profile) || !isFinite(rate) || rate < 8000 || rate > 768000)
        return Status::invalid_argument;
    QualitySettings s;
    s.profile = profile;
    const double seconds = profile == QualityProfile::economy        ? .012
                           : profile == QualityProfile::realtimeHigh ? .024
                                                                     : .06;
    std::size_t n = 128;
    while (n < 8192 && double(n) < seconds * rate)
        n *= 2;
    s.spectral = {n, n / (profile == QualityProfile::offline ? 8 : 4), WindowType::hann};
    s.separation.stft = s.spectral;
    s.separation.temporalFrames = s.separation.frequencyBins =
        profile == QualityProfile::economy        ? 9
        : profile == QualityProfile::realtimeHigh ? 17
                                                  : 31;
    s.hrtf.sourceSampleRate = s.hrtf.targetSampleRate = rate;
    s.hrtf.maximumTaps = profile == QualityProfile::economy        ? 256
                         : profile == QualityProfile::realtimeHigh ? 1024
                                                                   : 2048;
    s.resampling = profile == QualityProfile::economy        ? ResampleQuality::economy
                   : profile == QualityProfile::realtimeHigh ? ResampleQuality::balanced
                                                             : ResampleQuality::high;
    s.sampleInterpolation = profile == QualityProfile::economy ? SampleInterpolation::linear
                                                               : SampleInterpolation::bandLimited;
    s.reverb.lines = profile == QualityProfile::economy        ? 8
                     : profile == QualityProfile::realtimeHigh ? 16
                                                               : 32;
    s.oversampling = profile == QualityProfile::economy        ? 2
                     : profile == QualityProfile::realtimeHigh ? 4
                                                               : 8;
    s.truePeakFactor = profile == QualityProfile::offline ? 8 : 4;
    s.suggestedMaximumGrains = profile == QualityProfile::economy        ? 32
                               : profile == QualityProfile::realtimeHigh ? 128
                                                                         : 512;
    s.transitionFrames = std::size_t(rate * .01);
    output = s;
    return Status::ok;
}
template <Sample T> struct QualityGraph<T>::State {
    GraphTransition<T> graph;
    QualityProfile active = QualityProfile::realtimeHigh, staged = QualityProfile::realtimeHigh;
    bool running = false;
};
template <Sample T> QualityGraph<T>::QualityGraph() = default;
template <Sample T> QualityGraph<T>::~QualityGraph() = default;
template <Sample T> QualityGraph<T>::QualityGraph(QualityGraph&&) noexcept = default;
template <Sample T> QualityGraph<T>& QualityGraph<T>::operator=(QualityGraph&&) noexcept = default;
template <Sample T>
Status QualityGraph<T>::prepare(const ProcessSpec& spec, std::span<const DspBus> ins,
                                std::span<const DspBus> outs, std::size_t budget,
                                CpuBackend backend) noexcept {
    try {
        auto s = std::make_unique<State>();
        auto st = s->graph.prepare(spec, ins, outs, budget, backend);
        if (st != Status::ok)
            return st;
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status QualityGraph<T>::setInitial(QualityProfile profile, MultiBusGraph<T>&& g) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!valid(profile))
        return Status::invalid_argument;
    const auto st = state_->graph.setInitial(std::move(g));
    if (st == Status::ok) {
        state_->active = profile;
        state_->running = false;
    }
    return st;
}
template <Sample T>
Status QualityGraph<T>::stage(QualityProfile profile, MultiBusGraph<T>&& g) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!valid(profile))
        return Status::invalid_argument;
    const auto st = state_->graph.stage(std::move(g));
    if (st == Status::ok)
        state_->staged = profile;
    return st;
}
template <Sample T>
Status QualityGraph<T>::startTransition(std::size_t fade, std::size_t warmup) noexcept {
    if (!state_)
        return Status::not_prepared;
    const auto st = state_->graph.startTransition(fade, warmup);
    if (st == Status::ok)
        state_->running = true;
    return st;
}
template <Sample T>
Status QualityGraph<T>::process(std::span<const AudioBlock<const T>> in,
                                std::span<AudioBlock<T>> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    const auto st = s.graph.process(in, out);
    if (st == Status::ok && s.running && out.size() && out[0].frames() &&
        !s.graph.transitionRemainingFrames() && !s.graph.warmupRemainingFrames()) {
        s.active = s.staged;
        s.running = false;
    }
    return st;
}
template <Sample T> void QualityGraph<T>::reset() noexcept {
    if (state_) {
        state_->graph.reset();
        state_->running = false;
    }
}
template <Sample T> QualityProfile QualityGraph<T>::activeProfile() const noexcept {
    return state_ ? state_->active : QualityProfile::realtimeHigh;
}
template <Sample T> bool QualityGraph<T>::transitioning() const noexcept {
    return state_ && state_->running;
}
template <Sample T> std::size_t QualityGraph<T>::latencyFrames() const noexcept {
    return state_ ? state_->graph.latencyFrames() : 0;
}
template class QualityGraph<float>;
template class QualityGraph<double>;
} // namespace cydsp
