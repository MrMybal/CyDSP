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
    for (auto backend : {CpuBackend::avx2, CpuBackend::avx512, CpuBackend::neon, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) == Status::unsupported_backend)
            continue;
        for (auto type :
             {AdaaShape::wavefolder, AdaaShape::hardClip, AdaaShape::tanh, AdaaShape::cubic}) {
            const auto fold =
                type == AdaaShape::wavefolder ? detail::VectorAccess::table(ops).adaaFold
                : type == AdaaShape::cubic    ? detail::VectorAccess::table(ops).adaaCubic
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
// Independent libm oracles for the exact mean of the shape between two driven
// samples. They use well-conditioned identities, not the kernels' polynomials.
double exactTanhMean(double a, double b) {
    if (a == b)
        return std::tanh(a);
    const double m = .5 * (a + b), h = .5 * (a - b);
    if (std::abs(h) <= 1)
        return std::atanh(std::tanh(m) * std::tanh(h)) / h;
    return ((std::abs(a) - std::abs(b)) +
            (std::log1p(std::exp(-2 * std::abs(a))) - std::log1p(std::exp(-2 * std::abs(b))))) /
           (a - b);
}
double exactAtanMean(double a, double b) {
    if (a == b)
        return 2 / std::numbers::pi * std::atan(a);
    const double m = .5 * (a + b), step = a - b;
    const double turn = std::atan2(step, 1 + a * b);
    const double logarithm = std::abs(a) >= std::abs(b)
                                 ? std::log1p((a - b) * (a + b) / (1 + b * b))
                                 : -std::log1p((b - a) * (b + a) / (1 + a * a));
    return 2 / std::numbers::pi *
           (.5 * (std::atan(a) + std::atan(b)) + (m * turn - .5 * logarithm) / step);
}
template <class T> void saturation() {
    AudioBuffer<T> in, out;
    ok(in.resize(2, 257));
    ok(out.resize(2, 257));
    AdaaSaturator<T> p;
    ok(p.prepare({48000, 257, 2}));
    double previous[2]{}, memory[2]{};
    bool active[2]{};
    // x64 kernels evaluate the exact mean; other backends keep the cached
    // primitive difference with the midpoint limit below a 1e-5 relative step.
    VectorOps selected;
    ok(VectorOps::create(CpuBackend::automatic, selected));
    const bool exactTanh = detail::VectorAccess::table(selected).adaaTanh != nullptr,
               exactAtan = detail::VectorAccess::table(selected).adaaAtan != nullptr;
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
                    if (s.antialias && active[c] && s.shape == AdaaShape::tanh && exactTanh)
                        y = exactTanhMean(x, previous[c]);
                    else if (s.antialias && active[c] && s.shape == AdaaShape::atan && exactAtan)
                        y = exactAtanMean(x, previous[c]);
                    else
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
    for (auto backend : {CpuBackend::avx2, CpuBackend::avx512, CpuBackend::neon, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        if (detail::VectorAccess::kernels<float>(ops).firHead) {
            temporalFir<float>(backend, ops);
            temporalFir<double>(backend, ops);
        }
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
void transcendentalEndpointAccuracy() {
    double maximum[2]{};
    unsigned tested = 0;
    for (auto backend : {CpuBackend::avx2, CpuBackend::avx512, CpuBackend::neon, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        ++tested;
        const auto& table = detail::VectorAccess::table(ops);
        for (int shape = 0; shape < 2; ++shape) {
            const auto kernel = shape ? table.adaaAtan : table.adaaTanh;
            const auto exact = shape ? exactAtanMean : exactTanhMean;
            require(kernel != nullptr, "x64 transcendental ADAA kernel selected");
            for (double initial : {-1e6, -32., -8., -2.4, -1., -.41, -.1, -1e-9, 0., 1e-300, .1, .2,
                                   .67, 1., 1.5, 5.03, 8., 32., 1e6})
                for (double delta : {0., 1e-300, 1e-15, 1e-9, 9.999e-6, 1.0001e-5, .009999, .010001,
                                     .05, .1249, .1251, .26, 1., 3., 70.}) {
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
                        const auto error =
                            std::abs(output[i] - (exact(input[i], previous) + .3) * 16);
                        maximum[shape] = std::max(maximum[shape], error);
                        require(error < 1e-13, "exact ADAA mean for close/far endpoints, gain 16");
                        previous = input[i];
                    }
                }
            // Sign changes, signed zeros, tails, inactive history and output bounds.
            for (std::size_t count = 0; count <= 67; ++count)
                for (bool active : {false, true}) {
                    std::array<double, 70> x{}, output{};
                    output.fill(12345);
                    for (std::size_t i = 0; i < count; ++i)
                        x[i + 1] = i % 11 == 0   ? -0.
                                   : i % 7 == 0  ? 1e6
                                   : i % 5 == 0  ? -1e6
                                   : i % 13 == 0 ? x[i]
                                                 : 3 * std::sin(double(i) * .7 + double(count));
                    kernel(x.data() + 1, output.data() + 1, count, -.7, active, .25, .8);
                    double previous = active ? -.7 : x[1];
                    for (std::size_t i = 0; i < count; ++i) {
                        const auto error =
                            std::abs(output[i + 1] - (exact(x[i + 1], previous) - .25) * .8);
                        maximum[shape] = std::max(maximum[shape], error);
                        require(error < 1e-14, "exact ADAA mean tails/sign changes/first sample");
                        previous = x[i + 1];
                    }
                    require(output.front() == 12345 && output[count + 1] == 12345,
                            "transcendental ADAA respects output bounds");
                }
            const auto direct = shape ? table.shapeAtan : table.shapeTanh;
            require(direct != nullptr, "x64 memoryless shape kernel selected");
            for (std::size_t count = 0; count <= 67; ++count) {
                std::array<double, 70> x{}, output{};
                output.fill(12345);
                for (std::size_t i = 0; i < count; ++i)
                    x[i + 1] = i % 11 == 0  ? -0.
                               : i % 7 == 0 ? 1e6
                               : i % 5 == 0 ? -1e-9
                               : i % 3 == 0 ? .1 * std::sin(double(i))
                                            : 9 * std::sin(double(i) * .7 + double(count));
                direct(x.data() + 1, output.data() + 1, count, .25, .8);
                for (std::size_t i = 0; i < count; ++i) {
                    const double v = x[i + 1];
                    const double expected =
                        shape ? 2 / std::numbers::pi * std::atan(v) : std::tanh(v);
                    require(std::abs(output[i + 1] - (expected - .25) * .8) < 1e-15,
                            "memoryless SIMD shape matches libm");
                }
                require(output.front() == 12345 && output[count + 1] == 12345,
                        "memoryless SIMD shape respects output bounds");
                // In-place evaluation is part of the kernel contract.
                auto inPlace = x;
                direct(inPlace.data() + 1, inPlace.data() + 1, count, .25, .8);
                for (std::size_t i = 0; i < count; ++i)
                    require(inPlace[i + 1] == output[i + 1], "memoryless SIMD shape in place");
            }
        }
    }
    if (tested)
        std::cout << "QUALITY ADAA SIMD exact-mean maxAbsError tanh=" << maximum[0]
                  << " atan=" << maximum[1] << " backends=" << tested << '\n';
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
// Typed tile stages share the scalar operation order, so they are compared
// for exact equality with the processors' scalar expressions.
template <class T> void tileStages(const VectorOps& ops) {
    const auto& kernels = detail::VectorAccess::kernels<T>(ops);
    require(kernels.saturationDrive && kernels.saturationPublish && kernels.magnitude &&
                kernels.applyGains,
            "x64 typed tile stages selected");
    const std::array<double, 5> stage{4, .2, 2.5, 1.3, -.4};
    for (std::size_t count = 0; count <= 67; ++count) {
        std::array<T, 70> input{}, dry{}, output{};
        std::array<double, 70> wide{}, gains{};
        for (std::size_t i = 0; i < count; ++i) {
            input[i + 1] = T(i % 9 == 0 ? -0. : i % 7 == 0 ? 1e7 : 3 * std::sin(.9 * double(i)));
            dry[i + 1] = T(.7 * std::cos(.4 * double(i)));
            gains[i + 1] = i % 5 == 0 ? 1 : .05 + .3 * double(i % 6);
        }
        for (bool staged : {false, true}) {
            wide.fill(12345);
            kernels.saturationDrive(input.data() + 1, wide.data() + 1, count, stage.data(), staged);
            for (std::size_t i = 0; i < count; ++i) {
                double sample = double(input[i + 1]);
                if (staged)
                    sample = (sample * stage[0] + stage[1]) * stage[2];
                require(wide[i + 1] == std::clamp(sample * stage[3] + stage[4], -1e6, 1e6),
                        "vector drive stage equals the scalar stage");
            }
            require(wide.front() == 12345 && wide[count + 1] == 12345, "drive stage bounds");
        }
        for (bool mixed : {false, true}) {
            output.fill(T(12345));
            kernels.saturationPublish(dry.data() + 1, gains.data() + 1, output.data() + 1, count,
                                      .25, .75, mixed);
            for (std::size_t i = 0; i < count; ++i)
                require(output[i + 1] ==
                            (mixed ? T(.25 * double(dry[i + 1]) + .75 * double(T(gains[i + 1])))
                                   : T(gains[i + 1])),
                        "vector publication equals the scalar mix");
            require(output.front() == T(12345) && output[count + 1] == T(12345),
                    "publication bounds");
            for (double ceiling : {.5, std::numeric_limits<double>::infinity()}) {
                output.fill(T(12345));
                kernels.applyGains(input.data() + 1, gains.data() + 1, output.data() + 1, count,
                                   1.4, ceiling, dry.data() + 1, .25, .75, mixed);
                for (std::size_t i = 0; i < count; ++i) {
                    const double value =
                        std::clamp(double(input[i + 1]) * gains[i + 1] * 1.4, -ceiling, ceiling);
                    require(output[i + 1] ==
                                (mixed ? T(.25 * double(dry[i + 1]) + .75 * double(T(value)))
                                       : T(value)),
                            "vector gain application equals the scalar law");
                }
                require(output.front() == T(12345) && output[count + 1] == T(12345),
                        "gain application bounds");
            }
            // Exact in-place application, as used by the dynamics processors.
            auto inPlace = input;
            kernels.applyGains(inPlace.data() + 1, gains.data() + 1, inPlace.data() + 1, count, 1.4,
                               .5, inPlace.data() + 1, .25, .75, mixed);
            for (std::size_t i = 0; i < count; ++i) {
                const double value = std::clamp(double(input[i + 1]) * gains[i + 1] * 1.4, -.5, .5);
                require(
                    inPlace[i + 1] ==
                        (mixed ? T(.25 * double(input[i + 1]) + .75 * double(T(value))) : T(value)),
                    "in-place vector gain application");
            }
        }
        wide.fill(12345);
        kernels.magnitude(input.data() + 1, wide.data() + 1, count, false);
        kernels.magnitude(dry.data() + 1, wide.data() + 1, count, true);
        for (std::size_t i = 0; i < count; ++i)
            require(wide[i + 1] ==
                        std::max(std::abs(double(input[i + 1])), std::abs(double(dry[i + 1]))),
                    "linked detector magnitude");
        require(wide.front() == 12345 && wide[count + 1] == 12345, "magnitude bounds");
    }
}
template <class T> void directFoldStages(const VectorOps& ops) {
    const auto run = detail::VectorAccess::kernels<T>(ops).foldDirect;
    require(run != nullptr, "direct wavefolder SIMD selected");
    const std::array<double, 5> stage{1.5, -.25, 1.2, 3.1, .37};
    const auto dc = shape(AdaaShape::wavefolder, stage[4]);
    std::array<T, 68> input{}, output{};
    for (std::size_t i = 0; i < 68; ++i)
        input[i] = T(i % 7 == 0 ? 1e8 : 17.3 * std::sin(double(i) * .3));
    for (bool mixed : {false, true})
        for (bool alias : {false, true})
            for (std::size_t count = 0; count <= 67; ++count) {
                output.fill(T(12345));
                if (alias)
                    std::copy_n(input.begin(), count, output.begin());
                run(alias ? output.data() : input.data(), output.data(), count, stage.data(), dc,
                    .7, .25, .75, mixed);
                for (std::size_t i = 0; i < count; ++i) {
                    double x = input[i];
                    if (mixed)
                        x = (x * stage[0] + stage[1]) * stage[2];
                    x = std::clamp(x * stage[3] + stage[4], -1e6, 1e6);
                    const auto wet = T((shape(AdaaShape::wavefolder, x) - dc) * .7);
                    const auto expected =
                        mixed ? T(.25 * double(input[i]) + .75 * double(wet)) : wet;
                    require(output[i] == expected,
                            "fused wavefolder rounding, mixing and alias oracle");
                }
                require(output[count] == T(12345), "direct wavefolder tail bounds");
            }
}
template <class T> void signedZeroDcCache() {
    AdaaSaturator<T> p;
    ok(p.prepare({48000, 2, 1}));
    T input[]{T(0), T(-0.)}, output[2]{};
    const T* source[]{input};
    T* destination[]{output};
    for (auto type : {AdaaShape::tanh, AdaaShape::atan, AdaaShape::cubic, AdaaShape::wavefolder})
        for (double bias : {0., -0., 0., .3, -0.}) {
            SaturationParameters settings;
            settings.antialias = false;
            settings.shape = type;
            settings.bias = bias;
            ok(p.setParameters(settings));
            ok(p.process({source, 1, 2}, {destination, 1, 2}));
            for (std::size_t i = 0; i < 2; ++i) {
                const auto expected = T(shape(type, double(input[i]) + bias) - shape(type, bias));
                require(std::abs(double(output[i]) - double(expected)) < 1e-14,
                        "cached DC numerical oracle");
                // A nonzero bias may leave the existing SIMD/libm roundoff
                // residual. Zero bias isolates the actual signed-zero contract.
                if (bias == 0 && expected == T(0))
                    require(output[i] == T(0) && std::signbit(output[i]) == std::signbit(expected),
                            "DC cache preserves signed zero");
            }
        }
}
void curveKernels() {
    double worst = 0;
    unsigned tested = 0;
    for (auto backend : {CpuBackend::avx2, CpuBackend::avx512, CpuBackend::neon, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        ++tested;
        tileStages<float>(ops);
        tileStages<double>(ops);
        directFoldStages<float>(ops);
        directFoldStages<double>(ops);
        const auto& table = detail::VectorAccess::table(ops);
        require(table.expanderGain && table.dynamicEqGain && table.transientGain && table.minimum,
                "x64 gain-curve kernels selected");
        std::array<double, 68> envelope{}, slow{}, output{};
        for (std::size_t i = 0; i < 67; ++i) {
            envelope[i] = i % 13 == 0 ? 0 : std::pow(10., (-250 + 4.1 * double(i)) / 20);
            slow[i] = envelope[i] * (.2 + .03 * double(i));
        }
        const auto close = [&](double actual, double expected, const char* message) {
            const double error = std::abs(actual - expected) / (1 + std::abs(expected));
            worst = std::max(worst, std::abs(actual / expected - 1));
            require(error < 2e-13 && std::abs(actual / expected - 1) < 2e-13, message);
        };
        for (std::size_t count = 0; count <= 67; ++count) {
            for (double threshold : {-60., -24., 0.})
                for (double ratio : {1., 2., 8.}) {
                    output.fill(12345);
                    table.expanderGain(envelope.data(), output.data(), count, threshold, ratio);
                    for (std::size_t i = 0; i < count; ++i) {
                        const double over =
                            20 * std::log10(std::max(1e-12, envelope[i])) - threshold;
                        close(output[i],
                              std::pow(10.,
                                       (over < 0 ? std::max(-160., (ratio - 1) * over) : 0.) / 20),
                              "SIMD expander independent transfer oracle");
                    }
                    require(output[count] == 12345, "expander tail bounds");
                }
            for (double threshold : {-90., -35., -6.})
                for (double range : {-18., 0., 12.}) {
                    output.fill(12345);
                    table.dynamicEqGain(envelope.data(), output.data(), count, threshold, 1.5,
                                        range);
                    for (std::size_t i = 0; i < count; ++i) {
                        const double over =
                            20 * std::log10(std::max(1e-12, envelope[i])) - threshold;
                        const double expected =
                            std::pow(10., (1.5 + range * std::clamp(over / 24., 0., 1.)) / 20);
                        // The kernel returns the weight minus one.
                        close(output[i] + 1, expected, "SIMD dynamic EQ weight oracle");
                    }
                    require(output[count] == 12345, "dynamic EQ tail bounds");
                }
            for (double amount : {-1., .7})
                for (double sustain : {-.3, 1.}) {
                    const std::array<double, 3> parameters{amount, sustain, 24};
                    output.fill(12345);
                    table.transientGain(envelope.data(), slow.data(), output.data(), count,
                                        parameters.data());
                    for (std::size_t i = 0; i < count; ++i) {
                        const double attack = std::clamp(
                            (envelope[i] - slow[i]) / std::max(1e-12, envelope[i]), 0., 1.);
                        const double hold =
                            std::clamp(slow[i] / std::max(1e-12, envelope[i]), 0., 1.) *
                            (1 - attack);
                        close(output[i],
                              std::pow(10.,
                                       24 * std::clamp(amount * attack + sustain * hold, -1., 1.) /
                                           20),
                              "SIMD transient gain oracle");
                    }
                    require(output[count] == 12345, "transient tail bounds");
                    require(table.transientExponent != nullptr,
                            "precise transient SIMD stage available");
                    output.fill(12345);
                    table.transientExponent(envelope.data(), slow.data(), output.data(), count,
                                            parameters.data());
                    for (std::size_t i = 0; i < count; ++i) {
                        const auto attack = std::clamp(
                            (envelope[i] - slow[i]) / std::max(1e-12, envelope[i]), 0., 1.);
                        const auto sustain =
                            std::clamp(slow[i] / std::max(1e-12, envelope[i]), 0., 1.) *
                            (1 - attack);
                        const auto expected =
                            24 * std::clamp(amount * attack + sustain * parameters[1], -1., 1.) /
                            20;
                        require(output[i] == expected,
                                "precise transient scalar pow argument unchanged");
                    }
                    require(output[count] == 12345, "precise transient tail bounds");
                }
            double expected = std::numeric_limits<double>::infinity();
            for (std::size_t i = 0; i < count; ++i)
                expected = std::min(expected, slow[i]);
            require(table.minimum(slow.data(), count) == expected, "SIMD minimum");
        }
    }
    if (tested)
        std::cout << "QUALITY gain-curve SIMD maxRelativeError=" << worst << " backends=" << tested
                  << '\n';
}
// Independent scalar filters and decimal powers verify paired SIMD lanes,
// odd channel counts, mono/stereo sidechains, automation and in-place tiling.
template <class T> void pairedDynamicEq() {
    struct Filter {
        double z1 = 0, z2 = 0;
        double run(double x, const BiquadCoefficients& c) {
            const auto y = c.b0 * x + z1;
            z1 = c.b1 * x - c.a1 * y + z2;
            z2 = c.b2 * x - c.a2 * y;
            return y;
        }
    };
    for (std::size_t channels : {1u, 2u, 3u, 7u}) {
        AudioBuffer<T> input, side, output, alias;
        ok(input.resize(channels, 131));
        ok(side.resize(channels, 131));
        ok(output.resize(channels, 131));
        ok(alias.resize(channels, 131));
        DynamicEqualizer<T> whole, cut;
        ok(whole.prepare({48000, 131, channels}, 3));
        ok(cut.prepare({48000, 131, channels}, 3));
        std::vector<Filter> audio(3 * channels), detector(3 * channels);
        std::vector<double> envelope(3 * channels);
        for (unsigned block = 0; block < 18; ++block) {
            std::array<DynamicEqBand, 3> bands{{{200, 1, -30, -9, 10, 100, 0},
                                                {1200, 2, -35, -12, 5, 80, 1},
                                                {6000, .7, -40, 6, 2, 60, 0}}};
            if (block % 3 == 1)
                bands[1].staticGainDb = 3;
            if (block % 3 == 2) {
                bands[1].frequencyHz = 2300;
                bands[1].attackMs = 0;
            }
            const auto active = block % 4;
            ok(whole.setBands(std::span(bands).first(active)));
            ok(cut.setBands(std::span(bands).first(active)));
            if (block % 7 == 0) {
                whole.reset();
                cut.reset();
                std::fill(audio.begin(), audio.end(), Filter{});
                std::fill(detector.begin(), detector.end(), Filter{});
                std::fill(envelope.begin(), envelope.end(), 0.);
            }
            for (std::size_t ch = 0; ch < channels; ++ch)
                for (std::size_t i = 0; i < 131; ++i) {
                    input.block().channel(ch)[i] =
                        T(.7 * std::sin(.11 * double(block * 131 + i) + double(ch)));
                    side.block().channel(ch)[i] =
                        T(.4 * std::cos(.23 * double(block * 131 + i) - double(ch)));
                    alias.block().channel(ch)[i] = input.block().channel(ch)[i];
                }
            const auto mode = block % 3;
            const T* mono[]{side.block().channel(0).data()};
            const AudioBlock<const T> external =
                mode == 1 ? AudioBlock<const T>{mono, 1, 131} : side.block();
            const AudioBlock<const T> detectorBlock = mode == 0 ? input.block() : external;
            ok(whole.process(input.block(), detectorBlock, output.block()));
            for (std::size_t offset = 0; offset < 131; offset += 17) {
                const auto count = std::min<std::size_t>(17, 131 - offset);
                auto inPlace = alias.block().subBlock(offset, count);
                const AudioBlock<const T> sideBlock =
                    mode == 0 ? AudioBlock<const T>{inPlace} : external.subBlock(offset, count);
                ok(cut.process(inPlace, sideBlock, inPlace));
            }
            std::array<BiquadCoefficients, 3> coefficients;
            for (std::size_t b = 0; b < active; ++b)
                ok(designBiquad(BiquadType::bandPass, 48000, bands[b].frequencyHz, bands[b].q,
                                coefficients[b]));
            for (std::size_t ch = 0; ch < channels; ++ch)
                for (std::size_t i = 0; i < 131; ++i) {
                    double x = input.block().channel(ch)[i];
                    const double sideValue = detectorBlock.channel(mode == 1 ? 0 : ch)[i];
                    for (std::size_t b = 0; b < active; ++b) {
                        const auto id = b * channels + ch;
                        const auto& band = bands[b];
                        const auto level = std::abs(detector[id].run(sideValue, coefficients[b]));
                        const auto attack =
                            band.attackMs == 0 ? 0 : std::exp(-1000 / (band.attackMs * 48000));
                        const auto release = std::exp(-1000 / (band.releaseMs * 48000));
                        auto& env = envelope[id];
                        env = level + (level > env ? attack : release) * (env - level);
                        const auto over = 20 * std::log10(std::max(1e-12, env)) - band.thresholdDb;
                        const auto db =
                            band.staticGainDb + band.rangeDb * std::clamp(over / 24., 0., 1.);
                        x += (std::pow(10., db / 20) - 1) * audio[id].run(x, coefficients[b]);
                    }
                    const auto actual = output.block().channel(ch)[i];
                    require(std::abs(double(actual) - double(T(x))) <
                                (std::same_as<T, float> ? 3e-6 : 3e-12),
                            "paired dynamic EQ independent filter/envelope oracle");
                    require(actual == alias.block().channel(ch)[i],
                            "paired EQ in-place and block cuts");
                }
            auto invalid = bands;
            invalid.back().frequencyHz = std::numeric_limits<double>::quiet_NaN();
            require(whole.setBands(invalid) == Status::invalid_argument,
                    "cached EQ validates all bands");
        }
    }
}
// A constant channel takes a shortcut inside the tiled saturator. It must equal
// the general tile path, which blocks shorter than eight frames always use.
template <class T> void constantChannelSaturation() {
    AudioBuffer<T> in, whole, pieces;
    ok(in.resize(2, 71));
    ok(whole.resize(2, 71));
    ok(pieces.resize(2, 71));
    for (auto shape : {AdaaShape::tanh, AdaaShape::atan, AdaaShape::hardClip, AdaaShape::wavefolder,
                       AdaaShape::cubic, AdaaShape::hysteresis})
        for (bool antialias : {false, true})
            for (double level : {0., -0., .37, -2.5}) {
                AdaaSaturator<T> a, b;
                ok(a.prepare({48000, 71, 2}));
                ok(b.prepare({48000, 71, 2}));
                SaturationParameters p;
                p.shape = shape;
                p.antialias = antialias;
                p.drive = 2.5;
                p.bias = .1;
                p.outputGain = .9;
                ok(a.setParameters(p));
                ok(b.setParameters(p));
                for (int block = 0; block < 3; ++block) {
                    for (std::size_t ch = 0; ch < 2; ++ch)
                        for (std::size_t i = 0; i < 71; ++i)
                            in.block().channel(ch)[i] =
                                block == 1 ? T(.8 * std::sin(.3 * double(i) + double(ch)))
                                           : T(ch ? -level : level);
                    ok(a.processMixed(in.block(), whole.block(), {1.5, .05, 1}, .25, .75));
                    for (std::size_t offset = 0; offset < 71; offset += 7)
                        ok(b.processMixed(
                            in.block().subBlock(offset, std::min<std::size_t>(7, 71 - offset)),
                            pieces.block().subBlock(offset, std::min<std::size_t>(7, 71 - offset)),
                            {1.5, .05, 1}, .25, .75));
                    for (std::size_t ch = 0; ch < 2; ++ch)
                        for (std::size_t i = 0; i < 71; ++i)
                            require(whole.block().channel(ch)[i] == pieces.block().channel(ch)[i],
                                    "constant-channel saturation equals the general tile path");
                }
            }
}
} // namespace
void hotspotRegression() {
    audioMathAccuracy();
    transferKernels();
    curveKernels();
    signedZeroDcCache<float>();
    signedZeroDcCache<double>();
    pairedDynamicEq<float>();
    pairedDynamicEq<double>();
    transcendentalEndpointAccuracy();
    coefficientAndMaterialAccuracy();
    wavefolderKernels();
    saturation<float>();
    saturation<double>();
    constantChannelSaturation<float>();
    constantChannelSaturation<double>();
    dynamics<float>();
    dynamics<double>();
}
