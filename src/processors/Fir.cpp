#include "cydsp/processors/Fir.h"
#include "detail/CheckedSize.h"
#include "detail/VectorTable.h"
#include <algorithm>
#include <new>
#include <stdexcept>
namespace cydsp {
template <Sample T>
    requires(!std::is_const_v<T>)
void Fir<T>::swap(Fir& other) noexcept {
    std::swap(spec_, other.spec_);
    std::swap(ops_, other.ops_);
    coefficients_.swap(other.coefficients_);
    history_.swap(other.history_);
    std::swap(write_, other.write_);
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status Fir<T>::prepare(const ProcessSpec& spec, std::span<const T> coefficients,
                       CpuBackend backend) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    if (coefficients.empty() || coefficients.size() > 65536)
        return Status::invalid_argument;
    for (const auto value : coefficients)
        if (!isFinite(value))
            return Status::invalid_argument;
    std::size_t historySize{};
    if (!detail::checkedMultiply(2 * coefficients.size(), spec.channels, historySize))
        return Status::invalid_spec;
    Fir next;
    const auto status = VectorOps::create(backend, next.ops_);
    if (status != Status::ok)
        return status;
    try {
        next.coefficients_.assign(coefficients.rbegin(), coefficients.rend());
        next.history_.resize(historySize);
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_spec;
    }
    next.spec_ = spec;
    swap(next);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void Fir<T>::reset() noexcept {
    std::fill(history_.begin(), history_.end(), T{});
    write_ = 0;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status Fir<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    const auto count = taps();
    if (!count)
        return Status::not_prepared;
    const auto status = validateProcessBlocks(spec_, input, output);
    if (status != Status::ok)
        return status;
    for (std::size_t frame = 0; frame < input.frames(); ++frame) {
        const auto next = write_ + 1 == count ? 0 : write_ + 1;
        for (std::size_t ch = 0; ch < spec_.channels; ++ch) {
            auto* ring = history_.data() + ch * 2 * count;
            const auto sample = input.channel(ch)[frame];
            ring[write_] = sample;
            ring[write_ + count] = sample;
            T value{};
            value = detail::VectorAccess::kernels<T>(ops_).dot(ring + next, coefficients_.data(),
                                                               count);
            output.channel(ch)[frame] = value;
        }
        write_ = next;
    }
    return Status::ok;
}
template class Fir<float>;
template class Fir<double>;
} // namespace cydsp
