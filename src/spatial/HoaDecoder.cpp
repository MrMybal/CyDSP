// Original MIT implementation of the published magnitude least-squares objective.
// Mathematical reference: Schorkhuber, Zaunschirm, Holdrich, DAGA 2018,
// https://pub.dega-akustik.de/DAGA_2018/data/articles/000301.pdf
// No IEM/SPARTA or other third-party implementation is used.
#include "detail/HoaDecoder.h"
#include "cydsp/processors/RealFft.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>
#include <numbers>
#include <numeric>
namespace cydsp::detail {
namespace {
using C = std::complex<double>;
struct Failure {
    Status status;
};
void check(Status status) {
    if (status != Status::ok)
        throw Failure{status};
}
struct Budget {
    std::size_t left;
    void charge(std::size_t a, std::size_t b, std::size_t size) {
        if ((a && b > left / a) || (a * b && size > left / (a * b)))
            throw Failure{Status::invalid_spec};
        left -= a * b * size;
    }
};
double blend(double frequency, const HoaDecoderDesign& c) {
    if (c.transitionHz == 0)
        return frequency > c.crossoverHz ? 1 : 0;
    const double x = std::clamp((frequency - c.crossoverHz) / c.transitionHz + .5, 0., 1.);
    return .5 - .5 * std::cos(std::numbers::pi * x);
}
double legendre(unsigned degree, double x) {
    double p0 = 1, p1 = x;
    if (!degree)
        return 1;
    for (unsigned n = 2; n <= degree; ++n) {
        const double p = ((2 * double(n) - 1) * x * p1 - (double(n) - 1) * p0) / double(n);
        p0 = p1;
        p1 = p;
    }
    return p1;
}
std::vector<double> maxReWeights(unsigned order) {
    double root = 1;
    if (order) {
        const auto degree = order + 1;
        root = std::cos(std::numbers::pi * .75 / (double(degree) + .5));
        for (unsigned j = 0; j < 32; ++j) {
            const double p = legendre(degree, root);
            const double derivative =
                double(degree) * (root * p - legendre(degree - 1, root)) / (root * root - 1);
            const double next = root - p / derivative;
            if (std::abs(next - root) < 1e-15) {
                root = next;
                break;
            }
            root = next;
        }
    }
    std::vector<double> weights((order + 1) * (order + 1));
    for (unsigned l = 0; l <= order; ++l)
        std::fill(weights.begin() + l * l, weights.begin() + (l + 1) * (l + 1), legendre(l, root));
    return weights;
}
// SN3D real basis. The weights sum to one, making regularization grid-size invariant.
std::vector<double> projection(std::span<const double> y, std::span<const double> weights,
                               std::size_t channels, double regularization) {
    const auto m = weights.size(), n = channels;
    std::vector<double> inverse(2 * n * n);
    for (std::size_t row = 0; row < n; ++row) {
        for (std::size_t col = 0; col < n; ++col) {
            double value = 0;
            for (std::size_t i = 0; i < m; ++i)
                value += weights[i] * y[i * n + row] * y[i * n + col];
            inverse[row * 2 * n + col] = value + (row == col ? regularization : 0);
        }
        inverse[row * 2 * n + n + row] = 1;
    }
    for (std::size_t p = 0; p < n; ++p) {
        auto pivot = p;
        for (auto row = p + 1; row < n; ++row)
            if (std::abs(inverse[row * 2 * n + p]) > std::abs(inverse[pivot * 2 * n + p]))
                pivot = row;
        if (std::abs(inverse[pivot * 2 * n + p]) < 1e-14)
            throw Failure{Status::unsupported_format};
        for (std::size_t j = 0; j < 2 * n; ++j)
            std::swap(inverse[p * 2 * n + j], inverse[pivot * 2 * n + j]);
        const auto scale = inverse[p * 2 * n + p];
        for (std::size_t j = 0; j < 2 * n; ++j)
            inverse[p * 2 * n + j] /= scale;
        for (std::size_t row = 0; row < n; ++row)
            if (row != p) {
                const auto factor = inverse[row * 2 * n + p];
                for (std::size_t j = 0; j < 2 * n; ++j)
                    inverse[row * 2 * n + j] -= factor * inverse[p * 2 * n + j];
            }
    }
    std::vector<double> result(m * n);
    for (std::size_t i = 0; i < m; ++i)
        for (std::size_t row = 0; row < n; ++row)
            for (std::size_t k = 0; k < n; ++k)
                result[i * n + row] += weights[i] * y[i * n + k] * inverse[row * 2 * n + n + k];
    return result;
}
C response(std::span<const double> y, std::span<const C> w) {
    C sum{};
    for (std::size_t j = 0; j < w.size(); ++j)
        sum += y[j] * w[j];
    return sum;
}
} // namespace
Status designHoaFilters(unsigned order, double rate, std::span<const HrirPoint<double>> points,
                        const HoaDecoderDesign& c, std::vector<std::vector<double>>& output,
                        HoaDecoderReport& report, CpuBackend backend) {
    const auto m = points.size(), channels = std::size_t((order + 1) * (order + 1));
    if (order > 7 || !m || m > 100000 || !isFinite(rate) || rate < 8000 || rate > 768000 ||
        unsigned(c.method) > unsigned(HoaDecoderMethod::magLS) || !isFinite(c.crossoverHz) ||
        !isFinite(c.transitionHz) || c.transitionHz < 0 ||
        c.crossoverHz - c.transitionHz * .5 <= 0 ||
        c.crossoverHz + c.transitionHz * .5 >= rate * .5 || !isFinite(c.regularization) ||
        c.regularization < 0 || c.regularization > 1 || !c.iterations || c.iterations > 256 ||
        !c.maximumStorageBytes || !isPowerOfTwo(c.maximumFftSize) || c.maximumFftSize < 32 ||
        c.maximumFftSize > 65536 || (!c.spatialWeights.empty() && c.spatialWeights.size() != m) ||
        (c.method == HoaDecoderMethod::magLS && m < channels))
        return Status::invalid_spec;
    try {
        std::size_t natural = 1;
        for (const auto& p : points)
            for (unsigned ear = 0; ear < 2; ++ear) {
                const auto ir = ear ? p.right : p.left;
                const auto delay = ear ? p.rightDelayFrames : p.leftDelayFrames;
                if (ir.empty() || !isFinite(delay) || delay < 0 ||
                    delay > double(c.maximumFftSize) || ir.size() > c.maximumFftSize)
                    return Status::invalid_spec;
                const auto tail = ir.size() + std::size_t(std::ceil(delay)) + 1;
                if (tail > c.maximumFftSize / 2)
                    return Status::invalid_spec;
                natural = std::max(natural, tail);
            }
        std::size_t n = c.fftSize;
        if (!n) {
            n = 32;
            while (n < 2 * natural)
                n *= 2;
        }
        if (!isPowerOfTwo(n) || n < 2 * natural || n > c.maximumFftSize)
            return Status::invalid_spec;
        const auto bins = n / 2 + 1;
        Budget budget{c.maximumStorageBytes};
        budget.charge(m, channels, 2 * sizeof(double));        // basis + projection
        budget.charge(m, bins, sizeof(C));                     // one ear's target spectra
        budget.charge(2 * channels, bins, sizeof(C));          // designed coefficients
        budget.charge(2 * channels, n, sizeof(double));        // final FIRs
        budget.charge(channels, channels, 2 * sizeof(double)); // inversion
        budget.charge(m, 1, 2 * sizeof(C) + sizeof(double));
        budget.charge(n, 1, 16 * sizeof(C)); // conservative FFT/time/optimizer allowance
        std::vector<double> weights(m), y(m * channels);
        double weightSum = 0;
        for (std::size_t i = 0; i < m; ++i) {
            weights[i] = c.spatialWeights.empty() ? 1 : c.spatialWeights[i];
            if (!isFinite(weights[i]) || weights[i] < 0)
                return Status::invalid_argument;
            weightSum += weights[i];
            check(ambisonicCoefficients(order, points[i].direction,
                                        {y.data() + i * channels, channels}));
        }
        if (!isFinite(weightSum) || weightSum <= 0)
            return Status::invalid_argument;
        for (auto& w : weights)
            w /= weightSum;
        const auto project = projection(y, weights, channels, c.regularization);
        const auto taper = maxReWeights(order);
        RealFft<double> fft;
        check(fft.prepare(n, backend));
        std::vector<double> time(n);
        std::vector<C> spectrum(bins), targets(m * bins), coefficients(2 * channels * bins);
        std::vector<C> target(m), modified(m), ls(channels), current(channels), next(channels),
            previous(channels), older(channels), prediction(channels);
        double lowError = 0, lowEnergy = 0, highError = 0, initialHighError = 0, highEnergy = 0;
        std::array<double, 2> referenceEnergy{}, decoderEnergy{};
        for (unsigned ear = 0; ear < 2; ++ear) {
            for (std::size_t i = 0; i < m; ++i) {
                std::fill(time.begin(), time.end(), 0.);
                const auto ir = ear ? points[i].right : points[i].left;
                std::copy(ir.begin(), ir.end(), time.begin());
                check(fft.forward(time, spectrum));
                const double delay = ear ? points[i].rightDelayFrames : points[i].leftDelayFrames;
                for (std::size_t k = 0; k < bins; ++k) {
                    C h = spectrum[k] *
                          std::polar(1., -2 * std::numbers::pi * double(k) * delay / double(n));
                    if (k == 0 || k + 1 == bins)
                        h = {h.real(), 0};
                    targets[i * bins + k] = h;
                }
            }
            std::fill(previous.begin(), previous.end(), C{});
            std::fill(older.begin(), older.end(), C{});
            for (std::size_t k = 0; k < bins; ++k) {
                const double frequency = rate * double(k) / double(n), band = blend(frequency, c);
                for (std::size_t i = 0; i < m; ++i)
                    target[i] = targets[i * bins + k];
                const auto applyProjection = [&](std::span<const C> values, std::span<C> result) {
                    std::fill(result.begin(), result.end(), C{});
                    for (std::size_t i = 0; i < m; ++i)
                        for (std::size_t j = 0; j < channels; ++j)
                            result[j] += project[i * channels + j] * values[i];
                };
                applyProjection(target, ls);
                current = ls;
                const auto objective = [&](std::span<const C> w) {
                    double value = 0;
                    for (std::size_t i = 0; i < m; ++i) {
                        const auto h = response({y.data() + i * channels, channels}, w);
                        const double magnitude = std::abs(h) - std::abs(target[i]);
                        value += weights[i] * ((1 - band) * std::norm(h - target[i]) +
                                               band * magnitude * magnitude);
                    }
                    for (auto v : w)
                        value += c.regularization * std::norm(v);
                    return value;
                };
                if (c.method == HoaDecoderMethod::magLS && band > 0 && k + 1 < bins) {
                    C phaseStep{};
                    for (std::size_t j = 0; j < channels; ++j)
                        phaseStep += previous[j] * std::conj(older[j]);
                    const C rotation = std::polar(1., std::arg(phaseStep));
                    for (std::size_t j = 0; j < channels; ++j)
                        prediction[j] = previous[j] * rotation;
                    if (objective(prediction) < objective(current))
                        current = prediction;
                    double cost = objective(current);
                    for (std::size_t iteration = 0; iteration < c.iterations; ++iteration) {
                        for (std::size_t i = 0; i < m; ++i) {
                            const auto h = response({y.data() + i * channels, channels}, current);
                            const C phase =
                                std::abs(h) > 1e-30
                                    ? h / std::abs(h)
                                    : (std::abs(target[i]) > 1e-30 ? target[i] / std::abs(target[i])
                                                                   : C{1, 0});
                            modified[i] =
                                (1 - band) * target[i] + band * std::abs(target[i]) * phase;
                        }
                        applyProjection(modified, next);
                        const double nextCost = objective(next);
                        if (nextCost > cost + 1e-14 * std::max(1., cost))
                            break;
                        current.swap(next);
                        cost = nextCost;
                    }
                    if (band == 1) {
                        C alignment{};
                        for (std::size_t j = 0; j < channels; ++j)
                            alignment += std::conj(current[j]) * prediction[j];
                        if (std::abs(alignment) > 1e-30)
                            for (auto& v : current)
                                v *= std::polar(1., std::arg(alignment));
                    }
                }
                older = previous;
                previous = current;
                if (c.maxRe)
                    for (std::size_t j = 0; j < channels; ++j)
                        current[j] *= 1 + band * (taper[j] - 1);
                if (k == 0 || k + 1 == bins)
                    for (auto& v : current)
                        v = {v.real(), 0};
                if (k > 0 && k + 1 < bins)
                    for (std::size_t i = 0; i < m; ++i) {
                        const auto h = response({y.data() + i * channels, channels}, current);
                        if (band == 0) {
                            lowError += weights[i] * std::norm(h - target[i]);
                            lowEnergy += weights[i] * std::norm(target[i]);
                        }
                        if (band == 1) {
                            const double error = std::abs(h) - std::abs(target[i]);
                            const double initial =
                                std::abs(response({y.data() + i * channels, channels}, ls)) -
                                std::abs(target[i]);
                            highError += weights[i] * error * error;
                            initialHighError += weights[i] * initial * initial;
                            highEnergy += weights[i] * std::norm(target[i]);
                        }
                    }
                const double binWeight = (k == 0 || k + 1 == bins) ? 1. : 2.;
                for (std::size_t i = 0; i < m; ++i) {
                    const auto h = response({y.data() + i * channels, channels}, current);
                    referenceEnergy[ear] +=
                        binWeight * weights[i] * std::norm(target[i]) / double(n);
                    decoderEnergy[ear] += binWeight * weights[i] * std::norm(h) / double(n);
                }
                for (std::size_t j = 0; j < channels; ++j)
                    coefficients[(ear * channels + j) * bins + k] = current[j];
            }
        }
        std::vector<std::vector<double>> filters(2 * channels, std::vector<double>(n));
        for (std::size_t j = 0; j < channels; ++j)
            for (unsigned ear = 0; ear < 2; ++ear) {
                check(fft.inverse({coefficients.data() + (ear * channels + j) * bins, bins}, time));
                for (std::size_t t = 0; t < n; ++t) {
                    const double value = time[(t + n / 2) % n];
                    if (!isFinite(value))
                        return Status::invalid_argument;
                    filters[2 * j + ear][t] = value;
                }
            }
        report = {c.method,
                  n,
                  n,
                  n / 2,
                  std::sqrt(lowError / std::max(1e-30, lowEnergy)),
                  std::sqrt(highError / std::max(1e-30, highEnergy)),
                  std::sqrt(initialHighError / std::max(1e-30, highEnergy)),
                  c.maxRe};
        report.spectralMetricsAvailable = true;
        report.diffuseReferenceEnergy = referenceEnergy;
        report.diffuseDecoderEnergy = decoderEnergy;
        const double a = referenceEnergy[0] + referenceEnergy[1],
                     b = decoderEnergy[0] + decoderEnergy[1];
        if (isFinite(a) && isFinite(b) && a > 1e-30 && b > 1e-30) {
            report.diffuseGain = std::sqrt(a / b);
            report.diffuseMetricsAvailable = true;
        }
        output = std::move(filters);
        return Status::ok;
    } catch (const Failure& f) {
        return f.status;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::invalid_argument;
    }
}
} // namespace cydsp::detail
