#include "cydsp/spectral/HarmonicPercussive.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/math/VectorOps.h"
#include "detail/Analysis.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
namespace cydsp {
template <Sample T> struct HarmonicPercussive<T>::State {
    ProcessSpec spec;
    HpssConfig config;
    Stft<T> stft;
    VectorOps ops;
    AudioBuffer<T> packed, processed;
    std::vector<std::complex<T>> history;
    std::vector<double> power, mask;
    std::size_t write = 0, bins = 0;
    static void callback(void* p, std::size_t ch, std::span<std::complex<T>> x) noexcept {
        auto& s = *static_cast<State*>(p);
        const auto c = s.spec.channels, l = s.config.temporalFrames, b = s.bins;
        if (ch < c) {
            auto* target = s.history.data() + (ch * l + s.write) * b;
            std::copy(x.begin(), x.end(), target);
            for (std::size_t k = 0; k < b; ++k) {
                auto& value = s.power[s.write * b + k];
                if (ch == 0)
                    value = 0;
                value += std::norm(std::complex<double>(x[k])) / double(c);
            }
            std::fill(x.begin(), x.end(), std::complex<T>{});
            return;
        }
        const auto center = (s.write + l - l / 2) % l;
        if (ch == c) {
            std::array<double, 63> scratch{};
            for (std::size_t k = 0; k < b; ++k) {
                for (std::size_t t = 0; t < l; ++t)
                    scratch[t] = s.power[t * b + k];
                std::nth_element(scratch.begin(), scratch.begin() + l / 2, scratch.begin() + l);
                const double h = scratch[l / 2];
                const auto f = s.config.frequencyBins;
                for (std::size_t j = 0; j < f; ++j) {
                    const auto bin = std::clamp<std::ptrdiff_t>(
                        std::ptrdiff_t(k) + std::ptrdiff_t(j) - std::ptrdiff_t(f / 2), 0,
                        std::ptrdiff_t(b - 1));
                    scratch[j] = s.power[center * b + std::size_t(bin)];
                }
                std::nth_element(scratch.begin(), scratch.begin() + f / 2, scratch.begin() + f);
                const double percussive = scratch[f / 2];
                s.mask[k] = h + percussive > 1e-100 ? h / (h + percussive) : .5;
            }
        }
        const bool harmonic = ch < 2 * c;
        const auto source = harmonic ? ch - c : ch - 2 * c;
        const auto* original = s.history.data() + (source * l + center) * b;
        for (std::size_t k = 0; k < b; ++k)
            x[k] = original[k] * T(harmonic ? s.mask[k] : 1 - s.mask[k]);
        if (ch + 1 == 3 * c)
            s.write = (s.write + 1) % l;
    }
};
template <Sample T> HarmonicPercussive<T>::HarmonicPercussive() = default;
template <Sample T> HarmonicPercussive<T>::~HarmonicPercussive() = default;
template <Sample T>
HarmonicPercussive<T>::HarmonicPercussive(HarmonicPercussive&&) noexcept = default;
template <Sample T>
HarmonicPercussive<T>& HarmonicPercussive<T>::operator=(HarmonicPercussive&&) noexcept = default;
template <Sample T>
Status HarmonicPercussive<T>::prepare(const ProcessSpec& spec, const HpssConfig& config,
                                      CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || spec.channels > 8 || config.stft.fftFrames > 8192 ||
        !config.temporalFrames || config.temporalFrames > 63 || config.temporalFrames % 2 == 0 ||
        !config.frequencyBins || config.frequencyBins > 63 || config.frequencyBins % 2 == 0)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->config = config;
        auto expanded = spec;
        expanded.channels *= 3;
        auto status = s->stft.prepare(expanded, config.stft, backend);
        if (status != Status::ok)
            return status;
        status = VectorOps::create(backend, s->ops);
        if (status != Status::ok)
            return status;
        for (auto* buffer : {&s->packed, &s->processed}) {
            status = buffer->resize(expanded.channels, spec.maxBlockFrames);
            if (status != Status::ok)
                return status;
        }
        s->bins = s->stft.bins();
        s->history.resize(spec.channels * config.temporalFrames * s->bins);
        s->power.resize(config.temporalFrames * s->bins);
        s->mask.resize(s->bins);
        s->stft.setProcessor(State::callback, s.get());
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status HarmonicPercussive<T>::process(AudioBlock<const T> in, AudioBlock<T> h,
                                      AudioBlock<T> p) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    for (auto out : {h, p}) {
        status = validateProcessBlocks(s.spec, in, out);
        if (status != Status::ok)
            return status;
        status = validateDisjointBlocks(in, out);
        if (status != Status::ok)
            return status;
    }
    status = validateDisjointBlocks(AudioBlock<const T>(h), p);
    if (status != Status::ok)
        return status;
    auto packed = s.packed.block(in.frames());
    auto processed = s.processed.block(in.frames());
    for (std::size_t ch = 0; ch < in.channels(); ++ch) {
        (void)s.ops.copy(in.channel(ch), packed.channel(ch));
        s.ops.fill(packed.channel(ch + in.channels()), T{});
        s.ops.fill(packed.channel(ch + 2 * in.channels()), T{});
    }
    status = s.stft.process(packed, processed);
    if (status != Status::ok)
        return status;
    for (std::size_t ch = 0; ch < in.channels(); ++ch) {
        (void)s.ops.copy(processed.channel(ch + in.channels()), h.channel(ch));
        (void)s.ops.copy(processed.channel(ch + 2 * in.channels()), p.channel(ch));
    }
    return Status::ok;
}
template <Sample T> void HarmonicPercussive<T>::reset() noexcept {
    if (state_) {
        auto& s = *state_;
        s.stft.reset();
        s.write = 0;
        std::fill(s.history.begin(), s.history.end(), std::complex<T>{});
        std::fill(s.power.begin(), s.power.end(), 0);
    }
}
template <Sample T> std::size_t HarmonicPercussive<T>::latencyFrames() const noexcept {
    return state_ ? state_->config.stft.fftFrames +
                        state_->config.temporalFrames / 2 * state_->config.stft.hopFrames
                  : 0;
}
template class HarmonicPercussive<float>;
template class HarmonicPercussive<double>;
} // namespace cydsp
