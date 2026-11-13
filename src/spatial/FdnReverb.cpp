#include "cydsp/spatial/FdnReverb.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/core/CircularHistory.h"
#include "cydsp/math/VectorOps.h"
#include "cydsp/processors/Diffusion.h"
#include "detail/Analysis.h"
#include "detail/VectorTable.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <numbers>
#include <vector>
namespace cydsp {
namespace {
bool validFdn(const FdnParameters& p, double maximum) {
    if (!isFinite(p.preDelayMs) || p.preDelayMs < 0 || p.preDelayMs > maximum ||
        !isFinite(p.diffusion) || p.diffusion < 0 || p.diffusion > 1 || !isFinite(p.wet) ||
        p.wet < 0 || p.wet > 1 || !isFinite(p.dry) || p.dry < 0 || p.dry > 1)
        return false;
    for (double d : p.decaySeconds)
        if (!isFinite(d) || d < .05 || d > 120)
            return false;
    return true;
}
} // namespace
template <Sample T> struct FdnReverb<T>::State {
    ProcessSpec spec;
    FdnConfig config;
    FdnParameters parameters;
    VectorOps ops;
    Diffuser<T> diffuser;
    CircularHistory<T> predelay;
    AudioBuffer<T> scratch;
    std::vector<T> history, projection;
    std::vector<std::size_t> lengths, offsets, heads;
    std::vector<double> low, high, gain, target;
    double lowPole = 0, highPole = 0, preDelay = 0, wet = 1, dry = 0, injection = 1, diffusion = .7;
    std::size_t remaining = 0;
    void targets(const FdnParameters& p) {
        for (std::size_t line = 0; line < config.lines; ++line) {
            std::array<double, 3> g{};
            for (std::size_t b = 0; b < 3; ++b)
                g[b] = p.freeze ? 1
                                : std::pow(.001, double(lengths[line]) /
                                                     (spec.sampleRate * p.decaySeconds[b]));
            const double bound = g[2] + std::abs(g[1] - g[2]) + std::abs(g[0] - g[1]);
            if (!p.freeze && bound > .9999)
                for (auto& value : g)
                    value *= .9999 / bound;
            for (std::size_t b = 0; b < 3; ++b)
                target[line * 3 + b] = g[b];
        }
    }
};
template <Sample T> FdnReverb<T>::FdnReverb() = default;
template <Sample T> FdnReverb<T>::~FdnReverb() = default;
template <Sample T> FdnReverb<T>::FdnReverb(FdnReverb&&) noexcept = default;
template <Sample T> FdnReverb<T>& FdnReverb<T>::operator=(FdnReverb&&) noexcept = default;
template <Sample T>
Status FdnReverb<T>::prepare(const ProcessSpec& spec, const FdnConfig& config,
                             CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || config.lines < 4 || config.lines > 32 ||
        !isPowerOfTwo(config.lines) || spec.channels > std::min<std::size_t>(8, config.lines) ||
        !isFinite(config.roomScale) || config.roomScale < .25 || config.roomScale > 4 ||
        !isFinite(config.maximumPreDelayMs) || config.maximumPreDelayMs < 0 ||
        config.maximumPreDelayMs > 2000)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->config = config;
        s->parameters.preDelayMs = std::min(10., config.maximumPreDelayMs);
        auto status = VectorOps::create(backend, s->ops);
        if (status != Status::ok)
            return status;
        std::array<DiffusionStage, 4> stages{};
        constexpr std::array<double, 4> times{.0047, .0073, .0113, .0137};
        for (std::size_t i = 0; i < 4; ++i)
            stages[i] = {
                std::max<std::size_t>(1, std::size_t(std::llround(times[i] * spec.sampleRate))),
                .6};
        status = s->diffuser.prepare(spec, stages);
        if (status != Status::ok)
            return status;
        status = s->scratch.resize(spec.channels, spec.maxBlockFrames);
        if (status != Status::ok)
            return status;
        status = s->predelay.prepare(
            spec.channels,
            std::size_t(std::ceil(config.maximumPreDelayMs * .001 * spec.sampleRate)) + 4);
        if (status != Status::ok)
            return status;
        std::size_t size = 0;
        for (std::size_t line = 0; line < config.lines; ++line) {
            const double seconds =
                (.0217 + .00137 * double(line) + .000037 * double(line * line)) * config.roomScale;
            auto length =
                std::max<std::size_t>(3, std::size_t(std::llround(seconds * spec.sampleRate)));
            if (length % 2 == 0)
                ++length;
            s->lengths.push_back(length);
            s->offsets.push_back(size);
            size += length;
        }
        s->history.resize(size);
        s->heads.resize(config.lines);
        s->low.resize(config.lines);
        s->high.resize(config.lines);
        s->gain.resize(3 * config.lines);
        s->target.resize(3 * config.lines);
        s->projection.resize(spec.channels * config.lines);
        const double scale = 1 / std::sqrt(double(config.lines));
        for (std::size_t ch = 0; ch < spec.channels; ++ch)
            for (std::size_t line = 0; line < config.lines; ++line)
                s->projection[ch * config.lines + line] =
                    T((std::popcount(line & ch) % 2 ? -1 : 1) * scale);
        s->lowPole = std::exp(-2 * std::numbers::pi * std::min(250., spec.sampleRate * .1) /
                              spec.sampleRate);
        s->highPole = std::exp(-2 * std::numbers::pi * std::min(4000., spec.sampleRate * .4) /
                               spec.sampleRate);
        s->targets(s->parameters);
        s->gain = s->target;
        s->preDelay = s->parameters.preDelayMs * .001 * spec.sampleRate;
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status FdnReverb<T>::setParameters(const FdnParameters& p, std::size_t frames) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (!validFdn(p, s.config.maximumPreDelayMs) || frames > (1u << 30))
        return Status::invalid_argument;
    s.parameters = p;
    s.targets(p);
    s.remaining = frames;
    if (!frames) {
        std::copy(s.target.begin(), s.target.end(), s.gain.begin());
        s.wet = p.wet;
        s.dry = p.dry;
        s.injection = p.freeze ? 0 : 1;
        s.diffusion = p.diffusion;
        s.preDelay = p.preDelayMs * .001 * s.spec.sampleRate;
    }
    return Status::ok;
}
template <Sample T>
Status FdnReverb<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, input, output);
    if (status != Status::ok)
        return status;
    status = detail::validateAnalysis(s.spec, input);
    if (status != Status::ok)
        return status;
    status = s.diffuser.process(input, s.scratch.block(input.frames()));
    if (status != Status::ok)
        return status;
    std::array<T, 32> read{}, feedback{}, mixed{};
    std::array<T, 8> excitation{}, original{};
    const auto& kernels = detail::VectorAccess::kernels<T>(s.ops);
    for (std::size_t i = 0; i < input.frames(); ++i) {
        if (s.remaining) {
            const double fraction = 1 / double(s.remaining);
            for (std::size_t j = 0; j < s.gain.size(); ++j)
                s.gain[j] += (s.target[j] - s.gain[j]) * fraction;
            s.wet += (s.parameters.wet - s.wet) * fraction;
            s.dry += (s.parameters.dry - s.dry) * fraction;
            s.injection += ((s.parameters.freeze ? 0 : 1) - s.injection) * fraction;
            s.diffusion += (s.parameters.diffusion - s.diffusion) * fraction;
            s.preDelay +=
                (s.parameters.preDelayMs * .001 * s.spec.sampleRate - s.preDelay) * fraction;
            --s.remaining;
        }
        for (std::size_t ch = 0; ch < input.channels(); ++ch) {
            original[ch] = input.channel(ch)[i];
            excitation[ch] =
                T((1 - s.diffusion) * double(original[ch]) +
                  s.diffusion * double(s.scratch.block(input.frames()).channel(ch)[i]));
        }
        (void)s.predelay.pushFrame(std::span<const T>(excitation).first(input.channels()));
        for (std::size_t ch = 0; ch < input.channels(); ++ch)
            excitation[ch] = s.predelay.read(ch, s.preDelay);
        for (std::size_t line = 0; line < s.config.lines; ++line) {
            const double x = s.history[s.offsets[line] + s.heads[line]];
            read[line] = T(x);
            s.low[line] = x + s.lowPole * (s.low[line] - x);
            s.high[line] = x + s.highPole * (s.high[line] - x);
            feedback[line] = T(s.gain[line * 3 + 2] * x +
                               (s.gain[line * 3 + 1] - s.gain[line * 3 + 2]) * s.high[line] +
                               (s.gain[line * 3] - s.gain[line * 3 + 1]) * s.low[line]);
        }
        (void)orthogonalMix<T>(std::span<const T>(feedback).first(s.config.lines),
                               std::span<T>(mixed).first(s.config.lines));
        for (std::size_t line = 0; line < s.config.lines; ++line) {
            double injection = 0;
            for (std::size_t ch = 0; ch < input.channels(); ++ch)
                injection += double(excitation[ch]) * s.projection[ch * s.config.lines + line];
            s.history[s.offsets[line] + s.heads[line]] =
                T(double(mixed[line]) + injection * s.injection);
            if (++s.heads[line] == s.lengths[line])
                s.heads[line] = 0;
        }
        for (std::size_t ch = 0; ch < output.channels(); ++ch)
            output.channel(ch)[i] =
                T(double(kernels.dot(read.data(), s.projection.data() + ch * s.config.lines,
                                     s.config.lines)) *
                      s.wet +
                  double(original[ch]) * s.dry);
    }
    return Status::ok;
}
template <Sample T> void FdnReverb<T>::reset() noexcept {
    if (!state_)
        return;
    auto& s = *state_;
    std::fill(s.history.begin(), s.history.end(), T{});
    std::fill(s.heads.begin(), s.heads.end(), 0);
    std::fill(s.low.begin(), s.low.end(), 0.);
    std::fill(s.high.begin(), s.high.end(), 0.);
    s.predelay.reset();
    s.diffuser.reset();
    (void)setParameters(s.parameters, 0);
}
template <Sample T> std::size_t FdnReverb<T>::nominalTailFrames() const noexcept {
    if (!state_)
        return 0;
    if (state_->parameters.freeze)
        return std::numeric_limits<std::size_t>::max();
    return std::size_t(std::ceil((*std::max_element(state_->parameters.decaySeconds.begin(),
                                                    state_->parameters.decaySeconds.end()) +
                                  state_->parameters.preDelayMs * .001) *
                                 state_->spec.sampleRate));
}
template <Sample T> CpuBackend FdnReverb<T>::backend() const noexcept {
    return state_ ? state_->ops.backend() : CpuBackend::scalar;
}
template class FdnReverb<float>;
template class FdnReverb<double>;
} // namespace cydsp
