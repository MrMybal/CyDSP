// Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
// Explicit CPU-only before/after benchmark; no model training or GPU runtime.
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <vector>
using namespace cydsp;
void ok(Status s) {
    if (s != Status::ok)
        throw std::runtime_error("status " + std::to_string(int(s)));
}
volatile double checksum;
template <class F> double timing(F&& f) {
    for (unsigned i = 0; i < 24; ++i)
        f();
    std::array<double, 9> t;
    for (auto& v : t) {
        auto a = std::chrono::steady_clock::now();
        for (unsigned i = 0; i < 96; ++i)
            f();
        v = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - a)
                .count() /
            96;
    }
    std::sort(t.begin(), t.end());
    return t[4];
}
#ifdef CYDSP_NEURAL_BATCH
void transitionQuality(const NeuralHrtfModel& model) {
    constexpr std::size_t prefix = 8192, suffix = 512, block = 128;
    constexpr std::array<unsigned, 4> histories{0, 512, 2048, 8192};
    std::array<double, 4> error{}, maximum{};
    double energy = 0;
    for (unsigned trial = 0; trial < 16; ++trial) {
        const double a = .17 + trial * .371, b = a + 1.13;
        const Vec3 from{std::cos(a), std::sin(a), .2}, to{std::cos(b), std::sin(b), -.31};
        std::array<NeuralBinaural<double>, 4> actual;
        NeuralBinaural<double> oldReference, newReference;
        NeuralBinauralOptions cold;
        cold.transitionHistoryFrames = 0;
        ok(oldReference.prepare({48000, block, 1}, model, cold));
        ok(newReference.prepare({48000, block, 1}, model, cold));
        ok(oldReference.setDirection(from, 0));
        ok(newReference.setDirection(to, 0));
        for (std::size_t h = 0; h < 4; ++h) {
            auto options = cold;
            options.transitionHistoryFrames = histories[h];
            ok(actual[h].prepare({48000, block, 1}, model, options));
            ok(actual[h].setDirection(from, 0));
        }
        AudioBuffer<double> input, oldOut, newOut, out;
        ok(input.resize(1, block));
        ok(oldOut.resize(2, block));
        ok(newOut.resize(2, block));
        ok(out.resize(2, block));
        std::uint32_t seed = 20261117 + trial;
        for (std::size_t first = 0; first < prefix + suffix; first += block) {
            for (std::size_t i = 0; i < block; ++i) {
                seed = 1664525u * seed + 1013904223u;
                const double noise = double(seed >> 8) / 16777216. - .5;
                const double t = double(first + i);
                input.block().channel(0)[i] =
                    .04 * std::sin(.043 * t) + .027 * std::sin(.181 * t) + .02 * noise;
            }
            if (first == prefix)
                for (auto& voice : actual)
                    ok(voice.setDirection(to, 256));
            ok(oldReference.process(input.block(), oldOut.block()));
            ok(newReference.process(input.block(), newOut.block()));
            for (std::size_t h = 0; h < 4; ++h) {
                ok(actual[h].process(input.block(), out.block()));
                if (first >= prefix)
                    for (std::size_t e = 0; e < 2; ++e)
                        for (std::size_t i = 0; i < block; ++i) {
                            const double blend = std::min(1., double(first - prefix + i + 1) / 256),
                                         a = oldOut.block().channel(e)[i],
                                         b = newOut.block().channel(e)[i],
                                         expected = a + (b - a) * blend,
                                         delta = out.block().channel(e)[i] - expected;
                            if (!std::isfinite(delta))
                                throw std::runtime_error("nonfinite transition oracle");
                            error[h] += delta * delta;
                            maximum[h] = std::max(maximum[h], std::abs(delta));
                            if (h == 0)
                                energy += expected * expected;
                        }
            }
        }
    }
    if (maximum[3] > 2e-12)
        throw std::runtime_error("full retained history fails independent running-target oracle");
    std::cout << ",\"transitionQuality\":{\"cases\":16,\"prefixFrames\":" << prefix
              << ",\"evaluatedFrames\":" << suffix << ",\"profiles\":[";
    for (std::size_t h = 0; h < 4; ++h) {
        if (h)
            std::cout << ',';
        std::cout << "{\"historyFrames\":" << histories[h] << ",\"relativeRmsErrorDb\":"
                  << 10 * std::log10(std::max(1e-30, error[h] / energy))
                  << ",\"maxAbsError\":" << maximum[h] << "}";
    }
    std::cout << "]}";
}
#endif

int main(int argc, char** argv) {
    try {
        if (argc != 2)
            throw std::runtime_error("model.cyhrtf");
        std::ifstream f(argv[1], std::ios::binary | std::ios::ate);
        const auto size = f.tellg();
        if (size < 1 || size > 2000000)
            throw std::runtime_error("size");
        f.seekg(0);
        std::vector<std::byte> bytes{std::size_t(size)};
        f.read(reinterpret_cast<char*>(bytes.data()), size);
        NeuralHrtfModel model;
        ok(model.load(bytes));
        std::array<Vec3, 256> dirs;
        std::array<NeuralHrtfDesign, 256> designs;
        for (std::size_t i = 0; i < 256; ++i) {
            const double az = (i + .137) * 2 * std::numbers::pi / 256,
                         el = .6 * std::sin(2 * az + .23);
            dirs[i] = {std::cos(el) * std::cos(az), std::cos(el) * std::sin(az), std::sin(el)};
            NeuralHrtfParameters p;
            ok(model.predict(dirs[i], p));
            ok(designNeuralHrtf(p, designs[i]));
        }
        if (model.info().sampleRate != 48000)
            throw std::runtime_error("benchmark requires a 48 kHz model");
        NeuralHrtfParameters parameters;
        NeuralHrtfDesign completed;
        const double predictionUs = timing([&] {
            ok(model.predict(dirs[17], parameters));
            checksum = parameters.itdMicroseconds;
        });
        const double designUs = timing([&] {
            ok(designNeuralHrtf(parameters, completed));
            checksum = completed.delayFrames[0];
        });
        VectorOps ops;
        ok(VectorOps::create(CpuBackend::automatic, ops));
        bool first = true;
        std::cout << std::setprecision(12) << "{\"backend\":\"" << backendName(model.info().backend)
                  << "\",\"predictionUs\":" << predictionUs << ",\"designUs\":" << designUs
                  << ",\"sampleRate\":48000,\"blockFrames\":128,\"updateFrames\":768,"
                     "\"fadeFrames\":256,\"measurements\":[";
#ifdef CYDSP_NEURAL_BATCH
        for (unsigned history : {0u, 512u, 2048u})
            for (bool batch : {false, true})
#else
        for (unsigned history : {0u})
            for (bool batch : {false})
#endif
                for (std::size_t n : {1u, 16u, 64u}) {
                    std::vector<NeuralBinaural<float>> voice(n);
                    std::vector<NeuralHrtfDesign> pending(n);
                    NeuralBinauralOptions options;
#ifdef CYDSP_NEURAL_BATCH
                    options.transitionHistoryFrames = history;
#endif
                    for (auto& v : voice)
                        ok(v.prepare({48000, 128, 1}, model, options));
                    AudioBuffer<float> source, output, bus;
                    ok(source.resize(n, 128));
                    ok(output.resize(2 * n, 128));
                    ok(bus.resize(2, 128));
                    for (std::size_t v = 0; v < n; ++v)
                        for (std::size_t i = 0; i < 128; ++i)
                            source.block().channel(v)[i] =
                                float(.05 * std::sin(.17 * i + .013 * v) +
                                      .025 * std::cos(.079 * i + .007 * v));
                    std::size_t tick = 0;
                    auto reset = [&] {
                        tick = 0;
                        for (std::size_t v = 0; v < n; ++v) {
                            voice[v].reset();
                            ok(voice[v].setDesign(designs[(3 * v) % 256], 0));
                        }
                    };
                    auto run = [&](int motion) {
                        if (motion && tick % 6 == 0) {
                            for (std::size_t v = 0; v < n; ++v) {
                                const auto index = (tick / 6 + 3 * v) % 256;
                                if (motion == 2) {
                                    NeuralHrtfParameters p;
                                    ok(model.predict(dirs[index], p));
                                    ok(designNeuralHrtf(p, pending[v]));
                                } else
                                    pending[v] = designs[index];
                            }
#ifdef CYDSP_NEURAL_BATCH
                            if (batch)
                                ok(NeuralBinaural<float>::setDesigns(voice, pending, 256));
                            else
#endif
                                for (std::size_t v = 0; v < n; ++v)
                                    ok(voice[v].setDesign(pending[v], 256));
                        }
#ifdef CYDSP_NEURAL_BATCH
                        if (batch)
                            ok(NeuralBinaural<float>::processBatch(voice, source.block(),
                                                                   output.block()));
                        else
#endif
                            for (std::size_t v = 0; v < n; ++v) {
                                const float* a[]{source.block().channel(v).data()};
                                float* b[]{output.block().channel(2 * v).data(),
                                           output.block().channel(2 * v + 1).data()};
                                ok(voice[v].process({a, 1, 128}, {b, 2, 128}));
                            }
                        for (unsigned ear = 0; ear < 2; ++ear) {
                            std::fill(bus.block().channel(ear).begin(),
                                      bus.block().channel(ear).end(), 0.f);
                            for (std::size_t v = 0; v < n; ++v)
                                ok(ops.addScaled(output.block().channel(2 * v + ear),
                                                 bus.block().channel(ear), 1.f));
                        }
                        checksum = bus.block().channel(0)[127];
                        ++tick;
                    };
                    reset();
                    const double fixed = timing([&] { run(0); });
                    reset();
                    const double prepared = timing([&] { run(1); });
                    reset();
                    const double total = timing([&] { run(2); });
                    for (unsigned ear = 0; ear < 2; ++ear)
                        for (float x : bus.block().channel(ear))
                            if (!std::isfinite(x))
                                throw std::runtime_error("nonfinite output");
                    if (!first)
                        std::cout << ',';
                    first = false;
                    std::cout << "{\"voices\":" << n << ",\"batch\":" << (batch ? "true" : "false")
                              << ",\"historyFrames\":" << history << ",\"staticUs\":" << fixed
                              << ",\"movingPreparedUs\":" << prepared
                              << ",\"movingTotalUs\":" << total << "}";
                }
        std::cout << "],\"checksum\":" << checksum;
#ifdef CYDSP_NEURAL_BATCH
        transitionQuality(model);
#endif
        std::cout << "}\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
