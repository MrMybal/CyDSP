#include "cydsp/processors/FilterBank.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/math/VectorOps.h"
#include "cydsp/processors/TptFilter.h"
#include "detail/Analysis.h"
#include "detail/VectorTable.h"
#include <algorithm>
#include <vector>
namespace cydsp {
template <Sample T> struct FirFilterBank<T>::State {
    ProcessSpec spec;
    std::size_t taps = 0, count = 0, head = 0;
    VectorOps ops;
    AudioBuffer<T> history;
    std::vector<T> coefficients;
};
template <Sample T> FirFilterBank<T>::FirFilterBank() = default;
template <Sample T> FirFilterBank<T>::~FirFilterBank() = default;
template <Sample T> FirFilterBank<T>::FirFilterBank(FirFilterBank&&) noexcept = default;
template <Sample T>
FirFilterBank<T>& FirFilterBank<T>::operator=(FirFilterBank&&) noexcept = default;
template <Sample T>
Status FirFilterBank<T>::prepare(const ProcessSpec& spec, std::span<const double> frequencies,
                                 std::size_t taps, CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || frequencies.empty() || frequencies.size() > 31 ||
        spec.channels * (frequencies.size() + 1) > 256 || taps < 3 || taps > 8193 || taps % 2 == 0)
        return Status::invalid_spec;
    double previous = 0;
    for (double f : frequencies) {
        if (!isFinite(f) || f <= previous || f >= spec.sampleRate * .5)
            return Status::invalid_argument;
        previous = f;
    }
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->taps = taps;
        s->count = frequencies.size() + 1;
        auto result = VectorOps::create(backend, s->ops);
        if (result != Status::ok)
            return result;
        result = s->history.resize(spec.channels, taps * 2);
        if (result != Status::ok)
            return result;
        s->coefficients.resize(taps * frequencies.size());
        std::vector<double> temporary(taps);
        for (std::size_t k = 0; k < frequencies.size(); ++k) {
            result = designWindowedFir(temporary, frequencies[k] / spec.sampleRate);
            if (result != Status::ok)
                return result;
            for (std::size_t j = 0; j < taps; ++j)
                s->coefficients[k * taps + j] = T(temporary[taps - 1 - j]);
        }
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status FirFilterBank<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto result = detail::validateAnalysis(s.spec, in);
    if (result != Status::ok)
        return result;
    if (!out.isValid() || out.frames() != in.frames())
        return Status::invalid_argument;
    if (out.channels() != s.count * s.spec.channels)
        return Status::channel_mismatch;
    result = validateDisjointBlocks(in, out);
    if (result != Status::ok)
        return result;
    const auto& kernels = detail::VectorAccess::kernels<T>(s.ops);
    for (std::size_t i = 0; i < in.frames(); ++i) {
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            auto h = s.history.block().channel(ch);
            h[s.head] = h[s.head + s.taps] = in.channel(ch)[i];
            const T* first = h.data() + (s.head + 1) % s.taps;
            T previous{};
            for (std::size_t band = 0; band + 1 < s.count; ++band) {
                const T value = kernels.dot(first, s.coefficients.data() + band * s.taps, s.taps);
                out.channel(band * s.spec.channels + ch)[i] = value - previous;
                previous = value;
            }
            out.channel((s.count - 1) * s.spec.channels + ch)[i] = first[s.taps / 2] - previous;
        }
        if (++s.head == s.taps)
            s.head = 0;
    }
    return Status::ok;
}
template <Sample T>
Status FirFilterBank<T>::sumBands(AudioBlock<const T> in, AudioBlock<T> out) const noexcept {
    if (!state_)
        return Status::not_prepared;
    const auto& s = *state_;
    if (!in.isValid() || !out.isValid() || in.frames() != out.frames())
        return Status::invalid_argument;
    if (in.channels() != s.count * s.spec.channels || out.channels() != s.spec.channels)
        return Status::channel_mismatch;
    if (in.frames() > s.spec.maxBlockFrames)
        return Status::block_too_large;
    auto result = validateDisjointBlocks(in, out);
    if (result != Status::ok)
        return result;
    for (std::size_t ch = 0; ch < out.channels(); ++ch) {
        s.ops.fill(out.channel(ch), T{});
        for (std::size_t band = 0; band < s.count; ++band)
            (void)s.ops.addScaled(in.channel(band * s.spec.channels + ch), out.channel(ch), T{1});
    }
    return Status::ok;
}
template <Sample T> void FirFilterBank<T>::reset() noexcept {
    if (state_) {
        auto& s = *state_;
        for (std::size_t ch = 0; ch < s.spec.channels; ++ch)
            s.ops.fill(s.history.block().channel(ch), T{});
        s.head = 0;
    }
}
template <Sample T> std::size_t FirFilterBank<T>::bands() const noexcept {
    return state_ ? state_->count : 0;
}
template <Sample T> std::size_t FirFilterBank<T>::latencyFrames() const noexcept {
    return state_ ? (state_->taps - 1) / 2 : 0;
}
template <Sample T> CpuBackend FirFilterBank<T>::backend() const noexcept {
    return state_ ? state_->ops.backend() : CpuBackend::scalar;
}
template class FirFilterBank<float>;
template class FirFilterBank<double>;
} // namespace cydsp
