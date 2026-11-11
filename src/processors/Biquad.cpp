#include "cydsp/processors/Biquad.h"
#include "detail/BiquadAlgorithm.h"
#include <algorithm>
#include <new>
#include <stdexcept>
namespace cydsp {
template <Sample T>
    requires(!std::is_const_v<T>)
Status Biquad<T>::prepare(const ProcessSpec& spec) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    try {
        std::vector<State> next(spec.channels);
        states_.swap(next);
        spec_ = spec;
        prepared_ = true;
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_spec;
    }
}
template <Sample T>
    requires(!std::is_const_v<T>)
void Biquad<T>::reset() noexcept {
    std::fill(states_.begin(), states_.end(), State{});
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status Biquad<T>::setCoefficients(const BiquadCoefficients& value) noexcept {
    BiquadCoefficients rounded;
    if (!detail::roundBiquad<T>(value, rounded))
        return Status::invalid_argument;
    coefficients_ = rounded;
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status Biquad<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    if (!prepared_)
        return Status::not_prepared;
    const auto status = validateProcessBlocks(spec_, input, output);
    if (status != Status::ok)
        return status;
    const T b0 = static_cast<T>(coefficients_.b0), b1 = static_cast<T>(coefficients_.b1),
            b2 = static_cast<T>(coefficients_.b2), a1 = static_cast<T>(coefficients_.a1),
            a2 = static_cast<T>(coefficients_.a2);
    for (std::size_t ch = 0; ch < spec_.channels; ++ch) {
        auto state = states_[ch];
        auto source = input.channel(ch);
        auto destination = output.channel(ch);
        for (std::size_t i = 0; i < source.size(); ++i) {
            const auto sample =
                detail::biquadSample(source[i], state.z1, state.z2, b0, b1, b2, a1, a2);
            state.z1 = sample.z1;
            state.z2 = sample.z2;
            destination[i] = sample.output;
        }
        states_[ch] = state;
    }
    return Status::ok;
}
template class Biquad<float>;
template class Biquad<double>;
} // namespace cydsp
