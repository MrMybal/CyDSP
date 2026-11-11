#include <algorithm>
#include <array>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <limits>
#include <stdexcept>
#include <vector>
namespace {
using namespace cydsp;
constexpr std::array backends{CpuBackend::scalar, CpuBackend::sse2,   CpuBackend::avx2,
                              CpuBackend::neon,   CpuBackend::avx512, CpuBackend::sve};
void require(bool value, const char* why) {
    if (!value)
        throw std::runtime_error(why);
}
template <class T> constexpr double tolerance = std::same_as<T, float> ? 3e-4 : 2e-11;
template <class T> double direct(std::span<const T> x, std::span<const T> h, std::size_t f) {
    double y = 0;
    for (std::size_t k = 0; k < h.size() && k <= f; ++k)
        y += static_cast<double>(x[f - k]) * h[k];
    return y;
}
template <class T> void sharedMatrix(CpuBackend backend, ConvolutionMode mode) {
    constexpr std::size_t frames = 607, channels = 3;
    std::array<std::vector<T>, 10> impulses;
    std::array<ConvolutionRoute<T>, 10> routes;
    for (std::size_t r = 0; r < routes.size(); ++r) {
        impulses[r].resize(r == 0 ? 1 : 17 + r * 19);
        for (std::size_t k = 0; k < impulses[r].size(); ++k)
            impulses[r][k] = static_cast<T>(static_cast<int>((k + 3 * r) % 13) - 6) * T{0.0078125};
        routes[r] = {r % channels, (r / 3) % channels, impulses[r]};
    }
    for (std::size_t early : {1u, 8u, 32u})
        for (std::size_t late : {32u, 128u}) {
            MatrixConvolver<T> a, b;
            require(a.prepare({48000, frames, channels}, channels, routes, {mode, early, late},
                              backend) == Status::ok &&
                        b.prepare({48000, frames, channels}, channels, routes, {mode, early, late},
                                  backend) == Status::ok,
                    "shared MIMO prepare");
            AudioBuffer<T> input, out, inplace;
            require(input.resize(channels, frames) == Status::ok &&
                        out.resize(channels, frames) == Status::ok &&
                        inplace.resize(channels, frames) == Status::ok,
                    "shared MIMO buffers");
            for (std::size_t ch = 0; ch < channels; ++ch)
                for (std::size_t f = 0; f < frames; ++f)
                    input.block().channel(ch)[f] = inplace.block().channel(ch)[f] =
                        f < 257
                            ? static_cast<T>(static_cast<int>((f * 7 + ch) % 19) - 9) * T{0.0625}
                            : T{};
            require(a.process(input.block(), out.block()) == Status::ok, "shared MIMO whole");
            for (std::size_t f = 0; f < frames;) {
                auto n = std::min(frames - f, 1 + f % 47);
                auto block = inplace.block().subBlock(f, n);
                require(b.process(block, block) == Status::ok, "shared MIMO split");
                f += n;
                if (f >= 100 && f - n < 100) {
                    MatrixConvolver<T> moved(std::move(b));
                    require(b.process(input.block(0), inplace.block(0)) == Status::not_prepared,
                            "MIMO moved source empty");
                    b = std::move(moved);
                }
            }
            for (std::size_t ch = 0; ch < channels; ++ch)
                for (std::size_t f = 0; f < frames; ++f) {
                    double expected = 0;
                    for (const auto& route : routes)
                        if (route.outputChannel == ch)
                            expected += direct<T>(input.block().channel(route.inputChannel),
                                                  route.impulse, f);
                    require(std::abs(out.block().channel(ch)[f] - expected) < tolerance<T>,
                            "shared FFT MIMO direct oracle");
                    require(std::abs(out.block().channel(ch)[f] - inplace.block().channel(ch)[f]) <
                                tolerance<T>,
                            "shared FFT MIMO callback invariance");
                }
        }
}
template <class T> void warmTransitions(CpuBackend backend, ConvolutionMode mode) {
    constexpr std::size_t frames = 697, history = 273, fade = 37;
    const ConvolutionConfig config{mode, 8, 64};
    const std::array<T, 1> identity{T{1}};
    std::array<T, history> impulse{};
    impulse[0] = T{0.25};
    impulse[history - 1] = T{1.5};
    SmoothedConvolver<T> smooth;
    AsyncConvolver<T> asynchronous;
    require(smooth.prepare({48000, 53, 2}, identity, config, backend) == Status::ok &&
                asynchronous.prepare({48000, 53, 2}, identity, config, backend) == Status::ok,
            "warm plans");
    require(smooth.stageImpulse(impulse) == Status::ok &&
                smooth.stageImpulse({}) == Status::invalid_argument && smooth.hasStagedImpulse() &&
                smooth.startTransition(fade) == Status::ok &&
                smooth.warmupRemainingFrames() == history - 1 &&
                asynchronous.stageImpulse(impulse, fade) == Status::ok &&
                asynchronous.stageImpulse({}, fade) == Status::invalid_argument &&
                asynchronous.hasPendingImpulse() && smooth.configuration() == config &&
                asynchronous.configuration() == config,
            "warm activation");
    AudioBuffer<T> input, a, b;
    require(input.resize(2, frames) == Status::ok && a.resize(2, frames) == Status::ok &&
                b.resize(2, frames) == Status::ok,
            "warm buffers");
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t f = 0; f < frames; ++f)
            input.block().channel(ch)[f] = a.block().channel(ch)[f] = b.block().channel(ch)[f] =
                static_cast<T>(static_cast<int>((f + ch * 3) % 17) - 8) * T{0.125};
    for (std::size_t f = 0; f < frames;) {
        auto count = std::min(frames - f, 1 + f % 53);
        auto aa = a.block().subBlock(f, count), bb = b.block().subBlock(f, count);
        require(smooth.process(aa, aa) == Status::ok && asynchronous.process(bb, bb) == Status::ok,
                "warm in-place");
        if (f < 81 && f + count >= 81) {
            SmoothedConvolver<T> moved(std::move(smooth));
            require(smooth.process(aa, aa) == Status::not_prepared, "warm moved source empty");
            smooth = std::move(moved);
            require(smooth.configuration() == config, "warm move retains engine configuration");
        }
        f += count;
    }
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t f = 0; f < frames; ++f) {
            const auto blend =
                f < history - 1 ? 0.0
                                : std::min(1.0, static_cast<double>(f - (history - 1) + 1) / fade);
            const auto old = static_cast<double>(input.block().channel(ch)[f]);
            const auto expected =
                old + (direct<T>(input.block().channel(ch), impulse, f) - old) * blend;
            require(std::abs(a.block().channel(ch)[f] - expected) < tolerance<T> &&
                        std::abs(b.block().channel(ch)[f] - expected) < tolerance<T>,
                    "warm IR full-history direct reference");
        }
    require(!smooth.isTransitioning() && !asynchronous.isTransitioning(), "warm transitions end");
    // An immediate activation still waits for a complete new history.
    smooth.reset();
    require(smooth.stageImpulse(impulse) == Status::ok && smooth.startTransition(0) == Status::ok,
            "warm immediate stage");
    require(smooth.process(input.block(0), a.block(0)) == Status::ok &&
                smooth.warmupRemainingFrames() == history - 1,
            "empty callback retains warmup");
    smooth.reset();
    require(!smooth.isTransitioning() && smooth.warmupRemainingFrames() == 0,
            "reset adopts warming destination");
    require(smooth.process(input.block(53), a.block(53)) == Status::ok,
            "reset warmed bank process");
    for (std::size_t f = 0; f < 53; ++f)
        require(std::abs(a.block().channel(0)[f] - input.block().channel(0)[f] * T{0.25}) <
                    tolerance<T>,
                "reset removes warm history");
}
struct RateRun {
    std::vector<double> output;
    std::size_t consumed = 0;
};
template <class T> RateRun variableRun(CpuBackend backend, bool split) {
    ResampleSpec spec{48000, 48000, 1, 64, 80, 128, 1024, 0.75, 3.0};
    Resampler<T> rate;
    require(rate.prepare(spec, backend) == Status::ok &&
                rate.setRatio(0.5, 100) == Status::invalid_argument &&
                rate.setRatio(std::numeric_limits<double>::quiet_NaN()) == Status::invalid_argument,
            "variable ratio envelope");
    std::vector<T> samples(8192);
    for (std::size_t i = 0; i < samples.size(); ++i)
        samples[i] = static_cast<T>(std::sin(static_cast<double>(i) * 0.03125));
    std::array<T, 80> output{};
    T* destination = output.data();
    RateRun run;
    run.output.reserve(2048);
    std::size_t event = 0;
    const std::array<std::size_t, 3> positions{127, 400, 777}, durations{128, 64, 0};
    const std::array<double, 3> ratios{2.5, 0.75, 1.125};
    while (run.output.size() < 2048) {
        if (event < 3 && run.output.size() == positions[event]) {
            require(rate.setRatio(ratios[event], durations[event]) == Status::ok,
                    "RT ratio command");
            ++event;
        }
        auto capacity = std::min<std::size_t>(split ? 1 + run.output.size() % 79 : 80,
                                              2048 - run.output.size());
        if (event < 3)
            capacity = std::min(capacity, positions[event] - run.output.size());
        auto count = split ? 1 + run.consumed % 63 : 64;
        count = std::min(count, samples.size() - run.consumed);
        const T* source = samples.data() + run.consumed;
        const auto step = rate.process({&source, 1, count}, {&destination, 1, capacity});
        require(step.status == Status::ok && (step.inputConsumed || step.outputProduced),
                "variable progress");
        run.consumed += step.inputConsumed;
        for (std::size_t i = 0; i < step.outputProduced; ++i)
            run.output.push_back(output[i]);
    }
    require(rate.ratio() == 1.125 && rate.ratioTransitionRemainingFrames() == 0,
            "ratio final target");
    rate.reset();
    require(rate.ratio() == 1.125 && rate.setRatio(3, 1024) == Status::ok,
            "ratio reset retains latest target");
    return run;
}
template <class T> void variableRate(CpuBackend backend) {
    const auto a = variableRun<T>(backend, false), b = variableRun<T>(backend, true);
    require(a.consumed == b.consumed && a.output == b.output, "variable rate callback invariance");
    long double position = 0, ratio = 1, target = 1;
    std::size_t remaining = 0, expected = 0;
    for (std::size_t i = 0; i < 2048; ++i) {
        if (i == 127) {
            target = 2.5L;
            remaining = 128;
        }
        if (i == 400) {
            target = 0.75L;
            remaining = 64;
        }
        if (i == 777) {
            ratio = target = 1.125L;
            remaining = 0;
        }
        expected = 1 + static_cast<std::size_t>(std::floor(position));
        if (remaining) {
            ratio += (target - ratio) / static_cast<long double>(remaining);
            if (!--remaining)
                ratio = target;
        }
        position += ratio;
    }
    require(a.consumed == expected, "variable rate independent long-double clock oracle");
}
template <class T> void fusedGraph(CpuBackend backend) {
    constexpr std::size_t stages = 32, frames = 137;
    std::array<GraphInput, stages> edges;
    std::array<GraphNode<T>, stages> nodes;
    for (std::size_t i = 0; i < stages; ++i) {
        edges[i] = {i ? i - 1 : graphExternalInput, i % 2 ? 1.125 : 0.875};
        nodes[i] = {nullptr, nullptr, {&edges[i], 1}};
    }
    const std::array<GraphInput, 1> outputs{{{stages - 1, 1}}};
    AudioGraph<T> graph;
    require(graph.prepare({48000, frames, 2}, nodes, outputs, backend) == Status::ok &&
                graph.compiledNodeCount() == 1 && graph.workBufferCount() == 1,
            "linear graph fusion plan");
    AudioBuffer<T> buffer;
    require(buffer.resize(2, frames) == Status::ok, "fused graph buffer");
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t f = 0; f < frames; ++f)
            buffer.block().channel(ch)[f] =
                static_cast<T>(static_cast<int>(f % 23) - 11) * T{0.03125};
    require(graph.process(buffer.block(), buffer.block()) == Status::ok, "fused graph in-place");
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t f = 0; f < frames; ++f) {
            auto expected = static_cast<T>(static_cast<int>(f % 23) - 11) * T{0.03125};
            for (const auto& e : edges)
                expected = T{} + expected * static_cast<T>(e.gain);
            require(buffer.block().channel(ch)[f] == expected,
                    "fused graph preserves every rounding stage");
        }
    edges[7].gain = 0;
    require(graph.prepare({48000, frames, 2}, nodes, outputs, backend) == Status::ok,
            "zero graph prepare");
    std::fill(buffer.block().channel(0).begin(), buffer.block().channel(0).end(),
              std::numeric_limits<T>::quiet_NaN());
    require(graph.process(buffer.block(), buffer.block()) == Status::ok, "zero graph process");
    for (auto x : buffer.block().channel(0))
        require(x == T{} && !std::signbit(x),
                "fused zero mute stops NaNs and preserves signed zero");
}
} // namespace
void optimizationRegression() {
    for (auto backend : backends) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        for (auto mode :
             {ConvolutionMode::hybrid, ConvolutionMode::nonuniform, ConvolutionMode::scheduled}) {
            sharedMatrix<float>(backend, mode);
            sharedMatrix<double>(backend, mode);
        }
        for (auto mode :
             {ConvolutionMode::hybrid, ConvolutionMode::nonuniform, ConvolutionMode::scheduled}) {
            warmTransitions<float>(backend, mode);
            warmTransitions<double>(backend, mode);
        }
        variableRate<float>(backend);
        variableRate<double>(backend);
        fusedGraph<float>(backend);
        fusedGraph<double>(backend);
    }
}
