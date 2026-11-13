#include "detail/VectorTable.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <limits>
#include <numbers>
#include <stdexcept>
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
        const auto fold = detail::VectorAccess::table(ops).adaaFold;
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
                        !i && !active ? shape(AdaaShape::wavefolder, v)
                        : std::abs(delta) > 1e-5 * std::max({1., std::abs(v), std::abs(previous)})
                            ? (primitive(AdaaShape::wavefolder, v) -
                               primitive(AdaaShape::wavefolder, previous)) /
                                  delta
                            : shape(AdaaShape::wavefolder, .5 * (v + previous));
                    require(std::abs(output[i + 1] - (expected - .25) * .8) < 2e-12,
                            "SIMD ADAA remainder oracle/tails");
                    previous = v;
                }
                require(output.front() == 12345 && output[count + 1] == 12345,
                        "SIMD ADAA respects output bounds");
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
} // namespace
void hotspotRegression() {
    wavefolderKernels();
    saturation<float>();
    saturation<double>();
    dynamics<float>();
    dynamics<double>();
}
