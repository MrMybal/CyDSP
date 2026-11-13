#include "cydsp/processors/EchoCanceller.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/core/CircularHistory.h"
#include "detail/Analysis.h"
#include <algorithm>
#include <array>
#include <cmath>
namespace cydsp {
template <Sample T> struct EchoCanceller<T>::State {
    ProcessSpec spec;
    EchoCancellationConfig config;
    EchoCancellationParameters p;
    AdaptiveFilter<T> adaptive;
    CircularHistory<T> reference;
    AudioBuffer<T> ref, mic, predicted, error;
    EchoCancellationMetrics metrics;
    std::array<double, 8> micPower{}, predictionPower{}, errorPower{}, cross{};
    double farPower = 0, pole = 0;
    std::size_t hangover = 0, learning = 0;
};
template <Sample T> EchoCanceller<T>::EchoCanceller() = default;
template <Sample T> EchoCanceller<T>::~EchoCanceller() = default;
template <Sample T> EchoCanceller<T>::EchoCanceller(EchoCanceller&&) noexcept = default;
template <Sample T>
EchoCanceller<T>& EchoCanceller<T>::operator=(EchoCanceller&&) noexcept = default;
template <Sample T>
Status EchoCanceller<T>::prepare(const ProcessSpec& spec, const EchoCancellationConfig& config,
                                 CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || spec.channels > 8 || !config.taps ||
        config.taps > 4096 || config.referenceDelayFrames > std::size_t(spec.sampleRate * .5) ||
        !isFinite(config.initialLearningMs) || config.initialLearningMs < 0 ||
        config.initialLearningMs > 5000)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->config = config;
        s->pole = std::exp(-1 / (.01 * spec.sampleRate));
        s->learning = std::size_t(config.initialLearningMs * spec.sampleRate / 1000);
        auto single = spec;
        single.maxBlockFrames = 1;
        auto status = s->adaptive.prepare(single, config.taps, AdaptiveAlgorithm::nlms, backend);
        if (status != Status::ok)
            return status;
        status = s->reference.prepare(1, config.referenceDelayFrames + 1);
        if (status != Status::ok)
            return status;
        for (auto* b : {&s->ref, &s->mic, &s->predicted, &s->error}) {
            status = b->resize(spec.channels, 1);
            if (status != Status::ok)
                return status;
        }
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status EchoCanceller<T>::setParameters(const EchoCancellationParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!isFinite(p.step) || p.step < 0 || p.step > 1 || !isFinite(p.doubleTalkRatio) ||
        p.doubleTalkRatio < 1.1 || p.doubleTalkRatio > 100 || !isFinite(p.hangoverMs) ||
        p.hangoverMs < 0 || p.hangoverMs > 2000 || !isFinite(p.minimumReferencePower) ||
        p.minimumReferencePower < 1e-20 || p.minimumReferencePower > 1)
        return Status::invalid_argument;
    state_->p = p;
    return Status::ok;
}
template <Sample T>
Status EchoCanceller<T>::process(AudioBlock<const T> far, AudioBlock<const T> mic,
                                 AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto mono = s.spec;
    mono.channels = 1;
    auto status = detail::validateAnalysis(mono, far);
    if (status != Status::ok)
        return status;
    status = detail::validateAnalysis(s.spec, mic);
    if (status != Status::ok)
        return status;
    status = validateProcessBlocks(s.spec, mic, out);
    if (status != Status::ok)
        return status;
    if (far.frames() != mic.frames())
        return Status::invalid_argument;
    for (auto input : {far, mic}) {
        status = validateDisjointBlocks(input, out);
        if (status != Status::ok)
            return status;
    }
    const double a = 1 - s.pole;
    for (std::size_t i = 0; i < mic.frames(); ++i) {
        const T f = far.channel(0)[i];
        (void)s.reference.pushFrame({&f, 1});
        const T r = s.reference.read(0, double(s.config.referenceDelayFrames));
        s.farPower += a * (double(r) * r - s.farPower);
        bool near = false;
        if (!s.learning && s.farPower > s.p.minimumReferencePower)
            for (std::size_t ch = 0; ch < mic.channels(); ++ch) {
                const double coherence = s.cross[ch] * s.cross[ch] /
                                         std::max(1e-100, s.micPower[ch] * s.predictionPower[ch]);
                if (s.predictionPower[ch] > s.p.minimumReferencePower * .01 &&
                    s.errorPower[ch] > s.p.doubleTalkRatio * s.predictionPower[ch] &&
                    coherence < .65)
                    near = true;
            }
        if (near)
            s.hangover = std::size_t(s.p.hangoverMs * s.spec.sampleRate / 1000) + 1;
        s.metrics.referenceActive = s.farPower > s.p.minimumReferencePower;
        s.metrics.doubleTalk = s.hangover != 0;
        s.metrics.adapting = s.p.adaptationEnabled && s.metrics.referenceActive &&
                             (!s.metrics.doubleTalk || s.p.forceAdaptation);
        AdaptiveParameters p;
        p.step = s.p.step;
        p.adapt = s.metrics.adapting;
        p.coefficientLimit = 16;
        (void)s.adaptive.setParameters(p);
        for (std::size_t ch = 0; ch < mic.channels(); ++ch) {
            s.ref.block().channel(ch)[0] = r;
            s.mic.block().channel(ch)[0] = mic.channel(ch)[i];
        }
        (void)s.adaptive.process(s.ref.block(), s.mic.block(), s.predicted.block(),
                                 s.error.block());
        double desiredSum = 0, residualSum = 0;
        for (std::size_t ch = 0; ch < mic.channels(); ++ch) {
            const double d = mic.channel(ch)[i], y = s.predicted.block().channel(ch)[0],
                         e = s.error.block().channel(ch)[0];
            out.channel(ch)[i] = T(e);
            s.micPower[ch] += a * (d * d - s.micPower[ch]);
            s.predictionPower[ch] += a * (y * y - s.predictionPower[ch]);
            s.errorPower[ch] += a * (e * e - s.errorPower[ch]);
            s.cross[ch] += a * (d * y - s.cross[ch]);
            desiredSum += s.micPower[ch];
            residualSum += s.errorPower[ch];
        }
        s.metrics.residualPower = residualSum / double(mic.channels());
        s.metrics.echoReturnLossEnhancementDb =
            10 * std::log10(std::max(1e-100, desiredSum) / std::max(1e-100, residualSum));
        if (s.metrics.adapting) {
            ++s.metrics.adaptedFrames;
            if (s.learning)
                --s.learning;
        } else
            ++s.metrics.frozenFrames;
        if (s.hangover)
            --s.hangover;
    }
    return Status::ok;
}
template <Sample T> EchoCancellationMetrics EchoCanceller<T>::metrics() const noexcept {
    return state_ ? state_->metrics : EchoCancellationMetrics{};
}
template <Sample T>
std::span<const double> EchoCanceller<T>::coefficients(std::size_t c) const noexcept {
    return state_ ? state_->adaptive.coefficients(c) : std::span<const double>{};
}
template <Sample T> void EchoCanceller<T>::reset() noexcept {
    if (state_) {
        auto& s = *state_;
        s.adaptive.reset();
        s.reference.reset();
        s.metrics = {};
        s.micPower = s.predictionPower = s.errorPower = s.cross = {};
        s.farPower = 0;
        s.hangover = 0;
        s.learning = std::size_t(s.config.initialLearningMs * s.spec.sampleRate / 1000);
    }
}
template class EchoCanceller<float>;
template class EchoCanceller<double>;
} // namespace cydsp
