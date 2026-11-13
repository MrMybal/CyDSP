#pragma once
#include "cydsp/core/CircularHistory.h"
#include "cydsp/math/VectorOps.h"
#include "detail/VectorTable.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>
namespace cydsp::detail {
class HistorySinc {
  public:
    Status prepare(CpuBackend backend) noexcept {
        try {
            VectorOps ops;
            auto status = VectorOps::create(backend, ops);
            if (status != Status::ok)
                return status;
            std::vector<double> next(13 * 65 * 32);
            for (std::size_t band = 0; band < 13; ++band)
                for (std::size_t phase = 0; phase <= 64; ++phase) {
                    const double cutoff = .45 / (1 + double(band) / 4),
                                 fraction = double(phase) / 64;
                    double sum = 0;
                    for (std::size_t k = 0; k < 32; ++k) {
                        const double x = double(k) - 15 - fraction,
                                     angle = 2 * std::numbers::pi * cutoff * x,
                                     sinc = std::abs(angle) < 1e-12 ? 1 : std::sin(angle) / angle,
                                     window = .42 -
                                              .5 * std::cos(2 * std::numbers::pi * double(k) / 31) +
                                              .08 * std::cos(4 * std::numbers::pi * double(k) / 31);
                        next[(band * 65 + phase) * 32 + k] = 2 * cutoff * sinc * window;
                        sum += next[(band * 65 + phase) * 32 + k];
                    }
                    for (std::size_t k = 0; k < 32; ++k)
                        next[(band * 65 + phase) * 32 + k] /= sum;
                }
            coefficients_.swap(next);
            ops_ = ops;
            return Status::ok;
        } catch (...) {
            return Status::allocation_failed;
        }
    }
    template <Sample T>
    double read(const CircularHistory<T>& history, std::size_t channel, double age,
                double ratio) const noexcept {
        const double whole = std::floor(age), fraction = age - whole;
        const auto phase = std::min<std::size_t>(63, std::size_t(fraction * 64)),
                   bank = std::min<std::size_t>(
                       12, std::size_t(std::ceil(std::max(1., std::abs(ratio)) * 4)) - 4);
        std::array<double, 32> samples{};
        (void)history.readWindow(channel, std::ptrdiff_t(whole) - 15, samples);
        const auto* coefficients = coefficients_.data() + (bank * 65 + phase) * 32;
        const auto& kernels = detail::VectorAccess::kernels<double>(ops_);
        const double a = kernels.dot(samples.data(), coefficients, 32),
                     b = kernels.dot(samples.data(), coefficients + 32, 32),
                     mix = fraction * 64 - double(phase);
        return a + (b - a) * mix;
    }

  private:
    std::vector<double> coefficients_;
    VectorOps ops_;
};
} // namespace cydsp::detail
