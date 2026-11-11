#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <stdexcept>
#include <thread>
#include <vector>
namespace {
using namespace cydsp;
void require(bool value, const char* why) {
    if (!value)
        throw std::runtime_error(why);
}
template <class T> constexpr double tolerance = std::same_as<T, float> ? 2e-4 : 1e-10;
template <class T>
double convolution(std::span<const T> input, std::span<const T> ir, std::size_t frame) {
    double value = 0;
    for (std::size_t k = 0; k < ir.size() && k <= frame; ++k)
        value += static_cast<double>(input[frame - k]) * ir[k];
    return value;
}
template <class T> void nonuniform() {
    constexpr std::size_t frames = 401;
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::neon,
                         CpuBackend::avx512, CpuBackend::sve})
        for (std::size_t early : {1u, 4u, 16u})
            for (std::size_t late : {16u, 64u, 128u}) {
                std::vector<T> ir(late + 17);
                for (std::size_t i = 0; i < ir.size(); ++i)
                    ir[i] = static_cast<T>(static_cast<int>(i % 9) - 4) * T{0.03125};
                NonuniformConvolver<T> whole, split;
                const auto status = whole.prepare({48000, frames, 2}, ir, early, late, backend);
                if (status == Status::unsupported_backend)
                    continue;
                require(status == Status::ok && split.prepare({48000, frames, 2}, ir, early, late,
                                                              backend) == Status::ok,
                        "nonuniform prepare");
                AudioBuffer<T> input, output, inplace;
                require(input.resize(2, frames) == Status::ok &&
                            output.resize(2, frames) == Status::ok &&
                            inplace.resize(2, frames) == Status::ok,
                        "nonuniform buffers");
                for (std::size_t ch = 0; ch < 2; ++ch)
                    for (std::size_t f = 0; f < frames; ++f) {
                        const auto x =
                            f < 113
                                ? static_cast<T>(static_cast<int>((f + ch * 2) % 11) - 5) * T{0.125}
                                : T{};
                        input.block().channel(ch)[f] = x;
                        inplace.block().channel(ch)[f] = x;
                    }
                require(whole.process(input.block(), output.block()) == Status::ok,
                        "nonuniform whole");
                std::size_t offset = 0;
                while (offset < frames) {
                    const auto count = std::min(frames - offset, (offset % 23) + 1);
                    auto part = inplace.block().subBlock(offset, count);
                    require(split.process(part, part) == Status::ok, "nonuniform split");
                    offset += count;
                }
                for (std::size_t ch = 0; ch < 2; ++ch)
                    for (std::size_t f = 0; f < frames; ++f) {
                        require(std::abs(output.block().channel(ch)[f] -
                                         convolution<T>(input.block().channel(ch), ir, f)) <
                                    tolerance<T>,
                                "nonuniform direct convolution oracle");
                        require(std::abs(output.block().channel(ch)[f] -
                                         inplace.block().channel(ch)[f]) < tolerance<T>,
                                "nonuniform split in-place equivalence");
                    }
                require(whole.prepare({48000, frames, 2}, ir, 8, 3) == Status::invalid_argument &&
                            whole.tailFrames() == ir.size() - 1,
                        "nonuniform transaction");
                whole.reset();
                require(whole.process(input.block(), output.block()) == Status::ok,
                        "nonuniform reset");
            }
}
template <class T> void matrixConvolution() {
    constexpr std::size_t frames = 193;
    const std::array<T, 3> a{T{1}, T{0.5}, T{-0.25}}, b{T{0}, T{1}, T{0.5}};
    std::vector<T> c(41, T{0.03125});
    const std::array<ConvolutionRoute<T>, 5> routes{
        {{0, 0, a}, {1, 0, b}, {0, 1, c}, {1, 2, a}, {1, 2, b}}};
    MatrixConvolver<T> matrix;
    require(matrix.prepare({48000, frames, 2}, 3, routes, 4, 16) == Status::ok, "MIMO prepare");
    AudioBuffer<T> input, storage;
    require(input.resize(2, frames) == Status::ok && storage.resize(3, frames) == Status::ok,
            "MIMO buffers");
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t f = 0; f < frames; ++f) {
            const auto x =
                f < 71 ? static_cast<T>(static_cast<int>((f + ch) % 7) - 3) * T{0.25} : T{};
            input.block().channel(ch)[f] = x;
            storage.block().channel(ch)[f] = x;
        }
    T* source[]{storage.block().channel(0).data(), storage.block().channel(1).data()};
    T* output[]{storage.block().channel(1).data(), storage.block().channel(0).data(),
                storage.block().channel(2).data()};
    require(matrix.process({source, 2, frames}, {output, 3, frames}) == Status::ok,
            "MIMO rectangular cross-alias process");
    for (std::size_t ch = 0; ch < 3; ++ch)
        for (std::size_t f = 0; f < frames; ++f) {
            double expected = 0;
            for (const auto& route : routes)
                if (route.outputChannel == ch)
                    expected +=
                        convolution<T>(input.block().channel(route.inputChannel), route.impulse, f);
            require(std::abs(output[ch][f] - expected) < tolerance<T>,
                    "MIMO independent route sum oracle");
        }
    matrix.reset();
    require(matrix.tailFrames() == 40, "MIMO tail");
    MatrixConvolver<T> empty;
    require(empty.prepare({48000, frames, 2}, 3, {}, 4, 16) == Status::ok, "empty routing prepare");
    require(empty.process(input.block(), storage.block()) == Status::ok, "empty MIMO silence");
    for (std::size_t ch = 0; ch < 3; ++ch)
        for (auto sample : storage.block().channel(ch))
            require(sample == 0, "empty MIMO output");
}
template <class T> void equalizer() {
    SmoothedBiquadCascade<T> eq;
    require(eq.prepare({48000, 64, 2}, 4) == Status::ok, "EQ prepare");
    const std::array<BiquadCoefficients, 2> half{{{0.5, 0, 0, 0, 0}, {0.5, 0, 0, 0, 0}}};
    const std::array<BiquadCoefficients, 1> doubleGain{{{2, 0, 0, 0, 0}}};
    AudioBuffer<T> in, out;
    require(in.resize(2, 64) == Status::ok && out.resize(2, 64) == Status::ok, "EQ buffers");
    for (std::size_t ch = 0; ch < 2; ++ch)
        std::fill(in.block().channel(ch).begin(), in.block().channel(ch).end(),
                  static_cast<T>(ch + 1));
    require(eq.setCoefficients(half, 8) == Status::ok &&
                eq.process(in.block(3), out.block(3)) == Status::ok,
            "EQ first fade");
    require(eq.setCoefficients(doubleGain, 4) == Status::ok &&
                eq.setCoefficients({}, 4) == Status::ok,
            "EQ latest queued target");
    require(eq.process(in.block(9), out.block(9)) == Status::ok && !eq.isTransitioning() &&
                eq.targetSections() == 0,
            "EQ fade queue completion");
    for (std::size_t f = 0; f < 9; ++f) {
        const auto expected = f < 5 ? 1 - 0.75 * static_cast<double>(f + 4) / 8
                                    : 0.25 + 0.75 * static_cast<double>(f - 4) / 4;
        for (std::size_t ch = 0; ch < 2; ++ch)
            require(std::abs(out.block().channel(ch)[f] - expected * (ch + 1)) < tolerance<T>,
                    "EQ cascade gain fade oracle");
    }
    auto invalid = half;
    invalid[1].a2 = 1;
    require(eq.setCoefficients(invalid) == Status::invalid_argument && eq.targetSections() == 0,
            "EQ validates all sections before commit");
    require(eq.setCoefficients(half, 0) == Status::ok &&
                eq.process(in.block(), out.block()) == Status::ok,
            "EQ immediate");
    for (auto sample : out.block().channel(0))
        require(sample == T{0.25}, "EQ immediate output");
    eq.reset();
    auto moved = std::move(eq);
    require(eq.process(in.block(), out.block()) == Status::not_prepared && moved.maxSections() == 4,
            "EQ move");
}
template <class T>
std::vector<T> resampleStream(bool finish, std::uint32_t inRate, std::uint32_t outRate) {
    Resampler<T> rate;
    require(rate.prepare({inRate, outRate, 1, 32, 17, 32, 256}) == Status::ok, "EOS prepare");
    std::vector<T> samples(137);
    for (std::size_t i = 0; i < 97; ++i)
        samples[i] = static_cast<T>(static_cast<int>(i % 7) - 3) * T{0.125};
    AudioBuffer<T> output;
    require(output.resize(1, 17) == Status::ok, "EOS output");
    std::vector<T> result;
    const auto feed = [&](std::size_t start, std::size_t length) {
        std::size_t at = 0;
        while (at < length) {
            const T* ptr = samples.data() + start + at;
            const auto count = std::min(length - at, std::size_t{32});
            const auto step = rate.process({&ptr, 1, count}, output.block());
            require(step.status == Status::ok && (step.inputConsumed || step.outputProduced),
                    "EOS reference progress");
            at += step.inputConsumed;
            result.insert(result.end(), output.block().channel(0).begin(),
                          output.block().channel(0).begin() + step.outputProduced);
        }
    };
    feed(0, 97);
    if (finish) {
        while (!rate.isFinished()) {
            const auto step = rate.finish(output.block());
            require(step.status == Status::ok && step.inputConsumed == 0, "EOS finish counts");
            result.insert(result.end(), output.block().channel(0).begin(),
                          output.block().channel(0).begin() + step.outputProduced);
        }
        require(rate.finish(output.block()).outputProduced == 0 &&
                    rate.process({nullptr, 1, 0}, output.block()).status == Status::end_of_stream,
                "EOS terminal state");
        rate.reset();
        require(!rate.isFinished(), "EOS reset");
    } else {
        feed(97, 32);
        const T* ptr = samples.data();
        for (;;) {
            const auto step = rate.process({&ptr, 1, 0}, output.block());
            require(step.status == Status::ok, "reference drain");
            result.insert(result.end(), output.block().channel(0).begin(),
                          output.block().channel(0).begin() + step.outputProduced);
            if (!step.outputProduced)
                break;
        }
    }
    return result;
}
template <class T> void resamplerCompletion() {
    for (const auto& rates : {std::pair{48000u, 44100u}, std::pair{16000u, 48000u},
                              std::pair{48000u, 16000u}, std::pair{48000u, 48000u}}) {
        const auto automatic = resampleStream<T>(true, rates.first, rates.second),
                   manual = resampleStream<T>(false, rates.first, rates.second);
        require(automatic.size() == manual.size(), "EOS output length oracle");
        for (std::size_t i = 0; i < automatic.size(); ++i)
            require(std::abs(automatic[i] - manual[i]) < tolerance<T>,
                    "EOS explicit zero-stream oracle");
    }
    Resampler<T> rate;
    require(rate.prepare({48000, 44100, 1, 256, 512, 32, 256}) == Status::ok,
            "rate change prepare");
    AudioBuffer<T> in, out;
    require(in.resize(1, 256) == Status::ok && out.resize(1, 512) == Status::ok,
            "rate change buffers");
    std::fill(in.block().channel(0).begin(), in.block().channel(0).end(), T{1});
    for (int i = 0; i < 4; ++i)
        require(rate.process(in.block(), out.block()).status == Status::ok, "rate change warmup");
    require(rate.setRates(96000, 32000) == Status::ok &&
                rate.setRates(0, 1) == Status::invalid_argument,
            "rate change transaction");
    const auto step = rate.process(in.block(), out.block());
    require(step.status == Status::ok && step.outputProduced > 0, "changed rate process");
    for (std::size_t i = 0; i < step.outputProduced; ++i)
        require(std::abs(out.block().channel(0)[i] - 1) < tolerance<T>,
                "rate change retains steady history");
}
template <class T> void graph() {
    const std::array<GraphInput, 1> a{{{1, 2}}}, b{{{graphExternalInput, 0.5}}};
    const std::array<GraphInput, 2> c{{{0, 0.25}, {1, 0.5}}},
        outputs{{{2, 1}, {graphExternalInput, 0.25}}};
    const std::array<GraphNode<T>, 3> nodes{
        {{nullptr, nullptr, a}, {nullptr, nullptr, b}, {nullptr, nullptr, c}}};
    AudioGraph<T> graph;
    require(graph.prepare({48000, 137, 2}, nodes, outputs) == Status::ok, "DAG unsorted prepare");
    AudioBuffer<T> input;
    require(input.resize(2, 137) == Status::ok, "graph input");
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (auto& x : input.block().channel(ch))
            x = static_cast<T>(ch + 1);
    require(graph.process(input.block(), input.block()) == Status::ok, "DAG in-place");
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (auto x : input.block().channel(ch))
            require(x == static_cast<T>(ch + 1) * T{0.75}, "DAG independent branch mix oracle");
    const std::array<GraphInput, 1> self{{{0, 1}}};
    const std::array<GraphNode<T>, 1> cycle{{{nullptr, nullptr, self}}};
    const std::array<GraphInput, 1> cycleOutput{{{0, 1}}};
    require(graph.prepare({48000, 137, 2}, cycle, cycleOutput) == Status::invalid_argument,
            "graph rejects cycle transactionally");
    std::array<std::array<GraphInput, 1>, 32> edges;
    std::array<GraphNode<T>, 32> chain;
    for (std::size_t i = 0; i < 32; ++i) {
        edges[i][0] = {i ? i - 1 : graphExternalInput, 1};
        chain[i] = {nullptr, nullptr, edges[i]};
    }
    const std::array<GraphInput, 1> end{{{31, 1}}};
    require(graph.prepare({48000, 137, 2}, chain, end) == Status::ok &&
                graph.workBufferCount() == 1,
            "DAG lifetime memory reuse");
    graph.reset();
    require(graph.process(input.block(), input.block()) == Status::ok, "graph reused slots");
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (auto x : input.block().channel(ch))
            require(x == static_cast<T>(ch + 1) * T{0.75},
                    "graph retained output through reused slots");
    Gain<T> gain;
    require(gain.prepare({48000, 137, 2}) == Status::ok && gain.setGain(0.5) == Status::ok,
            "graph callback prepare");
    const auto callback =
        +[](void* context, AudioBlock<const T> source, AudioBlock<T> target) noexcept {
            return static_cast<Gain<T>*>(context)->process(source, target);
        };
    const std::array<GraphInput, 1> external{{{graphExternalInput, 1}}}, nodeOutput{{{0, 1}}};
    const std::array<GraphNode<T>, 1> callbackNode{{{callback, &gain, external}}};
    require(graph.prepare({48000, 137, 2}, callbackNode, nodeOutput) == Status::ok &&
                graph.process(input.block(), input.block()) == Status::ok,
            "graph prepared processor callback");
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (auto x : input.block().channel(ch))
            require(x == static_cast<T>(ch + 1) * T{0.375}, "graph callback oracle");
}
void asynchronous(ConvolutionMode mode) {
    AsyncConvolver<float> convolver;
    const std::array<float, 1> initial{1};
    require(convolver.prepare({48000, 17, 1}, initial, {mode, 8, 64}) == Status::ok,
            "async prepare");
    AudioBuffer<float> in, out;
    require(in.resize(1, 17) == Status::ok && out.resize(1, 17) == Status::ok, "async buffers");
    std::fill(in.block().channel(0).begin(), in.block().channel(0).end(), 1.f);
    const std::array<float, 1> two{2}, three{3};
    require(convolver.stageImpulse(two, 33) == Status::ok &&
                convolver.stageImpulse(three, 33) == Status::ok,
            "async latest pending replacement");
    require(convolver.process(in.block(0), out.block(0)) == Status::ok &&
                convolver.hasPendingImpulse(),
            "async empty call preserves publication");
    require(convolver.process(in.block(), out.block()) == Status::ok && convolver.isTransitioning(),
            "async begins fade");
    require(convolver.stageImpulse(two, 0) == Status::ok &&
                convolver.stageImpulse(initial, 0) == Status::update_queue_full,
            "async bounded full queue");
    require(convolver.process(in.block(), out.block()) == Status::ok &&
                convolver.process(in.block(), out.block()) == Status::ok,
            "async queued immediate activation");
    for (auto x : out.block().channel(0))
        require(x == 2, "async queued destination output");
    require(convolver.stageImpulse(initial, 0) == Status::ok &&
                convolver.process(in.block(), out.block()) == Status::ok,
            "async stress initial state");
    std::atomic<bool> done = false;
    std::atomic<int> error = 0;
    std::thread producer([&] {
        for (int i = 0; i < 200; ++i) {
            std::array<float, 257> impulse{};
            impulse[0] = impulse.back() = (1 + static_cast<float>(i % 17) / 16) * 0.5f;
            for (;;) {
                const auto status = convolver.stageImpulse(impulse, 33);
                if (status == Status::ok)
                    break;
                if (status != Status::update_queue_full) {
                    error = 1;
                    done = true;
                    return;
                }
                std::this_thread::yield();
            }
        }
        done.store(true, std::memory_order_release);
    });
    bool valid = true;
    while (!done.load(std::memory_order_acquire) || convolver.hasPendingImpulse() ||
           convolver.isTransitioning()) {
        valid = convolver.process(in.block(), out.block()) == Status::ok && valid;
        for (auto x : out.block().channel(0))
            valid = valid && isFinite(x) && x >= 0.9999f && x <= 2.0001f;
        std::this_thread::yield();
    }
    producer.join();
    require(valid && error.load() == 0, "async concurrent ownership and fade bounds");
    convolver.reset();
}
} // namespace
void completionRegression() {
    nonuniform<float>();
    nonuniform<double>();
    matrixConvolution<float>();
    matrixConvolution<double>();
    equalizer<float>();
    equalizer<double>();
    resamplerCompletion<float>();
    resamplerCompletion<double>();
    graph<float>();
    graph<double>();
    for (auto mode :
         {ConvolutionMode::hybrid, ConvolutionMode::nonuniform, ConvolutionMode::scheduled})
        asynchronous(mode);
}
