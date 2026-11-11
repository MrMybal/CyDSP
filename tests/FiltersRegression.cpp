#include <algorithm>
#include <array>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <limits>
#include <stdexcept>
#include <vector>
namespace {
using namespace cydsp;
void require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
template <class T> constexpr double tolerance = std::same_as<T, float> ? 2e-5 : 1e-12;
template <class T> void layouts() {
    for (std::size_t channels : {1u, 2u, 3u, 7u, 8u, 9u})
        for (std::size_t frames : {0u, 1u, 3u, 17u}) {
            AudioBuffer<T> input, output;
            require(input.resize(channels, frames + 2) == Status::ok &&
                        output.resize(channels, frames + 2) == Status::ok,
                    "layout buffer prepare");
            for (std::size_t ch = 0; ch < channels; ++ch) {
                for (std::size_t frame = 0; frame < frames; ++frame)
                    input.block().channel(ch)[frame + 1] = static_cast<T>(100 * ch + frame);
                output.block().channel(ch).front() = T{77};
                output.block().channel(ch).back() = T{88};
            }
            auto in = input.block().subBlock(1, frames), out = output.block().subBlock(1, frames);
            std::vector<T> flat(channels * frames + 2, T{99});
            auto samples = std::span{flat}.subspan(1, channels * frames);
            require(interleave(in, samples) == Status::ok, "interleave status");
            for (std::size_t frame = 0; frame < frames; ++frame)
                for (std::size_t ch = 0; ch < channels; ++ch)
                    require(samples[frame * channels + ch] == in.channel(ch)[frame],
                            "frame-major layout oracle");
            require(deinterleave(samples, out) == Status::ok, "deinterleave status");
            for (std::size_t ch = 0; ch < channels; ++ch) {
                for (std::size_t frame = 0; frame < frames; ++frame)
                    require(out.channel(ch)[frame] == in.channel(ch)[frame], "layout round trip");
                require(output.block().channel(ch).front() == 77 &&
                            output.block().channel(ch).back() == 88,
                        "layout output guards");
            }
            require(flat.front() == 99 && flat.back() == 99, "interleaved guards");
        }
    std::array<T, 8> a{1, 2, 3, 4, 5, 6, 7, 8}, flat{};
    T* pointers[]{a.data()};
    const auto saved = a;
    require(interleave({pointers, 1, 4}, std::span{a}.subspan(1, 4)) == Status::partial_overlap &&
                a == saved,
            "interleave overlap preserves data");
    T* duplicate[]{a.data(), a.data()};
    require(deinterleave(flat, {duplicate, 2, 4}) == Status::partial_overlap && a == saved,
            "deinterleave duplicate output");
    require(interleave({pointers, 1, 4}, std::span{flat}.first(5)) == Status::invalid_argument,
            "invalid flat length");
    require(interleave(AudioBlock<const T>{}, std::span<T>{}) == Status::ok, "empty interleave");
}
BiquadCoefficients coefficientFor(std::size_t voice) {
    if (voice % 3 == 0)
        return {0.5 + static_cast<double>(voice % 4) * 0.125, 0.25, 0.125, -0.5, 0.25};
    if (voice % 3 == 1)
        return {0, 1, 0, 0, 0};
    return {static_cast<double>(voice + 1) * 0.25, 0, 0, 0, 0};
}
template <class T> void banks() {
    for (const auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2,
                               CpuBackend::neon, CpuBackend::avx512, CpuBackend::sve})
        for (std::size_t voices = 1; voices <= 17; ++voices) {
            constexpr std::size_t frames = 137;
            BiquadBank<T> bank;
            const auto status = bank.prepare({48000, frames, voices}, backend);
            if (status == Status::unsupported_backend)
                continue;
            require(status == Status::ok && bank.backend() == backend && bank.voices() == voices,
                    "bank prepare");
            AudioBuffer<T> input, output;
            require(input.resize(voices, frames + 2) == Status::ok &&
                        output.resize(voices, frames + 2) == Status::ok,
                    "bank buffers");
            auto in = input.block().subBlock(1, frames), out = output.block().subBlock(1, frames);
            std::vector<std::vector<double>> expected(voices, std::vector<double>(frames));
            for (std::size_t voice = 0; voice < voices; ++voice) {
                const auto c = coefficientFor(voice);
                require(bank.setCoefficients(voice, c) == Status::ok, "per-voice coefficients");
                double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
                for (std::size_t frame = 0; frame < frames; ++frame) {
                    const auto x =
                        static_cast<T>(static_cast<int>((frame + voice) % 11) - 5) * T{0.125};
                    in.channel(voice)[frame] = x;
                    const auto y = c.b0 * x + c.b1 * x1 + c.b2 * x2 - c.a1 * y1 - c.a2 * y2;
                    expected[voice][frame] = y;
                    x2 = x1;
                    x1 = x;
                    y2 = y1;
                    y1 = y;
                }
                output.block().channel(voice).front() = T{77};
                output.block().channel(voice).back() = T{88};
            }
            require(bank.setAllCoefficients({1, 0, 0, 0, 1}) == Status::invalid_argument,
                    "bank rejects unstable global edit");
            require(bank.setCoefficients(voices, {}) == Status::invalid_argument,
                    "bank invalid voice");
            require(bank.prepare({0, frames, voices}, backend) == Status::invalid_spec &&
                        bank.voices() == voices,
                    "bank failed prepare preserves state");
            require(bank.process(in, out) == Status::ok, "bank process");
            for (std::size_t voice = 0; voice < voices; ++voice) {
                for (std::size_t frame = 0; frame < frames; ++frame)
                    require(std::abs(out.channel(voice)[frame] - expected[voice][frame]) <
                                tolerance<T>,
                            "bank direct-form-I oracle");
                require(output.block().channel(voice).front() == 77 &&
                            output.block().channel(voice).back() == 88,
                        "bank guards");
            }
            bank.reset();
            std::size_t offset = 0, step = 0;
            const std::array<std::size_t, 5> pattern{1, 63, 7, 64, 2};
            while (offset < frames) {
                const auto count = std::min(pattern[step++ % pattern.size()], frames - offset);
                require(bank.process(in.subBlock(offset, count), in.subBlock(offset, count)) ==
                            Status::ok,
                        "bank split/in-place");
                offset += count;
            }
            for (std::size_t voice = 0; voice < voices; ++voice)
                for (std::size_t frame = 0; frame < frames; ++frame)
                    require(in.channel(voice)[frame] == out.channel(voice)[frame],
                            "bank block-boundary invariance");
            auto moved = std::move(bank);
            require(!bank.voices() && moved.voices() == voices &&
                        bank.process(in.subBlock(0, 0), out.subBlock(0, 0)) == Status::not_prepared,
                    "bank move");
        }
}
template <class T> void transitions() {
    for (const auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2,
                               CpuBackend::neon, CpuBackend::avx512, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) == Status::unsupported_backend)
            continue;
        auto run = [&](bool split, bool inplace) {
            constexpr std::size_t voices = 9, frames = 32;
            SmoothedBiquad<T> filter;
            require(filter.prepare({48000, frames, voices}, backend) == Status::ok,
                    "smoothed prepare");
            AudioBuffer<T> input, output;
            require(input.resize(voices, frames) == Status::ok &&
                        output.resize(voices, frames) == Status::ok,
                    "smoothed buffers");
            auto in = input.block(), out = inplace ? input.block() : output.block();
            for (std::size_t ch = 0; ch < voices; ++ch)
                std::fill(in.channel(ch).begin(), in.channel(ch).end(), static_cast<T>(ch + 1));
            require(filter.setCoefficients({2, 0, 0, 0, 0}, 8) == Status::ok,
                    "start filter transition");
            require(filter.process(in.subBlock(0, 0), out.subBlock(0, 0)) == Status::ok &&
                        filter.transitionRemainingFrames() == 8,
                    "empty block preserves fade");
            require(filter.setCoefficients({1, 0, 0, 0, 1}, 4) == Status::invalid_argument &&
                        filter.targetCoefficients().b0 == 2,
                    "invalid target preserves fade");
            require(filter.process(in.subBlock(0, 3), out.subBlock(0, 3)) == Status::ok &&
                        filter.transitionRemainingFrames() == 5,
                    "partial transition");
            require(filter.setCoefficients({3, 0, 0, 0, 0}, 16) == Status::ok &&
                        filter.setCoefficients({4, 0, 0, 0, 0}, 4) == Status::ok,
                    "latest queued target");
            require(filter.setCoefficients({4, 0, 0, 0, 0}, 64) == Status::ok &&
                        filter.hasQueuedTransition(),
                    "equal target does not restart");
            std::size_t offset = 3, step = 0;
            const std::array<std::size_t, 4> pattern{2, 1, 7, 3};
            while (offset < frames) {
                const auto count =
                    std::min(split ? pattern[step++ % pattern.size()] : frames, frames - offset);
                require(filter.process(in.subBlock(offset, count), out.subBlock(offset, count)) ==
                            Status::ok,
                        "smoothed process");
                offset += count;
            }
            require(!filter.isTransitioning() && !filter.hasQueuedTransition(),
                    "queued fade completion");
            std::vector<T> result(voices * frames);
            for (std::size_t frame = 0; frame < frames; ++frame) {
                const auto gain = frame < 8
                                      ? 1 + static_cast<double>(frame + 1) / 8
                                      : (frame < 12 ? 2 + static_cast<double>(frame - 7) * 0.5 : 4);
                for (std::size_t ch = 0; ch < voices; ++ch) {
                    require(out.channel(ch)[frame] ==
                                static_cast<T>(gain * static_cast<double>(ch + 1)),
                            "crossfade/queue trajectory oracle");
                    result[frame * voices + ch] = out.channel(ch)[frame];
                }
            }
            require(filter.setCoefficients({2, 0, 0, 0, 0}, 8) == Status::ok, "next fade");
            filter.reset();
            require(!filter.isTransitioning() && filter.targetCoefficients().b0 == 2,
                    "reset snaps latest target");
            require(filter.setCoefficients({3, 0, 0, 0, 0}, 8) == Status::ok &&
                        filter.setCoefficients({4, 0, 0, 0, 0}, 8) == Status::ok &&
                        filter.setCoefficients({5, 0, 0, 0, 0}, 0) == Status::ok &&
                        !filter.isTransitioning(),
                    "immediate edit cancels queue");
            require(filter.prepare({48000, frames, voices}, backend) == Status::ok &&
                        !filter.isTransitioning() && filter.targetCoefficients().b0 == 5,
                    "prepare preserves latest target");
            auto moved = std::move(filter);
            require(filter.process(in.subBlock(0, 0), out.subBlock(0, 0)) == Status::not_prepared &&
                        moved.targetCoefficients().b0 == 5,
                    "smoothed move");
            return result;
        };
        const auto whole = run(false, false);
        require(whole == run(true, false) && whole == run(true, true),
                "fade block/in-place invariance");
    }
}
template <class T> void coalescingTransitions() {
    SmoothedBiquad<T> filter;
    require(filter.prepare({48000, 8, 1}, CpuBackend::scalar) == Status::ok, "coalesced prepare");
    std::array<T, 8> input{}, output{};
    input.fill(T{1});
    T* ip[]{input.data()};
    T* op[]{output.data()};
    AudioBlock<T> in{ip, 1, 8}, out{op, 1, 8};
    require(filter.setCoefficients({2, 0, 0, 0, 0}, 4) == Status::ok &&
                filter.process(in.subBlock(0, 1), out.subBlock(0, 1)) == Status::ok,
            "coalesced first frame");
    require(filter.setCoefficients({3, 0, 0, 0, 0}, 8) == Status::ok &&
                filter.hasQueuedTransition(),
            "coalesced queue");
    require(filter.setCoefficients({2, 0, 0, 0, 0}, 8) == Status::ok &&
                !filter.hasQueuedTransition() && filter.transitionRemainingFrames() == 3,
            "return to destination cancels queue");
    require(filter.process(in.subBlock(1, 3), out.subBlock(1, 3)) == Status::ok &&
                !filter.isTransitioning(),
            "coalesced completion");
    require(filter.setCoefficients({4, 0, 0, 0, 0}, 2) == Status::ok &&
                filter.setCoefficients({5, 0, 0, 0, 0}, 8) == Status::ok,
            "queue across callback boundary");
    require(filter.process(in.subBlock(4, 2), out.subBlock(4, 2)) == Status::ok &&
                filter.hasQueuedTransition() && filter.transitionRemainingFrames() == 0,
            "callback ends before queued transition");
    require(filter.setCoefficients({4, 0, 0, 0, 0}, 8) == Status::ok && !filter.isTransitioning(),
            "active target cancels deferred transition");
    require(filter.process(in.subBlock(6, 2), out.subBlock(6, 2)) == Status::ok,
            "coalesced final samples");
    require(output == std::array<T, 8>{T{1.25}, T{1.5}, T{1.75}, T{2}, T{3}, T{4}, T{4}, T{4}},
            "coalesced transition trajectory");
}
template <class T> void recursiveTransition() {
    constexpr std::size_t frames = 137;
    const BiquadCoefficients c{0.5, 0.25, 0.125, -0.5, 0.25};
    SmoothedBiquad<T> filter;
    require(filter.prepare({48000, frames, 1}, CpuBackend::scalar) == Status::ok &&
                filter.setCoefficients(c, 80) == Status::ok,
            "recursive transition prepare");
    std::array<T, frames> input{}, output{};
    std::array<double, frames> filtered{};
    double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    for (std::size_t i = 0; i < frames; ++i) {
        const auto x = static_cast<T>(static_cast<int>(i % 7) - 3) * T{0.25};
        input[i] = x;
        filtered[i] = c.b0 * x + c.b1 * x1 + c.b2 * x2 - c.a1 * y1 - c.a2 * y2;
        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = filtered[i];
    }
    T* ip[]{input.data()};
    T* op[]{output.data()};
    require(filter.process({ip, 1, frames}, {op, 1, frames}) == Status::ok,
            "recursive transition process");
    for (std::size_t i = 0; i < frames; ++i) {
        const auto alpha = std::min(1.0, static_cast<double>(i + 1) / 80);
        const auto expected = input[i] * (1 - alpha) + filtered[i] * alpha;
        require(std::abs(output[i] - expected) < tolerance<T>,
                "independent recursive crossfade oracle");
    }
}
} // namespace
void filtersRegression() {
    layouts<float>();
    layouts<double>();
    banks<float>();
    banks<double>();
    transitions<float>();
    transitions<double>();
    coalescingTransitions<float>();
    coalescingTransitions<double>();
    recursiveTransition<float>();
    recursiveTransition<double>();
}
