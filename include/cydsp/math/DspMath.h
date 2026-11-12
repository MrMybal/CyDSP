#pragma once
#include "cydsp/core/Types.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <span>
#include <stdexcept>
#include <vector>
namespace cydsp {
template <Sample T> [[nodiscard]] T decibelsToGain(T db, T floorDb = T{-160}) noexcept {
    return db <= floorDb ? T{} : static_cast<T>(std::pow(10.0, static_cast<double>(db) / 20.0));
}
template <Sample T> [[nodiscard]] T gainToDecibels(T gain, T floorDb = T{-160}) noexcept {
    return gain > T{} ? std::max(floorDb, static_cast<T>(20.0 * std::log10(gain))) : floorDb;
}
// Coefficients are in ascending power order; Horner evaluation allocates nothing.
template <Sample T>
[[nodiscard]] T evaluatePolynomial(std::span<const T> coefficients, T x) noexcept {
    T result{};
    for (std::size_t i = coefficients.size(); i != 0; --i)
        result = result * x + coefficients[i - 1];
    return result;
}
enum class SmoothingCurve { linear, multiplicative };
class SmoothedValue {
  public:
    [[nodiscard]] Status reset(double initial,
                               SmoothingCurve curve = SmoothingCurve::linear) noexcept {
        if (!isFinite(initial) || (curve == SmoothingCurve::multiplicative && initial <= 0))
            return Status::invalid_argument;
        current_ = target_ = initial;
        remaining_ = 0;
        curve_ = curve;
        return Status::ok;
    }
    [[nodiscard]] Status setTarget(double target, std::size_t frames) noexcept {
        if (!isFinite(target) || (curve_ == SmoothingCurve::multiplicative && target <= 0))
            return Status::invalid_argument;
        double step = 0;
        if (frames)
            step = (target - current_) / static_cast<double>(frames);
        if (frames && curve_ == SmoothingCurve::multiplicative)
            step = (std::log(target) - std::log(current_)) / static_cast<double>(frames);
        if (!isFinite(step))
            return Status::invalid_argument;
        target_ = target;
        remaining_ = frames;
        step_ = step;
        if (!frames)
            current_ = target;
        return Status::ok;
    }
    [[nodiscard]] double next() noexcept {
        if (remaining_) {
            current_ = curve_ == SmoothingCurve::linear ? current_ + step_
                                                        : std::exp(std::log(current_) + step_);
            if (!--remaining_)
                current_ = target_;
        }
        return current_;
    }
    [[nodiscard]] double current() const noexcept {
        return current_;
    }
    [[nodiscard]] double target() const noexcept {
        return target_;
    }
    [[nodiscard]] bool smoothing() const noexcept {
        return remaining_ != 0;
    }

  private:
    double current_ = 0, target_ = 0, step_ = 0;
    std::size_t remaining_ = 0;
    SmoothingCurve curve_ = SmoothingCurve::linear;
};
// Construction is non-real-time; immutable interpolation is safe in processing.
template <Sample T> class LookupTable {
  public:
    using Function = T (*)(T, void*) noexcept;
    [[nodiscard]] Status prepare(T minimum, T maximum, std::size_t points, Function function,
                                 void* context = nullptr) noexcept {
        if (!isFinite(minimum) || !isFinite(maximum) || maximum <= minimum ||
            !isFinite(static_cast<double>(maximum) - minimum) || points < 2 || !function)
            return Status::invalid_argument;
        try {
            std::vector<T> next(points);
            for (std::size_t i = 0; i < points; ++i) {
                next[i] =
                    function(static_cast<T>(minimum + (static_cast<double>(maximum) - minimum) *
                                                          (static_cast<double>(i) /
                                                           static_cast<double>(points - 1))),
                             context);
                if (!isFinite(next[i]))
                    return Status::invalid_argument;
            }
            values_.swap(next);
            minimum_ = minimum;
            maximum_ = maximum;
            return Status::ok;
        } catch (const std::bad_alloc&) {
            return Status::allocation_failed;
        } catch (const std::length_error&) {
            return Status::invalid_argument;
        }
    }
    [[nodiscard]] T lookup(T x) const noexcept {
        if (values_.empty() || !isFinite(x))
            return T{};
        const double position = std::clamp((static_cast<double>(x) - minimum_) /
                                               (static_cast<double>(maximum_) - minimum_),
                                           0.0, 1.0) *
                                static_cast<double>(values_.size() - 1);
        const auto first = static_cast<std::size_t>(position);
        if (first + 1 == values_.size())
            return values_.back();
        return static_cast<T>(values_[first] +
                              (static_cast<double>(values_[first + 1]) - values_[first]) *
                                  (position - static_cast<double>(first)));
    }

  private:
    std::vector<T> values_;
    T minimum_{}, maximum_{1};
};
} // namespace cydsp
