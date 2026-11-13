// Explicit opt-in measurements for the second ten-lot extension.
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <complex>
#include <cydsp/CyDSP.h>
#include <iostream>
#include <numbers>
#include <numeric>
#include <stdexcept>
#include <vector>
namespace {
using namespace cydsp;
volatile double sink = 0;
void (*separator)() = nullptr;
constexpr double pi = std::numbers::pi;
void ok(Status status) {
    if (status != Status::ok)
        throw std::runtime_error("advanced extension measurement status");
}
template <class T, class F>
void bench(const char* name, CpuBackend backend, std::size_t channels, F&& run) {
    for (unsigned i = 0; i < 48; ++i)
        run();
    std::array<double, 128> times{};
    for (auto& time : times) {
        const auto start = std::chrono::steady_clock::now();
        run();
        time = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - start)
                   .count();
    }
    const double mean = std::accumulate(times.begin(), times.end(), 0.) / times.size();
    std::sort(times.begin(), times.end());
    separator();
    std::cout << "{\"kind\":\"performance\",\"scope\":\"advancedExpansion10\",\"operation\":\""
              << name << "\",\"sampleType\":\"" << (std::same_as<T, float> ? "float" : "double")
              << "\",\"backend\":\"" << backendName(backend)
              << "\",\"blockFrames\":128,\"sampleRate\":48000,\"channels\":" << channels
              << ",\"iterations\":128,\"meanNs\":" << mean << ",\"medianNs\":" << times[64]
              << ",\"p95Ns\":" << times[121] << ",\"p99Ns\":" << times[126]
              << ",\"maxNs\":" << times.back() << "}";
}
template <class T> void quality(const char* name, double value, const char* unit) {
    if (!isFinite(value))
        throw std::runtime_error("nonfinite advanced quality metric");
    separator();
    std::cout << "{\"kind\":\"quality\",\"scope\":\"advancedExpansion10\",\"metric\":\"" << name
              << "\",\"sampleType\":\"" << (std::same_as<T, float> ? "float" : "double")
              << "\",\"value\":" << value << ",\"unit\":\"" << unit << "\"}";
}
std::vector<Vec3> sphere(std::size_t n) {
    std::vector<Vec3> result;
    for (std::size_t i = 0; i < n; ++i) {
        const double z = 1 - 2 * (double(i) + .5) / double(n), r = std::sqrt(1 - z * z),
                     phi = double(i) * pi * (3 - std::sqrt(5.));
        result.push_back({r * std::cos(phi), r * std::sin(phi), z});
    }
    return result;
}
template <class T> void measurements() {
    const ProcessSpec spec{48000, 128, 2}, mono{48000, 128, 1};
    AudioBuffer<T> in, out, one, h, p, field, decoded;
    for (auto* b : {&in, &out, &h, &p})
        ok(b->resize(2, 128));
    ok(one.resize(1, 128));
    ok(field.resize(64, 128));
    ok(decoded.resize(24, 128));
    for (std::size_t i = 0; i < 128; ++i) {
        one.block().channel(0)[i] = T(.2 * std::sin(.02 * double(i)));
        for (std::size_t ch = 0; ch < 2; ++ch)
            in.block().channel(ch)[i] = T((ch == 0 ? 1 : -.5) * one.block().channel(0)[i]);
    }
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::avx512,
                         CpuBackend::neon, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        {
            SourceBatch<T> batch;
            ok(batch.prepare(spec, 128, {}, {}, backend));
            for (std::size_t v = 0; v < 128; ++v) {
                BatchSource s;
                s.frequencyHz = 80 + 7 * double(v);
                s.gain = 1. / 128;
                s.channelGains[1] = .5;
                ok(batch.start(v, s));
            }
            bench<T>("sineBatch128Stereo", backend, 2, [&] {
                ok(batch.process(out.block()));
                sink = out.block().channel(0)[127];
            });
        }
        {
            std::array<Oscillator<T>, 128> voices;
            AudioBuffer<T> temp;
            ok(temp.resize(1, 128));
            for (std::size_t v = 0; v < 128; ++v) {
                ok(voices[v].prepare(mono));
                ok(voices[v].setFrequency(80 + 7 * double(v)));
            }
            bench<T>("individualSine128Stereo", backend, 2, [&] {
                for (std::size_t ch = 0; ch < 2; ++ch)
                    ops.fill(out.block().channel(ch), T{});
                for (auto& voice : voices) {
                    ok(voice.process(temp.block()));
                    ok(ops.addScaled(std::span<const T>(temp.block().channel(0)),
                                     out.block().channel(0), T(1. / 128)));
                    ok(ops.addScaled(std::span<const T>(temp.block().channel(0)),
                                     out.block().channel(1), T(.5 / 128)));
                }
                sink = out.block().channel(0)[127];
            });
        }
        {
            AudioBuffer<T> sample;
            ok(sample.resize(1, 32768));
            for (std::size_t i = 0; i < 32768; ++i)
                sample.block().channel(0)[i] = T(.2 * std::sin(.1 * double(i)));
            SampleData<T> asset;
            ok(asset.prepare(sample.block(), 48000));
            ok(asset.setLoop(0, 32768, 128));
            const SampleData<T>* assets[]{&asset};
            SourceBatch<T> grains;
            ok(grains.prepare(spec, 64, {}, assets, backend));
            for (std::size_t v = 0; v < 64; ++v) {
                BatchSource s;
                s.kind = BatchSourceKind::grain;
                s.samplePosition = double(v * 41);
                s.durationFrames = 480000;
                s.gain = 1. / 64;
                s.channelGains[1] = .5;
                s.playbackRatio = v % 2 ? -1 : 1.5;
                ok(grains.start(v, s));
            }
            bench<T>("sharedSampleGrains64", backend, 2, [&] {
                ok(grains.process(out.block()));
                sink = out.block().channel(0)[127];
            });
        }
        {
            HrtfSet<T> asset;
            HrtfPreparation config;
            config.detectOnsets = false;
            std::array<T, 128> ir{};
            for (std::size_t i = 0; i < 128; ++i)
                ir[i] = T(std::exp(-.05 * double(i)) * std::cos(.3 * double(i)) * .05);
            std::vector<HrirPoint<T>> points;
            for (auto d : sphere(32))
                points.push_back({d, ir, ir});
            ok(asset.prepare(points, config));
            HrirBinaural<T> renderer;
            ok(renderer.prepare(mono, asset, backend));
            std::size_t counter = 0;
            bench<T>("movingHrtf32x128", backend, 2, [&] {
                const double phi = .02 * double(counter++);
                ok(renderer.setDirection({std::cos(phi), std::sin(phi), 0}, 128));
                ok(renderer.process(one.block(), out.block()));
                sink = out.block().channel(0)[127];
            });
            HoaBinaural<T> hoa;
            ok(hoa.prepare({48000, 128, 16}, 3, asset, 1e-6, backend));
            AudioBuffer<T> input;
            ok(input.resize(16, 128));
            for (std::size_t i = 0; i < 128; ++i)
                input.block().channel(0)[i] = one.block().channel(0)[i];
            bench<T>("hoa3Binaural32Hrir128", backend, 2, [&] {
                ok(hoa.process(input.block(), out.block()));
                sink = out.block().channel(0)[127];
            });
        }
        {
            AmbisonicRenderer<T> renderer;
            ok(renderer.prepare(mono, 7, sphere(24), 1e-4, backend));
            ok(renderer.setDirection({.3, .4, .8}));
            ok(renderer.setRotation({std::cos(.2), 0, 0, std::sin(.2)}));
            AudioBuffer<T> rotated;
            ok(rotated.resize(64, 128));
            bench<T>("hoa7EncodeRotateDecode24", backend, 24, [&] {
                ok(renderer.encode(one.block(), field.block()));
                ok(renderer.rotateField(field.block(), rotated.block()));
                ok(renderer.decode(rotated.block(), decoded.block()));
                sink = decoded.block().channel(0)[127];
            });
        }
        {
            HarmonicPercussive<T> separation;
            ok(separation.prepare(spec, {{1024, 256, WindowType::hann}, 17, 17}, backend));
            bench<T>("hpss1024x17Stereo", backend, 2, [&] {
                ok(separation.process(in.block(), h.block(), p.block()));
                sink = h.block().channel(0)[127] + p.block().channel(1)[127];
            });
            SpectralPitchShifter<T> shifter;
            ok(shifter.prepare(spec, {1024, 256, WindowType::hann}, backend));
            SpectralPitchParameters parameters;
            parameters.ratio = 1.5;
            ok(shifter.setParameters(parameters));
            bench<T>("spectralPitch1024FormantsStereo", backend, 2, [&] {
                ok(shifter.process(in.block(), out.block()));
                sink = out.block().channel(0)[127];
            });
        }
        {
            EchoCanceller<T> echo;
            ok(echo.prepare(spec, {512, 0, 0}, backend));
            bench<T>("aec512TwoMicrophones", backend, 2, [&] {
                ok(echo.process(one.block(), in.block(), out.block()));
                sink = out.block().channel(0)[127];
            });
        }
    }
    {
        ContactExciter<T> contact;
        ok(contact.prepare(mono));
        ContactParameters parameters;
        parameters.kind = ContactKind::bowed;
        parameters.roughness = .7;
        ok(contact.setParameters(parameters));
        bench<T>("bowedContactMono", CpuBackend::scalar, 1, [&] {
            ok(contact.render(one.block()));
            sink = one.block().channel(0)[127];
        });
    }
    {
        const auto n = std::size_t(64);
        std::array<double, 4096> matrix{};
        std::array<double, 64> a{}, b{};
        Quaternion q{std::cos(.4), 0, std::sin(.4), 0};
        ok(ambisonicCoefficients(7, {.2, .7, .3}, a));
        ok(ambisonicCoefficients(7, rotate(q, {.2, .7, .3}), b));
        ok(ambisonicRotation(7, q, matrix));
        double error = 0;
        for (std::size_t row = 0; row < n; ++row) {
            double sum = 0;
            for (std::size_t c = 0; c < n; ++c)
                sum += matrix[row * n + c] * a[c];
            error = std::max(error, std::abs(sum - b[row]));
        }
        quality<T>("hoa7RotationMaxError", error, "coefficient");
    }
    {
        HarmonicPercussive<T> split;
        ok(split.prepare(mono, {{256, 64, WindowType::hann}, 9, 17}));
        AudioBuffer<T> x, y, z;
        for (auto* b : {&x, &y, &z})
            ok(b->resize(1, 128));
        std::vector<T> signal(8192);
        for (std::size_t i = 0; i < signal.size(); ++i)
            signal[i] = T(.3 * std::sin(2 * pi * 500 * double(i) / 48000) + (i == 1500 ? .8 : 0));
        double error = 0;
        for (std::size_t block = 0; block < 64; ++block) {
            std::copy_n(signal.begin() + block * 128, 128, x.block().channel(0).begin());
            ok(split.process(x.block(), y.block(), z.block()));
            for (std::size_t i = 0; i < 128; ++i) {
                const auto t = block * 128 + i;
                const double expected =
                    t >= split.latencyFrames() ? signal[t - split.latencyFrames()] : 0;
                error = std::max(error, std::abs(double(y.block().channel(0)[i]) +
                                                 double(z.block().channel(0)[i]) - expected));
            }
        }
        quality<T>("hpssReconstructionMaxError", error, "amplitude");
    }
    {
        std::vector<T> ir(16000);
        for (std::size_t i = 0; i < ir.size(); ++i)
            ir[i] = T(std::exp(-std::log(1000.) * double(i) / (8000 * .5)));
        ImpulseReport report;
        ok(analyzeImpulse(ir, 8000, report));
        quality<T>("analyticRt60ErrorSeconds", std::abs(report.broadband.t30.seconds - .5),
                   "seconds");
        quality<T>("analyticRt60FitRSquared", report.broadband.t30.rSquared, "0_to_1");
    }
    {
        // Dense synthetic vowel-like harmonic spectrum; compare normalized
        // envelope error at output harmonics with/without formant retention.
        auto envelope = [](double f) {
            return std::exp(-.5 * std::pow((f - 600) / 150, 2)) +
                   .7 * std::exp(-.5 * std::pow((f - 1600) / 200, 2)) + .005;
        };
        std::array<double, 2> errors{};
        for (int mode = 0; mode < 2; ++mode) {
            SpectralPitchShifter<T> pitch;
            ok(pitch.prepare({16000, 128, 1}, {1024, 256, WindowType::hann}));
            SpectralPitchParameters parameters;
            parameters.ratio = 2;
            parameters.preserveFormants = mode != 0;
            parameters.protectTransients = false;
            ok(pitch.setParameters(parameters));
            AudioBuffer<T> x, y;
            ok(x.resize(1, 128));
            ok(y.resize(1, 128));
            std::array<std::complex<double>, 24> sums{};
            for (std::size_t block = 0; block < 160; ++block) {
                for (std::size_t i = 0; i < 128; ++i) {
                    const auto t = block * 128 + i;
                    double sample = 0;
                    for (unsigned harmonic = 1; harmonic <= 60; ++harmonic) {
                        const double f = 62.5 * harmonic;
                        sample += .02 * envelope(f) * std::sin(2 * pi * f * double(t) / 16000);
                    }
                    x.block().channel(0)[i] = T(sample);
                }
                ok(pitch.process(x.block(), y.block()));
                if (block >= 96)
                    for (std::size_t i = 0; i < 128; ++i) {
                        const auto t = block * 128 + i;
                        for (std::size_t harmonic = 0; harmonic < sums.size(); ++harmonic)
                            sums[harmonic] += double(y.block().channel(0)[i]) *
                                              std::polar(1., -2 * pi * 125 * double(harmonic + 1) *
                                                                 double(t) / 16000);
                    }
            }
            double dot = 0, power = 0;
            for (std::size_t k = 0; k < sums.size(); ++k) {
                const double expected = envelope(125 * double(k + 1));
                dot += std::abs(sums[k]) * expected;
                power += std::norm(sums[k]);
            }
            const double scale = dot / std::max(1e-100, power);
            double error = 0, norm = 0;
            for (std::size_t k = 0; k < sums.size(); ++k) {
                const double expected = envelope(125 * double(k + 1));
                error += std::pow(scale * std::abs(sums[k]) - expected, 2);
                norm += expected * expected;
            }
            errors[std::size_t(mode)] = std::sqrt(error / norm);
        }
        quality<T>("vowelEnvelopeErrorWithoutRetention", errors[0], "normalized_rms");
        quality<T>("vowelEnvelopeErrorWithRetention", errors[1], "normalized_rms");
    }
}
} // namespace
void advancedExpansionMeasurements(void (*separate)()) {
    separator = separate;
    measurements<float>();
    measurements<double>();
}
