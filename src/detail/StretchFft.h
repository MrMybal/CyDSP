#pragma once
// Half-bin packed real transform equations adapted from Signalsmith Linear.
// Copyright (c) 2025 Signalsmith Audio
// Complete MIT notice: third_party/licenses/signalsmith-linear.txt
#include "cydsp/processors/ComplexFft.h"
#include "detail/FftPlanAccess.h"
#include <algorithm>
#include <bit>
#include <numbers>
namespace cydsp::detail {
struct StretchPreparationError {
    Status status;
};
template <Sample T> class StretchFft {
    using Complex = std::complex<T>;

  public:
    void select(CpuBackend backend) {
        requested_ = backend;
    }
    static std::size_t fastSizeAbove(std::size_t n) {
        n = std::max(n, std::size_t{4});
        const auto power = std::bit_ceil(n);
        const auto three = 3 * (power / 4);
        return three >= n ? three : power;
    }
    void resize(std::size_t n) {
        const auto half = n / 2;
        const bool radixThree = !std::has_single_bit(half);
        const auto part = radixThree ? half / 3 : half;
        if (n < 4 || n % 4 || (radixThree && half % 3) || !std::has_single_bit(part))
            throw StretchPreparationError{Status::invalid_argument};
        const auto status = plan_.prepare(part, requested_);
        if (status != Status::ok)
            throw StretchPreparationError{status};
        size_ = n;
        radixThree_ = radixThree;
        permutation_.resize(half);
        const auto reverse = FftPlanAccess::reverse(plan_);
        for (std::size_t i = 0; i < half; ++i)
            permutation_[i] = radixThree ? (i % 3) * part + reverse[i / 3] : reverse[i];
        radixWeights_.resize(radixThree ? part * 2 : 0);
        for (std::size_t i = 0; i < radixWeights_.size(); ++i)
            radixWeights_[i] =
                std::polar(T{1}, static_cast<T>(-2 * std::numbers::pi * static_cast<double>(i) /
                                                static_cast<double>(half)));
        work_.resize(half);
        transformed_.resize(radixThree ? half : 0);
        twist_.resize(n / 2);
        weights_.resize(n / 4);
        for (std::size_t i = 0; i < n / 2; ++i)
            twist_[i] =
                std::polar(T{1}, static_cast<T>(-2 * std::numbers::pi * static_cast<double>(i) /
                                                static_cast<double>(n)));
        for (std::size_t i = 0; i < n / 4; ++i)
            weights_[i] = std::polar(
                T{1}, static_cast<T>(-2 * std::numbers::pi * (static_cast<double>(i) + .5) /
                                         static_cast<double>(n) -
                                     std::numbers::pi / 2));
    }
    CpuBackend backend() const noexcept {
        return plan_.backend();
    }
    std::size_t steps() const {
        return 1;
    }
    void fft(const T* time, Complex* out) {
        const auto half = size_ / 2;
        for (std::size_t i = 0; i < half; ++i)
            work_[permutation_[i]] = multiply(Complex(time[2 * i], time[2 * i + 1]), twist_[i]);
        const auto frequency = transformPrepared(false);
        for (std::size_t i = 0; i < half / 2; ++i) {
            const auto j = half - 1 - i;
            const auto odd = (frequency[i] + std::conj(frequency[j])) * T{.5},
                       even = (frequency[i] - std::conj(frequency[j])) * T{.5};
            const auto rotated = multiply(even, weights_[i]);
            out[i] = odd + rotated;
            out[j] = {odd.real() - rotated.real(), rotated.imag() - odd.imag()};
        }
    }
    void ifft(const Complex* in, T* out) {
        const auto half = size_ / 2;
        for (std::size_t i = 0; i < half / 2; ++i) {
            const auto j = half - 1 - i;
            const auto odd = in[i] + std::conj(in[j]), rotated = in[i] - std::conj(in[j]);
            const auto even = multiply(rotated, std::conj(weights_[i]));
            work_[permutation_[i]] = odd + even;
            work_[permutation_[j]] = {odd.real() - even.real(), even.imag() - odd.imag()};
        }
        const auto time = transformPrepared(true);
        for (std::size_t i = 0; i < half; ++i) {
            const auto v = multiply(time[i], std::conj(twist_[i]));
            out[2 * i] = v.real();
            out[2 * i + 1] = v.imag();
        }
    }
    void fft(std::size_t step, const T* in, Complex* out) {
        if (!step)
            fft(in, out);
    }
    void ifft(std::size_t step, const Complex* in, T* out) {
        if (!step)
            ifft(in, out);
    }

  private:
    static Complex multiply(Complex a, Complex b) noexcept {
        return {a.real() * b.real() - a.imag() * b.imag(),
                a.real() * b.imag() + a.imag() * b.real()};
    }
    std::span<const Complex> transformPrepared(bool inverse) noexcept {
        const auto part = plan_.size();
        const auto pieces = radixThree_ ? 3u : 1u;
        for (unsigned r = 0; r < pieces; ++r)
            FftPlanAccess::transformPermutedUnscaled<T>(
                plan_, std::span<Complex>(work_).subspan(r * part, part), inverse);
        if (!radixThree_)
            return work_;
        // Cooley-Tukey 3 x power-of-two: CyDSP SIMD stages with fused input
        // packing/bit reversal, retaining compact sizes for common windows.
        const auto root = static_cast<T>(std::numbers::sqrt3 / 2) * (inverse ? T{1} : T{-1});
        for (std::size_t k = 0; k < part; ++k) {
            auto w1 = radixWeights_[k], w2 = radixWeights_[2 * k];
            if (inverse) {
                w1 = std::conj(w1);
                w2 = std::conj(w2);
            }
            const auto a = work_[k], b = multiply(work_[part + k], w1),
                       c = multiply(work_[2 * part + k], w2);
            const auto sum = b + c;
            const auto difference = b - c;
            const Complex middle = a - sum * T{.5};
            const Complex rotation{-root * difference.imag(), root * difference.real()};
            transformed_[k] = a + sum;
            transformed_[k + part] = middle + rotation;
            transformed_[k + 2 * part] = middle - rotation;
        }
        return transformed_;
    }
    bool radixThree_ = false;
    CpuBackend requested_ = CpuBackend::automatic;
    std::size_t size_ = 0;
    ComplexFft<T> plan_;
    std::vector<Complex> work_, transformed_, twist_, weights_, radixWeights_;
    std::vector<std::size_t> permutation_;
};
} // namespace cydsp::detail
