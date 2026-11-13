// Explicit opt-in timings and quality for the ten-lot extension.
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
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
void ok(Status status) {
    if (status != Status::ok)
        throw std::runtime_error("expansion measurement status");
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
    std::cout << "{\"kind\":\"performance\",\"scope\":\"expansion10\",\"operation\":\"" << name
              << "\",\"sampleType\":\"" << (std::same_as<T, float> ? "float" : "double")
              << "\",\"backend\":\"" << backendName(backend)
              << "\",\"blockFrames\":128,\"sampleRate\":48000,\"channels\":" << channels
              << ",\"iterations\":128,\"meanNs\":" << mean << ",\"medianNs\":" << times[64]
              << ",\"p95Ns\":" << times[121] << ",\"p99Ns\":" << times[126]
              << ",\"maxNs\":" << times.back() << "}";
}
template <class T> void quality(const char* name, double value, const char* unit) {
    separator();
    std::cout << "{\"kind\":\"quality\",\"scope\":\"expansion10\",\"metric\":\"" << name
              << "\",\"sampleType\":\"" << (std::same_as<T, float> ? "float" : "double")
              << "\",\"value\":" << value << ",\"unit\":\"" << unit << "\"}";
}
template <class T> void measure() {
    const ProcessSpec stereo{48000, 128, 2}, mono{48000, 128, 1};
    AudioBuffer<T> input, output, one, wide;
    ok(input.resize(2, 128));
    ok(output.resize(2, 128));
    ok(one.resize(1, 128));
    ok(wide.resize(32, 128));
    for (std::size_t i = 0; i < 128; ++i) {
        one.block().channel(0)[i] = T(.2 * std::sin(.031 * i));
        for (std::size_t ch = 0; ch < 2; ++ch)
            input.block().channel(ch)[i] = T(.2 * std::sin(.031 * i + .2 * ch));
    }
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::avx512,
                         CpuBackend::neon, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        {
            MultiTapDelay<T> p;
            ok(p.prepare(stereo, 2, 8192, 16, backend));
            std::array<DelayTap, 16> taps{};
            for (std::size_t i = 0; i < taps.size(); ++i)
                taps[i] = {i % 2, i % 2, double(47 + i * 41), .05, HistoryInterpolation::lagrange3};
            ok(p.setTaps(taps));
            bench<T>("multitap16", backend, 2, [&] {
                ok(p.process(input.block(), output.block()));
                sink = output.block().channel(0)[127];
            });
        }
        {
            Stft<T> p;
            ok(p.prepare(stereo, {1024, 256, WindowType::hann}, backend));
            bench<T>("stft1024hop256", backend, 2, [&] {
                ok(p.process(input.block(), output.block()));
                sink = output.block().channel(0)[127];
            });
        }
        {
            FirFilterBank<T> p;
            const std::array<double, 3> cross{300, 1500, 6000};
            ok(p.prepare(stereo, cross, 129, backend));
            T* ptr[8];
            for (std::size_t i = 0; i < 8; ++i)
                ptr[i] = wide.block().channel(i).data();
            AudioBlock<T> bands{ptr, 8, 128};
            bench<T>("firBank4x129", backend, 2, [&] {
                ok(p.process(input.block(), bands));
                sink = bands.channel(2)[127];
            });
        }
        {
            MultiBusGraph<T> p;
            const std::array<DspBus, 1> bus{{{0, 2}}};
            const std::array<DspConnection, 2> connections{
                {{0, 0, graphOutputNode, 0, .5}, {0, 0, graphOutputNode, 0, .5}}};
            ok(p.prepare(stereo, bus, bus, {}, connections, backend));
            const std::array<AudioBlock<const T>, 1> in{input.block()};
            std::array<AudioBlock<T>, 1> out{output.block()};
            bench<T>("graphTwoParallelRoutes", backend, 2, [&] {
                ok(p.process(in, out));
                sink = output.block().channel(0)[127];
            });
        }
        {
            FdnReverb<T> p;
            ok(p.prepare(stereo, {16, 1, 250}, backend));
            bench<T>("fdn16Stereo", backend, 2, [&] {
                ok(p.process(input.block(), output.block()));
                sink = output.block().channel(0)[127];
            });
        }
        {
            std::array<T, 128> left{}, right{};
            left[0] = 1;
            right[2] = T(.8);
            for (std::size_t k = 3; k < 128; ++k) {
                left[k] = T(.01 * std::exp(-double(k) / 30));
                right[k] = T(-.007 * std::exp(-double(k) / 40));
            }
            const HrirPoint<T> point{{1, 0, 0}, left, right, 0, 2};
            HrirBinaural<T> p;
            ok(p.prepare(mono, {&point, 1}, backend));
            bench<T>("hrir128Stereo", backend, 1, [&] {
                ok(p.process(one.block(), output.block()));
                sink = output.block().channel(0)[127];
            });
        }
        {
            AmbisonicRenderer<T> p;
            const std::array<Vec3, 8> speakers{{{1, 1, 1},
                                                {1, 1, -1},
                                                {1, -1, 1},
                                                {1, -1, -1},
                                                {-1, 1, 1},
                                                {-1, 1, -1},
                                                {-1, -1, 1},
                                                {-1, -1, -1}}};
            ok(p.prepare(mono, 3, speakers, .01, backend));
            AudioBuffer<T> field, rotated, decoded;
            ok(field.resize(16, 128));
            ok(rotated.resize(16, 128));
            ok(decoded.resize(8, 128));
            bench<T>("hoa3EncodeRotateDecode8", backend, 1, [&] {
                ok(p.encode(one.block(), field.block()));
                ok(p.rotateField(field.block(), rotated.block()));
                ok(p.decode(rotated.block(), decoded.block()));
                sink = decoded.block().channel(0)[127];
            });
        }
        {
            MultibandDynamics<T> p;
            const std::array<double, 2> cross{400, 4000};
            ok(p.prepare(stereo, cross, 129, backend));
            DynamicsParameters d;
            d.lookAheadMs = 0;
            ok(p.setBand(0, MultibandMode::compressor, d));
            ok(p.setBand(1, MultibandMode::compressor, d));
            ok(p.setBand(2, MultibandMode::saturator));
            bench<T>("multiband3Dynamics129", backend, 2, [&] {
                ok(p.process(input.block(), output.block()));
                sink = output.block().channel(0)[127];
            });
        }
        {
            TruePeakLimiter<T> p;
            ok(p.prepare(stereo, 4, 5, backend));
            bench<T>("truePeakLimiter4x5ms", backend, 2, [&] {
                ok(p.process(input.block(), output.block()));
                sink = output.block().channel(0)[127];
            });
        }
        {
            SpectralEffects<T> p;
            ok(p.prepare(stereo, {1024, 256, WindowType::hann}, backend));
            ok(p.setParameters({SpectralEffectMode::vocoder, 1, 8}));
            bench<T>("spectralVocoder1024", backend, 2, [&] {
                ok(p.process(input.block(), input.block(), output.block()));
                sink = output.block().channel(0)[127];
            });
        }
        {
            LiveGranulator<T> p;
            ok(p.prepare(stereo, 2, 4, 64, backend));
            for (int i = 0; i < 140; ++i)
                ok(p.process(input.block(), output.block()));
            LiveGrainParameters g;
            g.ageSeconds = .3;
            g.durationMs = 10000;
            g.channelGains = {.707106781, .707106781, 0, 0, 0, 0, 0, 0};
            for (int i = 0; i < 64; ++i)
                ok(p.trigger(g));
            bench<T>("liveGrains64Sinc32", backend, 2, [&] {
                ok(p.process(input.block(), output.block()));
                sink = output.block().channel(0)[127];
            });
        }
        {
            ModulatedOscillator<T> p;
            ok(p.prepare(stereo, {}, backend));
            ModulatedOscillatorParameters params;
            params.unison = 8;
            params.syncHz = 60;
            params.waveform = Waveform::saw;
            ok(p.setParameters(params));
            bench<T>("fmPmSyncUnison8", backend, 2, [&] {
                ok(p.process(one.block(), one.block(), output.block()));
                sink = output.block().channel(0)[127];
            });
        }
        {
            MusicalAnalyzer<T> p;
            ok(p.prepare(stereo, {}, backend));
            bench<T>("musicalAnalyzer2048", backend, 2, [&] {
                ok(p.process(input.block()));
                sink = p.features().fundamentalHz;
            });
        }
        {
            PitchEffects<T> p;
            ok(p.prepare(stereo, 4, {}, backend));
            const std::array<HarmonyVoice, 4> voices{
                {{0, .5, -.5, 0}, {7, .2, .5, 0}, {12, .2, 0, 0}, {.08, .1, 0, 15}}};
            ok(p.setVoices(voices));
            bench<T>("harmonizer4Sinc32", backend, 2, [&] {
                ok(p.process(input.block(), output.block()));
                sink = output.block().channel(0)[127];
            });
        }
        {
            SpectralDenoiser<T> p;
            ok(p.prepare(stereo, {1024, 256, WindowType::hann}, backend));
            std::vector<double> profile(2 * 513, .001);
            ok(p.setNoiseProfile(profile));
            bench<T>("denoiser1024", backend, 2, [&] {
                ok(p.process(input.block(), output.block()));
                sink = output.block().channel(0)[127];
            });
        }
        {
            AdaptiveFilter<T> p;
            ok(p.prepare(stereo, 64, AdaptiveAlgorithm::nlms, backend));
            AudioBuffer<T> residual;
            ok(residual.resize(2, 128));
            bench<T>("nlms64", backend, 2, [&] {
                ok(p.process(input.block(), input.block(), output.block(), residual.block()));
                sink = residual.block().channel(0)[127];
            });
        }
        {
            FirRateConverter<T> p;
            ok(p.prepare(stereo, 4, RateConversion::decimate, 32, backend));
            bench<T>("firDecimate4x129", backend, 2, [&] {
                ok(p.process(input.block(), output.block(32)));
                sink = output.block().channel(0)[31];
            });
        }
        {
            TransferAnalyzer<T> p;
            ok(p.prepare(stereo, 1024, 256, 32, backend));
            bench<T>("transferWelch1024", backend, 2, [&] {
                ok(p.process(input.block(), input.block()));
                sink = p.bin(0, 7).coherence;
            });
        }
    }
    if constexpr (std::same_as<T, double>) {
        ParameterBank p;
        std::array<ParameterDescriptor, 16> descriptors{};
        for (std::size_t i = 0; i < 16; ++i)
            descriptors[i] = {unsigned(i + 1), "parameter", ParameterUnit::none, 0, 1, .5};
        ok(p.prepare(descriptors, 128));
        AudioBuffer<double> values;
        ok(values.resize(16, 128));
        const std::array<ParameterEvent, 4> events{
            {{1, 0, .8, 256}, {2, 32, .2, 128}, {3, 64, .9, 128}, {4, 96, .1, 256}}};
        bench<T>("parameterBank16Events4", CpuBackend::scalar, 16, [&] {
            ok(p.render(events, values.block()));
            sink = values.block().channel(0)[127];
        });
    }
    {
        AnalyticBinaural<T> p;
        ok(p.prepare(stereo));
        const std::array<BinauralEmitter, 2> emitters{
            {{{1, .3, 0}, .5, .1}, {{1, -.3, 0}, .5, .1}}};
        ok(p.setEmitters(emitters, 0));
        bench<T>("analyticBinaural2Emitters", CpuBackend::scalar, 2, [&] {
            ok(p.process(input.block(), output.block()));
            sink = output.block().channel(0)[127];
        });
    }
    {
        AdaaSaturator<T> p;
        ok(p.prepare(stereo));
        bench<T>("adaaTanhStereo", CpuBackend::scalar, 2, [&] {
            ok(p.process(input.block(), output.block()));
            sink = output.block().channel(0)[127];
        });
    }
    {
        DelayEffect<T> p;
        ok(p.prepare(stereo));
        const std::array<EchoTap, 4> taps{{{100, .4}, {200, .3}, {350, .2}, {500, .1}}};
        ok(p.setTaps(taps));
        bench<T>("filteredEcho4Taps", CpuBackend::scalar, 2, [&] {
            ok(p.process(input.block(), output.block()));
            sink = output.block().channel(0)[127];
        });
    }
    {
        PlateReverb<T> p;
        ok(p.prepare(stereo));
        bench<T>("dattorroPlateStereo", CpuBackend::scalar, 2, [&] {
            ok(p.process(input.block(), output.block()));
            sink = output.block().channel(0)[127];
        });
    }
    {
        SpringReverb<T> p;
        ok(p.prepare(stereo));
        bench<T>("springStereo", CpuBackend::scalar, 2, [&] {
            ok(p.process(input.block(), output.block()));
            sink = output.block().channel(0)[127];
        });
    }
    {
        ModalResonator<T> p;
        ok(p.prepare(mono, 2, 32));
        ok(p.setSurface(ModalSurface::rectangularPlate, 150));
        bench<T>("modalPlate32", CpuBackend::scalar, 1, [&] {
            ok(p.process(one.block(), output.block()));
            sink = output.block().channel(0)[127];
        });
    }
    {
        Declicker<T> p;
        ok(p.prepare(stereo, 8));
        bench<T>("declickMedian17", CpuBackend::scalar, 2, [&] {
            ok(p.process(input.block(), output.block()));
            sink = output.block().channel(0)[127];
        });
    }
    // Independent quality records, not timing assertions.
    {
        Stft<T> p;
        ok(p.prepare(stereo, {256, 64, WindowType::hann}));
        std::vector<T> source(4096), rendered(4096);
        for (std::size_t i = 0; i < source.size(); ++i)
            source[i] = T(.4 * std::sin(.17 * i));
        for (std::size_t block = 0; block < 32; ++block) {
            for (std::size_t i = 0; i < 128; ++i)
                for (std::size_t ch = 0; ch < 2; ++ch)
                    input.block().channel(ch)[i] = source[block * 128 + i];
            ok(p.process(input.block(), output.block()));
            std::copy(output.block().channel(0).begin(), output.block().channel(0).end(),
                      rendered.begin() + block * 128);
        }
        double error = 0;
        for (std::size_t i = 256; i < 4096; ++i)
            error = std::max(error, std::abs(double(rendered[i]) - source[i - 256]));
        quality<T>("stft256ReconstructionMaxError", error, "amplitude");
    }
    {
        MusicalAnalyzer<T> p;
        ok(p.prepare(mono));
        for (unsigned b = 0; b < 40; ++b) {
            for (std::size_t i = 0; i < 128; ++i)
                one.block().channel(0)[i] =
                    T(.5 * std::sin(2 * std::numbers::pi * 440 * double(b * 128 + i) / 48000));
            ok(p.process(one.block()));
        }
        quality<T>("yin440AbsoluteErrorHz", std::abs(p.features().fundamentalHz - 440), "Hz");
        quality<T>("yin440Confidence", p.features().pitchConfidence, "unit_interval");
    }
    {
        std::array<double, 16> a{}, b{};
        std::array<double, 256> rotation{};
        const Quaternion q{std::cos(.3), 0, std::sin(.3), 0};
        ok(ambisonicCoefficients(3, {.3, .4, .5}, a));
        ok(ambisonicCoefficients(3, rotate(q, {.3, .4, .5}), b));
        ok(ambisonicRotation(3, q, rotation));
        double error = 0;
        for (std::size_t i = 0; i < 16; ++i) {
            double v = 0;
            for (std::size_t j = 0; j < 16; ++j)
                v += rotation[i * 16 + j] * a[j];
            error = std::max(error, std::abs(v - b[i]));
        }
        quality<T>("hoa3RotationMaxError", error, "coefficient");
    }
    {
        std::vector<T> tone(4096);
        for (std::size_t i = 0; i < tone.size(); ++i) {
            const double phase = 2 * std::numbers::pi * 997 * double(i) / 48000;
            tone[i] = T(.5 * std::sin(phase) + .005 * std::sin(2 * phase));
        }
        DistortionReport d;
        ok(measureDistortion<T>(tone, 48000, 997, d, 5));
        quality<T>("knownOnePercentThdError", std::abs(d.thd - .01), "linear_ratio");
    }
}
} // namespace
void expansionMeasurements(void (*separate)()) {
    separator = separate;
    measure<float>();
    measure<double>();
}
