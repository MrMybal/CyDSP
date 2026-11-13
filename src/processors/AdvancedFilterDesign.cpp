#include "cydsp/processors/AdvancedFilterDesign.h"
#include "cydsp/processors/ComplexFft.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <complex>
#include <numbers>
#include <vector>
namespace cydsp {
namespace {
using C = std::complex<double>;
C response(const BiquadCoefficients& c, C inverseZ) {
    return (c.b0 + c.b1 * inverseZ + c.b2 * inverseZ * inverseZ) /
           (1. + c.a1 * inverseZ + c.a2 * inverseZ * inverseZ);
}
std::array<C, 2> roots(double a, double b, double c) {
    const C d = std::sqrt(C(b * b - 4 * a * c, 0));
    return {(-b + d) / (2 * a), (-b - d) / (2 * a)};
}
bool pairRoots(const std::array<C, 32>& roots, std::size_t size,
               std::array<std::array<C, 2>, 16>& pairs) {
    std::array<bool, 32> used{};
    std::size_t count = 0;
    for (std::size_t i = 0; i < size; ++i) {
        if (used[i])
            continue;
        const auto a = roots[i];
        std::size_t best = size;
        double distance = std::numeric_limits<double>::infinity();
        for (std::size_t j = i + 1; j < size; ++j)
            if (!used[j]) {
                if (std::abs(a.imag()) <= 1e-8 && std::abs(roots[j].imag()) > 1e-8)
                    continue;
                const auto error = std::abs(roots[j] - std::conj(a));
                if (error < distance) {
                    best = j;
                    distance = error;
                }
            }
        if (best == size)
            return false;
        if (std::abs(a.imag()) > 1e-8 && distance > 1e-6 * (1 + std::abs(a)))
            return false;
        pairs[count++] = {a, roots[best]};
        used[i] = used[best] = true;
    }
    return count == size / 2;
}
} // namespace
Status designBandIir(const IirBandDesign& design, std::span<BiquadCoefficients> out,
                     std::size_t& count) noexcept {
    const auto& d = design.prototype;
    if (!isFinite(d.sampleRate) || d.sampleRate <= 0 || !isFinite(design.lowHz) ||
        !isFinite(design.highHz) || design.lowHz <= 0 || design.highHz <= design.lowHz ||
        design.highHz >= d.sampleRate * .5 || d.order < 1 || d.order > 16 || out.size() < d.order)
        return Status::invalid_argument;
    auto prototype = d;
    prototype.response = BiquadType::lowPass;
    prototype.cutoff = d.sampleRate * .25;
    std::array<BiquadCoefficients, 8> low{};
    std::size_t sections = 0;
    auto result = designIir(prototype, low, sections);
    if (result != Status::ok)
        return result;
    const double w1 = std::tan(std::numbers::pi * design.lowHz / d.sampleRate),
                 w2 = std::tan(std::numbers::pi * design.highHz / d.sampleRate), width = w2 - w1,
                 center = std::sqrt(w1 * w2);
    std::array<C, 32> poles{}, zeros{};
    std::size_t used = 0;
    const auto bilinear = [](C s) { return (1. + s) / (1. - s); };
    auto transformed = [&](C z) {
        if (std::abs(z + C(1, 0)) < 1e-12) {
            if (design.bandStop)
                return std::array<C, 2>{bilinear(C(0, center)), bilinear(C(0, -center))};
            return std::array<C, 2>{C(1, 0), C(-1, 0)};
        }
        const C s = (z - 1.) / (z + 1.);
        const C middle = design.bandStop ? width / s : width * s;
        const C square = std::sqrt(middle * middle - 4 * center * center);
        return std::array<C, 2>{bilinear((middle + square) * .5), bilinear((middle - square) * .5)};
    };
    double target = 1;
    for (std::size_t i = 0; i < sections; ++i) {
        const auto& section = low[i];
        target *= std::abs(response(section, C(1, 0)));
        const bool single = section.a2 == 0 && section.b2 == 0;
        const auto p =
            single ? std::array<C, 2>{C(-section.a1, 0), C{}} : roots(1, section.a1, section.a2);
        const auto z = single ? std::array<C, 2>{C(-section.b1 / section.b0, 0), C{}}
                              : roots(section.b0, section.b1, section.b2);
        for (unsigned j = 0; j < (single ? 1u : 2u); ++j) {
            const auto pp = transformed(p[j]), zz = transformed(z[j]);
            for (unsigned k = 0; k < 2; ++k) {
                poles[used] = pp[k];
                zeros[used] = zz[k];
                ++used;
            }
        }
    }
    if (used != 2 * d.order)
        return Status::invalid_argument;
    std::array<std::array<C, 2>, 16> pp{}, zz{};
    if (!pairRoots(poles, used, pp) || !pairRoots(zeros, used, zz))
        return Status::invalid_argument;
    std::array<BiquadCoefficients, 16> next{};
    const C reference = design.bandStop ? C(1, 0) : std::polar(1., -2 * std::atan(center));
    double amplitude = 1;
    for (std::size_t i = 0; i < d.order; ++i) {
        auto& c = next[i];
        const C ps = pp[i][0] + pp[i][1], pm = pp[i][0] * pp[i][1], zs = zz[i][0] + zz[i][1],
                zm = zz[i][0] * zz[i][1];
        if (std::abs(ps.imag()) > 1e-6 || std::abs(pm.imag()) > 1e-6 ||
            std::abs(zs.imag()) > 1e-6 || std::abs(zm.imag()) > 1e-6)
            return Status::invalid_argument;
        c = {1, -zs.real(), zm.real(), -ps.real(), pm.real()};
        if (!c.isStable())
            return Status::invalid_argument;
        amplitude *= std::abs(response(c, reference));
    }
    if (!isFinite(amplitude) || amplitude <= 0 || !isFinite(target) || target <= 0)
        return Status::invalid_argument;
    const double gain = std::pow(target / amplitude, 1. / double(d.order));
    for (std::size_t i = 0; i < d.order; ++i) {
        auto& c = next[i];
        c.b0 *= gain;
        c.b1 *= gain;
        c.b2 *= gain;
        if (!c.isStable())
            return Status::invalid_argument;
    }
    std::copy_n(next.begin(), d.order, out.begin());
    count = d.order;
    return Status::ok;
}
Status minimumPhaseFir(std::span<const double> input, std::span<double> output, double floorDb,
                       CpuBackend backend) noexcept {
    if (input.empty() || input.size() > 8193 || output.size() != input.size() ||
        !isFinite(floorDb) || floorDb > -20 || floorDb < -300)
        return Status::invalid_argument;
    for (double value : input)
        if (!isFinite(value) || std::abs(value) > 1e30)
            return Status::invalid_argument;
    try {
        const auto n = std::bit_ceil(std::max<std::size_t>(256, input.size() * 16));
        ComplexFft<double> fft;
        auto status = fft.prepare(n, backend);
        if (status != Status::ok)
            return status;
        std::vector<C> data(n);
        for (std::size_t i = 0; i < input.size(); ++i)
            data[i] = input[i];
        (void)fft.process(data, data, FftDirection::forward);
        double maximum = 0;
        for (auto value : data)
            maximum = std::max(maximum, std::abs(value));
        std::vector<double> next(input.size());
        if (maximum > 0) {
            const double floor = maximum * std::pow(10., floorDb / 20);
            for (auto& value : data)
                value = {std::log(std::max(floor, std::abs(value))), 0};
            (void)fft.process(data, data, FftDirection::inverse);
            data[0] = {data[0].real(), 0};
            for (std::size_t i = 1; i < n / 2; ++i)
                data[i] = {2 * data[i].real(), 0};
            data[n / 2] = {data[n / 2].real(), 0};
            std::fill(data.begin() + std::ptrdiff_t(n / 2 + 1), data.end(), C{});
            (void)fft.process(data, data, FftDirection::forward);
            for (auto& value : data)
                value = std::exp(value);
            (void)fft.process(data, data, FftDirection::inverse);
            for (std::size_t i = 0; i < next.size(); ++i) {
                next[i] = data[i].real();
                if (!isFinite(next[i]))
                    return Status::invalid_argument;
            }
        }
        std::copy(next.begin(), next.end(), output.begin());
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status designFirEqualizer(double rate, std::span<const FirEqPoint> points, std::span<double> output,
                          FirPhase phase, CpuBackend backend) noexcept {
    if (!isFinite(rate) || rate < 8000 || rate > 768000 || points.size() < 2 ||
        points.size() > 1024 || output.size() < 3 || output.size() > 8193 ||
        output.size() % 2 == 0 || unsigned(phase) > unsigned(FirPhase::minimum) ||
        points.front().frequencyHz != 0 || points.back().frequencyHz != rate * .5)
        return Status::invalid_argument;
    double previous = -1;
    for (const auto& point : points) {
        if (!isFinite(point.frequencyHz) || point.frequencyHz <= previous ||
            point.frequencyHz > rate * .5 || !isFinite(point.gainDb) ||
            std::abs(point.gainDb) > 120)
            return Status::invalid_argument;
        previous = point.frequencyHz;
    }
    try {
        const auto n = std::bit_ceil(std::max<std::size_t>(256, output.size() * 8));
        ComplexFft<double> fft;
        auto status = fft.prepare(n, backend);
        if (status != Status::ok)
            return status;
        std::vector<C> spectrum(n);
        std::size_t segment = 0;
        for (std::size_t k = 0; k <= n / 2; ++k) {
            const double frequency = rate * double(k) / double(n);
            while (segment + 2 < points.size() && frequency > points[segment + 1].frequencyHz)
                ++segment;
            const auto& a = points[segment];
            const auto& b = points[segment + 1];
            const double t = (frequency - a.frequencyHz) / (b.frequencyHz - a.frequencyHz);
            spectrum[k] = std::pow(10., (a.gainDb + t * (b.gainDb - a.gainDb)) / 20);
            if (k && k < n / 2)
                spectrum[n - k] = spectrum[k];
        }
        (void)fft.process(spectrum, spectrum, FftDirection::inverse);
        std::vector<double> next(output.size());
        const auto middle = output.size() / 2;
        for (std::size_t i = 0; i < next.size(); ++i) {
            const double window =
                .42 - .5 * std::cos(2 * std::numbers::pi * double(i) / double(next.size() - 1)) +
                .08 * std::cos(4 * std::numbers::pi * double(i) / double(next.size() - 1));
            next[i] = spectrum[(i + n - middle) % n].real() * window;
        }
        if (phase == FirPhase::minimum) {
            status = minimumPhaseFir(next, next, -180, backend);
            if (status != Status::ok)
                return status;
        }
        std::copy(next.begin(), next.end(), output.begin());
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
} // namespace cydsp
