#include "detail/FftPlanAccess.h"
#include "detail/Scalar.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <stdexcept>
#include <vector>
namespace {
using namespace cydsp;
void require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
template <class T> constexpr double tolerance = std::same_as<T, float> ? 3e-4 : 2e-11;
constexpr std::array backends{CpuBackend::scalar, CpuBackend::sse2,   CpuBackend::avx2,
                              CpuBackend::neon,   CpuBackend::avx512, CpuBackend::sve};
template <class T> void stages(CpuBackend backend) {
    VectorOps ops;
    require(VectorOps::create(backend, ops) == Status::ok, "stage backend");
    const auto& kernels = detail::VectorAccess::kernels<T>(ops);
    for (std::size_t n : {2u, 4u, 8u, 16u, 32u, 128u, 256u})
        for (std::size_t half = 1; half < n; half *= 2)
            for (bool inverse : {false, true}) {
                std::vector<T> weights(2 * half), actual(2 * n + 4, T{99}), reference(2 * n);
                for (std::size_t k = 0; k < half; ++k) {
                    weights[2 * k] =
                        static_cast<T>(std::cos(-3.14159265358979323846 * static_cast<double>(k) /
                                                static_cast<double>(half)));
                    weights[2 * k + 1] =
                        static_cast<T>(std::sin(-3.14159265358979323846 * static_cast<double>(k) /
                                                static_cast<double>(half)));
                }
                for (std::size_t i = 0; i < 2 * n; ++i)
                    reference[i] = actual[i + 1] =
                        static_cast<T>(static_cast<int>((i * 7) % 31) - 15) * T{0.03125};
                detail::scalarFftStage(reference.data(), weights.data(), n, half, inverse);
                kernels.fftStage(actual.data() + 1, weights.data(), n, half, inverse);
                require(actual[0] == T{99} && actual[2 * n + 1] == T{99}, "FFT stage span guard");
                for (std::size_t i = 0; i < 2 * n; ++i)
                    require(std::abs(actual[i + 1] - reference[i]) < tolerance<T>,
                            "FFT stage independent butterfly reference");
            }
}
template <class T> void scheduled(CpuBackend backend) {
    for (std::size_t p : {1u, 2u, 4u, 8u, 32u, 64u, 128u, 256u})
        for (std::size_t suffix : {0u, 1u, 17u}) {
            const auto early = std::min(p, std::size_t{8}),
                       length = 2 * p + suffix + (suffix ? 2 * p : 0), frames = 9 * p + 89;
            std::vector<T> ir(length);
            for (std::size_t k = 0; k < length; ++k)
                ir[k] = static_cast<T>(static_cast<int>((k * 5) % 17) - 8) * T{0.015625};
            ScheduledConvolver<T> whole, split;
            require(whole.prepare({48000, frames, 2}, ir, early, p, backend) == Status::ok &&
                        split.prepare({48000, frames, 2}, ir, early, p, backend) == Status::ok,
                    "scheduled prepare");
            require(whole.latencyFrames() == 0 && whole.tailFrames() == ir.size() - 1 &&
                        whole.schedulingActive() == (length > 2 * p),
                    "scheduled metadata");
            require(whole.maximumWorkUnitsPerFrame() ==
                        whole.workUnitsPerPartition() / p +
                            (whole.workUnitsPerPartition() % p ? 1 : 0),
                    "scheduled frame work bound");
            AudioBuffer<T> input, a, b, bad;
            require(input.resize(2, frames) == Status::ok && a.resize(2, frames) == Status::ok &&
                        b.resize(2, frames) == Status::ok && bad.resize(2, 16) == Status::ok,
                    "scheduled buffers");
            for (std::size_t ch = 0; ch < 2; ++ch)
                for (std::size_t f = 0; f < frames; ++f)
                    input.block().channel(ch)[f] = b.block().channel(ch)[f] =
                        f < 3 * p + 9
                            ? static_cast<T>(static_cast<int>((f * 3 + ch * 7) % 19) - 9) *
                                  T{0.0625}
                            : T{};
            require(whole.process(input.block(), a.block()) == Status::ok, "scheduled whole");
            for (std::size_t f = 0; f < frames;) {
                const auto count = std::min(frames - f, 1 + (f * 7) % 53);
                auto chunk = b.block().subBlock(f, count);
                require(split.process(bad.block(8), bad.block().subBlock(1, 8)) ==
                            Status::partial_overlap,
                        "scheduled invalid overlap");
                require(split.process(chunk, chunk) == Status::ok, "scheduled split in-place");
                f += count;
                if (f >= p + 1 && f - count < p + 1) {
                    require(split.prepare({48000, frames, 2}, ir, early, 3, backend) ==
                                Status::invalid_argument,
                            "scheduled failed prepare transaction");
                    ScheduledConvolver<T> moved(std::move(split));
                    require(split.process(chunk, chunk) == Status::not_prepared,
                            "scheduled move source empty");
                    split = std::move(moved);
                }
            }
            for (std::size_t ch = 0; ch < 2; ++ch)
                for (std::size_t f = 0; f < frames; ++f) {
                    double expected = 0;
                    for (std::size_t k = 0; k < ir.size() && k <= f; ++k)
                        expected += static_cast<double>(ir[k]) * input.block().channel(ch)[f - k];
                    require(std::abs(a.block().channel(ch)[f] - expected) < tolerance<T>,
                            "incremental FFT convolution direct oracle");
                    require(std::abs(a.block().channel(ch)[f] - b.block().channel(ch)[f]) <
                                tolerance<T>,
                            "scheduled callback invariance");
                }
            split.reset();
            require(split.process(input.block(), b.block()) == Status::ok, "scheduled reset");
            for (std::size_t ch = 0; ch < 2; ++ch)
                for (std::size_t f = 0; f < frames; ++f)
                    require(a.block().channel(ch)[f] == b.block().channel(ch)[f],
                            "scheduled reset reproduces initial state");
        }
    // Exercise FFT jobs beyond the 64-butterfly quantum, several channels and a sparse long tail.
    constexpr std::size_t p = 1024, frames = 8193;
    std::vector<T> ir(5 * p + 1);
    const std::array<std::size_t, 7> taps{0, 7, 2 * p - 1, 2 * p, 3 * p - 1, 4 * p + 3, 5 * p};
    for (auto k : taps)
        ir[k] = T{0.125};
    ScheduledConvolver<T> plan;
    require(plan.prepare({48000, 71, 3}, ir, 64, p, backend) == Status::ok,
            "large scheduled prepare");
    AudioBuffer<T> input, output;
    require(input.resize(3, frames) == Status::ok && output.resize(3, frames) == Status::ok,
            "large scheduled buffers");
    for (std::size_t ch = 0; ch < 3; ++ch)
        for (std::size_t f = 0; f < frames; ++f)
            input.block().channel(ch)[f] =
                f < 2049 ? static_cast<T>(static_cast<int>((f + ch) % 23) - 11) * T{0.03125} : T{};
    for (std::size_t f = 0; f < frames;) {
        const auto count = std::min(frames - f, 1 + f % 71);
        require(plan.process(input.block().subBlock(f, count), output.block().subBlock(f, count)) ==
                    Status::ok,
                "large scheduled split");
        f += count;
    }
    for (std::size_t ch = 0; ch < 3; ++ch)
        for (std::size_t f = 0; f < frames; ++f) {
            double expected = 0;
            for (auto k : taps)
                if (k <= f)
                    expected += 0.125 * input.block().channel(ch)[f - k];
            require(std::abs(output.block().channel(ch)[f] - expected) < tolerance<T>,
                    "large scheduled sparse direct oracle");
        }
}
template <class T> void unified(CpuBackend backend) {
    constexpr std::size_t frames = 977;
    const ProcessSpec spec{48000, frames, 2};
    std::vector<T> ir(273);
    for (std::size_t k = 0; k < ir.size(); ++k)
        ir[k] = static_cast<T>(static_cast<int>((k * 7) % 19) - 9) * T{0.015625};
    AudioBuffer<T> input, a, b, bad;
    require(input.resize(2, frames) == Status::ok && a.resize(2, frames) == Status::ok &&
                b.resize(2, frames) == Status::ok && bad.resize(2, 16) == Status::ok,
            "unified buffers");
    for (auto mode :
         {ConvolutionMode::hybrid, ConvolutionMode::nonuniform, ConvolutionMode::scheduled}) {
        const ConvolutionConfig config{mode, 8, 64};
        Convolver<T> whole, split;
        require(whole.prepare(spec, ir, config, backend) == Status::ok &&
                    split.prepare(spec, ir, config, backend) == Status::ok &&
                    whole.configuration() == config && whole.backend() == backend &&
                    whole.partitionFrames() == 8 && whole.latencyFrames() == 0 &&
                    whole.tailFrames() == ir.size() - 1,
                "unified prepare metadata");
        for (std::size_t ch = 0; ch < 2; ++ch)
            for (std::size_t f = 0; f < frames; ++f)
                input.block().channel(ch)[f] = b.block().channel(ch)[f] =
                    f < 401 ? static_cast<T>(static_cast<int>((f + ch * 5) % 17) - 8) * T{0.125}
                            : T{};
        require(whole.process(input.block(), a.block()) == Status::ok, "unified whole");
        for (std::size_t f = 0; f < frames;) {
            const auto count = std::min(frames - f, 1 + f % 53);
            auto chunk = b.block().subBlock(f, count);
            require(split.process(bad.block(8), bad.block().subBlock(1, 8)) ==
                        Status::partial_overlap,
                    "unified rejection before mutation");
            require(split.process(chunk, chunk) == Status::ok, "unified split in-place");
            if (f < 81 && f + count >= 81) {
                require(split.prepare(spec, ir, {ConvolutionMode::scheduled, 8, 3}, backend) ==
                                Status::invalid_argument &&
                            split.configuration() == config &&
                            split.prepare(spec, ir, {static_cast<ConvolutionMode>(99), 8, 64},
                                          backend) == Status::invalid_argument,
                        "unified engine change failure retains running history");
                Convolver<T> moved(std::move(split));
                require(split.partitionFrames() == 0 &&
                            split.process(chunk, chunk) == Status::not_prepared,
                        "unified moved source empty");
                split = std::move(moved);
            }
            f += count;
        }
        for (std::size_t ch = 0; ch < 2; ++ch)
            for (std::size_t f = 0; f < frames; ++f) {
                double expected = 0;
                for (std::size_t k = 0; k < ir.size() && k <= f; ++k)
                    expected += static_cast<double>(ir[k]) * input.block().channel(ch)[f - k];
                require(std::abs(a.block().channel(ch)[f] - expected) < tolerance<T> &&
                            std::abs(b.block().channel(ch)[f] - expected) < tolerance<T>,
                        "unified direct convolution and split/move oracle");
            }
        split.reset();
        require(split.process(input.block(), b.block()) == Status::ok, "unified reset");
        for (std::size_t ch = 0; ch < 2; ++ch)
            for (std::size_t f = 0; f < frames; ++f)
                require(a.block().channel(ch)[f] == b.block().channel(ch)[f],
                        "unified reset reproduces initial state");
        // Successful engine changes clear history and replace all configuration together.
        const auto next =
            mode == ConvolutionMode::hybrid ? ConvolutionMode::scheduled : ConvolutionMode::hybrid;
        require(split.prepare(spec, ir, {next, 8, 64}, backend) == Status::ok &&
                    split.configuration().mode == next &&
                    split.process(input.block(), b.block()) == Status::ok,
                "unified successful engine replacement");
        for (std::size_t ch = 0; ch < 2; ++ch)
            for (std::size_t f = 0; f < frames; ++f)
                require(std::abs(a.block().channel(ch)[f] - b.block().channel(ch)[f]) <
                            tolerance<T>,
                        "unified replacement clears history");
    }
}
template <class T> void configuredTransitions(CpuBackend backend) {
    constexpr std::size_t frames = 977, event = 131;
    const ProcessSpec spec{48000, 53, 2};
    std::vector<T> oldIr(97), newIr(273);
    for (std::size_t k = 0; k < oldIr.size(); ++k)
        oldIr[k] = static_cast<T>(static_cast<int>(k % 9) - 4) * T{0.015625};
    for (std::size_t k = 0; k < newIr.size(); ++k)
        newIr[k] = static_cast<T>(static_cast<int>((k * 3) % 13) - 6) * T{0.015625};
    AudioBuffer<T> input, a, b;
    require(input.resize(2, frames) == Status::ok && a.resize(2, frames) == Status::ok &&
                b.resize(2, frames) == Status::ok,
            "configured transition buffers");
    for (auto mode :
         {ConvolutionMode::hybrid, ConvolutionMode::nonuniform, ConvolutionMode::scheduled})
        for (bool prewarm : {false, true})
            for (std::size_t fade : {0u, 37u}) {
                const ConvolutionConfig config{mode, 8, 64};
                SmoothedConvolver<T> smooth;
                AsyncConvolver<T> asynchronous;
                require(smooth.prepare(spec, oldIr, config, backend) == Status::ok &&
                            asynchronous.prepare(spec, oldIr, config, backend) == Status::ok,
                        "configured transition preparation");
                for (std::size_t ch = 0; ch < 2; ++ch)
                    for (std::size_t f = 0; f < frames; ++f)
                        input.block().channel(ch)[f] = a.block().channel(ch)[f] =
                            b.block().channel(ch)[f] =
                                f < 501 ? static_cast<T>(static_cast<int>((f + ch * 5) % 17) - 8) *
                                              T{0.125}
                                        : T{};
                for (std::size_t f = 0; f < frames;) {
                    if (f == event) {
                        require(smooth.stageImpulse(newIr) == Status::ok &&
                                    smooth.startTransition(fade, prewarm) == Status::ok &&
                                    asynchronous.stageImpulse(newIr, fade, prewarm) == Status::ok,
                                "configured live IR update");
                        require(asynchronous.process(input.block(0), b.block(0)) == Status::ok &&
                                    asynchronous.hasPendingImpulse(),
                                "configured empty call preserves pending");
                    }
                    auto count = std::min(frames - f, 1 + f % 53);
                    if (f < event)
                        count = std::min(count, event - f);
                    auto aa = a.block().subBlock(f, count), bb = b.block().subBlock(f, count);
                    require(smooth.process(aa, aa) == Status::ok &&
                                asynchronous.process(bb, bb) == Status::ok,
                            "configured transition in-place");
                    f += count;
                }
                const auto start = event + (prewarm ? newIr.size() - 1 : 0);
                for (std::size_t ch = 0; ch < 2; ++ch)
                    for (std::size_t f = 0; f < frames; ++f) {
                        double old = 0, next = 0;
                        for (std::size_t k = 0; k < oldIr.size() && k <= f; ++k)
                            old += static_cast<double>(oldIr[k]) * input.block().channel(ch)[f - k];
                        if (f >= event)
                            for (std::size_t k = 0; k < newIr.size() && k <= f - event; ++k)
                                next += static_cast<double>(newIr[k]) *
                                        input.block().channel(ch)[f - k];
                        const auto blend = f < start ? 0.0
                                           : fade
                                               ? std::min(1.0, static_cast<double>(f - start + 1) /
                                                                   static_cast<double>(fade))
                                               : 1.0;
                        const auto expected = old + (next - old) * blend;
                        require(std::abs(a.block().channel(ch)[f] - expected) < tolerance<T> &&
                                    std::abs(b.block().channel(ch)[f] - expected) < tolerance<T>,
                                "configured warm/cold/instant live history direct oracle");
                    }
                require(!smooth.isTransitioning() && !asynchronous.isTransitioning() &&
                            !asynchronous.hasPendingImpulse(),
                        "configured transitions complete");
            }
}
} // namespace
void schedulingRegression() {
    for (auto backend : backends) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        configuredTransitions<float>(backend);
        configuredTransitions<double>(backend);
        unified<float>(backend);
        unified<double>(backend);
        stages<float>(backend);
        stages<double>(backend);
        scheduled<float>(backend);
        scheduled<double>(backend);
    }
}
