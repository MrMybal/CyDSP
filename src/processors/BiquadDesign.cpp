// RBJ equations adapted from DSPFilters, Copyright (c) 2009 by Vinnie Falco.
// MIT license: third_party/licenses/DSPFilters.txt; pinned source: third_party/manifest.json.
// CyDSP adds normalized types, argument checks and transactional coefficient publication.
#include "cydsp/processors/Biquad.h"
#include "detail/AudioMath.h"
#include <cmath>
#include <numbers>
namespace cydsp {
Status designBiquad(BiquadType type, double sampleRate, double frequency, double q,
                    BiquadCoefficients& result) noexcept {
    if (!isFinite(sampleRate) || sampleRate <= 0 || !isFinite(frequency) || frequency <= 0 ||
        frequency >= sampleRate * 0.5 || !isFinite(q) || q <= 0)
        return Status::invalid_argument;
    const double angle = 2 * std::numbers::pi * (frequency / sampleRate);
    const double cs = std::cos(angle), alpha = std::sin(angle) / (2 * q), a0 = 1 + alpha;
    BiquadCoefficients next{};
    switch (type) {
    case BiquadType::lowPass:
        next.b0 = (1 - cs) * 0.5;
        next.b1 = 1 - cs;
        next.b2 = next.b0;
        break;
    case BiquadType::highPass:
        next.b0 = (1 + cs) * 0.5;
        next.b1 = -(1 + cs);
        next.b2 = next.b0;
        break;
    case BiquadType::bandPass:
        next.b0 = alpha;
        next.b1 = 0;
        next.b2 = -alpha;
        break;
    case BiquadType::notch:
        next.b0 = 1;
        next.b1 = -2 * cs;
        next.b2 = 1;
        break;
    case BiquadType::allPass:
        next.b0 = 1 - alpha;
        next.b1 = -2 * cs;
        next.b2 = 1 + alpha;
        break;
    default:
        return Status::invalid_argument;
    }
    next.b0 /= a0;
    next.b1 /= a0;
    next.b2 /= a0;
    next.a1 = -2 * cs / a0;
    next.a2 = (1 - alpha) / a0;
    if (!next.isStable())
        return Status::invalid_argument;
    result = next;
    return Status::ok;
}
} // namespace cydsp

namespace cydsp {
namespace {
bool validEq(double sampleRate, double frequency, double shape, double gain) noexcept {
    return isFinite(sampleRate) && sampleRate > 0 && isFinite(frequency) && frequency > 0 &&
           frequency < sampleRate * 0.5 && isFinite(shape) && shape > 0 && isFinite(gain) &&
           gain >= -120 && gain <= 120;
}
Status publishEq(double a0, BiquadCoefficients next, BiquadCoefficients& result) noexcept {
    if (!isFinite(a0) || a0 <= 0)
        return Status::invalid_argument;
    const double inverse = 1 / a0;
    next.b0 *= inverse;
    next.b1 *= inverse;
    next.b2 *= inverse;
    next.a1 *= inverse;
    next.a2 *= inverse;
    if (!next.isStable())
        return Status::invalid_argument;
    result = next;
    return Status::ok;
}
} // namespace
Status designPeakingBiquad(double sampleRate, double frequency, double q, double gain,
                           BiquadCoefficients& result) noexcept {
    if (!validEq(sampleRate, frequency, q, gain))
        return Status::invalid_argument;
    const auto angle = 2 * std::numbers::pi * (frequency / sampleRate), cs = std::cos(angle),
               alpha = std::sin(angle) / (2 * q),
               a = detail::audioExp(gain * (std::numbers::ln10 / 40));
    return publishEq(1 + alpha / a, {1 + alpha * a, -2 * cs, 1 - alpha * a, -2 * cs, 1 - alpha / a},
                     result);
}
Status PeakingBiquadDesigner::prepare(double sampleRate, double frequency, double q) noexcept {
    if (!validEq(sampleRate, frequency, q, 0))
        return Status::invalid_argument;
    const auto angle = 2 * std::numbers::pi * (frequency / sampleRate);
    const auto cosine = std::cos(angle), alpha = std::sin(angle) / (2 * q);
    if (!isFinite(alpha))
        return Status::invalid_argument;
    cosine_ = cosine;
    alpha_ = alpha;
    prepared_ = true;
    return Status::ok;
}
Status PeakingBiquadDesigner::design(double gain, BiquadCoefficients& result) const noexcept {
    if (!prepared_)
        return Status::not_prepared;
    if (!isFinite(gain) || gain < -120 || gain > 120)
        return Status::invalid_argument;
    const auto a = detail::audioExp(gain * (std::numbers::ln10 / 40));
    return publishEq(1 + alpha_ / a,
                     {1 + alpha_ * a, -2 * cosine_, 1 - alpha_ * a, -2 * cosine_, 1 - alpha_ / a},
                     result);
}
Status designShelfBiquad(bool high, double sampleRate, double frequency, double slope, double gain,
                         BiquadCoefficients& result) noexcept {
    if (!validEq(sampleRate, frequency, slope, gain) || slope > 1)
        return Status::invalid_argument;
    const auto a = detail::audioExp(gain * (std::numbers::ln10 / 40)),
               angle = 2 * std::numbers::pi * (frequency / sampleRate), cs = std::cos(angle);
    const auto alpha = std::sin(angle) * 0.5 * std::sqrt((a + 1 / a) * (1 / slope - 1) + 2),
               sq = 2 * std::sqrt(a) * alpha;
    if (high)
        return publishEq((a + 1) - (a - 1) * cs + sq,
                         {a * ((a + 1) + (a - 1) * cs + sq), -2 * a * ((a - 1) + (a + 1) * cs),
                          a * ((a + 1) + (a - 1) * cs - sq), 2 * ((a - 1) - (a + 1) * cs),
                          (a + 1) - (a - 1) * cs - sq},
                         result);
    return publishEq((a + 1) + (a - 1) * cs + sq,
                     {a * ((a + 1) - (a - 1) * cs + sq), 2 * a * ((a - 1) - (a + 1) * cs),
                      a * ((a + 1) - (a - 1) * cs - sq), -2 * ((a - 1) + (a + 1) * cs),
                      (a + 1) + (a - 1) * cs - sq},
                     result);
}
} // namespace cydsp
