#include "cydsp/processors/AdaptiveFilter.h"
#include "cydsp/math/VectorOps.h"
#include "detail/Analysis.h"
#include "detail/VectorTable.h"
#include <algorithm>
#include <cmath>
#include <vector>
namespace cydsp {
template <Sample T> struct AdaptiveFilter<T>::State {
    ProcessSpec spec;
    std::size_t taps = 0, head = 0;
    AdaptiveAlgorithm algorithm;
    AdaptiveParameters p;
    VectorOps ops;
    std::vector<double> history, weights;
    std::uint64_t clipped = 0;
};
template <Sample T> AdaptiveFilter<T>::AdaptiveFilter() = default;
template <Sample T> AdaptiveFilter<T>::~AdaptiveFilter() = default;
template <Sample T> AdaptiveFilter<T>::AdaptiveFilter(AdaptiveFilter&&) noexcept = default;
template <Sample T>
AdaptiveFilter<T>& AdaptiveFilter<T>::operator=(AdaptiveFilter&&) noexcept = default;
template <Sample T>
Status AdaptiveFilter<T>::prepare(const ProcessSpec& spec, std::size_t taps,
                                  AdaptiveAlgorithm algorithm, CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || spec.channels > 16 || !taps || taps > 4096 ||
        unsigned(algorithm) > unsigned(AdaptiveAlgorithm::nlms))
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->taps = taps;
        s->algorithm = algorithm;
        auto status = VectorOps::create(backend, s->ops);
        if (status != Status::ok)
            return status;
        s->history.resize(2 * taps * spec.channels);
        s->weights.resize(taps * spec.channels);
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status AdaptiveFilter<T>::setParameters(const AdaptiveParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!isFinite(p.step) || p.step < 0 || p.step >= 2 || !isFinite(p.leak) || p.leak < 0 ||
        p.leak > 1 || !isFinite(p.regularization) || p.regularization < 1e-20 ||
        p.regularization > 1e6 || !isFinite(p.coefficientLimit) || p.coefficientLimit < .01 ||
        p.coefficientLimit > 1e6)
        return Status::invalid_argument;
    state_->p = p;
    return Status::ok;
}
template <Sample T>
Status AdaptiveFilter<T>::process(AudioBlock<const T> reference, AudioBlock<const T> desired,
                                  AudioBlock<T> prediction, AudioBlock<T> residual) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = detail::validateAnalysis(s.spec, reference);
    if (status != Status::ok)
        return status;
    status = detail::validateAnalysis(s.spec, desired);
    if (status != Status::ok)
        return status;
    if (reference.frames() != desired.frames() || prediction.frames() != reference.frames() ||
        residual.frames() != reference.frames())
        return Status::invalid_argument;
    if (prediction.channels() != s.spec.channels || residual.channels() != s.spec.channels)
        return Status::channel_mismatch;
    for (auto out : {prediction, residual})
        for (auto in : {reference, desired}) {
            status = validateDisjointBlocks(in, out);
            if (status != Status::ok)
                return status;
        }
    status = validateDisjointBlocks(AudioBlock<const T>(prediction), residual);
    if (status != Status::ok)
        return status;
    const auto& kernels = detail::VectorAccess::kernels<double>(s.ops);
    for (std::size_t i = 0; i < reference.frames(); ++i) {
        for (std::size_t ch = 0; ch < s.spec.channels; ++ch) {
            auto* h = s.history.data() + ch * 2 * s.taps;
            h[s.head] = h[s.head + s.taps] = reference.channel(ch)[i];
            const auto* x = h + s.head;
            auto* w = s.weights.data() + ch * s.taps;
            const double y = kernels.dot(x, w, s.taps), error = double(desired.channel(ch)[i]) - y;
            prediction.channel(ch)[i] = T(y);
            residual.channel(ch)[i] = T(error);
            if (s.p.adapt) {
                const double energy =
                    s.algorithm == AdaptiveAlgorithm::nlms ? kernels.dot(x, x, s.taps) : 0;
                const double step = s.algorithm == AdaptiveAlgorithm::nlms
                                        ? s.p.step / (energy + s.p.regularization)
                                        : s.p.step;
                // The same SIMD table used for prediction also updates the path.
                // Separate passes preserve strict non-fused multiply/add semantics.
                kernels.scale(w, w, s.taps, 1 - s.p.leak);
                kernels.addScaled(x, w, s.taps, step * error);
                for (std::size_t k = 0; k < s.taps; ++k) {
                    const double update = w[k];
                    const double bounded =
                        std::clamp(update, -s.p.coefficientLimit, s.p.coefficientLimit);
                    if (bounded != update)
                        ++s.clipped;
                    w[k] = bounded;
                }
            }
        }
        s.head = s.head == 0 ? s.taps - 1 : s.head - 1;
    }
    return Status::ok;
}
template <Sample T>
std::span<const double> AdaptiveFilter<T>::coefficients(std::size_t ch) const noexcept {
    return state_ && ch < state_->spec.channels
               ? std::span<const double>(state_->weights.data() + ch * state_->taps, state_->taps)
               : std::span<const double>{};
}
template <Sample T> std::uint64_t AdaptiveFilter<T>::clippedUpdates() const noexcept {
    return state_ ? state_->clipped : 0;
}
template <Sample T> void AdaptiveFilter<T>::reset() noexcept {
    if (state_) {
        std::fill(state_->history.begin(), state_->history.end(), 0);
        std::fill(state_->weights.begin(), state_->weights.end(), 0);
        state_->head = 0;
        state_->clipped = 0;
    }
}
template class AdaptiveFilter<float>;
template class AdaptiveFilter<double>;
} // namespace cydsp
