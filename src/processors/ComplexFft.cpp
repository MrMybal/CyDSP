#include "cydsp/processors/ComplexFft.h"
#include "detail/Alias.h"
#include "detail/FftAlgorithm.h"
#include "detail/FftPlanAccess.h"
#include "detail/VectorTable.h"
#include <cmath>
#include <new>
#include <numbers>
#include <stdexcept>
#include <utility>
namespace cydsp {
template <Sample T>
    requires(!std::is_const_v<T>)
Status ComplexFft<T>::prepare(std::size_t size, CpuBackend backend) noexcept {
    if (!isPowerOfTwo(size) || size > (std::size_t{1} << 24))
        return Status::invalid_argument;
    VectorOps ops;
    auto status = VectorOps::create(backend, ops);
    if (status != Status::ok)
        return status;
    try {
        std::vector<std::size_t> reverse(size);
        std::vector<std::complex<T>> twiddles(size - 1);
        detail::fillBitReverse(reverse);
        for (std::size_t half = 1; half < size; half *= 2)
            for (std::size_t j = 0; j < half; ++j) {
                const auto angle =
                    -std::numbers::pi * static_cast<double>(j) / static_cast<double>(half);
                twiddles[half - 1 + j] = {static_cast<T>(std::cos(angle)),
                                          static_cast<T>(std::sin(angle))};
            }
        reverse_.swap(reverse);
        twiddles_.swap(twiddles);
        ops_ = ops;
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status ComplexFft<T>::process(std::span<const std::complex<T>> input,
                              std::span<std::complex<T>> output,
                              FftDirection direction) const noexcept {
    const auto count = size();
    if (!count)
        return Status::not_prepared;
    if (input.size() != count || output.size() != count ||
        (direction != FftDirection::forward && direction != FftDirection::inverse))
        return Status::invalid_argument;
    if (input.data() != output.data() &&
        detail::overlaps(input.data(), input.size_bytes(), output.data(), output.size_bytes()))
        return Status::partial_overlap;
    const bool inverse = direction == FftDirection::inverse;
    std::size_t first = 1;
    if (input.data() == output.data()) {
        for (std::size_t i = 0; i < input.size(); ++i)
            if (reverse_[i] > i)
                std::swap(output[i], output[reverse_[i]]);
    } else {
        first = detail::FftPlanAccess::permuteHead<T>(
            *this, reinterpret_cast<const T*>(input.data()), reinterpret_cast<T*>(output.data()),
            count, inverse);
        if (first == 1)
            for (std::size_t i = 0; i < input.size(); ++i)
                output[reverse_[i]] = input[i];
    }
    detail::FftPlanAccess::transformPermutedUnscaled(*this, output, inverse, first);
    if (direction == FftDirection::inverse) {
        const auto scale = T{1} / static_cast<T>(input.size());
        for (auto& value : output)
            value *= scale;
    }
    return Status::ok;
}
template class ComplexFft<float>;
template class ComplexFft<double>;
} // namespace cydsp
