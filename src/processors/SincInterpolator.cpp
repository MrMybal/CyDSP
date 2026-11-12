#include "cydsp/processors/SincInterpolator.h"
#include "cydsp/math/VectorOps.h"
#include "detail/Alias.h"
#include "detail/WindowedSinc.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>
namespace cydsp {
template <Sample T> struct SincInterpolator<T>::State {
    ProcessSpec spec;
    double ratio = 1;
    std::size_t taps = 0, phases = 0;
    VectorOps ops;
    std::vector<double> table, weights;
};
template <Sample T> SincInterpolator<T>::SincInterpolator() = default;
template <Sample T> SincInterpolator<T>::~SincInterpolator() = default;
template <Sample T> SincInterpolator<T>::SincInterpolator(SincInterpolator&&) noexcept = default;
template <Sample T>
SincInterpolator<T>& SincInterpolator<T>::operator=(SincInterpolator&&) noexcept = default;
template <Sample T>
Status SincInterpolator<T>::prepare(const ProcessSpec& spec, double inputRate, std::size_t taps,
                                    std::size_t phases, CpuBackend backend) noexcept {
    if (!spec.isValid() || !isFinite(inputRate) || inputRate <= 0 || inputRate > 768000 ||
        spec.sampleRate > 768000 || taps < 8 || taps > 512 || taps % 2 || phases < 2 ||
        phases > 65536)
        return Status::invalid_spec;
    try {
        auto next = std::make_unique<State>();
        next->spec = spec;
        next->ratio = inputRate / spec.sampleRate;
        next->taps = taps;
        next->phases = phases;
        auto status = VectorOps::create(backend, next->ops);
        if (status != Status::ok)
            return status;
        next->table.resize((phases + 1) * taps);
        next->weights.resize(taps);
        const auto cutoff = .94 * std::min(1.0, 1 / next->ratio),
                   centre = static_cast<double>(taps / 2 - 1);
        for (std::size_t p = 0; p <= phases; ++p) {
            double sum = 0;
            for (std::size_t i = 0; i < taps; ++i) {
                const auto h =
                    detail::windowedSinc(centre - static_cast<double>(i) +
                                             static_cast<double>(p) / static_cast<double>(phases),
                                         cutoff, static_cast<double>(taps) * .5);
                next->table[p * taps + i] = h;
                sum += h;
            }
            if (!isFinite(sum) || std::abs(sum) < 1e-30)
                return Status::invalid_spec;
            for (std::size_t i = 0; i < taps; ++i)
                next->table[p * taps + i] /= sum;
        }
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_spec;
    }
}
template <Sample T>
Status SincInterpolator<T>::process(AudioBlock<const T> source, double first,
                                    AudioBlock<T> output) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks<T>(s.spec, output, output);
    if (status != Status::ok)
        return status;
    if (!source.isValid() || source.channels() != s.spec.channels || !isFinite(first) || first < 0)
        return Status::invalid_argument;
    if (!output.frames())
        return Status::ok;
    const auto final = first + static_cast<double>(output.frames() - 1) * s.ratio;
    if (!isFinite(final) || first < static_cast<double>(s.taps / 2 - 1) ||
        final >= static_cast<double>(source.frames()) - static_cast<double>(s.taps / 2))
        return Status::invalid_argument;
    for (std::size_t ch = 0; ch < output.channels(); ++ch)
        for (std::size_t in = 0; in < source.channels(); ++in)
            if (detail::overlaps(output.channel(ch).data(), output.frames() * sizeof(T),
                                 source.channel(in).data(), source.frames() * sizeof(T)))
                return Status::partial_overlap;
    for (std::size_t f = 0; f < output.frames(); ++f) {
        const auto position = first + static_cast<double>(f) * s.ratio, base = std::floor(position),
                   phase = (position - base) * static_cast<double>(s.phases);
        const auto p = std::min(static_cast<std::size_t>(phase), s.phases - 1);
        const auto mix = phase - static_cast<double>(p);
        for (std::size_t i = 0; i < s.taps; ++i)
            s.weights[i] = s.table[p * s.taps + i] +
                           (s.table[(p + 1) * s.taps + i] - s.table[p * s.taps + i]) * mix;
        const auto begin = static_cast<std::size_t>(base) - (s.taps / 2 - 1);
        for (std::size_t ch = 0; ch < output.channels(); ++ch) {
            double value = 0;
            (void)s.ops.dot(source.channel(ch).subspan(begin, s.taps), s.weights, value);
            output.channel(ch)[f] = static_cast<T>(value);
        }
    }
    return Status::ok;
}
template <Sample T>
Status SincInterpolator<T>::processPositions(AudioBlock<const T> source,
                                             std::span<const double> positions,
                                             AudioBlock<T> output) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    const auto checked = validateProcessBlocks<T>(s.spec, output, output);
    if (checked != Status::ok)
        return checked;
    if (!source.isValid() || source.channels() != s.spec.channels ||
        positions.size() != output.frames())
        return Status::invalid_argument;
    const auto left = double(s.taps / 2 - 1), right = double(s.taps / 2);
    for (std::size_t i = 0; i < positions.size(); ++i) {
        if (!isFinite(positions[i]) || positions[i] < left ||
            positions[i] >= double(source.frames()) - right ||
            (i && (positions[i] < positions[i - 1] ||
                   positions[i] - positions[i - 1] > s.ratio * (1 + 1e-10))))
            return Status::invalid_argument;
    }
    for (std::size_t ch = 0; ch < output.channels(); ++ch)
        for (std::size_t in = 0; in < source.channels(); ++in)
            if (detail::overlaps(output.channel(ch).data(), output.frames() * sizeof(T),
                                 source.channel(in).data(), source.frames() * sizeof(T)))
                return Status::partial_overlap;
    for (std::size_t f = 0; f < output.frames(); ++f) {
        const auto base = std::floor(positions[f]),
                   phase = (positions[f] - base) * double(s.phases);
        const auto p = std::min(std::size_t(phase), s.phases - 1);
        const auto mix = phase - double(p);
        for (std::size_t i = 0; i < s.taps; ++i)
            s.weights[i] = s.table[p * s.taps + i] +
                           (s.table[(p + 1) * s.taps + i] - s.table[p * s.taps + i]) * mix;
        const auto begin = std::size_t(base) - (s.taps / 2 - 1);
        for (std::size_t ch = 0; ch < output.channels(); ++ch) {
            double value = 0;
            (void)s.ops.dot(source.channel(ch).subspan(begin, s.taps), s.weights, value);
            output.channel(ch)[f] = T(value);
        }
    }
    return Status::ok;
}
template <Sample T> CpuBackend SincInterpolator<T>::backend() const noexcept {
    return state_ ? state_->ops.backend() : CpuBackend::scalar;
}
template class SincInterpolator<float>;
template class SincInterpolator<double>;
} // namespace cydsp
