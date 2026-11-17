// Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
#include <bit>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cydsp/CyDSP.h>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <string_view>
using namespace cydsp;
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
void require(bool x, const char* message) {
    if (!x)
        throw std::runtime_error(message);
}
void u32(std::vector<std::byte>& bytes, std::uint32_t v) {
    for (unsigned i = 0; i < 4; ++i)
        bytes.push_back(std::byte((v >> (8 * i)) & 255));
}
void f32(std::vector<std::byte>& bytes, float x) {
    u32(bytes, std::bit_cast<std::uint32_t>(x));
}
std::vector<std::byte> fixture() {
    std::vector<std::byte> bytes;
    for (const char c : std::string_view("CYHRTF02"))
        bytes.push_back(std::byte(c));
    for (const auto v : {9u, 16u, 27u, 4u, 1u, 48000u})
        u32(bytes, v);
    f32(bytes, -6);
    // Original synthetic constant network: identity magnitude, total ITD=10.25 frames.
    constexpr std::size_t parameters = 9 * 16 + 16 + 16 * 16 + 16 + 16 * 27 + 27;
    for (std::size_t i = 0; i < parameters; ++i)
        f32(bytes, i == parameters - 1 ? float(std::atanh((10.25 / 48000 * 1e6) / 1200)) : 0.f);
    for (const float c : {150.f, 1000.f, 6000.f, 18000.f})
        f32(bytes, c);
    for (int i = 0; i < 4; ++i)
        f32(bytes, .01f);
    f32(bytes, float(std::numbers::pi));
    return bytes;
}
template <class T> void audio(const NeuralHrtfModel& model) {
    NeuralBinaural<T> renderer, partitioned;
    AudioBuffer<T> input, output, splitOutput;
    require(renderer.prepare({48000, 64, 1}, model) == Status::ok, "neural prepare");
    require(partitioned.prepare({48000, 64, 1}, model) == Status::ok, "neural partition prepare");
    require(renderer.prepare({96000, 64, 1}, model) == Status::invalid_spec,
            "reject rate mismatch");
    require(input.resize(1, 64) == Status::ok && output.resize(2, 64) == Status::ok &&
                splitOutput.resize(2, 64) == Status::ok,
            "neural buffers");
    require(renderer.process(input.block(), output.block()) == Status::not_prepared,
            "require initial direction");
    require(renderer.setDirection({1, 0, 0}, 0) == Status::ok &&
                partitioned.setDirection({1, 0, 0}, 0) == Status::ok,
            "neural direction");
    input.block().channel(0)[0] = T(1);
    require(renderer.process(input.block(), output.block()) == Status::ok, "neural impulse");
    require(partitioned.process(input.block().subBlock(0, 13),
                                splitOutput.block().subBlock(0, 13)) == Status::ok &&
                partitioned.process(input.block().subBlock(13, 51),
                                    splitOutput.block().subBlock(13, 51)) == Status::ok,
            "neural split blocks");
    double energy[2]{};
    for (std::size_t ear = 0; ear < 2; ++ear)
        for (std::size_t i = 0; i < 64; ++i) {
            require(output.block().channel(ear)[i] == splitOutput.block().channel(ear)[i],
                    "block partition invariant");
            energy[ear] +=
                double(output.block().channel(ear)[i]) * double(output.block().channel(ear)[i]);
        }
    require(std::abs(double(output.block().channel(0)[0]) - std::pow(10., -6. / 20)) < 1e-7,
            "restore absolute magnitude offset");
    require(output.block().channel(1)[9] == 0 && output.block().channel(1)[10] != 0,
            "causal integer delay");
    require(std::abs(energy[0] - energy[1]) < 2e-7, "fractional delay preserves broadband energy");
    NeuralHrtfParameters p;
    require(model.predict({1, 0, 0}, p) == Status::ok, "neural parameters");
    NeuralHrtfDesign design;
    require(designNeuralHrtf(p, design) == Status::ok, "neural design");
    design.gain = {.25, .75};
    require(renderer.setDesign(design, 79) == Status::ok, "start full transition");
    require(renderer.setDesign(design, 79) == Status::transition_in_progress, "reject busy update");
    require(renderer.process(input.block(), output.block()) == Status::ok &&
                renderer.transitionRemainingFrames() == 15,
            "transition block accounting");
    renderer.reset();
    require(renderer.transitionRemainingFrames() == 0, "reset chooses latest endpoint");
    design.sections[0][0].a2 = 1;
    require(renderer.setDesign(design) == Status::invalid_argument, "reject unstable host design");
    const T* source = input.block().channel(0).data();
    T* aliases[]{input.block().channel(0).data(), output.block().channel(1).data()};
    require(renderer.process({&source, 1, 64}, {aliases, 2, 64}) == Status::partial_overlap,
            "reject overlapping audio");
    struct Context {
        NeuralBinaural<T>* r;
        AudioBuffer<T>* in;
        AudioBuffer<T>* out;
        bool pass = true;
    } context{&renderer, &input, &output};
    runRealtimeAudit(
        [](void* raw) noexcept {
            auto& c = *static_cast<Context*>(raw);
            c.r->reset();
            for (int i = 0; i < 4; ++i) {
                c.pass &= c.r->setDirection({1, 0, 0}, 64) == Status::ok;
                c.pass &= c.r->process(c.in->block(), c.out->block()) == Status::ok;
            }
        },
        &context);
    require(context.pass, "allocation-free updates and processing");
}
} // namespace
void neuralOptimizationRegression();
void neuralHrtfRegression() {
    auto bytes = fixture();
    NeuralHrtfModel model;
    require(model.load(bytes, CpuBackend::scalar) == Status::ok, "synthetic native model load");
    require(model.info().parameters == 891 && model.info().sampleRate == 48000, "model dimensions");
    NeuralHrtfParameters p;
    require(model.predict({0, 0, 0}, p) == Status::invalid_argument && p.sectionCount == 0,
            "zero direction preserves output");
    require(model.predict({1, 0, 0}, p) == Status::ok, "constant model");
    NeuralHrtfDesign d;
    require(designNeuralHrtf(p, d) == Status::ok && std::abs(d.delayFrames[1] - 10.25) < 1e-5 &&
                d.delayFrames[0] == 0,
            "ITD residual causal design");
    for (std::size_t cut = 0; cut < bytes.size(); ++cut)
        require(model.load({bytes.data(), cut}) != Status::ok, "all truncations rejected");
    auto invalid = bytes;
    invalid.push_back(std::byte{});
    require(model.load(invalid) != Status::ok && model.info().parameters == 891,
            "failed load preserves shared model");
    invalid = bytes;
    // Nonfinite first weight, valid outer shape.
    invalid[36] = std::byte{0};
    invalid[37] = std::byte{0};
    invalid[38] = std::byte{128};
    invalid[39] = std::byte{127};
    require(model.load(invalid) == Status::unsupported_format, "nonfinite weights rejected");
    NeuralHrtfModel shared = model;
    require(model.load({}) != Status::ok && shared.predict({4, 0, 0}, p) == Status::ok,
            "shared immutable ownership");
    std::array<Vec3, 2> dirs{{{1, 0, 0}, {0, 0, 0}}};
    std::array<NeuralHrtfParameters, 2> result{};
    require(shared.predict(dirs, result) == Status::invalid_argument && result[0].sectionCount == 0,
            "whole batch preflight");
    std::array<Vec3, 9> batchDirs{};
    std::array<NeuralHrtfParameters, 9> batchResult{};
    for (std::size_t i = 0; i < batchDirs.size(); ++i)
        batchDirs[i] = {1, double(i), -1};
    require(shared.predict(batchDirs, batchResult) == Status::ok, "tiled prediction remainder");
    for (std::size_t i = 0; i < batchDirs.size(); ++i) {
        NeuralHrtfParameters one;
        require(shared.predict(batchDirs[i], one) == Status::ok &&
                    one.itdMicroseconds == batchResult[i].itdMicroseconds &&
                    one.levelDb == batchResult[i].levelDb,
                "tiled scalar equivalence");
    }
    // Independent complex-transfer oracle: compensate a deliberately asymmetric cascade.
    p.sections[0][0].gainDb = 12;
    require(designNeuralHrtf(p, d) == Status::ok, "asymmetric cascade design");
    double num = 0, den = 0;
    const double mean = double(18 + 128) * .5;
    for (int k = 18; k <= 128; ++k) {
        FilterResponse l, r;
        const double hz = 48000. * k / 4096,
                     centered = 2 * std::numbers::pi * (double(k) - mean) * 48000 / 4096;
        require(iirResponse({d.sections[0].data(), d.sectionCount}, 48000, hz, l) == Status::ok &&
                    iirResponse({d.sections[1].data(), d.sectionCount}, 48000, hz, r) == Status::ok,
                "phase oracle");
        num += std::arg(l.gain * std::conj(r.gain)) * centered;
        den += centered * centered;
    }
    require(std::abs(d.intrinsicItdMicroseconds - num / den * 1e6) < 1e-7,
            "independent phase-slope compensation");
    audio<float>(shared);
    audio<double>(shared);
    neuralOptimizationRegression();
}
