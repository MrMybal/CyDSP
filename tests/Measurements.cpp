#include "detail/FftAlgorithm.h"
#include "detail/FftPlanAccess.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cydsp/CyDSP.h>
#if defined(CYDSP_MEASURE_STRETCH)
#include "detail/StretchAlgorithm.h"
#include <signalsmith-stretch.h>
#endif
#include <iomanip>
#include <iostream>
#include <numbers>
#include <numeric>
#include <stdexcept>
#include <string_view>
#include <vector>
using namespace cydsp;
namespace {
volatile double sink = 0;
void require(bool ok) {
    if (!ok)
        throw std::runtime_error("measurement processing failed");
}
bool first = true;
void separator() {
    if (!first)
        std::cout << ",\n";
    first = false;
}
template <class F>
void bench(const char* name, const char* type, CpuBackend backend, std::size_t size,
           std::size_t channels, F run) {
    for (int i = 0; i < 24; ++i)
        run();
    const int batch = std::string_view(name) == "dot" ? 256 : 1;
    std::vector<double> times(400);
    for (auto& elapsed : times) {
        const auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < batch; ++i)
            run();
        elapsed = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - start)
                      .count() /
                  batch;
    }
    const auto mean = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
    const auto frames =
        std::string_view(name) == "biquadBank256"
            ? std::size_t{256}
            : (std::string_view(name) == "dot" || std::string_view(name).starts_with("complexFFT")
                   ? size
                   : std::size_t{128});
    std::sort(times.begin(), times.end());
    separator();
    std::cout << "{\"kind\":\"performance\",\"operation\":\"" << name << "\",\"sampleType\":\""
              << type << "\",\"backend\":\"" << backendName(backend) << "\",\"size\":" << size
              << ",\"channels\":" << channels << ",\"batchSize\":" << batch
              << ",\"blockFrames\":" << frames << ",\"meanNs\":" << mean
              << ",\"iterations\":" << times.size() << ",\"medianNs\":" << times[200]
              << ",\"p95Ns\":" << times[380] << ",\"p99Ns\":" << times[396]
              << ",\"maxNs\":" << times.back() << "}";
}
template <class T> void performance() {
    const char* type = std::same_as<T, float> ? "float" : "double";
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::neon,
                         CpuBackend::avx512, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        for (std::size_t n : {32u, 128u, 512u, 4096u}) {
            std::vector<T> a(n, T{0.125}), b(n, T{0.25});
            T value{};
            bench("dot", type, backend, n, 1, [&] {
                require(ops.dot(a, b, value) == Status::ok);
                sink = value;
            });
        }
        for (std::size_t n : {64u, 256u, 1024u, 4096u}) {
            ComplexFft<T> fft;
            require(fft.prepare(n, backend) == Status::ok);
            std::vector<std::complex<T>> in(n, {T{0.125}, T{0.25}}), out(n);
            bench("complexFFT", type, backend, n, 1, [&] {
                require(fft.process(in, out, FftDirection::forward) == Status::ok);
                sink = out[0].real();
            });
            // The previous group-by-group dispatcher, with the same tables and arithmetic.
            const auto reverse = detail::FftPlanAccess::reverse(fft);
            const auto weights = detail::FftPlanAccess::complexTwiddles(fft);
            const auto& kernels = detail::FftPlanAccess::kernels(fft);
            bench("complexFFTReference", type, backend, n, 1, [&] {
                detail::fftTransform<T>(
                    in, out, reverse, weights, false,
                    [&](auto even, auto odd, auto twiddle, bool inverse) noexcept {
                        kernels.butterfly(
                            reinterpret_cast<T*>(even.data()), reinterpret_cast<T*>(odd.data()),
                            reinterpret_cast<const T*>(twiddle.data()), even.size(), inverse);
                    });
                sink = out[0].real();
            });
        }
        for (std::size_t voices : {2u, 8u, 32u}) {
            BiquadBank<T> bank;
            require(bank.prepare({48000, 256, voices}, backend) == Status::ok);
            require(bank.setAllCoefficients({0.5, 0.25, 0.125, -0.5, 0.25}) == Status::ok);
            AudioBuffer<T> in, out;
            require(in.resize(voices, 256) == Status::ok && out.resize(voices, 256) == Status::ok);
            for (std::size_t ch = 0; ch < voices; ++ch)
                std::fill(in.block().channel(ch).begin(), in.block().channel(ch).end(), T{0.125});
            bench("biquadBank256", type, backend, voices, voices, [&] {
                require(bank.process(in.block(), out.block()) == Status::ok);
                sink = out.block().channel(0)[255];
            });
        }
        AudioBuffer<T> in, out;
        require(in.resize(2, 128) == Status::ok && out.resize(2, 128) == Status::ok);
        for (std::size_t ch = 0; ch < 2; ++ch)
            std::fill(in.block().channel(ch).begin(), in.block().channel(ch).end(), T{0.125});
        for (std::size_t taps : {32u, 128u, 512u}) {
            std::vector<T> ir(taps, static_cast<T>(0.001));
            Fir<T> fir;
            require(fir.prepare({48000, 128, 2}, ir, backend) == Status::ok);
            bench("fir128", type, backend, taps, 2, [&] {
                require(fir.process(in.block(), out.block()) == Status::ok);
                sink = out.block().channel(0)[127];
            });
        }
        for (std::size_t taps : {512u, 8192u, 32768u}) {
            std::vector<T> ir(taps, static_cast<T>(0.00001));
            HybridConvolver<T> convolver;
            require(convolver.prepare({48000, 128, 2}, ir, 128, backend) == Status::ok);
            bench("hybrid128", type, backend, taps, 2, [&] {
                require(convolver.process(in.block(), out.block()) == Status::ok);
                sink = out.block().channel(0)[127];
            });
        }
        {
            std::vector<T> ir(32768, static_cast<T>(0.00001));
            NonuniformConvolver<T> convolver;
            require(convolver.prepare({48000, 128, 2}, ir, 128, 1024, backend) == Status::ok);
            bench("nonuniform128", type, backend, ir.size(), 2, [&] {
                require(convolver.process(in.block(), out.block()) == Status::ok);
                sink = out.block().channel(0)[127];
            });
        }
        {
            std::vector<T> ir(32768, static_cast<T>(0.00001));
            ScheduledConvolver<T> convolver;
            require(convolver.prepare({48000, 128, 2}, ir, 128, 1024, backend) == Status::ok);
            bench("scheduled128", type, backend, ir.size(), 2, [&] {
                require(convolver.process(in.block(), out.block()) == Status::ok);
                sink = out.block().channel(0)[127];
            });
            separator();
            std::cout << "{\"kind\":\"scheduling\",\"backend\":\"" << backendName(backend)
                      << "\",\"sampleType\":\"" << type
                      << "\",\"partitionFrames\":1024,\"workUnitsPerPartition\":"
                      << convolver.workUnitsPerPartition()
                      << ",\"maximumWorkUnitsPerFrame\":" << convolver.maximumWorkUnitsPerFrame()
                      << "}";
        }
        for (auto mode :
             {ConvolutionMode::hybrid, ConvolutionMode::nonuniform, ConvolutionMode::scheduled}) {
            const ConvolutionConfig config{mode, 128, 1024};
            const auto configuredName = mode == ConvolutionMode::hybrid ? "configuredHybrid128"
                                        : mode == ConvolutionMode::nonuniform
                                            ? "configuredNonuniform128"
                                            : "configuredScheduled128";
            const auto smoothName = mode == ConvolutionMode::hybrid ? "smoothedHybridFade128"
                                    : mode == ConvolutionMode::nonuniform
                                        ? "smoothedNonuniformFade128"
                                        : "smoothedScheduledFade128";
            const auto asyncName = mode == ConvolutionMode::hybrid       ? "asyncHybridFade128"
                                   : mode == ConvolutionMode::nonuniform ? "asyncNonuniformFade128"
                                                                         : "asyncScheduledFade128";
            std::vector<T> ir(32768, static_cast<T>(0.00001));
            Convolver<T> configured;
            SmoothedConvolver<T> smooth;
            AsyncConvolver<T> asynchronous;
            const ProcessSpec spec{48000, 128, 2};
            require(configured.prepare(spec, ir, config, backend) == Status::ok &&
                    smooth.prepare(spec, ir, config, backend) == Status::ok &&
                    asynchronous.prepare(spec, ir, config, backend) == Status::ok);
            bench(configuredName, type, backend, ir.size(), 2, [&] {
                require(configured.process(in.block(), out.block()) == Status::ok);
                sink = out.block().channel(0)[127];
            });
            // Preparation/publication and prewarming are outside the measured fade interval.
            // The long duration keeps both engines active for all warmups and timed samples.
            require(smooth.stageImpulse(ir) == Status::ok &&
                    smooth.startTransition(1u << 20) == Status::ok &&
                    asynchronous.stageImpulse(ir, 1u << 20) == Status::ok);
            for (std::size_t f = 0; f < ir.size() - 1;) {
                const auto count = std::min(std::size_t{128}, ir.size() - 1 - f);
                require(smooth.process(in.block(count), out.block(count)) == Status::ok &&
                        asynchronous.process(in.block(count), out.block(count)) == Status::ok);
                f += count;
            }
            require(smooth.warmupRemainingFrames() == 0 &&
                    asynchronous.warmupRemainingFrames() == 0);
            bench(smoothName, type, backend, ir.size(), 2, [&] {
                require(smooth.process(in.block(), out.block()) == Status::ok);
                sink = out.block().channel(0)[127];
            });
            bench(asyncName, type, backend, ir.size(), 2, [&] {
                require(asynchronous.process(in.block(), out.block()) == Status::ok);
                sink = out.block().channel(0)[127];
            });
            require(smooth.isTransitioning() && asynchronous.isTransitioning());
        }
        {
            constexpr std::size_t channels = 4, taps = 8192;
            std::vector<T> impulse(taps, static_cast<T>(0.00001));
            std::array<ConvolutionRoute<T>, channels * channels> routes;
            std::array<NonuniformConvolver<T>, channels * channels> independent;
            MatrixConvolver<T> matrix, scheduledMatrix;
            AudioBuffer<T> source, target, mono;
            require(source.resize(channels, 128) == Status::ok &&
                    target.resize(channels, 128) == Status::ok &&
                    mono.resize(1, 128) == Status::ok);
            for (std::size_t ch = 0; ch < channels; ++ch)
                std::fill(source.block().channel(ch).begin(), source.block().channel(ch).end(),
                          T{0.125});
            for (std::size_t r = 0; r < routes.size(); ++r) {
                routes[r] = {r % channels, r / channels, impulse};
                require(independent[r].prepare({48000, 128, 1}, impulse, 128, 1024, backend) ==
                        Status::ok);
            }
            require(matrix.prepare({48000, 128, channels}, channels, routes, 128, 1024, backend) ==
                    Status::ok);
            require(scheduledMatrix.prepare({48000, 128, channels}, channels, routes,
                                            {ConvolutionMode::scheduled, 128, 1024},
                                            backend) == Status::ok);
            bench("matrixScheduled128", type, backend, taps, channels, [&] {
                require(scheduledMatrix.process(source.block(), target.block()) == Status::ok);
                sink = target.block().channel(0)[127];
            });
            separator();
            std::cout
                << "{\"kind\":\"scheduling\",\"operation\":\"matrixScheduled128\",\"backend\":\""
                << backendName(backend) << "\",\"sampleType\":\"" << type << "\",\"size\":" << taps
                << ",\"channels\":4,\"routes\":16,\"partitionFrames\":1024,"
                   "\"workUnitsPerPartition\":"
                << scheduledMatrix.workUnitsPerPartition()
                << ",\"maximumWorkUnitsPerFrame\":" << scheduledMatrix.maximumWorkUnitsPerFrame()
                << "}";
            bench("matrixShared128", type, backend, taps, channels, [&] {
                require(matrix.process(source.block(), target.block()) == Status::ok);
                sink = target.block().channel(0)[127];
            });
            bench("matrixIndependent128", type, backend, taps, channels, [&] {
                for (std::size_t ch = 0; ch < channels; ++ch)
                    ops.fill(target.block().channel(ch), T{});
                for (std::size_t r = 0; r < routes.size(); ++r) {
                    const T* input = source.block().channel(routes[r].inputChannel).data();
                    require(independent[r].process({&input, 1, 128}, mono.block()) == Status::ok);
                    require(ops.addScaled(mono.block().channel(0),
                                          target.block().channel(routes[r].outputChannel),
                                          T{1}) == Status::ok);
                }
                sink = target.block().channel(0)[127];
            });
        }
        {
            constexpr std::size_t channels = 4, taps = 32768;
            const ProcessSpec spec{48000, 128, channels};
            std::vector<T> impulse(taps, static_cast<T>(0.00001));
            std::array<ConvolutionRoute<T>, channels * channels> routes;
            for (std::size_t r = 0; r < routes.size(); ++r)
                routes[r] = {r % channels, r / channels, impulse};
            AudioBuffer<T> source, target;
            require(source.resize(channels, 128) == Status::ok &&
                    target.resize(channels, 128) == Status::ok);
            for (std::size_t ch = 0; ch < channels; ++ch)
                std::fill(source.block().channel(ch).begin(), source.block().channel(ch).end(),
                          T{0.125});
            MatrixConvolver<T> normal, scheduled;
            require(normal.prepare(spec, channels, routes, 128, 1024, backend) == Status::ok &&
                    scheduled.prepare(spec, channels, routes,
                                      {ConvolutionMode::scheduled, 128, 1024},
                                      backend) == Status::ok);
            bench("matrixNonuniform128", type, backend, taps, channels, [&] {
                require(normal.process(source.block(), target.block()) == Status::ok);
                sink = target.block().channel(0)[127];
            });
            bench("matrixScheduled128", type, backend, taps, channels, [&] {
                require(scheduled.process(source.block(), target.block()) == Status::ok);
                sink = target.block().channel(0)[127];
            });
            separator();
            std::cout
                << "{\"kind\":\"scheduling\",\"operation\":\"matrixScheduled128\",\"backend\":\""
                << backendName(backend) << "\",\"sampleType\":\"" << type << "\",\"size\":" << taps
                << ",\"channels\":4,\"routes\":16,\"partitionFrames\":1024,"
                   "\"workUnitsPerPartition\":"
                << scheduled.workUnitsPerPartition()
                << ",\"maximumWorkUnitsPerFrame\":" << scheduled.maximumWorkUnitsPerFrame() << "}";
        }
        {
            constexpr std::size_t count = 32;
            const auto copy = [](void*, AudioBlock<const T> source, AudioBlock<T> target) noexcept {
                for (std::size_t ch = 0; ch < source.channels(); ++ch)
                    std::copy(source.channel(ch).begin(), source.channel(ch).end(),
                              target.channel(ch).begin());
                return Status::ok;
            };
            std::array<GraphInput, count> edges;
            std::array<GraphNode<T>, count> nodes;
            for (std::size_t n = 0; n < count; ++n) {
                edges[n] = {n ? n - 1 : graphExternalInput, 0.999};
                nodes[n] = {nullptr, nullptr, {&edges[n], 1}};
            }
            const std::array<GraphInput, 1> ends{{{count - 1, 1}}};
            AudioGraph<T> fused, callbacks;
            require(fused.prepare({48000, 128, 2}, nodes, ends, backend) == Status::ok);
            for (auto& node : nodes)
                node.process = copy;
            require(callbacks.prepare({48000, 128, 2}, nodes, ends, backend) == Status::ok);
            bench("graphFused32x128", type, backend, count, 2, [&] {
                require(fused.process(in.block(), out.block()) == Status::ok);
                sink = out.block().channel(0)[127];
            });
            bench("graphCallbacks32x128", type, backend, count, 2, [&] {
                require(callbacks.process(in.block(), out.block()) == Status::ok);
                sink = out.block().channel(0)[127];
            });
        }
        {
            Resampler<T> variable;
            AudioBuffer<T> source;
            bool high = false;
            require(variable.prepare({48000, 48000, 2, 512, 128, 128, 1024, 0.75, 3}, backend) ==
                        Status::ok &&
                    source.resize(2, 512) == Status::ok);
            bench("variableResample128", type, backend, 128, 2, [&] {
                high = !high;
                require(variable.setRatio(high ? 1.2 : 0.9, 512) == Status::ok);
                const auto result = variable.process(source.block(), out.block());
                require(result.status == Status::ok && result.outputProduced == 128);
                sink = out.block().channel(0)[127];
            });
        }
        Resampler<T> rate;
        require(rate.prepare({48000, 44100, 2, 128, 128, 64, 1024}, backend) == Status::ok);
        bench("resample48to44", type, backend, 128, 2, [&] {
            require(rate.process(in.block(), out.block()).status == Status::ok);
            sink = out.block().channel(0)[0];
        });
    }
}
template <class T>
double tone(std::uint32_t inputRate, std::uint32_t outputRate, double frequency, std::size_t taps,
            std::size_t phases = 1024, double analysisFrequency = 0) {
    constexpr std::size_t count = 8192;
    const auto skip =
        static_cast<std::size_t>(4 * taps * static_cast<double>(outputRate) / inputRate) + 128;
    const auto needed = count + skip;
    Resampler<T> rate;
    require(rate.prepare({inputRate, outputRate, 1, 257, 1024, taps, phases}) == Status::ok);
    AudioBuffer<T> input, output;
    require(input.resize(1, 257) == Status::ok && output.resize(1, 1024) == Status::ok);
    std::vector<double> samples;
    samples.reserve(needed + 1024);
    std::size_t position = 0;
    while (samples.size() < needed) {
        for (std::size_t i = 0; i < 257; ++i)
            input.block().channel(0)[i] = static_cast<T>(std::sin(
                2 * std::numbers::pi * frequency * static_cast<double>(position + i) / inputRate));
        std::size_t used = 0;
        while (used < 257 && samples.size() < needed) {
            const auto result =
                rate.process(input.block().subBlock(used, 257 - used), output.block());
            require(result.status == Status::ok && (result.inputConsumed || result.outputProduced));
            used += result.inputConsumed;
            for (std::size_t i = 0; i < result.outputProduced; ++i)
                samples.push_back(output.block().channel(0)[i]);
        }
        position += used;
    }
    double re = 0, im = 0, normalization = 0;
    for (std::size_t i = 0; i < count; ++i) {
        const auto angle = 2 * std::numbers::pi *
                           (analysisFrequency ? analysisFrequency : frequency) *
                           static_cast<double>(i) / outputRate;
        const auto weight =
            analysisFrequency
                ? 0.5 - 0.5 * std::cos(2 * std::numbers::pi * static_cast<double>(i) / count)
                : 1.0;
        normalization += weight;
        re += weight * samples[skip + i] * std::cos(angle);
        im += weight * samples[skip + i] * std::sin(angle);
    }
    return 2 * std::hypot(re, im) / normalization;
}
void quality() {
    for (const auto& rates : {std::pair{48000u, 44100u}, std::pair{44100u, 48000u},
                              std::pair{48000u, 16000u}, std::pair{96000u, 44100u}})
        for (std::size_t taps : {64u, 128u, 256u}) {
            const auto limit = static_cast<double>(std::min(rates.first, rates.second)) * 0.5;
            for (const double fraction : {0.01, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8}) {
                const auto bin = std::max(1.0, std::round(limit * fraction * 8192 / rates.second));
                const auto frequency = bin * rates.second / 8192;
                const auto gain = tone<double>(rates.first, rates.second, frequency, taps);
                require(isFinite(gain) && gain > 0);
                separator();
                std::cout << "{\"kind\":\"quality\",\"metric\":\"passbandGainDb\",\"inputRate\":"
                          << rates.first << ",\"outputRate\":" << rates.second
                          << ",\"taps\":" << taps << ",\"frequencyHz\":" << frequency
                          << ",\"valueDb\":" << 20 * std::log10(gain) << "}";
            }
            if (rates.first > rates.second) {
                for (int point = 0; point < 12; ++point) {
                    const auto low = 0.525 * rates.second, high = 0.49 * rates.first;
                    if (low >= high)
                        continue;
                    const auto desired = low + (high - low) * point / 11;
                    const auto frequency =
                        std::round(desired * 8192 / rates.second) * rates.second / 8192;
                    const auto gain = tone<double>(rates.first, rates.second, frequency, taps);
                    require(isFinite(gain));
                    separator();
                    std::cout
                        << "{\"kind\":\"quality\",\"metric\":\"stopbandGainDb\",\"inputRate\":"
                        << rates.first << ",\"outputRate\":" << rates.second << ",\"taps\":" << taps
                        << ",\"frequencyHz\":" << frequency
                        << ",\"valueDb\":" << 20 * std::log10(std::max(gain, 1e-15)) << "}";
                }
            }
        }
}
void presetQuality() {
    for (const auto& rates : {std::pair{48000u, 44100u}, std::pair{44100u, 48000u},
                              std::pair{48000u, 16000u}, std::pair{96000u, 44100u}})
        for (const auto quality :
             {ResampleQuality::economy, ResampleQuality::balanced, ResampleQuality::high}) {
            const auto spec =
                resampleSpecForQuality({rates.first, rates.second, 1, 257, 1024}, quality);
            const auto limit = static_cast<double>(std::min(rates.first, rates.second)) * 0.5;
            double ripple = 0, worstStop = -300;
            for (const auto fraction : {0.01, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8}) {
                const auto frequency =
                    std::max(1.0, std::round(limit * fraction * 8192 / rates.second)) *
                    rates.second / 8192;
                const auto gain =
                    tone<double>(rates.first, rates.second, frequency, spec.taps, spec.phases);
                ripple = std::max(ripple, std::abs(20 * std::log10(std::max(gain, 1e-15))));
            }
            if (rates.first > rates.second)
                for (int point = 0; point < 12; ++point) {
                    const auto low = 0.525 * rates.second, high = 0.49 * rates.first;
                    if (low >= high)
                        continue;
                    const auto frequency =
                        std::round((low + (high - low) * point / 11) * 8192 / rates.second) *
                        rates.second / 8192;
                    const auto gain =
                        tone<double>(rates.first, rates.second, frequency, spec.taps, spec.phases);
                    worstStop = std::max(worstStop, 20 * std::log10(std::max(gain, 1e-15)));
                }
            separator();
            std::cout << "{\"kind\":\"quality\",\"metric\":\"resamplePreset\",\"preset\":"
                      << static_cast<int>(quality) << ",\"inputRate\":" << rates.first
                      << ",\"outputRate\":" << rates.second << ",\"taps\":" << spec.taps
                      << ",\"phases\":" << spec.phases
                      << ",\"maxSampledPassbandErrorDb\":" << ripple
                      << ",\"worstSampledStopbandGainDb\":" << worstStop << "}";
            require(ripple < 0.1 && (rates.first <= rates.second || worstStop < -60));
        }
}
void imageQuality() {
    for (const auto quality :
         {ResampleQuality::economy, ResampleQuality::balanced, ResampleQuality::high}) {
        const auto spec = resampleSpecForQuality({16000, 48000, 1, 257, 1024}, quality);
        const auto amplitude = tone<double>(16000, 48000, 1000, spec.taps, spec.phases, 15000);
        const auto db = 20 * std::log10(std::max(amplitude, 1e-15));
        separator();
        std::cout << "{\"kind\":\"quality\",\"metric\":\"upsampleImageGainDb\",\"inputRate\":16000,"
                     "\"outputRate\":48000,\"sourceHz\":1000,\"analysisHz\":15000,\"preset\":"
                  << static_cast<int>(quality) << ",\"valueDb\":" << db << "}";
        require(db < -60);
    }
}
template <class T> void fftQuality() {
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::neon,
                         CpuBackend::avx512, CpuBackend::sve})
        for (std::size_t n : {64u, 256u, 1024u, 4096u}) {
            ComplexFft<T> fft;
            const auto status = fft.prepare(n, backend);
            if (status == Status::unsupported_backend)
                continue;
            require(status == Status::ok);
            std::vector<std::complex<T>> input(n), spectrum(n), output(n);
            for (std::size_t i = 0; i < n; ++i)
                input[i] = {static_cast<T>(std::sin(static_cast<double>(i) * 0.13)),
                            static_cast<T>(std::cos(static_cast<double>(i) * 0.17))};
            require(fft.process(input, spectrum, FftDirection::forward) == Status::ok &&
                    fft.process(spectrum, output, FftDirection::inverse) == Status::ok);
            double error = 0;
            for (std::size_t i = 0; i < n; ++i)
                error = std::max(error, static_cast<double>(std::abs(input[i] - output[i])));
            require(error < (std::same_as<T, float> ? 1e-4 : 1e-10));
            separator();
            std::cout
                << "{\"kind\":\"quality\",\"metric\":\"fftRoundtripMaxError\",\"sampleType\":\""
                << (std::same_as<T, float> ? "float" : "double") << "\",\"backend\":\""
                << backendName(backend) << "\",\"size\":" << n << ",\"value\":" << error << "}";
        }
}
void transitionQuality() {
    SmoothedConvolver<double> convolver;
    const std::array<double, 1> from{1}, to{-1};
    require(convolver.prepare({48000, 64, 1}, from, 8) == Status::ok &&
            convolver.stageImpulse(to) == Status::ok &&
            convolver.startTransition(512) == Status::ok);
    AudioBuffer<double> input, output;
    require(input.resize(1, 64) == Status::ok && output.resize(1, 64) == Status::ok);
    std::fill(input.block().channel(0).begin(), input.block().channel(0).end(), 1.0);
    double previous = 1, maximumStep = 0, error = 0;
    std::size_t frame = 0;
    for (int block = 0; block < 10; ++block) {
        require(convolver.process(input.block(), output.block()) == Status::ok);
        for (auto value : output.block().channel(0)) {
            maximumStep = std::max(maximumStep, std::abs(value - previous));
            const auto expected =
                1 - 2 * static_cast<double>(std::min(frame + 1, std::size_t{512})) / 512;
            error = std::max(error, std::abs(value - expected));
            previous = value;
            ++frame;
        }
    }
    require(error < 1e-12 && maximumStep <= 2.0 / 512 + 1e-12);
    separator();
    std::cout << "{\"kind\":\"quality\",\"metric\":\"unitStepIRFadeMaxSampleStep\",\"fadeFrames\":"
                 "512,\"value\":"
              << maximumStep << ",\"trajectoryMaxError\":" << error << "}";
}
template <class T> void configuredTransitionQuality() {
    constexpr std::size_t frames = 977, event = 131, fade = 37, length = 273;
    AudioBuffer<T> input, a, b;
    require(input.resize(2, frames) == Status::ok && a.resize(2, frames) == Status::ok &&
            b.resize(2, frames) == Status::ok);
    const std::array<T, 1> oldIr{T{1}};
    std::vector<T> ir(length);
    for (std::size_t k = 0; k < length; ++k)
        ir[k] = static_cast<T>(static_cast<int>((k * 3) % 13) - 6) * T{0.015625};
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t f = 0; f < frames; ++f)
            input.block().channel(ch)[f] =
                f < 501 ? static_cast<T>(static_cast<int>((f + ch * 5) % 17) - 8) * T{0.125} : T{};
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::neon,
                         CpuBackend::avx512, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        for (auto mode :
             {ConvolutionMode::hybrid, ConvolutionMode::nonuniform, ConvolutionMode::scheduled}) {
            SmoothedConvolver<T> smooth;
            AsyncConvolver<T> asynchronous;
            const ConvolutionConfig config{mode, 8, 64};
            require(smooth.prepare({48000, 53, 2}, oldIr, config, backend) == Status::ok &&
                    asynchronous.prepare({48000, 53, 2}, oldIr, config, backend) == Status::ok);
            for (std::size_t f = 0; f < frames;) {
                if (f == event)
                    require(smooth.stageImpulse(ir) == Status::ok &&
                            smooth.startTransition(fade) == Status::ok &&
                            asynchronous.stageImpulse(ir, fade) == Status::ok);
                auto count = std::min(frames - f, 1 + f % 53);
                if (f < event)
                    count = std::min(count, event - f);
                require(smooth.process(input.block().subBlock(f, count),
                                       a.block().subBlock(f, count)) == Status::ok &&
                        asynchronous.process(input.block().subBlock(f, count),
                                             b.block().subBlock(f, count)) == Status::ok);
                f += count;
            }
            double error = 0;
            for (std::size_t ch = 0; ch < 2; ++ch)
                for (std::size_t f = 0; f < frames; ++f) {
                    double value = 0;
                    if (f >= event)
                        for (std::size_t k = 0; k < length && k <= f - event; ++k)
                            value += static_cast<double>(ir[k]) * input.block().channel(ch)[f - k];
                    const auto blend =
                        f < event + length - 1
                            ? 0.0
                            : std::min(1.0,
                                       static_cast<double>(f - (event + length - 1) + 1) / fade);
                    const auto old = static_cast<double>(input.block().channel(ch)[f]);
                    const auto expected = old + (value - old) * blend;
                    error = std::max({error, std::abs(a.block().channel(ch)[f] - expected),
                                      std::abs(b.block().channel(ch)[f] - expected)});
                }
            require(error < (std::same_as<T, float> ? 1e-4 : 1e-10));
            separator();
            std::cout
                << "{\"kind\":\"quality\",\"metric\":\"configuredIrTransitionMaxError\",\"mode\":\""
                << (mode == ConvolutionMode::hybrid       ? "hybrid"
                    : mode == ConvolutionMode::nonuniform ? "nonuniform"
                                                          : "scheduled")
                << "\",\"sampleType\":\"" << (std::same_as<T, float> ? "float" : "double")
                << "\",\"backend\":\"" << backendName(backend) << "\",\"value\":" << error
                << ",\"impulseFrames\":" << length << ",\"updateFrame\":" << event
                << ",\"warmupFrames\":" << length - 1 << ",\"fadeFrames\":" << fade << "}";
        }
    }
}
template <class T> void scheduledMatrixQuality() {
    constexpr std::size_t frames = 2049, sourceFrames = 769, ins = 4, outs = 5;
    AudioBuffer<T> source, output;
    require(source.resize(ins, frames) == Status::ok && output.resize(outs, frames) == Status::ok);
    for (std::size_t ch = 0; ch < ins; ++ch)
        for (std::size_t f = 0; f < sourceFrames; ++f)
            source.block().channel(ch)[f] =
                static_cast<T>(static_cast<int>((f * 7 + ch * 3) % 19) - 9) * T{0.0625};
    std::array<std::vector<T>, 6> impulses;
    const std::array<std::size_t, 6> lengths{1, 128, 129, 273, 521, 273}, from{3, 0, 2, 3, 1, 3},
        to{0, 1, 2, 4, 0, 4};
    std::array<ConvolutionRoute<T>, 6> routes;
    std::vector<double> expected(outs * frames);
    for (std::size_t r = 0; r < routes.size(); ++r) {
        impulses[r].resize(lengths[r]);
        for (std::size_t k = 0; k < lengths[r]; ++k)
            impulses[r][k] = static_cast<T>(static_cast<int>((k + 3 * r) % 13) - 6) * T{0.0078125};
        routes[r] = {from[r], to[r], impulses[r]};
        for (std::size_t k = 0; k < lengths[r]; ++k)
            for (std::size_t f = 0; f < sourceFrames; ++f)
                expected[to[r] * frames + f + k] +=
                    static_cast<double>(impulses[r][k]) * source.block().channel(from[r])[f];
    }
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::neon,
                         CpuBackend::avx512, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        MatrixConvolver<T> matrix;
        require(matrix.prepare({48000, 137, ins}, outs, routes, {ConvolutionMode::scheduled, 8, 64},
                               backend) == Status::ok);
        for (std::size_t f = 0; f < frames;) {
            const auto count = std::min(frames - f, 1 + (f * 3) % 137);
            require(matrix.process(source.block().subBlock(f, count),
                                   output.block().subBlock(f, count)) == Status::ok);
            f += count;
        }
        double error = 0;
        for (std::size_t ch = 0; ch < outs; ++ch)
            for (std::size_t f = 0; f < frames; ++f)
                error = std::max(
                    error, std::abs(output.block().channel(ch)[f] - expected[ch * frames + f]));
        require(error < (std::same_as<T, float> ? 1e-4 : 1e-10));
        separator();
        std::cout << "{\"kind\":\"quality\",\"metric\":\"scheduledMimoMaxError\",\"backend\":\""
                  << backendName(backend) << "\",\"sampleType\":\""
                  << (std::same_as<T, float> ? "float" : "double") << "\",\"value\":" << error
                  << ",\"inputChannels\":4,\"outputChannels\":5,\"routes\":6,\"latePartition\":64,"
                     "\"latencyFrames\":"
                  << matrix.latencyFrames() << "}";
    }
}
void capabilities() {
    const auto cpu = detectCpuFeatures();
    separator();
    std::cout << "{\"kind\":\"capabilities\",\"avx512CpuOs\":" << (cpu.avx512 ? "true" : "false")
              << ",\"sveCpuOs\":" << (cpu.sve ? "true" : "false") << ",\"availableBackends\":[";
    bool comma = false;
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::neon,
                         CpuBackend::avx512, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        if (comma)
            std::cout << ',';
        std::cout << '"' << backendName(backend) << '"';
        comma = true;
    }
    std::cout << "]}";
}
void variableQuality() {
    Resampler<double> rate;
    require(rate.prepare({48000, 48000, 1, 512, 128, 256, 1024, 0.75, 3}) == Status::ok);
    AudioBuffer<double> source, output;
    require(source.resize(1, 512) == Status::ok && output.resize(1, 128) == Status::ok);
    std::fill(source.block().channel(0).begin(), source.block().channel(0).end(), 1.0);
    double error = 0, step = 0, previous = 1;
    std::size_t produced = 0;
    while (produced < 5120) {
        if (produced == 512)
            require(rate.setRatio(3, 2048) == Status::ok);
        if (produced == 2560)
            require(rate.setRatio(0.75, 2048) == Status::ok);
        const auto result = rate.process(source.block(), output.block());
        require(result.status == Status::ok && result.outputProduced == 128);
        if (produced >= 512)
            for (auto value : output.block().channel(0)) {
                error = std::max(error, std::abs(value - 1));
                step = std::max(step, std::abs(value - previous));
                previous = value;
            }
        produced += 128;
    }
    require(error < 1e-10 && step < 1e-10);
    separator();
    std::cout << "{\"kind\":\"quality\",\"metric\":\"variableRatioDc\",\"maximumError\":" << error
              << ",\"maximumSampleStep\":" << step << ",\"minimumRatio\":0.75,\"maximumRatio\":3}";
}
template <class T> void scheduledQuality() {
    constexpr std::size_t frames = 2049, sourceFrames = 769, impulseFrames = 521;
    AudioBuffer<T> source, output;
    require(source.resize(2, frames) == Status::ok && output.resize(2, frames) == Status::ok);
    std::vector<T> impulse(impulseFrames);
    for (std::size_t k = 0; k < impulse.size(); ++k)
        impulse[k] = static_cast<T>(static_cast<int>((k * 7) % 29) - 14) * T{0.00390625};
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t f = 0; f < sourceFrames; ++f)
            source.block().channel(ch)[f] =
                static_cast<T>(static_cast<int>((f * 3 + ch * 5) % 31) - 15) * T{0.0625};
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::neon,
                         CpuBackend::avx512, CpuBackend::sve}) {
        ScheduledConvolver<T> convolver;
        const auto status = convolver.prepare({48000, 137, 2}, impulse, 8, 64, backend);
        if (status == Status::unsupported_backend)
            continue;
        require(status == Status::ok);
        for (std::size_t f = 0; f < frames;) {
            const auto count = std::min(frames - f, std::size_t{137});
            require(convolver.process(source.block().subBlock(f, count),
                                      output.block().subBlock(f, count)) == Status::ok);
            f += count;
        }
        double error = 0;
        for (std::size_t ch = 0; ch < 2; ++ch)
            for (std::size_t f = 0; f < frames; ++f) {
                double expected = 0;
                for (std::size_t k = 0; k < impulse.size() && k <= f; ++k)
                    expected += static_cast<double>(impulse[k]) * source.block().channel(ch)[f - k];
                error = std::max(
                    error, std::abs(static_cast<double>(output.block().channel(ch)[f]) - expected));
            }
        require(error < (std::same_as<T, float> ? 1e-4 : 1e-10));
        separator();
        std::cout
            << "{\"kind\":\"quality\",\"metric\":\"scheduledConvolutionMaxError\",\"backend\":\""
            << backendName(backend) << "\",\"sampleType\":\""
            << (std::same_as<T, float> ? "float" : "double")
            << "\",\"inputFrames\":769,\"impulseFrames\":521,\"latePartition\":64,\"blockLimit\":"
               "137,\"latencyFrames\":0,\"value\":"
            << error << "}";
    }
}
template <class T> void expandedMeasurements() {
    const ProcessSpec spec{48000, 128, 2};
    AudioBuffer<T> input, output;
    require(input.resize(2, 128) == Status::ok && output.resize(2, 128) == Status::ok);
    for (std::size_t ch = 0; ch < 2; ++ch)
        std::fill(input.block().channel(ch).begin(), input.block().channel(ch).end(), T{.125});
    const auto* type = std::same_as<T, float> ? "float" : "double";
    auto measure = [&](const char* name, auto& processor) {
        require(processor.prepare(spec) == Status::ok);
        bench(name, type, CpuBackend::scalar, 128, 2, [&] {
            require(processor.process(input.block(), output.block()) == Status::ok);
            sink = output.block().channel(0)[127];
        });
    };
    Dynamics<T> dynamics;
    measure("dynamicsCompressor", dynamics);
    TptFilter<T> filter;
    measure("stateVariableTpt", filter);
    LadderFilter<T> ladder;
    measure("ladderLowpass", ladder);
    Chorus<T> chorus;
    measure("chorus", chorus);
    Phaser<T> phaser;
    measure("phaser", phaser);
    Reverb<T> reverb;
    measure("schroederReverb", reverb);
    Oversampler<T> over;
    require(over.prepare(spec, 4) == Status::ok);
    bench("oversample4Roundtrip", type, CpuBackend::automatic, 4, 2, [&] {
        AudioBlock<T> high;
        require(over.processUp(input.block(), high) == Status::ok &&
                over.processDown(output.block()) == Status::ok);
        sink = output.block().channel(0)[127];
    });
    // Third harmonic of 9.6 kHz aliases to 19.2 kHz without oversampling.
    over.reset();
    std::complex<double> alias{};
    double phase = 0;
    std::size_t count = 0;
    for (unsigned block = 0; block < 400; ++block) {
        for (std::size_t i = 0; i < 128; ++i) {
            const auto x = static_cast<T>(.8 * std::sin(phase));
            phase += 2 * std::numbers::pi * .2;
            if (phase >= 2 * std::numbers::pi)
                phase -= 2 * std::numbers::pi;
            input.block().channel(0)[i] = input.block().channel(1)[i] = x;
        }
        AudioBlock<T> high;
        require(over.processUp(input.block(), high) == Status::ok);
        for (std::size_t ch = 0; ch < 2; ++ch)
            for (auto& x : high.channel(ch))
                x = x * x * x;
        require(over.processDown(output.block()) == Status::ok);
        if (block >= 20)
            for (std::size_t i = 0; i < 128; ++i) {
                alias += static_cast<double>(output.block().channel(0)[i]) *
                         std::polar(1.0, -2 * std::numbers::pi * .4 * static_cast<double>(count++));
            }
    }
    const auto amplitude = 2 * std::abs(alias) / static_cast<double>(count),
               reference = std::pow(.8, 3) / 4,
               db = 20 * std::log10(std::max(1e-20, amplitude / reference));
    require(db < -40);
    separator();
    std::cout << "{\"kind\":\"quality\",\"metric\":\"oversamplingCubicAliasReductionDb\","
                 "\"sampleType\":\""
              << type
              << "\",\"factor\":4,\"sampleRate\":48000,\"inputFrequencyHz\":9600,"
                 "\"aliasFrequencyHz\":19200,\"latencyFrames\":"
              << over.latencyFrames() << ",\"value\":" << db << "}";
}
template <class T> void dawMeasurements() {
    const char* type = std::same_as<T, float> ? "float" : "double";
    VectorOps selected;
    require(VectorOps::create(CpuBackend::automatic, selected) == Status::ok);
    const auto backend = selected.backend();
    AudioBuffer<T> input, output;
    require(input.resize(2, 1024) == Status::ok && output.resize(2, 128) == Status::ok);
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t i = 0; i < 1024; ++i)
            input.block().channel(ch)[i] = static_cast<T>(
                .2 * std::sin(.13 * static_cast<double>(i) + static_cast<double>(ch)));
    SincInterpolator<T> sinc;
    require(sinc.prepare({44100, 128, 2}, 48000, 64, 1024, backend) == Status::ok);
    bench("randomAccessSinc128", type, backend, 64, 2, [&] {
        require(sinc.process(input.block(), 64.375, output.block()) == Status::ok);
        sink = output.block().channel(0)[127];
    });
#if defined(CYDSP_MEASURE_STRETCH)
    signalsmith::stretch::SignalsmithStretch<T> original(1);
    detail::StretchAlgorithm<T> integrated(1);
    integrated.select(backend);
    original.configure(2, 4096, 1024, false);
    integrated.configure(2, 4096, 1024, false);
    const T* in[]{input.block().channel(0).data(), input.block().channel(1).data()};
    T* out[]{output.block().channel(0).data(), output.block().channel(1).data()};
    bench("stretchOriginal4096", type, CpuBackend::scalar, 4096, 2, [&] {
        original.process(in, 128, out, 128);
        sink = out[0][127];
    });
    bench("stretchCyDSP4096", type, backend, 4096, 2, [&] {
        integrated.process(in, 128, out, 128);
        sink = out[0][127];
    });
    // Typical preset as a separate comparison: FFT padding may differ from upstream.
    original.presetDefault(2, T{48000}, true);
    integrated.presetDefault(2, T{48000}, true);
    bench("stretchOriginalPreset", type, CpuBackend::scalar,
          static_cast<std::size_t>(original.blockSamples()), 2, [&] {
              original.process(in, 128, out, 128);
              sink = out[0][127];
          });
    bench("stretchCyDSPPreset", type, backend, static_cast<std::size_t>(integrated.blockSamples()),
          2, [&] {
              integrated.process(in, 128, out, 128);
              sink = out[0][127];
          });
#endif
}

template <class T> void integrationMeasurements() {
    const auto* type = std::same_as<T, float> ? "float" : "double";
    const ProcessSpec spec{48000, 128, 2};
    AudioBuffer<T> input, output;
    require(input.resize(2, 128) == Status::ok && output.resize(2, 128) == Status::ok);
    for (std::size_t c = 0; c < 2; ++c)
        for (std::size_t i = 0; i < 128; ++i)
            input.block().channel(c)[i] = static_cast<T>(.2 * std::sin(double(i)));
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::avx512,
                         CpuBackend::neon, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        for (auto mode : {OversamplingMode::halfBandFir, OversamplingMode::iir}) {
            Oversampler<T> over;
            require(over.prepare(spec, 4, mode, backend) == Status::ok);
            bench(mode == OversamplingMode::iir ? "oversample4IirRoundtrip"
                                                : "oversample4HalfBandRoundtrip",
                  type, backend, 4, 2, [&] {
                      AudioBlock<T> high;
                      require(over.processUp(input.block(), high) == Status::ok &&
                              over.processDown(output.block()) == Status::ok);
                      sink = output.block().channel(0)[127];
                  });
        }
        Dynamics<T> gate;
        require(gate.prepare(spec, DynamicsMode::noiseGate) == Status::ok);
        DynamicsParameters p;
        p.holdMs = 10;
        p.hysteresisDb = 6;
        p.lookAheadMs = 5;
        require(gate.setParameters(p) == Status::ok);
        bench("gateLookahead5ms", type, CpuBackend::scalar, 240, 2, [&] {
            require(gate.process(input.block(), output.block()) == Status::ok);
            sink = output.block().channel(0)[127];
        });
        std::vector<T> cycle(2048);
        for (std::size_t i = 0; i < cycle.size(); ++i)
            cycle[i] = static_cast<T>(2 * double(i) / double(cycle.size()) - 1);
        Wavetable<T> table;
        require(table.prepare(cycle, backend) == Status::ok);
        WavetableOscillator<T> oscillator;
        require(oscillator.prepare(spec, table) == Status::ok &&
                oscillator.setFrequency(440) == Status::ok);
        bench("wavetableOscillator2048", type, backend, 2048, 2, [&] {
            require(oscillator.process(output.block()) == Status::ok);
            sink = output.block().channel(0)[127];
        });
    }
    AudioBuffer<T> sample;
    require(sample.resize(1, 8192) == Status::ok);
    for (std::size_t i = 0; i < 8192; ++i)
        sample.block().channel(0)[i] =
            static_cast<T>(std::sin(2 * std::numbers::pi * .35 * double(i)));
    SampleData<T> data;
    require(data.prepare(sample.block(), 48000) == Status::ok);
    double linear = 0, filtered = 0;
    for (std::size_t i = 128; i < 3968; ++i) {
        auto a = double(data.value(0, double(i) * 2, 2, SampleInterpolation::linear));
        auto b = double(data.value(0, double(i) * 2, 2, SampleInterpolation::bandLimited));
        linear += a * a;
        filtered += b * b;
    }
    separator();
    std::cout
        << "{\"kind\":\"quality\",\"metric\":\"samplerOctaveAliasReductionDb\",\"sampleType\":\""
        << type << "\",\"inputFrequencyCyclesPerSample\":0.35,\"playbackRatio\":2,\"reductionDb\":"
        << 10 * std::log10(linear / std::max(filtered, 1e-300)) << "}";
}
} // namespace
int main() {
    try {
        std::cout << std::setprecision(12) << "[\n";
        capabilities();
        integrationMeasurements<float>();
        integrationMeasurements<double>();
        dawMeasurements<float>();
        dawMeasurements<double>();
        expandedMeasurements<float>();
        expandedMeasurements<double>();
        scheduledMatrixQuality<float>();
        scheduledMatrixQuality<double>();
        configuredTransitionQuality<float>();
        configuredTransitionQuality<double>();
        performance<float>();
        performance<double>();
        quality();
        presetQuality();
        imageQuality();
        fftQuality<float>();
        fftQuality<double>();
        transitionQuality();
        variableQuality();
        scheduledQuality<float>();
        scheduledQuality<double>();
        std::cout << "\n]\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
