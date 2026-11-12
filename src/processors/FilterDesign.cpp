// Chebyshev/Laguerre/Bessel equations adapted from DSPFilters.
// Copyright (c) 2009 by Vinnie Falco
// MIT: third_party/licenses/DSPFilters.txt; exact originals in manifest.
// CyDSP supplies bounded solving, -3dB normalization, validation and SOS conversion.
#include "cydsp/processors/FilterDesign.h"
#include "cydsp/processors/TptFilter.h"
#include "detail/FilterPrototypes.h"
namespace cydsp {
namespace {
using detail::Complex;
bool laguerre(unsigned degree, const std::array<Complex, 17>& a, Complex& x) noexcept {
    constexpr std::array<double, 9> fractions{0, .5, .25, .75, .13, .38, .62, .88, 1};
    for (unsigned iter = 1; iter <= 80; ++iter) {
        Complex b = a[degree], d = 0, f = 0;
        const auto ax = std::abs(x);
        double error = std::abs(b);
        for (unsigned j = degree; j-- > 0;) {
            f = x * f + d;
            d = x * d + b;
            b = x * b + a[j];
            error = std::abs(b) + ax * error;
        }
        if (std::abs(b) <= error * std::numeric_limits<double>::epsilon())
            return true;
        const auto g = d / b, g2 = g * g, h = g2 - 2.0 * f / b;
        const auto square = std::sqrt(double(degree - 1) * (double(degree) * h - g2));
        auto plus = g + square;
        const auto minus = g - square;
        if (std::abs(plus) < std::abs(minus))
            plus = minus;
        const auto step =
            std::abs(plus) > 0 ? double(degree) / plus : std::polar(1 + ax, double(iter));
        const auto next = x - step;
        if (x == next)
            return true;
        x = iter % 10 ? next : x - fractions[iter / 10] * step;
    }
    return false;
}
bool bessel(unsigned degree, detail::Prototype& proto) noexcept {
    std::array<Complex, 17> a{}, deflated{}, roots{};
    auto factorial = [](unsigned n) {
        double v = 1;
        for (unsigned i = 2; i <= n; ++i)
            v *= i;
        return v;
    };
    for (unsigned k = 0; k <= degree; ++k)
        a[k] = factorial(2 * degree - k) /
               (std::pow(2.0, double(degree - k)) * factorial(k) * factorial(degree - k));
    deflated = a;
    for (unsigned j = degree; j > 0; --j) {
        Complex x = 0;
        if (!laguerre(j, deflated, x))
            return false;
        roots[j - 1] = x;
        auto b = deflated[j];
        for (unsigned k = j; k-- > 0;) {
            const auto c = deflated[k];
            deflated[k] = b;
            b = x * b + c;
        }
    }
    for (unsigned j = 0; j < degree; ++j)
        if (!laguerre(degree, a, roots[j]))
            return false;
    auto magnitude = [&](double w) {
        double result = 1;
        for (unsigned j = 0; j < degree; ++j)
            result *= std::abs(roots[j] / (Complex(0, w) - roots[j]));
        return result;
    };
    double low = 0, high = double(degree) * 4;
    for (unsigned i = 0; i < 80; ++i) {
        const auto mid = (low + high) * .5;
        if (magnitude(mid) > .7071067811865475244)
            low = mid;
        else
            high = mid;
    }
    const auto scale = (low + high) * .5;
    for (unsigned j = 0; j < degree; ++j) {
        auto p = roots[j] / scale;
        if (p.real() >= 0 || !isFinite(p.real()) || !isFinite(p.imag()))
            return false;
        if (std::abs(p.imag()) < 1e-7)
            proto.add({p.real(), 0}, detail::infinity());
        else if (p.imag() > 0)
            proto.addPoleZeroConjugatePairs(p, detail::infinity());
    }
    return proto.count == (degree + 1) / 2;
}
} // namespace
Status designIir(const IirDesign& d, std::span<BiquadCoefficients> output,
                 std::size_t& count) noexcept {
    if (d.order < 1 || d.order > 16 || output.size() < (d.order + 1) / 2 ||
        !isFinite(d.sampleRate) || d.sampleRate <= 0 || !isFinite(d.cutoff) || d.cutoff <= 0 ||
        d.cutoff >= d.sampleRate * .5 ||
        (d.response != BiquadType::lowPass && d.response != BiquadType::highPass) ||
        !isFinite(d.rippleDb) || d.rippleDb <= 0 || d.rippleDb > 24 || !isFinite(d.stopBandDb) ||
        d.stopBandDb <= d.rippleDb || d.stopBandDb > 160 || !isFinite(d.ellipticRolloff) ||
        d.ellipticRolloff < 0 || d.ellipticRolloff > 4)
        return Status::invalid_argument;
    if (d.family == IirFamily::butterworth)
        return designButterworth(d.response, d.order, d.sampleRate, d.cutoff, output, count);
    detail::Prototype proto;
    if (d.family == IirFamily::bessel) {
        if (!bessel(d.order, proto))
            return Status::invalid_argument;
    } else if (d.family == IirFamily::elliptic) {
        detail::EllipticPrototype elliptic;
        elliptic.design(int(d.order), d.rippleDb, d.ellipticRolloff);
        proto = elliptic;
    } else if (d.family == IirFamily::chebyshevI || d.family == IirFamily::chebyshevII) {
        const bool second = d.family == IirFamily::chebyshevII;
        const double eps = second ? 1 / std::sqrt(std::pow(10., d.stopBandDb / 10) - 1)
                                  : std::sqrt(std::pow(10., d.rippleDb / 10) - 1);
        const auto v = std::asinh(1 / eps) / d.order, sh = -std::sinh(v), ch = std::cosh(v);
        for (unsigned i = 0; i < d.order / 2; ++i) {
            const double k = 2 * double(i) + 1,
                         angle = (k - d.order) * std::numbers::pi / (2 * d.order);
            Complex p{sh * std::cos(angle), ch * std::sin(angle)};
            if (second) {
                const auto norm = std::norm(p);
                p /= norm;
                proto.addPoleZeroConjugatePairs(
                    p, {0, 1 / std::cos(k * std::numbers::pi / (2 * d.order))});
            } else
                proto.addPoleZeroConjugatePairs(p, detail::infinity());
        }
        if (d.order % 2)
            proto.add({second ? 1 / sh : sh, 0}, detail::infinity());
        else if (!second)
            proto.gain = std::pow(10., -d.rippleDb / 20);
    } else
        return Status::invalid_argument;
    if (!proto.valid || proto.count != (d.order + 1) / 2)
        return Status::invalid_argument;
    std::array<BiquadCoefficients, 8> next{};
    const auto g = std::tan(std::numbers::pi * d.cutoff / d.sampleRate);
    const bool high = d.response == BiquadType::highPass;
    const auto normal = high ? -1.0 : 1.0;
    const auto gain = std::pow(proto.gain, 1.0 / double(proto.count));
    auto transform = [&](Complex s) {
        if (!isFinite(s.real()))
            return Complex(high ? 1.0 : -1.0, 0);
        s = high ? g / s : g * s;
        return (1.0 + s) / (1.0 - s);
    };
    for (std::size_t i = 0; i < proto.count; ++i) {
        const auto& pair = proto.pairs[i];
        const auto p = transform(pair.pole), z = transform(pair.zero);
        auto& c = next[i];
        c.a1 = -(pair.single ? 1 : 2) * p.real();
        c.a2 = pair.single ? 0 : std::norm(p);
        c.b0 = 1;
        c.b1 = -(pair.single ? 1 : 2) * z.real();
        c.b2 = pair.single ? 0 : std::norm(z);
        const auto scale = gain * (1 + c.a1 * normal + c.a2) / (c.b0 + c.b1 * normal + c.b2);
        c.b0 *= scale;
        c.b1 *= scale;
        c.b2 *= scale;
        if (!c.isStable())
            return Status::invalid_argument;
    }
    std::copy_n(next.begin(), proto.count, output.begin());
    count = proto.count;
    return Status::ok;
}
} // namespace cydsp
