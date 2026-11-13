#include "cydsp/analysis/ImpulseAnalysis.h"
#include "cydsp/processors/Biquad.h"
#include "detail/Analysis.h"
#include <algorithm>
#include <cmath>
#include <vector>
namespace cydsp {
namespace {
DecayFit fitDecay(std::span<const double> db, double rate, double upper, double lower) noexcept {
    DecayFit fit;
    double n = 0, sx = 0, sy = 0, sxx = 0, sxy = 0, syy = 0;
    bool reached = false;
    for (std::size_t i = 0; i < db.size(); ++i) {
        if (db[i] <= lower) {
            reached = true;
            break;
        }
        if (db[i] > upper)
            continue;
        const double x = double(i) / rate, y = db[i];
        ++n;
        sx += x;
        sy += y;
        sxx += x * x;
        sxy += x * y;
        syy += y * y;
    }
    const double xx = n * sxx - sx * sx, yy = n * syy - sy * sy, xy = n * sxy - sx * sy;
    if (!reached || n < 8 || xx < 1e-20 || yy < 1e-20 || xy >= 0)
        return fit;
    fit.seconds = -60 * xx / xy;
    fit.rSquared = std::clamp(xy * xy / (xx * yy), 0., 1.);
    fit.valid = isFinite(fit.seconds) && fit.seconds > 0 && fit.seconds < 600 && fit.rSquared >= .8;
    return fit;
}
ImpulseBandAnalysis decay(std::span<const double> x, double rate, double center,
                          bool subtractNoise) {
    ImpulseBandAnalysis result;
    result.centerHz = center;
    std::vector<double> integration(x.size());
    double noise = 0;
    if (subtractNoise) {
        const auto count = std::max<std::size_t>(1, x.size() / 10);
        for (std::size_t i = x.size() - count; i < x.size(); ++i)
            noise += x[i] * x[i];
        noise /= double(count);
    }
    detail::CompensatedSum energy;
    for (std::size_t i = x.size(); i-- > 0;) {
        energy.add(x[i] * x[i]);
        integration[i] = std::max(0., energy.value() - noise * double(x.size() - i));
    }
    result.energy = energy.value();
    const double total = integration.empty() ? 0 : integration[0];
    if (total < 1e-100)
        return result;
    for (auto& e : integration)
        e = 10 * std::log10(std::max(1e-100, e / total));
    result.edt = fitDecay(integration, rate, 0, -10);
    result.t20 = fitDecay(integration, rate, -5, -25);
    result.t30 = fitDecay(integration, rate, -5, -35);
    return result;
}
double ratioDb(double a, double b) noexcept {
    return std::clamp(10 * std::log10(std::max(a, 1e-100) / std::max(b, 1e-100)), -200., 200.);
}
template <Sample T>
Status analyze(std::span<const T> ir, double rate, ImpulseReport& output,
               const ImpulseAnalysisConfig& config) noexcept {
    if (ir.empty() || ir.size() > (1u << 26) || !isFinite(rate) || rate < 8000 || rate > 768000 ||
        !isFinite(config.onsetThresholdDb) || config.onsetThresholdDb < -100 ||
        config.onsetThresholdDb > -3 || !isFinite(config.directWindowMs) ||
        config.directWindowMs < .01 || config.directWindowMs > 50 ||
        !isFinite(config.reflectionThresholdDb) || config.reflectionThresholdDb < -100 ||
        config.reflectionThresholdDb > 0)
        return Status::invalid_argument;
    double peak = 0;
    std::size_t peakAt = 0;
    for (std::size_t i = 0; i < ir.size(); ++i) {
        if (!isFinite(ir[i]) || std::abs(double(ir[i])) > 1e50)
            return Status::invalid_argument;
        if (std::abs(double(ir[i])) > peak) {
            peak = std::abs(double(ir[i]));
            peakAt = i;
        }
    }
    try {
        ImpulseReport r;
        r.noiseCompensated = config.subtractTailNoise;
        constexpr std::array<double, 9> centers{63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
        for (std::size_t b = 0; b < centers.size(); ++b)
            r.octaveBands[b].centerHz = centers[b];
        if (peak == 0) {
            r.silent = true;
            output = r;
            return Status::ok;
        }
        const double threshold = peak * std::pow(10., config.onsetThresholdDb / 20);
        std::size_t onset = 0;
        while (onset + 1 < ir.size() && std::abs(double(ir[onset])) < threshold)
            ++onset;
        r.onsetSeconds = double(onset) / rate;
        r.peakSeconds = double(peakAt) / rate;
        std::vector<double> signal(ir.size() - onset), filtered(signal.size());
        double early50 = 0, early80 = 0, direct = 0, timeEnergy = 0;
        for (std::size_t i = 0; i < signal.size(); ++i) {
            signal[i] = double(ir[onset + i]);
            const double e = signal[i] * signal[i], t = double(i) / rate;
            r.energy += e;
            timeEnergy += t * e;
            if (t < .05)
                early50 += e;
            if (t < .08)
                early80 += e;
            if (t < config.directWindowMs / 1000)
                direct += e;
        }
        r.centerTimeSeconds = timeEnergy / r.energy;
        r.clarity50Db = ratioDb(early50, std::max(0., r.energy - early50));
        r.clarity80Db = ratioDb(early80, std::max(0., r.energy - early80));
        r.definition50 = early50 / r.energy;
        r.directToReverberantDb = ratioDb(direct, std::max(0., r.energy - direct));
        r.broadband = decay(signal, rate, 0, config.subtractTailNoise);
        const double reflectionThreshold = peak * std::pow(10., config.reflectionThresholdDb / 20);
        std::size_t last = 0;
        for (std::size_t i = 1; i + 1 < signal.size() && double(i) / rate < .08; ++i)
            if (double(i) / rate > config.directWindowMs / 1000 &&
                i - last >= std::size_t(rate * .001) &&
                std::abs(signal[i]) >= reflectionThreshold &&
                std::abs(signal[i]) > std::abs(signal[i - 1]) &&
                std::abs(signal[i]) >= std::abs(signal[i + 1])) {
                if (r.reflectionCount == r.reflections.size())
                    break;
                r.reflections[r.reflectionCount++] = {double(i) / rate, signal[i]};
                last = i;
            }
        for (std::size_t b = 0; b < centers.size(); ++b) {
            if (centers[b] * std::sqrt(2.) >= rate * .5)
                continue;
            BiquadCoefficients c;
            auto status = designBiquad(BiquadType::bandPass, rate, centers[b], std::sqrt(2.), c);
            if (status != Status::ok)
                return status;
            double z1 = 0, z2 = 0;
            for (std::size_t i = 0; i < signal.size(); ++i) {
                const double y = c.b0 * signal[i] + z1;
                z1 = c.b1 * signal[i] - c.a1 * y + z2;
                z2 = c.b2 * signal[i] - c.a2 * y;
                filtered[i] = y;
            }
            r.octaveBands[b] = decay(filtered, rate, centers[b], config.subtractTailNoise);
        }
        output = r;
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
double best(const ImpulseBandAnalysis& b) noexcept {
    return b.t30.valid   ? b.t30.seconds
           : b.t20.valid ? b.t20.seconds
           : b.edt.valid ? b.edt.seconds
                         : 0;
}
} // namespace
Status analyzeImpulse(std::span<const float> ir, double fs, ImpulseReport& r,
                      const ImpulseAnalysisConfig& c) noexcept {
    return analyze(ir, fs, r, c);
}
Status analyzeImpulse(std::span<const double> ir, double fs, ImpulseReport& r,
                      const ImpulseAnalysisConfig& c) noexcept {
    return analyze(ir, fs, r, c);
}
Status matchFdnToImpulse(const ImpulseReport& r, FdnParameters& output) noexcept {
    if (r.silent || !isFinite(r.energy) || r.energy <= 0 ||
        r.reflectionCount > r.reflections.size())
        return Status::invalid_argument;
    if (r.reflectionCount && (!isFinite(r.reflections[0].seconds) || r.reflections[0].seconds < 0))
        return Status::invalid_argument;
    FdnParameters p;
    bool any = false;
    for (std::size_t band = 0; band < 3; ++band) {
        double sum = 0, count = 0;
        for (std::size_t j = 0; j < 3; ++j) {
            const double t = best(r.octaveBands[band * 3 + j]);
            if (isFinite(t) && t > 0) {
                sum += t;
                ++count;
            }
        }
        double t = count ? sum / count : best(r.broadband);
        if (!isFinite(t) || t <= 0)
            t = 1.5;
        else
            any = true;
        p.decaySeconds[band] = std::clamp(t, .05, 30.);
    }
    if (!any || !isFinite(r.directToReverberantDb))
        return Status::unsupported_format;
    p.preDelayMs = r.reflectionCount ? std::clamp(r.reflections[0].seconds * 1000, 0., 250.) : 0;
    const double directFraction =
        1 / (1 + std::pow(10., -std::clamp(r.directToReverberantDb, -200., 200.) / 10));
    p.wet = std::sqrt(1 - directFraction);
    p.dry = std::sqrt(directFraction);
    output = p;
    return Status::ok;
}
} // namespace cydsp
