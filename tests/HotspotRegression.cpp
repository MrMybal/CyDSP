#include "detail/AudioMath.h"
#include "detail/VectorTable.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <vector>
using namespace cydsp;
namespace {
void require(bool v, const char* m) {
    if (!v)
        throw std::runtime_error(m);
}
void ok(Status s) {
    require(s == Status::ok, "hotspot regression status");
}
double shape(AdaaShape type, double x) {
    switch (type) {
    case AdaaShape::atan:
        return 2 / std::numbers::pi * std::atan(x);
    case AdaaShape::cubic:
        return std::abs(x) <= 1 ? x - x * x * x / 3 : std::copysign(2. / 3, x);
    case AdaaShape::hardClip:
        return std::clamp(x, -1., 1.);
    case AdaaShape::wavefolder: {
        double r = std::remainder(x, 4.);
        return std::abs(r) <= 1 ? r : std::copysign(2 - std::abs(r), r);
    }
    default:
        return std::tanh(x);
    }
}
double primitive(AdaaShape type, double x) {
    double a = std::abs(x);
    switch (type) {
    case AdaaShape::atan:
        return 2 / std::numbers::pi * (x * std::atan(x) - .5 * std::log1p(x * x));
    case AdaaShape::cubic:
        return a <= 1 ? .5 * x * x - x * x * x * x / 12 : 2 * a / 3 - .25;
    case AdaaShape::hardClip:
        return a <= 1 ? .5 * x * x : a - .5;
    case AdaaShape::wavefolder: {
        double r = std::abs(std::remainder(x, 4.));
        return r <= 1 ? .5 * r * r : 2 * r - .5 * r * r - 1;
    }
    default:
        return a + std::log1p(std::exp(-2 * a)) - std::numbers::ln2;
    }
}
void wavefolderKernels() {
    for (auto backend : {CpuBackend::avx2, CpuBackend::avx512}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) == Status::unsupported_backend)
            continue;
        for (auto type : {AdaaShape::wavefolder, AdaaShape::hardClip, AdaaShape::tanh}) {
            const auto fold =
                type == AdaaShape::wavefolder ? detail::VectorAccess::table(ops).adaaFold
                : type == AdaaShape::hardClip ? detail::VectorAccess::table(ops).adaaClip
                                              : detail::VectorAccess::table(ops).adaaTanh;
            require(fold != nullptr, "x64 ADAA kernel selected");
            for (std::size_t count = 0; count <= 67; ++count)
                for (bool active : {false, true}) {
                    std::array<double, 70> x{}, output{};
                    output.fill(12345);
                    for (std::size_t i = 0; i < count; ++i)
                        x[i + 1] = i % 9 == 0   ? std::nextafter(2., 0.)
                                   : i % 7 == 0 ? 1e6
                                   : i % 5 == 0 ? -1e6
                                                : 4 * std::sin(double(i) * .7);
                    fold(x.data() + 1, output.data() + 1, count, -.7, active, .25, .8);
                    double previous = -.7;
                    for (std::size_t i = 0; i < count; ++i) {
                        const double v = x[i + 1], delta = v - previous;
                        const double expected =
                            !i && !active ? shape(type, v)
                            : std::abs(delta) >
                                    1e-5 * std::max({1., std::abs(v), std::abs(previous)})
                                ? (primitive(type, v) - primitive(type, previous)) / delta
                                : shape(type, .5 * (v + previous));
                        require(std::abs(output[i + 1] - (expected - .25) * .8) < 2e-12,
                                "SIMD ADAA remainder oracle/tails");
                        previous = v;
                    }
                    require(output.front() == 12345 && output[count + 1] == 12345,
                            "SIMD ADAA respects output bounds");
                }
        }
    }
}
template <class T> void saturation() {
    AudioBuffer<T> in, out;
    ok(in.resize(2, 257));
    ok(out.resize(2, 257));
    AdaaSaturator<T> p;
    ok(p.prepare({48000, 257, 2}));
    double previous[2]{}, memory[2]{};
    bool active[2]{};
    for (int block = 0; block < 120; ++block) {
        SaturationParameters s;
        s.shape = AdaaShape((block / 3) % 6);
        s.antialias = block % 7 != 0;
        s.drive = block % 4 == 0 ? 64 : 1.3;
        s.bias = .7 * std::sin(block);
        s.outputGain = .8;
        ok(p.setParameters(s));
        if (block % 19 == 0) {
            p.reset();
            std::fill_n(previous, 2, 0);
            std::fill_n(memory, 2, 0);
            std::fill_n(active, 2, false);
        }
        for (std::size_t c = 0; c < 2; ++c)
            for (std::size_t i = 0; i < 257; ++i) {
                const double x = block % 11 == 0
                                     ? .125
                                     : 8 * std::sin(double(block * 257 + i) * .31 + double(c));
                in.block().channel(c)[i] = T(x);
            }
        // Include exact folds, adjacent representable boundary values and clamp extremes.
        for (std::size_t i = 0; i < 8; ++i)
            in.block().channel(0)[i] = T(std::array<double, 8>{0, 1, -1, 2, -2, 4, 1e8, -1e8}[i]);
        ok(p.process(in.block(), out.block()));
        for (std::size_t c = 0; c < 2; ++c)
            for (std::size_t i = 0; i < 257; ++i) {
                const double x =
                    std::clamp(double(in.block().channel(c)[i]) * s.drive + s.bias, -1e6, 1e6);
                double y;
                if (s.shape == AdaaShape::hysteresis) {
                    memory[c] = std::clamp(memory[c], x - s.hysteresisWidth, x + s.hysteresisWidth);
                    y = std::tanh(x + s.hysteresisAmount * memory[c]) - std::tanh(s.bias);
                } else {
                    const double delta = x - previous[c];
                    y = s.antialias && active[c]
                            ? (std::abs(delta) >
                                       1e-5 * std::max({1., std::abs(x), std::abs(previous[c])})
                                   ? (primitive(s.shape, x) - primitive(s.shape, previous[c])) /
                                         delta
                                   : shape(s.shape, .5 * (x + previous[c])))
                            : shape(s.shape, x);
                    y -= shape(s.shape, s.bias);
                }
                const T expected = T(y * s.outputGain);
                require(std::abs(double(out.block().channel(c)[i]) - double(expected)) <
                            (std::same_as<T, float> ? 2e-6 : 2e-12),
                        "ADAA primitive cache/automation differs from uncached integral");
                previous[c] = x;
                active[c] = true;
            }
        const auto before = out.block().channel(0)[0];
        in.block().channel(1)[256] = std::numeric_limits<T>::quiet_NaN();
        require(p.process(in.block(), out.block()) == Status::invalid_argument &&
                    out.block().channel(0)[0] == before,
                "ADAA invalid input must be transactional");
    }
}
template <class T> void dynamics() {
    AudioBuffer<T> in, out, side;
    ok(in.resize(2, 257));
    ok(out.resize(2, 257));
    ok(side.resize(2, 257));
    Dynamics<T> p;
    ok(p.prepare({48000, 257, 2}, DynamicsMode::compressor, 10));
    double values[2]{};
    for (int b = 0; b < 80; ++b) {
        DynamicsParameters s;
        s.thresholdDb = -24 + (b % 5);
        s.ratio = 1 + (b % 7);
        s.kneeDb = b % 3 ? 6 : 0;
        s.makeupDb = 3;
        s.linkedChannels = (b / 4) % 2 == 0;
        s.detector = b % 3 ? DetectorMode::peak : DetectorMode::rms;
        s.attackMs = b % 4;
        s.releaseMs = 10 + b;
        ok(p.setParameters(s));
        if (b % 13 == 0) {
            p.reset();
            values[0] = values[1] = 0;
        }
        for (std::size_t c = 0; c < 2; ++c)
            for (std::size_t i = 0; i < 257; ++i) {
                in.block().channel(c)[i] = T(.7 * std::sin(double(b * 257 + i) * .27 + c));
                side.block().channel(c)[i] =
                    T((b % 9 == 0 ? 1e-8 : .3) * std::cos(double(b * 257 + i) * .13 + c));
            }
        ok(p.process(in.block(), side.block(), out.block()));
        double lastDb = 0, minimumGain = 1e300;
        const double attack = s.attackMs ? std::exp(-1000. / s.attackMs / 48000) : 0,
                     release = std::exp(-1000. / s.releaseMs / 48000);
        for (std::size_t i = 0; i < 257; ++i)
            for (std::size_t c = 0; c < 2; ++c) {
                const double mag = s.linkedChannels
                                       ? std::max(std::abs(double(side.block().channel(0)[i])),
                                                  std::abs(double(side.block().channel(1)[i])))
                                       : std::abs(double(side.block().channel(c)[i]));
                const double det = s.detector == DetectorMode::rms ? mag * mag : mag;
                values[c] = det + (det > values[c] ? attack : release) * (values[c] - det);
                double env = s.detector == DetectorMode::rms ? std::sqrt(std::max(0., values[c]))
                                                             : values[c];
                double over = 20 * std::log10(std::max(1e-12, env)) - s.thresholdDb, db = 0,
                       slope = 1 / s.ratio - 1, half = s.kneeDb * .5;
                if (s.kneeDb > 0 && std::abs(over) < half) {
                    const double t = over + half;
                    db = slope * t * t / (2 * s.kneeDb);
                } else if (over >= half)
                    db = slope * over;
                minimumGain =
                    std::min(minimumGain, std::pow(10., db / 20) * std::pow(10., s.makeupDb / 20));
                const T expected = T(double(in.block().channel(c)[i]) * std::pow(10., db / 20) *
                                     std::pow(10., s.makeupDb / 20));
                require(std::abs(double(out.block().channel(c)[i]) - double(expected)) <
                            (std::same_as<T, float> ? 1e-6 : 1e-12),
                        "linked dynamics changes detector history/transfer curve");
                lastDb = db;
            }
        require(std::abs(p.gainReductionDb() - lastDb) < 1e-12, "linked dynamics meter");
        require(std::abs(p.minimumAppliedGain() - minimumGain) < 1e-12,
                "minimum block detector gain");
        const auto meter = p.gainReductionDb();
        ok(p.process(in.block(0), side.block(0), out.block(0)));
        require(p.gainReductionDb() == meter, "empty block preserves gain meter");
    }
}
template <class T> void temporalFir(CpuBackend backend, const VectorOps& ops) {
    const auto fir = detail::VectorAccess::kernels<T>(ops).firHead;
    if (!fir)
        return;
    for (std::size_t frames = 0; frames <= 67; ++frames)
        for (std::size_t taps : {1u, 2u, 3u, 4u, 7u, 32u, 128u, 131u}) {
            std::vector<T> input(frames + taps - 1), impulse(taps), output(frames + 2, T(12345));
            for (std::size_t i = 0; i < input.size(); ++i)
                input[i] = T(.3 * std::sin(i * .37));
            for (std::size_t i = 0; i < taps; ++i)
                impulse[i] = T(.1 * std::cos(i * .21));
            std::fill(output.begin() + 1, output.end() - 1, T(.25));
            fir(input.data(), impulse.data(), output.data() + 1, frames, taps);
            for (std::size_t f = 0; f < frames; ++f) {
                long double expected = .25;
                for (std::size_t k = 0; k < taps; ++k)
                    expected += static_cast<long double>(input[f + k]) * impulse[k];
                require(std::abs(double(output[f + 1]) - double(expected)) <
                            (std::same_as<T, float> ? 5e-7 : 3e-14),
                        "temporal FIR independent convolution oracle");
            }
            require(output.front() == T(12345) && output.back() == T(12345),
                    "temporal FIR output canaries");
        }
    (void)backend;
}
void transferKernels() {
    for (auto backend : {CpuBackend::avx2, CpuBackend::avx512}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        temporalFir<float>(backend, ops);
        temporalFir<double>(backend, ops);
        const auto curve = detail::VectorAccess::table(ops).compressorGain;
        require(curve != nullptr, "linked transfer SIMD kernel selected");
        for (double threshold : {-160., -24., 24.})
            for (double ratio : {1., 2., 4., 6., 12., 1000.})
                for (double knee : {0., 6., 60.})
                    for (bool limiter : {false, true}) {
                        std::array<double, 67> input{}, output{};
                        for (std::size_t i = 0; i < input.size(); ++i) {
                            const auto db = i < 32 ? threshold - knee + 2 * knee * i / 31
                                                   : -240 + 2240. * (i - 32) / 34;
                            input[i] = std::pow(10., db / 20);
                        }
                        for (std::size_t count = 0; count <= input.size(); ++count) {
                            output.fill(12345);
                            curve(input.data(), output.data(), count, threshold, ratio, knee,
                                  limiter);
                            for (std::size_t i = 0; i < count; ++i) {
                                const auto over =
                                    20 * std::log10(std::max(1e-12, input[i])) - threshold;
                                const auto slope = limiter ? -1 : 1 / ratio - 1;
                                const auto db = knee > 0 && std::abs(over) < knee * .5
                                                    ? slope * (over + knee * .5) *
                                                          (over + knee * .5) / (2 * knee)
                                                    : slope * std::max(0., over);
                                require(std::abs(output[i] - std::pow(10., db / 20)) < 2e-13,
                                        "SIMD compression knee/ratio independent transfer oracle");
                            }
                            if (count < input.size())
                                require(output[count] == 12345, "SIMD compression tail bounds");
                        }
                    }
    }
}
void tanhEndpointAccuracy() {
    double maximum = 0;
    unsigned tested = 0;
    for (auto backend : {CpuBackend::avx2, CpuBackend::avx512}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        ++tested;
        const auto kernel = detail::VectorAccess::table(ops).adaaTanh;
        for (double initial : {-1e6, -32., -8., -1., -.1, 0., .1, 1., 8., 32., 1e6})
            for (double delta : {0., 9.999e-6, 1.0001e-5, .009999, .010001, .05, 1.}) {
                std::array<double, 67> input{}, output{};
                double previous = initial;
                for (std::size_t i = 0; i < input.size(); ++i) {
                    input[i] = std::clamp(previous + (i % 3 ? 1 : -1) * delta *
                                                         std::max(1., std::abs(previous)),
                                          -1e6, 1e6);
                    previous = input[i];
                }
                kernel(input.data(), output.data(), input.size(), initial, true, -.3, 16);
                previous = initial;
                for (std::size_t i = 0; i < input.size(); ++i) {
                    const auto d = input[i] - previous;
                    const auto value =
                        std::abs(d) > 1e-5 * std::max({1., std::abs(input[i]), std::abs(previous)})
                            ? (primitive(AdaaShape::tanh, input[i]) -
                               primitive(AdaaShape::tanh, previous)) /
                                  d
                            : std::tanh(.5 * (input[i] + previous));
                    const auto error = std::abs(output[i] - (value + .3) * 16);
                    maximum = std::max(maximum, error);
                    require(error < 2e-12, "tanh close endpoints/high gain independent integral");
                    previous = input[i];
                }
            }
    }
    if (tested)
        std::cout << "QUALITY tanh SIMD maxAbsError=" << maximum << " backends=" << tested << '\n';
}
void coefficientAndMaterialAccuracy() {
    for (double frequency : {10., 500., 2800., 16000., 23900.})
        for (double q : {.1, .7, 10.})
            for (double gain : {-120., -24., -.000001, 0., 6., 120.}) {
                BiquadCoefficients actual;
                ok(designPeakingBiquad(48000, frequency, q, gain, actual));
                const auto angle = 2 * std::numbers::pi * frequency / 48000;
                const auto a = std::pow(10., gain / 40), alpha = std::sin(angle) / (2 * q);
                const double a0 = 1 + alpha / a;
                const BiquadCoefficients reference{(1 + alpha * a) / a0, -2 * std::cos(angle) / a0,
                                                   (1 - alpha * a) / a0, -2 * std::cos(angle) / a0,
                                                   (1 - alpha / a) / a0};
                const double values[]{actual.b0, actual.b1, actual.b2, actual.a1, actual.a2};
                const double expected[]{reference.b0, reference.b1, reference.b2, reference.a1,
                                        reference.a2};
                for (unsigned i = 0; i < 5; ++i)
                    require(std::abs(values[i] - expected[i]) <
                                8e-14 * std::max(1., std::abs(expected[i])),
                            "EQ coefficients preserve RBJ reference and stability");
                require(actual.isStable(), "optimized EQ design remains stable");
            }
    for (double absorption : {0., 1e-12, .2, .7, 1.})
        for (double loss : {0., 1e-12, 6., 24., 300.})
            for (double scale : {0., .5, 1., 2., 3.7, 1e6}) {
                AcousticMaterial m;
                m.absorption.fill(absorption);
                m.transmissionLossDb.fill(loss);
                MaterialResponse actual;
                ok(materialResponse(m, m.referenceThicknessMetres * scale, actual));
                const auto remaining = 1 - (1 - std::pow(1 - absorption, scale));
                const auto transmission = std::min(remaining, std::pow(10., -loss * scale / 10));
                require(std::abs(actual.transmission[0] - std::sqrt(transmission)) < 2e-12 &&
                            std::abs(actual.reflection[0] -
                                     std::sqrt(std::max(0., remaining - transmission) *
                                               (1 - m.scattering))) < 2e-12,
                        "material response edge/energy reference");
            }
}
void audioMathAccuracy() {
    for (int i = 0; i <= 200000; ++i) {
        const double x = -700 + 1400. * i / 200000;
        const double reference = std::exp(x), actual = detail::audioExp(x);
        require(std::abs(actual / reference - 1) < 8e-14, "range-reduced exp accuracy");
        const double logarithm = std::log(reference);
        require(std::abs(detail::audioLog(reference) - logarithm) <
                    8e-15 * std::max(1., std::abs(logarithm)),
                "range-reduced log accuracy");
    }
    for (unsigned i = 0; i <= 128; ++i) {
        const double centre = 1 + double(i) / 128;
        for (double x : {std::nextafter(centre, 0.), centre, std::nextafter(centre, 3.)})
            require(std::abs(detail::audioLog(x) - std::log(x)) < 4e-16,
                    "log table boundary accuracy");
    }
    require(detail::audioLog(1) == 0 && detail::audioExp(0) == 1 &&
                std::isinf(detail::audioLog(0)) && std::isnan(detail::audioLog(-1)) &&
                detail::audioLog(std::numeric_limits<double>::denorm_min()) ==
                    std::log(std::numeric_limits<double>::denorm_min()),
            "numerical kernel exceptional-value fallbacks");
}
} // namespace
void hotspotRegression() {
    audioMathAccuracy();
    transferKernels();
    tanhEndpointAccuracy();
    coefficientAndMaterialAccuracy();
    wavefolderKernels();
    saturation<float>();
    saturation<double>();
    dynamics<float>();
    dynamics<double>();
}
