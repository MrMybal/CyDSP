#include "cydsp/processors/RealFft.h"
#include "detail/Alias.h"
#include "detail/RealFftAlgorithm.h"
#include <algorithm>
#include <cmath>
#include <new>
#include <numbers>
#include <stdexcept>
namespace cydsp {
template <Sample T>
    requires(!std::is_const_v<T>)
Status RealFft<T>::prepare(std::size_t count, CpuBackend backend) noexcept {
    if (!isPowerOfTwo(count) || count > (std::size_t{1} << 24))
        return Status::invalid_argument;
    const auto half = std::max(std::size_t{1}, count / 2);
    RealFft next;
    const auto status = next.fft_.prepare(half, backend);
    if (status != Status::ok)
        return status;
    try {
        next.scratch_.resize(half);
        next.twiddles_.resize(half - 1);
        for (std::size_t k = 1; k < half; ++k) {
            const auto angle =
                -std::numbers::pi * static_cast<double>(k) / static_cast<double>(half);
            next.twiddles_[k - 1] = {static_cast<T>(std::cos(angle)),
                                     static_cast<T>(std::sin(angle))};
        }
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
    next.size_ = count;
    swap(next);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status RealFft<T>::forward(std::span<const T> input, std::span<std::complex<T>> output) noexcept {
    if (!size())
        return Status::not_prepared;
    if (input.size() != size() || output.size() != bins())
        return Status::invalid_argument;
    if (detail::overlaps(input.data(), input.size_bytes(), output.data(), output.size_bytes()))
        return Status::partial_overlap;
    if (size() == 1) {
        output[0] = {input[0], T{0}};
        return Status::ok;
    }
    detail::packReal<T>(input, scratch_);
    (void)fft_.process(scratch_, scratch_, FftDirection::forward);
    detail::splitRealSpectrum<T>(scratch_, twiddles_, output);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status RealFft<T>::inverse(std::span<const std::complex<T>> input, std::span<T> output) noexcept {
    if (!size())
        return Status::not_prepared;
    if (input.size() != bins() || output.size() != size())
        return Status::invalid_argument;
    if (input.front().imag() != T{0} || (size() > 1 && input.back().imag() != T{0}))
        return Status::invalid_argument;
    if (detail::overlaps(input.data(), input.size_bytes(), output.data(), output.size_bytes()))
        return Status::partial_overlap;
    if (size() == 1) {
        output[0] = input[0].real();
        return Status::ok;
    }
    detail::mergeRealSpectrum<T>(input, twiddles_, scratch_);
    (void)fft_.process(scratch_, scratch_, FftDirection::inverse);
    detail::scatterReal<T>(scratch_, output);
    return Status::ok;
}
template class RealFft<float>;
template class RealFft<double>;
} // namespace cydsp
