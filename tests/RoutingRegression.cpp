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
template <class T> constexpr double tolerance = std::same_as<T, float> ? 5e-5 : 1e-11;
template <class T> void matrices() {
    constexpr std::size_t frames = 137;
    for (const auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2,
                               CpuBackend::neon, CpuBackend::avx512, CpuBackend::sve})
        for (const std::size_t inputs : {1u, 2u, 3u, 7u})
            for (const std::size_t outputs : {1u, 2u, 5u, 9u}) {
                std::vector<double> gains(inputs * outputs);
                for (std::size_t i = 0; i < gains.size(); ++i)
                    gains[i] = static_cast<double>(static_cast<int>(i % 7) - 3) * 0.25;
                ChannelMatrix<T> matrix;
                const auto status =
                    matrix.prepare({48000, frames, inputs}, outputs, gains, backend);
                if (status == Status::unsupported_backend)
                    continue;
                require(status == Status::ok && matrix.backend() == backend &&
                            matrix.inputChannels() == inputs && matrix.outputChannels() == outputs,
                        "matrix prepare");
                AudioBuffer<T> input, output;
                require(input.resize(inputs, frames + 2) == Status::ok &&
                            output.resize(outputs, frames + 2) == Status::ok,
                        "matrix buffers");
                auto in = input.block().subBlock(1, frames),
                     out = output.block().subBlock(1, frames);
                for (std::size_t ch = 0; ch < inputs; ++ch)
                    for (std::size_t frame = 0; frame < frames; ++frame)
                        in.channel(ch)[frame] =
                            static_cast<T>(static_cast<int>((frame + ch) % 13) - 6) * T{0.125};
                for (std::size_t ch = 0; ch < outputs; ++ch) {
                    output.block().channel(ch).front() = T{77};
                    output.block().channel(ch).back() = T{88};
                }
                require(matrix.process(in, out) == Status::ok, "rectangular matrix process");
                for (std::size_t ch = 0; ch < outputs; ++ch) {
                    for (std::size_t frame = 0; frame < frames; ++frame) {
                        double expected = 0;
                        for (std::size_t source = 0; source < inputs; ++source)
                            expected += static_cast<double>(in.channel(source)[frame]) *
                                        gains[ch * inputs + source];
                        require(std::abs(out.channel(ch)[frame] - expected) < tolerance<T>,
                                "matrix finite sum oracle");
                    }
                    require(output.block().channel(ch).front() == 77 &&
                                output.block().channel(ch).back() == 88,
                            "matrix guards");
                }
            }
    // Exact cross-channel aliases must be safe across multiple private tiles.
    AudioBuffer<T> storage;
    require(storage.resize(3, frames) == Status::ok, "routing storage");
    for (std::size_t ch = 0; ch < 3; ++ch)
        for (std::size_t f = 0; f < frames; ++f)
            storage.block().channel(ch)[f] = static_cast<T>(ch * 1000 + f);
    T* reverse[]{storage.block().channel(2).data(), storage.block().channel(1).data(),
                 storage.block().channel(0).data()};
    ChannelMatrix<T> permutation;
    const std::array<double, 9> identity{1, 0, 0, 0, 1, 0, 0, 0, 1};
    require(permutation.prepare({48000, frames, 3}, 3, identity) == Status::ok &&
                permutation.process(storage.block(), {reverse, 3, frames}) == Status::ok,
            "in-place channel permutation");
    for (std::size_t ch = 0; ch < 3; ++ch)
        for (std::size_t f = 0; f < frames; ++f)
            require(storage.block().channel(ch)[f] == static_cast<T>((2 - ch) * 1000 + f),
                    "permutation snapshot oracle");
    const std::array<double, 3> downmix{0.25, 0.5, 0.25};
    require(permutation.prepare({48000, frames, 3}, 1, downmix) == Status::ok &&
                permutation.process(storage.block(), {reverse, 1, frames}) == Status::ok,
            "rectangular in-place downmix");
    for (std::size_t f = 0; f < frames; ++f)
        require(reverse[0][f] == static_cast<T>(1000 + f), "downmix snapshot oracle");
}
template <class T> void matrixRamps() {
    constexpr std::size_t frames = 137;
    const std::array<double, 4> identity{1, 0, 0, 1}, swap{0, 1, 1, 0};
    ChannelMatrix<T> whole, split;
    require(whole.prepare({48000, frames, 2}, 2, identity) == Status::ok &&
                split.prepare({48000, frames, 2}, 2, identity) == Status::ok,
            "ramp prepare");
    AudioBuffer<T> input, wa, sa;
    require(input.resize(2, frames) == Status::ok && wa.resize(2, frames) == Status::ok &&
                sa.resize(2, frames) == Status::ok,
            "ramp buffers");
    for (auto& x : input.block().channel(0))
        x = T{2};
    for (auto& x : input.block().channel(1))
        x = T{6};
    require(whole.setMatrix(swap, 80) == Status::ok && split.setMatrix(swap, 80) == Status::ok,
            "ramp set");
    require(whole.process(input.block().subBlock(0, 0), wa.block(0)) == Status::ok &&
                whole.rampRemainingFrames() == 80,
            "empty matrix preserves ramp");
    require(whole.process(input.block(), wa.block()) == Status::ok, "whole ramp");
    std::size_t offset = 0;
    for (const std::size_t count : {3u, 64u, 1u, 69u}) {
        require(split.setMatrix(swap, 1) == Status::ok, "identical target retains duration");
        require(split.process(input.block().subBlock(offset, count),
                              sa.block().subBlock(offset, count)) == Status::ok,
                "split matrix ramp");
        offset += count;
    }
    for (std::size_t f = 0; f < frames; ++f) {
        const auto alpha = static_cast<double>(std::min(f + 1, std::size_t{80})) / 80;
        require(std::abs(wa.block().channel(0)[f] - (2 + 4 * alpha)) < tolerance<T> &&
                    std::abs(wa.block().channel(1)[f] - (6 - 4 * alpha)) < tolerance<T>,
                "matrix shared-frame ramp oracle");
        for (std::size_t ch = 0; ch < 2; ++ch)
            require(wa.block().channel(ch)[f] == sa.block().channel(ch)[f],
                    "matrix split equivalence");
    }
    require(split.setMatrix(identity, 20) == Status::ok, "retarget ramp");
    auto bad = identity;
    bad[2] = std::numeric_limits<double>::quiet_NaN();
    require(split.setMatrix(bad, 1) == Status::invalid_argument &&
                split.rampRemainingFrames() == 20,
            "invalid matrix leaves ramp intact");
    T* partial[]{input.block().channel(0).data() + 1, input.block().channel(1).data() + 1};
    require(split.process(input.block(10), {partial, 2, 10}) == Status::partial_overlap &&
                split.rampRemainingFrames() == 20,
            "partial alias preserves ramp");
    T* duplicate[]{wa.block().channel(0).data(), wa.block().channel(0).data()};
    require(split.process(input.block(10), {duplicate, 2, 10}) == Status::partial_overlap,
            "duplicate matrix output");
    require(split.prepare({48000, frames, 2}, std::numeric_limits<std::size_t>::max(), {}) ==
                    Status::invalid_argument &&
                split.rampRemainingFrames() == 20,
            "matrix shape overflow transaction");
    split.reset();
    require(split.rampRemainingFrames() == 0 &&
                split.process(input.block(), sa.block()) == Status::ok,
            "matrix reset snaps target");
    for (std::size_t ch = 0; ch < 2; ++ch)
        require(std::equal(input.block().channel(ch).begin(), input.block().channel(ch).end(),
                           sa.block().channel(ch).begin()),
                "reset identity oracle");
    auto moved = std::move(split);
    require(split.process(input.block(), sa.block()) == Status::not_prepared &&
                moved.inputChannels() == 2,
            "matrix move");
    ChannelMatrix<double> extreme;
    const std::array<double, 1> hi{std::numeric_limits<double>::max()}, lo{-hi[0]};
    require(extreme.prepare({48000, 8, 1}, 1, hi) == Status::ok &&
                extreme.setMatrix(lo, 8) == Status::invalid_argument &&
                extreme.setMatrix(lo, 0) == Status::ok,
            "matrix finite difference policy");
}
template <class T>
double causal(std::span<const T> input, std::span<const T> impulse, std::size_t frame,
              std::size_t first) {
    double sum = 0;
    for (std::size_t tap = 0; tap < impulse.size() && tap <= frame - first; ++tap)
        sum += static_cast<double>(impulse[tap]) * input[frame - tap];
    return sum;
}
template <class T> void convolutionTransitions() {
    constexpr std::size_t frames = 241, change = 39, duration = 73;
    std::vector<T> oldIr(81), newIr(49);
    for (std::size_t i = 0; i < oldIr.size(); ++i)
        oldIr[i] = static_cast<T>(static_cast<int>(i % 7) - 3) * T{0.015625};
    for (std::size_t i = 0; i < newIr.size(); ++i)
        newIr[i] = static_cast<T>(static_cast<int>(i % 5) - 2) * T{0.03125};
    for (const auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2,
                               CpuBackend::neon, CpuBackend::avx512, CpuBackend::sve}) {
        SmoothedConvolver<T> whole, split;
        const auto status = whole.prepare({48000, frames, 2}, oldIr, 8, backend);
        if (status == Status::unsupported_backend)
            continue;
        require(status == Status::ok &&
                    split.prepare({48000, frames, 2}, oldIr, 8, backend) == Status::ok,
                "convolution transition prepare");
        AudioBuffer<T> input, output, inplace;
        require(input.resize(2, frames) == Status::ok && output.resize(2, frames) == Status::ok &&
                    inplace.resize(2, frames) == Status::ok,
                "convolution transition buffers");
        for (std::size_t ch = 0; ch < 2; ++ch)
            for (std::size_t f = 0; f < frames; ++f) {
                const auto value =
                    f < 150 ? static_cast<T>(static_cast<int>((f + ch * 3) % 11) - 5) * T{0.125}
                            : T{};
                input.block().channel(ch)[f] = value;
                inplace.block().channel(ch)[f] = value;
            }
        require(whole.process(input.block(change), output.block(change)) == Status::ok &&
                    split.process(inplace.block(change), inplace.block(change)) == Status::ok,
                "old convolution prefix");
        require(whole.stageImpulse(newIr) == Status::ok &&
                    split.stageImpulse(newIr) == Status::ok && whole.hasStagedImpulse() &&
                    whole.tailFrames() == 80,
                "stage retains active IR");
        const std::array<T, 1> invalid{std::numeric_limits<T>::infinity()};
        require(whole.stageImpulse(invalid) == Status::invalid_argument && whole.hasStagedImpulse(),
                "failed stage retains pending plan");
        require(whole.startTransition(duration, false) == Status::ok &&
                    split.startTransition(duration, false) == Status::ok &&
                    whole.tailFrames() == 80 && whole.latencyFrames() == 0,
                "start convolution fade");
        require(whole.stageImpulse(oldIr) == Status::transition_in_progress &&
                    whole.startTransition(0, false) == Status::transition_in_progress &&
                    whole.transitionRemainingFrames() == duration,
                "active fade rejects plan replacement");
        require(whole.process(input.block(0), output.block(0)) == Status::ok &&
                    whole.transitionRemainingFrames() == duration,
                "empty convolution fade");
        T* partial[]{input.block().channel(0).data() + 1, input.block().channel(1).data() + 1};
        require(whole.process(input.block(10), {partial, 2, 10}) == Status::partial_overlap &&
                    whole.transitionRemainingFrames() == duration,
                "invalid convolution process preserves fade");
        require(whole.process(input.block().subBlock(change, frames - change),
                              output.block().subBlock(change, frames - change)) == Status::ok,
                "whole convolution fade");
        std::size_t offset = change;
        for (const std::size_t count : {1u, 31u, 40u, 2u, 64u, 64u}) {
            const auto part = inplace.block().subBlock(offset, count);
            require(split.process(part, part) == Status::ok, "split in-place convolution fade");
            offset += count;
        }
        require(offset == frames && !whole.isTransitioning() && !whole.hasStagedImpulse() &&
                    whole.tailFrames() == 48,
                "convolution completion metadata");
        for (std::size_t ch = 0; ch < 2; ++ch)
            for (std::size_t f = 0; f < frames; ++f) {
                const auto samples = input.block().channel(ch);
                const auto from = causal<T>(samples, oldIr, f, 0);
                double expected = from;
                if (f >= change) {
                    const auto to = causal<T>(samples, newIr, f, change);
                    const auto alpha = static_cast<double>(std::min(f - change + 1, duration)) /
                                       static_cast<double>(duration);
                    expected = (1 - alpha) * from + alpha * to;
                }
                require(std::abs(output.block().channel(ch)[f] - expected) < tolerance<T>,
                        "direct convolution fade oracle");
                require(std::abs(output.block().channel(ch)[f] - inplace.block().channel(ch)[f]) <
                            tolerance<T>,
                        "convolution split and in-place equivalence");
            }
        require(whole.stageImpulse(oldIr) == Status::ok, "restage retired slot");
        whole.discardStagedImpulse();
        require(!whole.hasStagedImpulse() && whole.startTransition() == Status::invalid_argument,
                "discard stage retains no request");
        require(whole.stageImpulse(oldIr) == Status::ok && whole.stageImpulse(newIr) == Status::ok,
                "replace inactive staged plan");
        whole.reset();
        require(whole.hasStagedImpulse() && whole.startTransition(0, false) == Status::ok &&
                    !whole.isTransitioning(),
                "reset preserves stage and immediate switch");
        require(whole.stageImpulse(oldIr) == Status::ok &&
                    whole.startTransition(16, false) == Status::ok,
                "reset running fade setup");
        whole.reset();
        require(!whole.isTransitioning() && whole.tailFrames() == 80,
                "reset snaps to destination IR");
        require(whole.process(input.block(), output.block()) == Status::ok,
                "reset cold convolution process");
        for (std::size_t ch = 0; ch < 2; ++ch)
            for (std::size_t f = 0; f < frames; ++f)
                require(std::abs(output.block().channel(ch)[f] -
                                 causal<T>(input.block().channel(ch), oldIr, f, 0)) < tolerance<T>,
                        "reset destination convolution oracle");
        require(whole.prepare({48000, frames, 2}, {}, 8) == Status::invalid_argument &&
                    whole.tailFrames() == 80,
                "failed convolution preparation retains plan");
        require(whole.stageImpulse(newIr) == Status::ok &&
                    whole.startTransition(17, false) == Status::ok,
                "move running fade setup");
        auto moved = std::move(whole);
        require(whole.process(input.block(), output.block()) == Status::not_prepared &&
                    moved.transitionRemainingFrames() == 17,
                "convolver move retains transition");
    }
}
} // namespace
void routingRegression() {
    matrices<float>();
    matrices<double>();
    matrixRamps<float>();
    matrixRamps<double>();
    convolutionTransitions<float>();
    convolutionTransitions<double>();
}
