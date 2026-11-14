#include "cydsp/processors/RealFft.h"
#include "detail/Alias.h"
#include "detail/FftPlanAccess.h"
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
        next.natural_.resize(half);
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
    // The real input already is the packed even+i*odd sequence in natural order.
    const auto first = detail::FftPlanAccess::permuteHead<T>(
        fft_, input.data(), reinterpret_cast<T*>(scratch_.data()), scratch_.size(), false);
    if (first == 1) {
        const auto reverse = detail::FftPlanAccess::reverse(fft_);
        for (std::size_t i = 0; i < scratch_.size(); ++i)
            scratch_[reverse[i]] = {input[2 * i], input[2 * i + 1]};
    }
    detail::FftPlanAccess::transformPermutedUnscaled<T>(fft_, scratch_, false, first);
    const auto split = detail::FftPlanAccess::kernels(fft_).realSplit;
    if (split && scratch_.size() >= 4)
        split(reinterpret_cast<const T*>(scratch_.data()),
              reinterpret_cast<const T*>(twiddles_.data()), reinterpret_cast<T*>(output.data()),
              scratch_.size());
    else
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
    const auto& kernels = detail::FftPlanAccess::kernels(fft_);
    std::size_t first = 1;
    if (kernels.realMergeRange && scratch_.size() >= 16) {
        natural_[0] = detail::mergeRealBin<T>(input, twiddles_, 0);
        kernels.realMergeRange(reinterpret_cast<const T*>(input.data()),
                               reinterpret_cast<const T*>(twiddles_.data()),
                               reinterpret_cast<T*>(natural_.data() + 1), scratch_.size(), 1,
                               scratch_.size());
        first = detail::FftPlanAccess::permuteHead<T>(
            fft_, reinterpret_cast<const T*>(natural_.data()),
            reinterpret_cast<T*>(scratch_.data()), scratch_.size(), true);
        if (first == 1) {
            const auto reverse = detail::FftPlanAccess::reverse(fft_);
            for (std::size_t k = 0; k < scratch_.size(); ++k)
                scratch_[reverse[k]] = natural_[k];
        }
    } else {
        const auto reverse = detail::FftPlanAccess::reverse(fft_);
        for (std::size_t k = 0; k < scratch_.size(); ++k)
            scratch_[reverse[k]] = detail::mergeRealBin<T>(input, twiddles_, k);
    }
    detail::FftPlanAccess::transformPermutedUnscaled<T>(fft_, scratch_, true, first);
    // The packed result is the interleaved real output: one scaling pass.
    kernels.scale(reinterpret_cast<const T*>(scratch_.data()), output.data(), output.size(),
                  T{1} / static_cast<T>(scratch_.size()));
    return Status::ok;
}
template class RealFft<float>;
template class RealFft<double>;
} // namespace cydsp
