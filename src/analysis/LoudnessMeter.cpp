#include "cydsp/analysis/LoudnessMeter.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/processors/Biquad.h"
#include "detail/Analysis.h"
#include <algorithm>
#include <numbers>
#include <vector>
namespace cydsp {
namespace {
double loudness(double energy) noexcept {
    return energy > 0 ? -.691 + 10 * std::log10(energy) : -std::numeric_limits<double>::infinity();
}
// Independent bilinear parameterization of the BS.1770 two-stage K weighting.
// Exact published 48 kHz coefficients are used at the reference sample rate.
BiquadCoefficients kShelf(double rate) noexcept {
    if (rate == 48000)
        return {1.53512485958697, -2.69169618940638, 1.19839281085285, -1.69065929318241,
                .73248077421585};
    const double k = std::tan(std::numbers::pi * 1681.974450955533 / rate), q = .7071752369554196;
    const double high = std::pow(10., 3.999843853973347 / 20),
                 middle = std::pow(high, .4996667741545416);
    const double denominator = 1 + k / q + k * k;
    return {(high + middle * k / q + k * k) / denominator, 2 * (k * k - high) / denominator,
            (high - middle * k / q + k * k) / denominator, 2 * (k * k - 1) / denominator,
            (1 - k / q + k * k) / denominator};
}
BiquadCoefficients kHighPass(double rate) noexcept {
    if (rate == 48000)
        return {1, -2, 1, -1.99004745483398, .99007225036621};
    const double k = std::tan(std::numbers::pi * 38.13547087602444 / rate), q = .5003270373238773,
                 denominator = 1 + k / q + k * k;
    return {1, -2, 1, 2 * (k * k - 1) / denominator, (1 - k / q + k * k) / denominator};
}
double percentile(std::span<const double> sorted, double fraction) noexcept {
    const double position = fraction * double(sorted.size() - 1);
    const auto first = std::size_t(position), second = std::min(first + 1, sorted.size() - 1);
    return sorted[first] + (sorted[second] - sorted[first]) * (position - double(first));
}
} // namespace
template <Sample T> struct LoudnessMeter<T>::State {
    ProcessSpec spec;
    Biquad<double> shelf, highpass;
    AudioBuffer<double> scratch;
    std::vector<double> weights, energy, blocks, shortBlocks, sort;
    std::size_t momentaryFrames = 0, shortFrames = 0, hop = 0, phase = 0, head = 0, filled = 0,
                used = 0, shortUsed = 0;
    detail::CompensatedSum momentary, shortTerm;
};
template <Sample T> LoudnessMeter<T>::LoudnessMeter() = default;
template <Sample T> LoudnessMeter<T>::~LoudnessMeter() = default;
template <Sample T> LoudnessMeter<T>::LoudnessMeter(LoudnessMeter&&) noexcept = default;
template <Sample T>
LoudnessMeter<T>& LoudnessMeter<T>::operator=(LoudnessMeter&&) noexcept = default;
template <Sample T>
Status LoudnessMeter<T>::prepare(const ProcessSpec& spec, const LoudnessConfig& config) noexcept {
    if (!detail::validAnalysisSpec(spec) || !config.maximumGatingBlocks ||
        config.maximumGatingBlocks > 10000000 ||
        (config.channelWeights.empty() ? spec.channels > 2
                                       : config.channelWeights.size() != spec.channels))
        return Status::invalid_spec;
    for (double weight : config.channelWeights)
        if (!isFinite(weight) || weight < 0 || weight > 16)
            return Status::invalid_argument;
    try {
        auto n = std::make_unique<State>();
        n->spec = spec;
        n->weights.assign(spec.channels, 1.);
        if (!config.channelWeights.empty())
            std::copy(config.channelWeights.begin(), config.channelWeights.end(),
                      n->weights.begin());
        n->momentaryFrames = std::size_t(std::llround(spec.sampleRate * .4));
        n->shortFrames = std::size_t(std::llround(spec.sampleRate * 3));
        n->hop = std::size_t(std::llround(spec.sampleRate * .1));
        n->energy.resize(n->shortFrames);
        n->blocks.resize(config.maximumGatingBlocks);
        n->shortBlocks.resize(config.maximumGatingBlocks);
        n->sort.resize(config.maximumGatingBlocks);
        ProcessSpec filterSpec = spec;
        filterSpec.maxBlockFrames = std::min<std::size_t>(64, spec.maxBlockFrames);
        auto status = n->scratch.resize(spec.channels, filterSpec.maxBlockFrames);
        if (status != Status::ok)
            return status;
        status = n->shelf.prepare(filterSpec);
        if (status != Status::ok)
            return status;
        status = n->shelf.setCoefficients(kShelf(spec.sampleRate));
        if (status != Status::ok)
            return status;
        status = n->highpass.prepare(filterSpec);
        if (status != Status::ok)
            return status;
        status = n->highpass.setCoefficients(kHighPass(spec.sampleRate));
        if (status != Status::ok)
            return status;
        state_.swap(n);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status LoudnessMeter<T>::process(AudioBlock<const T> in) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    std::size_t extra = 0, extraShort = 0;
    for (std::size_t time = s.hop - s.phase; time <= in.frames(); time += s.hop) {
        if (s.filled + time >= s.momentaryFrames)
            ++extra;
        if (s.filled + time >= s.shortFrames)
            ++extraShort;
    }
    if (extra > s.blocks.size() - s.used || extraShort > s.shortBlocks.size() - s.shortUsed)
        return Status::would_block;
    const auto tile = s.scratch.block().frames();
    for (std::size_t at = 0; at < in.frames(); at += tile) {
        const auto count = std::min(tile, in.frames() - at);
        auto block = s.scratch.block(count);
        for (std::size_t ch = 0; ch < in.channels(); ++ch)
            for (std::size_t i = 0; i < count; ++i)
                block.channel(ch)[i] = double(in.channel(ch)[at + i]);
        (void)s.shelf.process(block, block);
        (void)s.highpass.process(block, block);
        for (std::size_t i = 0; i < count; ++i) {
            double value = 0;
            for (std::size_t ch = 0; ch < in.channels(); ++ch)
                value += s.weights[ch] * block.channel(ch)[i] * block.channel(ch)[i];
            const double oldMomentary =
                s.filled >= s.momentaryFrames
                    ? s.energy[(s.head + s.shortFrames - s.momentaryFrames) % s.shortFrames]
                    : 0;
            s.momentary.add(value);
            s.momentary.add(-oldMomentary);
            s.shortTerm.add(value);
            s.shortTerm.add(-s.energy[s.head]);
            s.energy[s.head] = value;
            s.head = (s.head + 1) % s.shortFrames;
            s.filled = std::min(s.filled + 1, s.shortFrames);
            if (++s.phase == s.hop) {
                s.phase = 0;
                if (s.filled >= s.momentaryFrames)
                    s.blocks[s.used++] =
                        std::max(0., s.momentary.value()) / double(s.momentaryFrames);
                if (s.filled >= s.shortFrames)
                    s.shortBlocks[s.shortUsed++] =
                        std::max(0., s.shortTerm.value()) / double(s.shortFrames);
            }
        }
    }
    return Status::ok;
}
template <Sample T> void LoudnessMeter<T>::reset() noexcept {
    if (!state_)
        return;
    auto& s = *state_;
    s.shelf.reset();
    s.highpass.reset();
    s.phase = s.head = s.filled = s.used = s.shortUsed = 0;
    s.momentary = {};
    s.shortTerm = {};
    std::fill(s.energy.begin(), s.energy.end(), 0.);
}
template <Sample T> LoudnessLevels LoudnessMeter<T>::levels() const noexcept {
    LoudnessLevels result;
    if (!state_)
        return result;
    const auto& s = *state_;
    result.momentaryReady = s.filled >= s.momentaryFrames;
    result.shortTermReady = s.filled >= s.shortFrames;
    if (result.momentaryReady)
        result.momentaryLufs =
            loudness(std::max(0., s.momentary.value()) / double(s.momentaryFrames));
    if (result.shortTermReady)
        result.shortTermLufs = loudness(std::max(0., s.shortTerm.value()) / double(s.shortFrames));
    return result;
}
template <Sample T> double LoudnessMeter<T>::integratedLufs() const noexcept {
    if (!state_)
        return -std::numeric_limits<double>::infinity();
    const auto& s = *state_;
    const double absolute = std::pow(10., (-70 + .691) / 10);
    double sum = 0;
    std::size_t count = 0;
    for (std::size_t i = 0; i < s.used; ++i)
        if (s.blocks[i] > absolute) {
            sum += s.blocks[i];
            ++count;
        }
    if (!count)
        return -std::numeric_limits<double>::infinity();
    const double threshold = std::max(absolute, sum / double(count) * .1);
    sum = 0;
    count = 0;
    for (std::size_t i = 0; i < s.used; ++i)
        if (s.blocks[i] > threshold) {
            sum += s.blocks[i];
            ++count;
        }
    return count ? loudness(sum / double(count)) : -std::numeric_limits<double>::infinity();
}
template <Sample T> double LoudnessMeter<T>::loudnessRangeLu() noexcept {
    if (!state_)
        return 0;
    auto& s = *state_;
    const double absolute = std::pow(10., (-70 + .691) / 10);
    double sum = 0;
    std::size_t count = 0;
    for (std::size_t i = 0; i < s.shortUsed; ++i)
        if (s.shortBlocks[i] > absolute) {
            sum += s.shortBlocks[i];
            ++count;
        }
    if (!count)
        return 0;
    const double threshold = std::max(absolute, sum / double(count) * .01);
    count = 0;
    for (std::size_t i = 0; i < s.shortUsed; ++i)
        if (s.shortBlocks[i] > threshold)
            s.sort[count++] = loudness(s.shortBlocks[i]);
    if (!count)
        return 0;
    auto values = std::span<double>(s.sort).first(count);
    std::sort(values.begin(), values.end());
    return percentile(values, .95) - percentile(values, .10);
}
template <Sample T> std::size_t LoudnessMeter<T>::gatingBlocks() const noexcept {
    return state_ ? state_->used : 0;
}
template <Sample T> std::size_t LoudnessMeter<T>::capacity() const noexcept {
    return state_ ? state_->blocks.size() : 0;
}
template class LoudnessMeter<float>;
template class LoudnessMeter<double>;
} // namespace cydsp
