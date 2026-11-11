#include "cydsp/processors/BiquadCascade.h"
#include <new>
#include <stdexcept>
namespace cydsp {
template <Sample T>
    requires(!std::is_const_v<T>)
void BiquadCascade<T>::swap(BiquadCascade& other) noexcept {
    std::swap(spec_, other.spec_);
    sections_.swap(other.sections_);
    std::swap(ops_, other.ops_);
    std::swap(prepared_, other.prepared_);
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status BiquadCascade<T>::prepare(const ProcessSpec& spec,
                                 std::span<const BiquadCoefficients> coefficients) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    if (coefficients.size() > 64)
        return Status::invalid_argument;
    BiquadCascade next;
    auto status = VectorOps::create(CpuBackend::automatic, next.ops_);
    if (status != Status::ok)
        return status;
    try {
        next.sections_.resize(coefficients.size());
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
    for (std::size_t i = 0; i < coefficients.size(); ++i) {
        status = next.sections_[i].prepare(spec);
        if (status != Status::ok)
            return status;
        status = next.sections_[i].setCoefficients(coefficients[i]);
        if (status != Status::ok)
            return status;
    }
    next.spec_ = spec;
    next.prepared_ = true;
    swap(next);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status BiquadCascade<T>::setSection(std::size_t index,
                                    const BiquadCoefficients& coefficients) noexcept {
    if (!prepared_)
        return Status::not_prepared;
    if (index >= sections_.size())
        return Status::invalid_argument;
    return sections_[index].setCoefficients(coefficients);
}
template <Sample T>
    requires(!std::is_const_v<T>)
void BiquadCascade<T>::reset() noexcept {
    for (auto& section : sections_)
        section.reset();
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status BiquadCascade<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    if (!prepared_)
        return Status::not_prepared;
    const auto status = validateProcessBlocks(spec_, input, output);
    if (status != Status::ok)
        return status;
    if (sections_.empty()) {
        for (std::size_t ch = 0; ch < spec_.channels; ++ch)
            (void)ops_.copy(input.channel(ch), output.channel(ch));
    } else {
        (void)sections_.front().process(input, output);
        for (std::size_t i = 1; i < sections_.size(); ++i)
            (void)sections_[i].process(output, output);
    }
    return Status::ok;
}
template class BiquadCascade<float>;
template class BiquadCascade<double>;
} // namespace cydsp
