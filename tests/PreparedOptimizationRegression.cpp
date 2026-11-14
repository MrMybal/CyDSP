#include "detail/VectorTable.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cydsp/CyDSP.h>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <vector>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool v, const char* m) {
    if (!v)
        throw std::runtime_error(m);
}
void ok(Status s) {
    require(s == Status::ok, "prepared optimization status");
}
bool same(const BiquadCoefficients& a, const BiquadCoefficients& b) {
    return a.b0 == b.b0 && a.b1 == b.b1 && a.b2 == b.b2 && a.a1 == b.a1 && a.a2 == b.a2;
}
template <class T> void fftQuality() {
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::avx512,
                         CpuBackend::neon, CpuBackend::sve}) {
        VectorOps probe;
        if (VectorOps::create(backend, probe) != Status::ok)
            continue;
        for (std::size_t n = 2; n <= 2048; n *= 2) {
            RealFft<T> fft;
            ok(fft.prepare(n, backend));
            std::vector<T> in(n), restored(n);
            std::vector<std::complex<T>> spectrum(n / 2 + 1);
            for (std::size_t i = 0; i < n; ++i)
                in[i] = T(.3 * std::sin(.173 * double(i)) + .2 * std::cos(.29 * double(i)));
            ok(fft.forward(in, spectrum));
            for (std::size_t j = 0; j < 17; ++j) {
                const auto k = (n / 2) * j / 16;
                std::complex<long double> expected{};
                for (std::size_t i = 0; i < n; ++i) {
                    const auto angle =
                        -2 * std::numbers::pi_v<long double> * static_cast<long double>(k) * i / n;
                    expected += static_cast<long double>(in[i]) *
                                std::complex<long double>(std::cos(angle), std::sin(angle));
                }
                const auto error = std::abs(std::complex<long double>(spectrum[k]) - expected);
                const auto tolerance = std::same_as<T, float> ? 2e-5L : 1e-11L;
                require(error < tolerance * (1 + std::abs(expected)),
                        "fused FFT independent long-double DFT");
            }
            require(spectrum.front().imag() == 0 && spectrum.back().imag() == 0,
                    "real FFT endpoint contract");
            ok(fft.inverse(spectrum, restored));
            for (std::size_t i = 0; i < n; ++i)
                require(std::abs(double(restored[i] - in[i])) <
                            (std::same_as<T, float> ? 5e-7 : 2e-14),
                        "fused FFT inverse normalization");
        }
    }
}
template <class T> void directClipKernels() {
    for (auto backend : {CpuBackend::avx2, CpuBackend::avx512}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        const auto kernel = detail::VectorAccess::kernels<T>(ops).clipDirect;
        require(kernel != nullptr, "direct clip backend registered");
        for (std::size_t n : {1, 3, 4, 7, 16, 63, 64, 65, 257})
            for (double drive : {.001, 1., 3., 64.})
                for (double bias : {-4., -0., 0., .2, 4.})
                    for (double gain : {-16., .25, 1., 16.}) {
                        std::vector<T> input(n), output(n + 2, T(7));
                        const auto dc = std::clamp(bias, -1., 1.);
                        for (std::size_t i = 0; i < n; ++i)
                            input[i] = i % 7 == 0    ? T(-0.)
                                       : i % 11 == 0 ? T(1e30)
                                                     : T(4 * std::sin(.27 * double(i)));
                        kernel(input.data(), output.data() + 1, n, drive, bias, dc, gain);
                        require(output.front() == T(7) && output.back() == T(7),
                                "direct clip bounds");
                        for (std::size_t i = 0; i < n; ++i) {
                            const auto expected = T(
                                (std::clamp(double(input[i]) * drive + bias, -1., 1.) - dc) * gain);
                            require(output[i + 1] == expected &&
                                        std::signbit(output[i + 1]) == std::signbit(expected),
                                    "direct clip independent double arithmetic and signed zero");
                        }
                    }
    }
}
void preparedControls() {
    PeakingBiquadDesigner designer;
    BiquadCoefficients out;
    out.b0 = 7;
    require(designer.design(0, out) == Status::not_prepared && out.b0 == 7,
            "unprepared peaking plan");
    for (double rate : {44100., 48000., 96000.})
        for (double frequency : {20., 2800., rate * .499})
            for (double q : {.1, .8, 24.}) {
                ok(designer.prepare(rate, frequency, q));
                for (double gain : {-120., -36., -9., 0., 18., 120.}) {
                    BiquadCoefficients direct, prepared, rounded;
                    const auto a = designPeakingBiquad(rate, frequency, q, gain, direct);
                    const auto b = designer.design(gain, prepared);
                    require(a == b && (a != Status::ok || same(direct, prepared)),
                            "prepared peaking exact design parity");
                    if (a == Status::ok) {
                        const BiquadCoefficients f{float(direct.b0), float(direct.b1),
                                                   float(direct.b2), float(direct.a1),
                                                   float(direct.a2)};
                        const auto status = designer.designFor<float>(gain, rounded);
                        require(status == (f.isStable() ? Status::ok : Status::invalid_argument),
                                "rounded coefficient stability");
                        if (status == Status::ok)
                            require(same(f, rounded), "float coefficient publication");
                    }
                }
            }
    ok(designer.prepare(48000, 2800, .8));
    ok(designer.design(-3, out));
    const auto saved = out;
    require(designer.prepare(0, 0, 0) == Status::invalid_argument, "invalid peaking prepare");
    ok(designer.design(-3, out));
    require(same(out, saved), "peaking failed preparation retains plan");
    require(designer.design(std::numeric_limits<double>::quiet_NaN(), out) ==
                    Status::invalid_argument &&
                same(out, saved),
            "peaking failed design preserves output");
    for (double speed : {1., 343., 10000.})
        for (double limit : {0., 12., 96.}) {
            DopplerPlan plan;
            ok(plan.prepare(speed, 1.7, limit));
            AcousticMedium medium;
            medium.speedMetresPerSecond = speed;
            for (double vs : {-1e7, -300., 0., 250., 1e7})
                for (double vl : {-1e7, -200., 0., 120., 1e7}) {
                    RelativeSource source;
                    source.sourceRadialVelocity = vs;
                    source.listenerRadialVelocity = vl;
                    double expected = 0, actual = 0;
                    ok(dopplerRatio(source, medium, expected, 1.7, limit));
                    ok(plan.ratio(vs, vl, actual));
                    require(actual == expected, "prepared Doppler parity");
                }
            double sentinel = 7;
            require(plan.ratio(std::numeric_limits<double>::infinity(), 0, sentinel) ==
                            Status::invalid_argument &&
                        sentinel == 7,
                    "Doppler invalid output preservation");
            require(plan.prepare(0, 1, 12) == Status::invalid_argument, "Doppler invalid prepare");
            ok(plan.ratio(0, 0, sentinel));
            require(sentinel == 1, "Doppler retained plan");
        }
    for (double absorption : {0., 1e-12, .2, .7, 1.})
        for (double loss : {0., 6., 120., 300.})
            for (double scale : {0., .5, 1., 2., 3.7, 1e6}) {
                AcousticMaterial m;
                m.absorption.fill(absorption);
                m.transmissionLossDb.fill(loss);
                PreparedAcousticMaterial plan;
                ok(plan.prepare(m));
                MaterialResponse raw, prepared;
                ok(materialResponse(m, m.referenceThicknessMetres * scale, raw));
                ok(plan.response(m.referenceThicknessMetres * scale, prepared));
                AcousticMaterialBand band{absorption, loss, m.referenceThicknessMetres,
                                          m.scattering};
                PreparedMaterialBand mono;
                ok(mono.prepare(band));
                MaterialBandResponse a, b;
                ok(materialBandResponse(band, m.referenceThicknessMetres * scale, a));
                ok(mono.response(m.referenceThicknessMetres * scale, b));
                require(raw.transmission == prepared.transmission &&
                            raw.reflection == prepared.reflection &&
                            a.transmission == b.transmission && a.reflection == b.reflection &&
                            a.transmission == raw.transmission[0] &&
                            a.reflection == raw.reflection[0],
                        "scalar/prepared/equal-band material parity");
                require(a.transmission * a.transmission + a.reflection * a.reflection <= 1 + 1e-12,
                        "material energy bound");
                band.absorption = -1;
                require(mono.prepare(band) == Status::invalid_argument, "material invalid prepare");
                ok(mono.response(m.referenceThicknessMetres * scale, b));
                require(a.transmission == b.transmission && a.reflection == b.reflection,
                        "material retained preparation");
            }
    QualitySettings q;
    for (auto profile : {QualityProfile::realtimeHigh, QualityProfile::offline}) {
        ok(qualitySettings(profile, 48000, q));
        require(q.saturationAntialias && q.saturationMath == MathAccuracy::precise &&
                    q.transientMath == MathAccuracy::precise,
                "high quality preserves precise ADAA");
    }
    ok(qualitySettings(QualityProfile::economy, 48000, q));
    require(!q.saturationAntialias && q.saturationMath == MathAccuracy::fast &&
                q.transientMath == MathAccuracy::fast,
            "explicit economy quality controls");
    SaturationParameters defaults;
    require(defaults.antialias && defaults.math == MathAccuracy::precise,
            "unchanged default saturation quality");
}
template <class T> void transientGain() {
    AudioBuffer<T> input, output, gains;
    ok(input.resize(2, 257));
    ok(output.resize(2, 257));
    ok(gains.resize(2, 257));
    for (auto math : {MathAccuracy::precise, MathAccuracy::fast}) {
        TransientShaper<T> audio, gain;
        ok(audio.prepare({48000, 257, 2}));
        ok(gain.prepare({48000, 257, 2}));
        TransientParameters p;
        p.attack = .7;
        p.sustain = -.3;
        p.math = math;
        ok(audio.setParameters(p));
        ok(gain.setParameters(p));
        double f[2]{}, s[2]{};
        const auto fast = std::exp(-1000 / (p.fastMs * 48000)),
                   slow = std::exp(-1000 / (p.slowMs * 48000)),
                   release = std::exp(-1000 / (p.releaseMs * 48000));
        for (int block = 0; block < 24; ++block) {
            for (std::size_t ch = 0; ch < 2; ++ch)
                for (std::size_t i = 0; i < 257; ++i)
                    input.block().channel(ch)[i] =
                        block % 3 == 0
                            ? T(0)
                            : T(.4 * std::sin(.07 * double(block * 257 + i) + double(ch)));
            ok(audio.process(input.block(), output.block()));
            ok(gain.processGain(input.block(), gains.block()));
            for (std::size_t ch = 0; ch < 2; ++ch)
                for (std::size_t i = 0; i < 257; ++i) {
                    const double x = input.block().channel(ch)[i], level = std::abs(x);
                    f[ch] = level + (level > f[ch] ? fast : release) * (f[ch] - level);
                    s[ch] = level + (level > s[ch] ? slow : release) * (s[ch] - level);
                    const double attack =
                        std::clamp((f[ch] - s[ch]) / std::max(1e-12, f[ch]), 0., 1.);
                    const double sustain =
                        std::clamp(s[ch] / std::max(1e-12, f[ch]), 0., 1.) * (1 - attack);
                    const double expected = std::pow(
                        10., p.maximumGainDb *
                                 std::clamp(p.attack * attack + p.sustain * sustain, -1., 1.) / 20);
                    if (math == MathAccuracy::precise)
                        require(output.block().channel(ch)[i] == T(x * expected) &&
                                    gains.block().channel(ch)[i] == T(expected),
                                "precise transient retains the reference pow formula");
                    const double tolerance = std::same_as<T, float> ? 2e-6 : 2e-12;
                    require(std::abs(double(gains.block().channel(ch)[i]) - expected) <
                                tolerance * (1 + expected),
                            "gain-only transient independent envelope oracle");
                    require(std::abs(double(output.block().channel(ch)[i]) - x * expected) <
                                tolerance * (1 + std::abs(x * expected)),
                            "transient retained audio quality");
                }
        }
    }
}
template <class T> void saturationQuality() {
    AudioBuffer<T> in, precise, fast;
    ok(in.resize(2, 257));
    ok(precise.resize(2, 257));
    ok(fast.resize(2, 257));
    AdaaSaturator<T> a, b;
    ok(a.prepare({48000, 257, 2}));
    ok(b.prepare({48000, 257, 2}));
    double maximum = 0;
    for (double drive : {.001, .4, 1., 4., 64.})
        for (double bias : {-4., 0., .3, 4.}) {
            a.reset();
            b.reset();
            SaturationParameters p;
            p.shape = AdaaShape::atan;
            p.drive = drive;
            p.bias = bias;
            p.outputGain = 16;
            ok(a.setParameters(p));
            p.math = MathAccuracy::fast;
            ok(b.setParameters(p));
            for (int block = 0; block < 14; ++block) {
                for (std::size_t ch = 0; ch < 2; ++ch)
                    for (std::size_t i = 0; i < 257; ++i) {
                        const double phase = double(block * 257 + i);
                        const double x = block % 3 == 0 ? .0001 + .0000001 * phase
                                                        : std::sin(.173 * phase + double(ch)) +
                                                              .4 * std::cos(1.47 * phase);
                        in.block().channel(ch)[i] = T(x);
                    }
                ok(a.process(in.block(), precise.block()));
                ok(b.process(in.block(), fast.block()));
                for (std::size_t ch = 0; ch < 2; ++ch)
                    for (std::size_t i = 0; i < 257; ++i)
                        maximum =
                            std::max(maximum, std::abs(double(precise.block().channel(ch)[i]) -
                                                       double(fast.block().channel(ch)[i])));
            }
        }
    std::cout << "QUALITY optional atan " << (std::same_as<T, float> ? "float" : "double")
              << " maxAbsError=" << maximum << '\n';
    require(maximum < (std::same_as<T, float> ? 3e-6 : 3e-8),
            "optional atan numerical error budget");
    SaturationParameters invalid;
    invalid.math = static_cast<MathAccuracy>(999);
    require(a.setParameters(invalid) == Status::invalid_argument, "unknown quality enum rejected");
}
template <class T> void mixedDynamics() {
    AudioBuffer<T> in, expected, actual, inplace;
    ok(in.resize(2, 257));
    ok(expected.resize(2, 257));
    ok(actual.resize(2, 257));
    ok(inplace.resize(2, 257));
    for (auto mode : {DynamicsMode::compressor, DynamicsMode::limiter, DynamicsMode::expander,
                      DynamicsMode::noiseGate})
        for (double lookAhead : {0., .375}) {
            Dynamics<T> separate, fused, alias;
            DryWetMixer<T> mixer;
            const ProcessSpec spec{48000, 257, 2};
            ok(separate.prepare(spec, mode, 1));
            ok(fused.prepare(spec, mode, 1));
            ok(alias.prepare(spec, mode, 1));
            ok(mixer.prepare(spec, 48));
            DynamicsParameters p;
            p.lookAheadMs = lookAhead;
            p.attackMs = 1;
            p.releaseMs = 20;
            p.ratio = 6;
            ok(separate.setParameters(p));
            ok(fused.setParameters(p));
            ok(alias.setParameters(p));
            ok(mixer.setMix(.37, MixLaw::linear));
            ok(mixer.setWetLatency(separate.latencyFrames()));
            for (int block = 0; block < 12; ++block) {
                for (std::size_t ch = 0; ch < 2; ++ch)
                    for (std::size_t i = 0; i < 257; ++i)
                        in.block().channel(ch)[i] =
                            T(.8 * std::sin(.13 * double(block * 257 + i) + double(ch)));
                for (std::size_t ch = 0; ch < 2; ++ch)
                    std::copy(in.block().channel(ch).begin(), in.block().channel(ch).end(),
                              inplace.block().channel(ch).begin());
                ok(mixer.pushDrySamples(in.block()));
                ok(separate.process(in.block(), expected.block()));
                ok(mixer.mixWetSamples(expected.block()));
                ok(fused.processMixed(in.block(), actual.block(), 1 - .37, .37));
                ok(alias.processMixed(inplace.block(), inplace.block(), 1 - .37, .37));
                for (std::size_t ch = 0; ch < 2; ++ch)
                    for (std::size_t i = 0; i < 257; ++i)
                        require(inplace.block().channel(ch)[i] == actual.block().channel(ch)[i],
                                "fused dynamics exact in-place parity");
                for (std::size_t ch = 0; ch < 2; ++ch)
                    for (std::size_t i = 0; i < 257; ++i)
                        require(actual.block().channel(ch)[i] == expected.block().channel(ch)[i],
                                "fused dynamics matches separate latency-aligned mix");
                require(fused.minimumAppliedGain() == separate.minimumAppliedGain() &&
                            fused.gainReductionDb() == separate.gainReductionDb(),
                        "fused dynamics metering preserved");
            }
        }
}
template <class T> void mixedSaturation() {
    AudioBuffer<T> in, out, inplace;
    AudioBuffer<double> driven, reference;
    ok(in.resize(2, 257));
    ok(out.resize(2, 257));
    ok(inplace.resize(2, 257));
    ok(driven.resize(2, 257));
    ok(reference.resize(2, 257));
    const SaturationInputStage stage{4, .2, 2.5};
    for (auto shape : {AdaaShape::tanh, AdaaShape::atan, AdaaShape::cubic, AdaaShape::hardClip,
                       AdaaShape::wavefolder, AdaaShape::hysteresis})
        for (bool aa : {false, true}) {
            AdaaSaturator<T> fused, alias;
            AdaaSaturator<double> separate;
            ok(fused.prepare({48000, 257, 2}));
            ok(alias.prepare({48000, 257, 2}));
            ok(separate.prepare({48000, 257, 2}));
            SaturationParameters p;
            p.shape = shape;
            p.antialias = aa;
            p.bias = .1;
            p.drive = 2;
            ok(fused.setParameters(p));
            ok(alias.setParameters(p));
            ok(separate.setParameters(p));
            for (int block = 0; block < 8; ++block) {
                for (std::size_t ch = 0; ch < 2; ++ch)
                    for (std::size_t i = 0; i < 257; ++i) {
                        const T value =
                            block % 3 == 0
                                ? T(.17)
                                : T(.3 * std::sin(.11 * double(block * 257 + i) + double(ch)));
                        in.block().channel(ch)[i] = value;
                        driven.block().channel(ch)[i] =
                            (double(value) * stage.gain + stage.bias) * stage.scale;
                    }
                for (std::size_t ch = 0; ch < 2; ++ch)
                    std::copy(in.block().channel(ch).begin(), in.block().channel(ch).end(),
                              inplace.block().channel(ch).begin());
                ok(separate.process(driven.block(), reference.block()));
                ok(fused.processMixed(in.block(), out.block(), stage, .3, .7));
                ok(alias.processMixed(inplace.block(), inplace.block(), stage, .3, .7));
                for (std::size_t ch = 0; ch < 2; ++ch)
                    for (std::size_t i = 0; i < 257; ++i)
                        require(inplace.block().channel(ch)[i] == out.block().channel(ch)[i],
                                "fused saturation exact in-place parity");
                for (std::size_t ch = 0; ch < 2; ++ch)
                    for (std::size_t i = 0; i < 257; ++i) {
                        const T expected = T(.3 * double(in.block().channel(ch)[i]) +
                                             .7 * double(T(reference.block().channel(ch)[i])));
                        require(std::abs(double(out.block().channel(ch)[i]) - double(expected)) <
                                    (std::same_as<T, float> ? 2e-6 : 3e-11),
                                "fused saturation input/mix oracle");
                    }
            }
        }
}
void inputControlSnapshot() {
    AdaaSaturator<double> processor;
    ok(processor.prepare({48000, 1, 2}));
    SaturationParameters p;
    p.shape = AdaaShape::hardClip;
    p.antialias = false;
    ok(processor.setParameters(p));
    const double samples[2]{.05, .08};
    const double* input[]{&samples[0], &samples[1]};
    SaturationInputStage stage{4, .2, 2.5};
    double* output[]{&stage.gain, &stage.bias};
    ok(processor.processMixed({input, 2, 1}, {output, 2, 1}, stage));
    require(stage.gain == 1 && stage.bias == 1,
            "input-stage controls snapshot before caller-owned output writes");
}
struct Audit {
    AudioBuffer<float> in, out;
    TransientShaper<float> transient;
    Dynamics<float> dynamics;
    AdaaSaturator<float> saturation;
    PeakingBiquadDesigner eq;
    PreparedMaterialBand material;
    DopplerPlan doppler;
    bool valid = true;
};
void audit(void* context) noexcept {
    auto& a = *static_cast<Audit*>(context);
    BiquadCoefficients coefficients;
    MaterialBandResponse response;
    double ratio = 0;
    a.valid = a.transient.processGain(a.in.block(), a.out.block()) == Status::ok &&
              a.dynamics.processMixed(a.in.block(), a.out.block(), .3, .7) == Status::ok &&
              a.saturation.processMixed(a.in.block(), a.out.block(), {}, .3, .7) == Status::ok &&
              a.eq.designFor<float>(-3, coefficients) == Status::ok &&
              a.material.response(.3, response) == Status::ok &&
              a.doppler.ratio(-3, 1, ratio) == Status::ok;
}
} // namespace
void preparedOptimizationRegression() {
    directClipKernels<float>();
    directClipKernels<double>();
    fftQuality<float>();
    fftQuality<double>();
    preparedControls();
    transientGain<float>();
    transientGain<double>();
    saturationQuality<float>();
    saturationQuality<double>();
    mixedDynamics<float>();
    mixedDynamics<double>();
    mixedSaturation<float>();
    mixedSaturation<double>();
    inputControlSnapshot();
    Audit a;
    ok(a.in.resize(1, 64));
    ok(a.out.resize(1, 64));
    ok(a.transient.prepare({48000, 64, 1}));
    TransientParameters transientFast;
    transientFast.math = MathAccuracy::fast;
    ok(a.transient.setParameters(transientFast));
    ok(a.dynamics.prepare({48000, 64, 1}));
    ok(a.saturation.prepare({48000, 64, 1}));
    SaturationParameters fast;
    fast.shape = AdaaShape::atan;
    fast.math = MathAccuracy::fast;
    ok(a.saturation.setParameters(fast));
    for (std::size_t i = 0; i < 64; ++i)
        a.in.block().channel(0)[i] = float(.3 * std::sin(.7 * double(i)));
    ok(a.eq.prepare(48000, 2800, .8));
    ok(a.material.prepare({}));
    ok(a.doppler.prepare());
    runRealtimeAudit(audit, &a);
    require(a.valid, "new APIs allocate/lock-free processing audit");
}
