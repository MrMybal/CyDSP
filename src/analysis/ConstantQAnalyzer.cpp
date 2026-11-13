#include "cydsp/analysis/ConstantQAnalyzer.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/math/VectorOps.h"
#include "cydsp/math/Window.h"
#include "detail/Analysis.h"
#include "detail/VectorTable.h"
#include <algorithm>
#include <numbers>
#include <numeric>
#include <vector>
namespace cydsp {
template <Sample T> struct ConstantQAnalyzer<T>::State {
    ProcessSpec spec;
    ConstantQConfig config;
    VectorOps ops;
    AudioBuffer<double> history;
    std::vector<double> frequencies, real, imag, magnitudes;
    std::vector<std::size_t> offsets, lengths;
    std::size_t largest = 0, head = 0, filled = 0, since = 0;
    std::uint64_t frames = 0;
    void analyze() noexcept {
        const auto& kernels = detail::VectorAccess::kernels<double>(ops);
        for (std::size_t ch = 0; ch < spec.channels; ++ch) {
            const auto* oldest = history.block().channel(ch).data() + head;
            for (std::size_t bin = 0; bin < config.bins; ++bin) {
                const auto length = lengths[bin], offset = offsets[bin];
                const auto* input = oldest + largest - length;
                const double r = kernels.dot(input, real.data() + offset, length),
                             i = kernels.dot(input, imag.data() + offset, length);
                magnitudes[ch * config.bins + bin] = std::hypot(r, i);
            }
        }
        ++frames;
        since = 0;
    }
};
template <Sample T> ConstantQAnalyzer<T>::ConstantQAnalyzer() = default;
template <Sample T> ConstantQAnalyzer<T>::~ConstantQAnalyzer() = default;
template <Sample T> ConstantQAnalyzer<T>::ConstantQAnalyzer(ConstantQAnalyzer&&) noexcept = default;
template <Sample T>
ConstantQAnalyzer<T>& ConstantQAnalyzer<T>::operator=(ConstantQAnalyzer&&) noexcept = default;
template <Sample T>
Status ConstantQAnalyzer<T>::prepare(const ProcessSpec& spec, const ConstantQConfig& config,
                                     CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || !isFinite(config.minimumHz) || config.minimumHz <= 0 ||
        config.binsPerOctave < 1 || config.binsPerOctave > 96 || !config.bins ||
        config.bins > 512 || !config.hopFrames || config.hopFrames > 65536 ||
        config.maximumKernelFrames < 4 || config.maximumKernelFrames > (1u << 20))
        return Status::invalid_spec;
    try {
        auto n = std::make_unique<State>();
        n->spec = spec;
        n->config = config;
        auto status = VectorOps::create(backend, n->ops);
        if (status != Status::ok)
            return status;
        const double q = 1 / std::expm1(std::numbers::ln2 / double(config.binsPerOctave));
        std::size_t total = 0;
        for (std::size_t bin = 0; bin < config.bins; ++bin) {
            const double frequency =
                config.minimumHz * std::exp2(double(bin) / double(config.binsPerOctave));
            const double requested = std::max(4., std::ceil(q * spec.sampleRate / frequency));
            if (!isFinite(frequency) || frequency >= spec.sampleRate * .5 ||
                requested > double(config.maximumKernelFrames))
                return Status::invalid_argument;
            const auto length = std::size_t(requested);
            if (total > (1u << 24) - length)
                return Status::invalid_argument;
            n->frequencies.push_back(frequency);
            n->lengths.push_back(length);
            n->offsets.push_back(total);
            total += length;
            n->largest = std::max(n->largest, length);
        }
        n->real.resize(total);
        n->imag.resize(total);
        n->magnitudes.resize(spec.channels * config.bins);
        status = n->history.resize(spec.channels, n->largest * 2);
        if (status != Status::ok)
            return status;
        std::vector<double> window;
        for (std::size_t bin = 0; bin < config.bins; ++bin) {
            const auto length = n->lengths[bin], offset = n->offsets[bin];
            window.resize(length);
            status = makeWindow(WindowType::hann, std::span<double>(window));
            if (status != Status::ok)
                return status;
            const double scale = 2 / std::accumulate(window.begin(), window.end(), 0.);
            for (std::size_t i = 0; i < length; ++i) {
                const double phase =
                    2 * std::numbers::pi * n->frequencies[bin] * double(i) / spec.sampleRate;
                n->real[offset + i] = window[i] * scale * std::cos(phase);
                n->imag[offset + i] = -window[i] * scale * std::sin(phase);
            }
        }
        state_.swap(n);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status ConstantQAnalyzer<T>::process(AudioBlock<const T> in) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    for (std::size_t i = 0; i < in.frames(); ++i) {
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            auto channel = s.history.block().channel(ch);
            channel[s.head] = channel[s.head + s.largest] = double(in.channel(ch)[i]);
        }
        s.head = (s.head + 1) % s.largest;
        s.filled = std::min(s.filled + 1, s.largest);
        ++s.since;
        if (s.filled == s.largest && (!s.frames || s.since >= s.config.hopFrames))
            s.analyze();
    }
    return Status::ok;
}
template <Sample T> void ConstantQAnalyzer<T>::reset() noexcept {
    if (!state_)
        return;
    auto& s = *state_;
    s.head = s.filled = s.since = 0;
    s.frames = 0;
    for (std::size_t ch = 0; ch < s.spec.channels; ++ch)
        std::fill(s.history.block().channel(ch).begin(), s.history.block().channel(ch).end(), 0.);
    std::fill(s.magnitudes.begin(), s.magnitudes.end(), 0.);
}
template <Sample T> bool ConstantQAnalyzer<T>::hasFrame() const noexcept {
    return state_ && state_->frames > 0;
}
template <Sample T> std::uint64_t ConstantQAnalyzer<T>::frameCount() const noexcept {
    return state_ ? state_->frames : 0;
}
template <Sample T>
std::span<const double> ConstantQAnalyzer<T>::centerFrequencies() const noexcept {
    return state_ ? std::span<const double>(state_->frequencies) : std::span<const double>{};
}
template <Sample T>
std::span<const double> ConstantQAnalyzer<T>::magnitudes(std::size_t ch) const noexcept {
    return state_ && ch < state_->spec.channels
               ? std::span<const double>(state_->magnitudes)
                     .subspan(ch * state_->config.bins, state_->config.bins)
               : std::span<const double>{};
}
template <Sample T> std::size_t ConstantQAnalyzer<T>::windowFrames(std::size_t bin) const noexcept {
    return state_ && bin < state_->lengths.size() ? state_->lengths[bin] : 0;
}
template <Sample T> CpuBackend ConstantQAnalyzer<T>::backend() const noexcept {
    return state_ ? state_->ops.backend() : CpuBackend::scalar;
}
template class ConstantQAnalyzer<float>;
template class ConstantQAnalyzer<double>;
} // namespace cydsp
