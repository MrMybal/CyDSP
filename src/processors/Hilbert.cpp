#include "cydsp/processors/Hilbert.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/math/VectorOps.h"
#include "detail/Analysis.h"
#include "detail/VectorTable.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>
namespace cydsp {
template <Sample T> struct HilbertTransform<T>::State {
    ProcessSpec spec;
    std::size_t taps = 0, head = 0;
    VectorOps ops;
    AudioBuffer<T> history;
    std::vector<T> coefficients;
};
template <Sample T> HilbertTransform<T>::HilbertTransform() = default;
template <Sample T> HilbertTransform<T>::~HilbertTransform() = default;
template <Sample T> HilbertTransform<T>::HilbertTransform(HilbertTransform&&) noexcept = default;
template <Sample T>
HilbertTransform<T>& HilbertTransform<T>::operator=(HilbertTransform&&) noexcept = default;
template <Sample T>
Status HilbertTransform<T>::prepare(const ProcessSpec& spec, std::size_t taps,
                                    CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || taps < 7 || taps > 4097 || taps % 2 == 0)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->taps = taps;
        auto result = VectorOps::create(backend, s->ops);
        if (result != Status::ok)
            return result;
        result = s->history.resize(spec.channels, 2 * taps);
        if (result != Status::ok)
            return result;
        s->coefficients.resize(taps);
        for (std::size_t i = 0; i < taps; ++i) {
            const auto n = std::ptrdiff_t(i) - std::ptrdiff_t(taps / 2);
            const double window =
                .42 - .5 * std::cos(2 * std::numbers::pi * double(i) / double(taps - 1)) +
                .08 * std::cos(4 * std::numbers::pi * double(i) / double(taps - 1));
            s->coefficients[taps - 1 - i] =
                n % 2 ? T(2 * window / (std::numbers::pi * double(n))) : T{};
        }
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status HilbertTransform<T>::process(AudioBlock<const T> in, AudioBlock<T> real,
                                    AudioBlock<T> imaginary) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto result = detail::validateAnalysis(s.spec, in);
    if (result != Status::ok)
        return result;
    for (auto output : {real, imaginary}) {
        if (!output.isValid() || output.frames() != in.frames())
            return Status::invalid_argument;
        if (output.channels() != in.channels())
            return Status::channel_mismatch;
        result = validateDisjointBlocks(in, output);
        if (result != Status::ok)
            return result;
    }
    result = validateDisjointBlocks(AudioBlock<const T>(real), imaginary);
    if (result != Status::ok)
        return result;
    const auto& kernels = detail::VectorAccess::kernels<T>(s.ops);
    for (std::size_t i = 0; i < in.frames(); ++i) {
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            auto h = s.history.block().channel(ch);
            h[s.head] = h[s.head + s.taps] = in.channel(ch)[i];
            const auto* oldest = h.data() + (s.head + 1) % s.taps;
            real.channel(ch)[i] = oldest[s.taps / 2];
            imaginary.channel(ch)[i] = kernels.dot(oldest, s.coefficients.data(), s.taps);
        }
        if (++s.head == s.taps)
            s.head = 0;
    }
    return Status::ok;
}
template <Sample T> void HilbertTransform<T>::reset() noexcept {
    if (state_) {
        for (std::size_t ch = 0; ch < state_->spec.channels; ++ch)
            state_->ops.fill(state_->history.block().channel(ch), T{});
        state_->head = 0;
    }
}
template <Sample T> std::size_t HilbertTransform<T>::latencyFrames() const noexcept {
    return state_ ? state_->taps / 2 : 0;
}
template class HilbertTransform<float>;
template class HilbertTransform<double>;
} // namespace cydsp
