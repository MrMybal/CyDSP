#include "cydsp/processors/Oversampler.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/processors/BiquadCascade.h"
#include "cydsp/processors/FilterDesign.h"
#include "cydsp/processors/Fir.h"
#include "cydsp/processors/TptFilter.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
namespace cydsp {
template <Sample T> struct Oversampler<T>::State {
    ProcessSpec spec;
    unsigned factor = 1;
    OversamplingMode mode = OversamplingMode::fir;
    struct Stage {
        Fir<T> firUp, firDown;
        BiquadCascade<T> iirUp, iirDown;
        AudioBuffer<T> zero, high, filtered;
    };
    std::vector<Stage> stages;
    double dcLatency = 0;
    Fir<T> up, down;
    AudioBuffer<T> zero, high, filtered;
    std::size_t pending = 0;
    bool armed = false;
};
template <Sample T> Oversampler<T>::Oversampler() = default;
template <Sample T> Oversampler<T>::~Oversampler() = default;
template <Sample T> Oversampler<T>::Oversampler(Oversampler&&) noexcept = default;
template <Sample T> Oversampler<T>& Oversampler<T>::operator=(Oversampler&&) noexcept = default;
template <Sample T>
Status Oversampler<T>::prepare(const ProcessSpec& spec, unsigned factor,
                               CpuBackend backend) noexcept {
    return prepare(spec, factor, OversamplingMode::fir, backend);
}
template <Sample T>
Status Oversampler<T>::prepare(const ProcessSpec& spec, unsigned factor, OversamplingMode mode,
                               CpuBackend backend) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    if (static_cast<unsigned>(mode) > static_cast<unsigned>(OversamplingMode::iir))
        return Status::invalid_argument;
    if ((factor != 1 && factor != 2 && factor != 4 && factor != 8 && factor != 16) ||
        spec.maxBlockFrames > std::numeric_limits<std::size_t>::max() / factor ||
        !isFinite(spec.sampleRate * factor))
        return Status::invalid_argument;
    try {
        auto next = std::make_unique<State>();
        next->spec = spec;
        next->factor = factor;
        next->mode = mode;
        if (mode != OversamplingMode::fir && factor > 1) {
            VectorOps checked;
            auto result = VectorOps::create(backend, checked);
            if (result != Status::ok)
                return result;
            unsigned level = 0;
            for (unsigned value = factor; value > 1; value /= 2)
                ++level;
            next->stages.resize(level);
            double multiplier = 2;
            for (auto& stage : next->stages) {
                ProcessSpec stageSpec{spec.sampleRate * multiplier,
                                      spec.maxBlockFrames * std::size_t(multiplier), spec.channels};
                for (auto* buffer : {&stage.zero, &stage.high, &stage.filtered}) {
                    result = buffer->resize(stageSpec.channels, stageSpec.maxBlockFrames);
                    if (result != Status::ok)
                        return result;
                }
                if (mode == OversamplingMode::halfBandFir) {
                    std::array<double, 65> designed{};
                    result = designWindowedFir(designed, .25, false, 100);
                    if (result != Status::ok)
                        return result;
                    std::array<T, 65> taps{};
                    double oddSum = 0;
                    for (std::size_t i = 1; i < 65; i += 2)
                        oddSum += designed[i];
                    for (std::size_t i = 1; i < 65; i += 2)
                        taps[i] = static_cast<T>(designed[i] * .5 / oddSum);
                    taps[32] = T{.5};
                    result = stage.firUp.prepare(stageSpec, taps, backend);
                    if (result != Status::ok)
                        return result;
                    result = stage.firDown.prepare(stageSpec, taps, backend);
                    if (result != Status::ok)
                        return result;
                    next->dcLatency += 64 / multiplier;
                } else {
                    IirDesign design;
                    design.family = IirFamily::elliptic;
                    design.order = 8;
                    design.sampleRate = stageSpec.sampleRate;
                    design.cutoff = stageSpec.sampleRate * .24;
                    design.rippleDb = .05;
                    design.ellipticRolloff = 0;
                    std::array<BiquadCoefficients, 8> c{};
                    std::size_t n = 0;
                    result = designIir(design, c, n);
                    if (result != Status::ok)
                        return result;
                    double response = 1;
                    for (std::size_t i = 0; i < n; ++i)
                        response *= (c[i].b0 + c[i].b1 + c[i].b2) / (1 + c[i].a1 + c[i].a2);
                    c[0].b0 /= response;
                    c[0].b1 /= response;
                    c[0].b2 /= response;
                    for (std::size_t i = 0; i < n; ++i)
                        next->dcLatency +=
                            2 *
                            ((c[i].b1 + 2 * c[i].b2) / (c[i].b0 + c[i].b1 + c[i].b2) -
                             (c[i].a1 + 2 * c[i].a2) / (1 + c[i].a1 + c[i].a2)) /
                            multiplier;
                    result = stage.iirUp.prepare(stageSpec, std::span(c).first(n));
                    if (result != Status::ok)
                        return result;
                    result = stage.iirDown.prepare(stageSpec, std::span(c).first(n));
                    if (result != Status::ok)
                        return result;
                }
                multiplier *= 2;
            }
            state_.swap(next);
            return Status::ok;
        }
        next->dcLatency = factor > 1 ? 32 : 0;
        ProcessSpec high{spec.sampleRate * factor, spec.maxBlockFrames * factor, spec.channels};
        for (auto* buffer : {&next->zero, &next->high, &next->filtered}) {
            const auto status = buffer->resize(high.channels, high.maxBlockFrames);
            if (status != Status::ok)
                return status;
        }
        std::vector<double> designed(factor == 1 ? 1 : 32 * factor + 1, 1);
        if (factor > 1) {
            const auto status = designWindowedFir(designed, 0.46 / factor, false, 100);
            if (status != Status::ok)
                return status;
        }
        std::vector<T> taps(designed.size());
        std::transform(designed.begin(), designed.end(), taps.begin(),
                       [](double v) { return static_cast<T>(v); });
        auto status = next->up.prepare(high, taps, backend);
        if (status != Status::ok)
            return status;
        status = next->down.prepare(high, taps, backend);
        if (status != Status::ok)
            return status;
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
}
template <Sample T>
Status Oversampler<T>::processUp(AudioBlock<const T> in, AudioBlock<T>& result) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (s.armed)
        return Status::transition_in_progress;
    if (!in.isValid())
        return Status::invalid_argument;
    if (in.channels() != s.spec.channels)
        return Status::channel_mismatch;
    if (in.frames() > s.spec.maxBlockFrames)
        return Status::block_too_large;
    if (!s.stages.empty()) {
        AudioBlock<const T> input = in;
        for (auto& stage : s.stages) {
            const auto size = input.frames() * 2;
            auto zero = stage.zero.block(size), high = stage.high.block(size);
            for (std::size_t ch = 0; ch < in.channels(); ++ch) {
                std::fill(zero.channel(ch).begin(), zero.channel(ch).end(), T{});
                for (std::size_t i = 0; i < input.frames(); ++i)
                    zero.channel(ch)[i * 2] = input.channel(ch)[i] * T{2};
            }
            const auto r = s.mode == OversamplingMode::halfBandFir
                               ? stage.firUp.process(zero, high)
                               : stage.iirUp.process(zero, high);
            if (r != Status::ok)
                return r;
            input = high;
        }
        s.armed = true;
        s.pending = in.frames();
        result = s.stages.back().high.block(in.frames() * s.factor);
        return Status::ok;
    }
    const auto size = in.frames() * s.factor;
    auto zero = s.zero.block(size), high = s.high.block(size);
    for (std::size_t ch = 0; ch < in.channels(); ++ch) {
        std::fill(zero.channel(ch).begin(), zero.channel(ch).end(), T{});
        for (std::size_t i = 0; i < in.frames(); ++i)
            zero.channel(ch)[i * s.factor] = in.channel(ch)[i] * static_cast<T>(s.factor);
    }
    auto status = s.up.process(zero, high);
    if (status != Status::ok)
        return status;
    s.armed = true;
    s.pending = in.frames();
    result = high;
    return Status::ok;
}
template <Sample T> Status Oversampler<T>::processDown(AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (!s.armed)
        return Status::not_prepared;
    if (out.frames() != s.pending)
        return Status::invalid_argument;
    auto status = validateProcessBlocks<T>(s.spec, out, out);
    if (status != Status::ok)
        return status;
    if (!s.stages.empty()) {
        std::size_t frames = s.pending * s.factor;
        for (std::size_t index = s.stages.size(); index-- > 0;) {
            auto& stage = s.stages[index];
            auto high = stage.high.block(frames), filtered = stage.filtered.block(frames);
            const auto r = s.mode == OversamplingMode::halfBandFir
                               ? stage.firDown.process(high, filtered)
                               : stage.iirDown.process(high, filtered);
            if (r != Status::ok)
                return r;
            auto low = index ? s.stages[index - 1].high.block(frames / 2) : out;
            for (std::size_t ch = 0; ch < out.channels(); ++ch)
                for (std::size_t i = 0; i < frames / 2; ++i)
                    low.channel(ch)[i] = filtered.channel(ch)[2 * i];
            frames /= 2;
        }
        s.armed = false;
        return Status::ok;
    }
    auto high = s.high.block(s.pending * s.factor),
         filtered = s.filtered.block(s.pending * s.factor);
    status = s.down.process(high, filtered);
    if (status != Status::ok)
        return status;
    for (std::size_t ch = 0; ch < out.channels(); ++ch)
        for (std::size_t i = 0; i < out.frames(); ++i)
            out.channel(ch)[i] = filtered.channel(ch)[i * s.factor];
    s.armed = false;
    return Status::ok;
}
template <Sample T> void Oversampler<T>::reset() noexcept {
    if (state_) {
        for (auto& stage : state_->stages) {
            stage.firUp.reset();
            stage.firDown.reset();
            stage.iirUp.reset();
            stage.iirDown.reset();
        }
        state_->up.reset();
        state_->down.reset();
        state_->armed = false;
        state_->pending = 0;
    }
}
template <Sample T> unsigned Oversampler<T>::factor() const noexcept {
    return state_ ? state_->factor : 0;
}
template <Sample T> std::size_t Oversampler<T>::latencyFrames() const noexcept {
    return state_ && linearPhase() ? static_cast<std::size_t>(std::llround(state_->dcLatency)) : 0;
}
template <Sample T> bool Oversampler<T>::linearPhase() const noexcept {
    return state_ && state_->mode != OversamplingMode::iir;
}
template <Sample T> double Oversampler<T>::latencyAtDcFrames() const noexcept {
    return state_ ? state_->dcLatency : 0;
}
template class Oversampler<float>;
template class Oversampler<double>;
} // namespace cydsp
