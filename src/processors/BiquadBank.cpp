#include "cydsp/processors/BiquadBank.h"
#include "detail/AudioLayout.h"
#include "detail/BiquadAlgorithm.h"
#include "detail/CheckedSize.h"
#include "detail/VectorTable.h"
#include <algorithm>
#include <new>
#include <stdexcept>
namespace cydsp {
template <Sample T>
    requires(!std::is_const_v<T>)
void BiquadBank<T>::swap(BiquadBank& other) noexcept {
    std::swap(spec_, other.spec_);
    std::swap(ops_, other.ops_);
    coefficients_.swap(other.coefficients_);
    states_.swap(other.states_);
    workspace_.swap(other.workspace_);
    std::swap(tileFrames_, other.tileFrames_);
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status BiquadBank<T>::prepare(const ProcessSpec& spec, CpuBackend backend) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    const auto tile = std::min(spec.maxBlockFrames, std::size_t{64});
    std::size_t coefficients{}, states{}, samples{};
    if (!detail::checkedMultiply(spec.channels, 5, coefficients) ||
        !detail::checkedMultiply(spec.channels, 2, states) ||
        !detail::checkedMultiply(spec.channels, tile, samples))
        return Status::invalid_spec;
    BiquadBank next;
    const auto status = VectorOps::create(backend, next.ops_);
    if (status != Status::ok)
        return status;
    try {
        next.coefficients_.resize(coefficients);
        next.states_.resize(states);
        next.workspace_.resize(samples);
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_spec;
    }
    std::fill_n(next.coefficients_.begin(), spec.channels, T{1});
    next.spec_ = spec;
    next.tileFrames_ = tile;
    swap(next);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status BiquadBank<T>::setCoefficients(std::size_t voice, const BiquadCoefficients& value) noexcept {
    if (!voices())
        return Status::not_prepared;
    if (voice >= voices())
        return Status::invalid_argument;
    BiquadCoefficients rounded;
    if (!detail::roundBiquad<T>(value, rounded))
        return Status::invalid_argument;
    const T values[]{static_cast<T>(rounded.b0), static_cast<T>(rounded.b1),
                     static_cast<T>(rounded.b2), static_cast<T>(rounded.a1),
                     static_cast<T>(rounded.a2)};
    for (std::size_t field = 0; field < 5; ++field)
        coefficients_[field * voices() + voice] = values[field];
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status BiquadBank<T>::setAllCoefficients(const BiquadCoefficients& value) noexcept {
    if (!voices())
        return Status::not_prepared;
    BiquadCoefficients rounded;
    if (!detail::roundBiquad<T>(value, rounded))
        return Status::invalid_argument;
    const T values[]{static_cast<T>(rounded.b0), static_cast<T>(rounded.b1),
                     static_cast<T>(rounded.b2), static_cast<T>(rounded.a1),
                     static_cast<T>(rounded.a2)};
    for (std::size_t field = 0; field < 5; ++field)
        std::fill_n(coefficients_.data() + field * voices(), voices(), values[field]);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void BiquadBank<T>::reset() noexcept {
    std::fill(states_.begin(), states_.end(), T{});
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status BiquadBank<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    if (!voices())
        return Status::not_prepared;
    const auto status = validateProcessBlocks(spec_, input, output);
    if (status != Status::ok)
        return status;
    const auto& table = detail::VectorAccess::table(ops_);
    for (std::size_t offset = 0; offset < input.frames();) {
        const auto count = std::min(tileFrames_, input.frames() - offset);
        auto packed = std::span{workspace_}.first(count * voices());
        detail::interleaveUnchecked<T>(input.subBlock(offset, count), packed);
        if constexpr (std::same_as<T, float>)
            table.f32.biquadBank(packed.data(), voices(), count, coefficients_.data(),
                                 states_.data());
        else
            table.f64.biquadBank(packed.data(), voices(), count, coefficients_.data(),
                                 states_.data());
        detail::deinterleaveUnchecked<T>(packed, output.subBlock(offset, count));
        offset += count;
    }
    return Status::ok;
}
template class BiquadBank<float>;
template class BiquadBank<double>;
} // namespace cydsp
