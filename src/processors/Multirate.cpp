#include "cydsp/processors/Multirate.h"
#include "cydsp/math/VectorOps.h"
#include "cydsp/processors/Diffusion.h"
#include "cydsp/processors/Fir.h"
#include "cydsp/processors/TptFilter.h"
#include "detail/Analysis.h"
#include "detail/VectorTable.h"
#include <algorithm>
#include <array>
#include <bit>
#include <vector>
namespace cydsp {
template <Sample T> struct FirRateConverter<T>::State {
    ProcessSpec spec;
    unsigned factor = 2;
    RateConversion mode;
    std::size_t taps = 0, head = 0, phase = 0;
    std::vector<T> history, coefficients;
    VectorOps ops;
};
template <Sample T> FirRateConverter<T>::FirRateConverter() = default;
template <Sample T> FirRateConverter<T>::~FirRateConverter() = default;
template <Sample T> FirRateConverter<T>::FirRateConverter(FirRateConverter&&) noexcept = default;
template <Sample T>
FirRateConverter<T>& FirRateConverter<T>::operator=(FirRateConverter&&) noexcept = default;
template <Sample T>
Status FirRateConverter<T>::prepare(const ProcessSpec& spec, unsigned factor, RateConversion mode,
                                    std::size_t tpp, CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || factor < 2 || factor > 32 || !isPowerOfTwo(factor) ||
        unsigned(mode) > unsigned(RateConversion::interpolate) || tpp < 8 || tpp > 128)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->factor = factor;
        s->mode = mode;
        auto status = VectorOps::create(backend, s->ops);
        if (status != Status::ok)
            return status;
        std::vector<double> prototype(factor * tpp + 1);
        status = designWindowedFir(prototype, .45 / factor, false, 100);
        if (status != Status::ok)
            return status;
        s->taps = mode == RateConversion::decimate ? prototype.size()
                                                   : (prototype.size() + factor - 1) / factor;
        s->history.resize(s->taps * 2 * spec.channels);
        s->coefficients.resize(s->taps * (mode == RateConversion::decimate ? 1 : factor));
        for (std::size_t phase = 0; phase < (mode == RateConversion::decimate ? 1u : factor);
             ++phase)
            for (std::size_t k = 0; k < s->taps; ++k) {
                const auto index = mode == RateConversion::decimate
                                       ? s->taps - 1 - k
                                       : (s->taps - 1 - k) * factor + phase;
                s->coefficients[phase * s->taps + k] =
                    index < prototype.size()
                        ? T(prototype[index] * (mode == RateConversion::interpolate ? factor : 1))
                        : T{};
            }
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> std::size_t FirRateConverter<T>::outputFrames(std::size_t n) const noexcept {
    if (!state_ || n > state_->spec.maxBlockFrames)
        return 0;
    return state_->mode == RateConversion::decimate ? (state_->phase + n) / state_->factor
                                                    : n * state_->factor;
}
template <Sample T>
Status FirRateConverter<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    if (!out.isValid() || out.frames() != outputFrames(in.frames()))
        return Status::invalid_argument;
    if (out.channels() != in.channels())
        return Status::channel_mismatch;
    status = validateDisjointBlocks(in, out);
    if (status != Status::ok)
        return status;
    const auto& kernels = detail::VectorAccess::kernels<T>(s.ops);
    std::size_t produced = 0;
    for (std::size_t i = 0; i < in.frames(); ++i) {
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            auto* h = s.history.data() + ch * 2 * s.taps;
            h[s.head] = h[s.head + s.taps] = in.channel(ch)[i];
        }
        const bool emit = s.mode == RateConversion::interpolate || ++s.phase == s.factor;
        if (emit) {
            if (s.mode == RateConversion::decimate)
                s.phase = 0;
            const auto count = s.mode == RateConversion::interpolate ? s.factor : 1u;
            for (unsigned p = 0; p < count; ++p) {
                for (std::size_t ch = 0; ch < in.channels(); ++ch) {
                    const auto* x = s.history.data() + ch * 2 * s.taps + (s.head + 1) % s.taps;
                    out.channel(ch)[produced] =
                        kernels.dot(x, s.coefficients.data() + p * s.taps, s.taps);
                }
                ++produced;
            }
        }
        if (++s.head == s.taps)
            s.head = 0;
    }
    return Status::ok;
}
template <Sample T> void FirRateConverter<T>::reset() noexcept {
    if (state_) {
        std::fill(state_->history.begin(), state_->history.end(), T{});
        state_->head = state_->phase = 0;
    }
}
template <Sample T> double FirRateConverter<T>::outputSampleRate() const noexcept {
    return state_ ? (state_->mode == RateConversion::decimate
                         ? state_->spec.sampleRate / state_->factor
                         : state_->spec.sampleRate * state_->factor)
                  : 0;
}
namespace {
std::size_t packetIndex(std::size_t k, std::size_t n) noexcept {
    std::size_t gray = k ^ (k >> 1), reversed = 0;
    for (std::size_t m = n; m > 1; m >>= 1) {
        reversed = (reversed << 1) | (gray & 1);
        gray >>= 1;
    }
    return reversed;
}
} // namespace

template <Sample T>
Status DyadicFilterBank<T>::prepare(const ProcessSpec& spec, std::size_t bands) noexcept {
    if (!detail::validAnalysisSpec(spec) || bands < 2 || bands > 32 || !isPowerOfTwo(bands) ||
        bands * spec.channels > 256)
        return Status::invalid_spec;
    try {
        std::vector<T> packet(bands * spec.channels);
        packet_.swap(packet);
        spec_ = spec;
        bands_ = bands;
        pending_ = 0;
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> std::size_t DyadicFilterBank<T>::analysisFrames(std::size_t n) const noexcept {
    return bands_ && n <= spec_.maxBlockFrames ? (pending_ + n) / bands_ : 0;
}
template <Sample T>
Status DyadicFilterBank<T>::analyze(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!bands_)
        return Status::not_prepared;
    auto status = detail::validateAnalysis(spec_, in);
    if (status != Status::ok)
        return status;
    if (!out.isValid() || out.frames() != analysisFrames(in.frames()))
        return Status::invalid_argument;
    if (out.channels() != bands_ * spec_.channels)
        return Status::channel_mismatch;
    status = validateDisjointBlocks(in, out);
    if (status != Status::ok)
        return status;
    std::size_t emitted = 0;
    std::array<T, 32> transformed{};
    for (std::size_t i = 0; i < in.frames(); ++i) {
        for (std::size_t ch = 0; ch < in.channels(); ++ch)
            packet_[ch * bands_ + pending_] = in.channel(ch)[i];
        if (++pending_ == bands_) {
            for (std::size_t ch = 0; ch < in.channels(); ++ch) {
                (void)orthogonalMix<T>({packet_.data() + ch * bands_, bands_},
                                       {transformed.data(), bands_}, OrthogonalMix::hadamard);
                for (std::size_t b = 0; b < bands_; ++b)
                    out.channel(b * in.channels() + ch)[emitted] =
                        transformed[packetIndex(b, bands_)];
            }
            pending_ = 0;
            ++emitted;
        }
    }
    return Status::ok;
}
template <Sample T>
Status DyadicFilterBank<T>::synthesize(AudioBlock<const T> in, AudioBlock<T> out) const noexcept {
    if (!bands_)
        return Status::not_prepared;
    auto spec = spec_;
    spec.channels = bands_ * spec.channels;
    spec.maxBlockFrames = (spec_.maxBlockFrames + bands_ - 1) / bands_;
    auto status = detail::validateAnalysis(spec, in);
    if (status != Status::ok)
        return status;
    if (!out.isValid() || out.frames() != in.frames() * bands_)
        return Status::invalid_argument;
    if (out.channels() != spec_.channels)
        return Status::channel_mismatch;
    status = validateDisjointBlocks(in, out);
    if (status != Status::ok)
        return status;
    std::array<T, 32> a{}, b{};
    for (std::size_t frame = 0; frame < in.frames(); ++frame)
        for (std::size_t ch = 0; ch < out.channels(); ++ch) {
            for (std::size_t k = 0; k < bands_; ++k)
                a[packetIndex(k, bands_)] = in.channel(k * out.channels() + ch)[frame];
            (void)orthogonalMix<T>({a.data(), bands_}, {b.data(), bands_}, OrthogonalMix::hadamard);
            for (std::size_t k = 0; k < bands_; ++k)
                out.channel(ch)[frame * bands_ + k] = b[k];
        }
    return Status::ok;
}
template <Sample T> void DyadicFilterBank<T>::reset() noexcept {
    std::fill(packet_.begin(), packet_.end(), T{});
    pending_ = 0;
}
template class FirRateConverter<float>;
template class FirRateConverter<double>;
template class DyadicFilterBank<float>;
template class DyadicFilterBank<double>;
} // namespace cydsp
