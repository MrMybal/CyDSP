#include <algorithm>
#include <array>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <stdexcept>
#include <vector>
namespace {
using namespace cydsp;
void require(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
template <class T> constexpr double tolerance = std::same_as<T, float> ? 3e-4 : 2e-11;
template <class T>
void matrixCase(CpuBackend backend, std::size_t p, std::size_t ins, std::size_t outs,
                bool sparse = false) {
    const auto early = std::min(p, std::size_t{8}), frames = std::max(11 * p + 57, std::size_t{97}),
               sourceFrames = 3 * p + 17;
    const ConvolutionConfig config{ConvolutionMode::scheduled, early, p};
    const ProcessSpec spec{48000, frames, ins}, splitSpec{48000, 71, ins};
    const std::array<std::size_t, 8> lengths{1,         2 * p, 2 * p + 1, 4 * p + 13,
                                             3 * p - 1, p + 1, 7 * p + 3, 7 * p + 3};
    std::array<std::vector<T>, 8> impulses;
    std::array<ConvolutionRoute<T>, 8> routes;
    std::array<bool, 5> activeInputs{}, activeOutputs{};
    std::size_t products = 0;
    for (std::size_t r = 0; r < routes.size(); ++r) {
        impulses[r].resize(lengths[r]);
        for (std::size_t k = 0; k < lengths[r]; ++k)
            if (!sparse || k == 0 || k == p - 1 || k == p || k == 2 * p - 1 || k == 2 * p ||
                k == 3 * p + 1 || k == lengths[r] - 1)
                impulses[r][k] =
                    static_cast<T>(static_cast<int>((k + 3 * r) % 13) - 6) * T{0.0078125};
        routes[r] = {r % 2 ? 0 : ins - 1, (r / 2) % (outs - 1), impulses[r]};
        if (lengths[r] > 2 * p) {
            activeInputs[routes[r].inputChannel] = true;
            activeOutputs[routes[r].outputChannel] = true;
            products += 1 + (lengths[r] - 2 * p - 1) / p;
        }
    }
    // Duplicate source/destination pairs with different copied IRs must add independently.
    routes.back().inputChannel = routes[6].inputChannel;
    routes.back().outputChannel = routes[6].outputChannel;
    MatrixConvolver<T> whole, split;
    require(whole.prepare(spec, outs, routes, config, backend) == Status::ok &&
                split.prepare(splitSpec, outs, routes, config, backend) == Status::ok,
            "progressive MIMO prepare");
    const auto chunks = (p + 63) / 64, macChunks = (p + 64) / 64;
    std::size_t stages = 0;
    for (auto n = p; n > 1; n /= 2)
        ++stages;
    const auto transforms =
        static_cast<std::size_t>(std::count(activeInputs.begin(), activeInputs.end(), true) +
                                 std::count(activeOutputs.begin(), activeOutputs.end(), true));
    const auto units =
        transforms * (2 * chunks + stages * ((p / 2 + 63) / 64)) + products * macChunks;
    require(whole.configuration() == config && whole.backend() == backend &&
                whole.schedulingActive() && whole.latencyFrames() == 0 &&
                whole.tailFrames() == *std::max_element(lengths.begin(), lengths.end()) - 1 &&
                whole.workUnitsPerPartition() == units &&
                whole.maximumWorkUnitsPerFrame() == units / p + (units % p ? 1 : 0),
            "progressive MIMO shared transform job budget");
    AudioBuffer<T> input, output, storage;
    require(input.resize(ins, frames) == Status::ok && output.resize(outs, frames) == Status::ok &&
                storage.resize(std::max(ins, outs), frames) == Status::ok,
            "progressive MIMO buffers");
    for (std::size_t ch = 0; ch < ins; ++ch)
        for (std::size_t f = 0; f < frames; ++f)
            input.block().channel(ch)[f] = storage.block().channel(ch)[f] =
                f < sourceFrames
                    ? static_cast<T>(static_cast<int>((f * 7 + ch * 3) % 19) - 9) * T{0.0625}
                    : T{};
    std::array<T*, 5> sourcePointers{}, outputPointers{};
    for (std::size_t ch = 0; ch < ins; ++ch)
        sourcePointers[ch] = storage.block().channel(ch).data();
    for (std::size_t ch = 0; ch < outs; ++ch)
        outputPointers[ch] = storage.block().channel((ch + 1) % std::max(ins, outs)).data();
    const AudioBlock<T> source{sourcePointers.data(), ins, frames};
    const AudioBlock<T> target{outputPointers.data(), outs, frames};
    std::vector<double> expected(outs * frames);
    for (const auto& route : routes)
        for (std::size_t k = 0; k < route.impulse.size(); ++k)
            if (route.impulse[k] != T{})
                for (std::size_t f = 0; f < sourceFrames; ++f)
                    expected[route.outputChannel * frames + f + k] +=
                        static_cast<double>(route.impulse[k]) *
                        input.block().channel(route.inputChannel)[f];
    require(whole.process(input.block(), output.block()) == Status::ok, "progressive MIMO whole");
    for (std::size_t f = 0; f < frames;) {
        require(split.process(source.subBlock(0, 8), target.subBlock(1, 8)) ==
                        Status::partial_overlap &&
                    split.process(source.subBlock(0, 72), target.subBlock(0, 72)) ==
                        Status::block_too_large,
                "progressive MIMO invalid request before mutation");
        const auto count = std::min(frames - f, 1 + (f * 7) % 71);
        require(split.process(source.subBlock(f, count), target.subBlock(f, count)) == Status::ok,
                "progressive MIMO rectangular cross-alias split");
        if (f < p + 3 && f + count >= p + 3) {
            require(split.prepare(splitSpec, outs, routes, {ConvolutionMode::scheduled, early, 3},
                                  backend) == Status::invalid_argument &&
                        split.configuration() == config && split.workUnitsPerPartition() == units,
                    "progressive MIMO failed preparation retains partial work");
            MatrixConvolver<T> moved(std::move(split));
            require(!split.schedulingActive() && split.workUnitsPerPartition() == 0 &&
                        split.process(source.subBlock(f, 0), target.subBlock(f, 0)) ==
                            Status::not_prepared,
                    "progressive MIMO move source empty");
            split = std::move(moved);
        }
        f += count;
    }
    for (std::size_t ch = 0; ch < outs; ++ch)
        for (std::size_t f = 0; f < frames; ++f) {
            require(std::abs(output.block().channel(ch)[f] - expected[ch * frames + f]) <
                            tolerance<T> &&
                        std::abs(target.channel(ch)[f] - expected[ch * frames + f]) < tolerance<T>,
                    "progressive shared MIMO direct convolution complete-tail oracle");
            if (ch == outs - 1)
                require(output.block().channel(ch)[f] == T{} && target.channel(ch)[f] == T{},
                        "progressive disconnected output silence");
        }
    // Reset with a partially completed FFT must erase every signal buffer and scheduler cursor.
    split.reset();
    for (std::size_t f = 0; f < p + 17;) {
        const auto count = std::min(std::size_t{71}, p + 17 - f);
        require(split.process(input.block().subBlock(f, count),
                              output.block().subBlock(f, count)) == Status::ok,
                "progressive MIMO reset partial process");
        f += count;
    }
    split.reset();
    require(split.process(input.block(71), output.block(71)) == Status::ok,
            "progressive MIMO partial reset");
    for (std::size_t ch = 0; ch < outs; ++ch)
        for (std::size_t f = 0; f < 71; ++f)
            require(std::abs(output.block().channel(ch)[f] - expected[ch * frames + f]) <
                        tolerance<T>,
                    "progressive MIMO partial reset direct oracle");
    // Reconfigure to a different engine while history is live; the new plan starts empty.
    require(split.prepare(splitSpec, outs, routes, {ConvolutionMode::hybrid, early, 3}, backend) ==
                    Status::ok &&
                !split.schedulingActive() && split.workUnitsPerPartition() == 0 &&
                split.process(input.block(71), output.block(71)) == Status::ok,
            "MIMO successful engine replacement");
    for (std::size_t ch = 0; ch < outs; ++ch)
        for (std::size_t f = 0; f < 71; ++f)
            require(std::abs(output.block().channel(ch)[f] - expected[ch * frames + f]) <
                        tolerance<T>,
                    "MIMO successful engine change clears history");
    require(split.prepare(splitSpec, outs, {}, config, backend) == Status::ok &&
                !split.schedulingActive() && split.workUnitsPerPartition() == 0 &&
                split.maximumWorkUnitsPerFrame() == 0 && split.tailFrames() == 0 &&
                split.process(input.block(71), output.block(71)) == Status::ok,
            "progressive empty route preparation");
    for (std::size_t ch = 0; ch < outs; ++ch)
        for (auto value : output.block(71).channel(ch))
            require(value == T{}, "empty MIMO silence");
}
} // namespace
void matrixSchedulingRegression() {
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::neon,
                         CpuBackend::avx512, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        for (auto p : {1u, 2u, 4u, 8u, 32u, 64u, 128u, 256u}) {
            matrixCase<float>(backend, p, 4, 3);
            matrixCase<double>(backend, p, 4, 3);
            matrixCase<float>(backend, p, 2, 5);
            matrixCase<double>(backend, p, 2, 5);
        }
        matrixCase<float>(backend, 1024, 4, 3, true);
        matrixCase<double>(backend, 1024, 4, 3, true);
    }
}
