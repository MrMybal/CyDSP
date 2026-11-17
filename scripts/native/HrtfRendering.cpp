// Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
// Explicit offline quality/performance harness for the public native API.
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <stdexcept>
using namespace cydsp;
namespace {
void ok(Status s) {
    if (s != Status::ok)
        throw std::runtime_error("CyDSP status " + std::to_string(int(s)));
}
template <class T> std::vector<T> read(std::ifstream& s, std::size_t count) {
    if (count > 32000000)
        throw std::runtime_error("harness size limit");
    std::vector<T> out(count);
    if (!s.read(reinterpret_cast<char*>(out.data()), std::streamsize(count * sizeof(T))))
        throw std::runtime_error("truncated harness asset");
    return out;
}
template <class T> void write(std::ofstream& s, const T* data, std::size_t count) {
    if (!s.write(reinterpret_cast<const char*>(data), std::streamsize(count * sizeof(T))))
        throw std::runtime_error("harness output error");
}
template <class F> void timing(const char* name, std::size_t voices, F&& fn) {
    std::vector<double> samples(160);
    for (int i = 0; i < 20; ++i)
        fn();
    for (auto& value : samples) {
        const auto start = std::chrono::steady_clock::now();
        fn();
        value = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start)
                    .count();
    }
    std::sort(samples.begin(), samples.end());
    std::cout << "{\"kind\":\"" << name << "\",\"voices\":" << voices
              << ",\"medianUs\":" << samples[80] << ",\"p95Us\":" << samples[152]
              << ",\"maxUs\":" << samples.back() << "}";
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc != 4 && argc != 5)
            throw std::runtime_error("weights.bin corpus.bin audio.bin");
        std::ifstream modelFile(argv[1], std::ios::binary | std::ios::ate);
        const auto length = modelFile.tellg();
        if (length <= 0 || length > 2000000)
            throw std::runtime_error("model size");
        modelFile.seekg(0);
        auto weights = read<std::byte>(modelFile, std::size_t(length));
        NeuralHrtfModel model;
        CpuBackend backend = CpuBackend::automatic;
        if (argc == 5) {
            const std::string name = argv[4];
            if (name == "scalar")
                backend = CpuBackend::scalar;
            else if (name == "sse2")
                backend = CpuBackend::sse2;
            else if (name == "avx2")
                backend = CpuBackend::avx2;
            else if (name == "avx512")
                backend = CpuBackend::avx512;
            else if (name != "automatic")
                throw std::runtime_error("unknown backend");
        }
        ok(model.load(weights, backend));
        const auto info = model.info();
        std::ifstream source(argv[2], std::ios::binary);
        auto magic = read<char>(source, 8);
        if (std::string(magic.begin(), magic.end()) != "CYHRQA02")
            throw std::runtime_error("corpus magic");
        auto shape = read<std::uint32_t>(source, 3);
        const auto points = shape[0], taps = shape[1], tests = shape[2];
        if (!points || points > 100000 || !taps || taps > 1024 || !tests || tests > points)
            throw std::runtime_error("corpus dimensions");
        auto xyz = read<float>(source, points * 3);
        auto impulses = read<float>(source, points * 2 * taps);
        auto delays = read<double>(source, points * 2);
        auto indices = read<std::uint32_t>(source, tests);
        std::vector<Vec3> directions(points);
        std::vector<HrirPoint<float>> hrir(points);
        for (std::size_t i = 0; i < points; ++i) {
            directions[i] = {xyz[3 * i], xyz[3 * i + 1], xyz[3 * i + 2]};
            hrir[i] = {directions[i],
                       {impulses.data() + 2 * i * taps, taps},
                       {impulses.data() + (2 * i + 1) * taps, taps},
                       delays[2 * i],
                       delays[2 * i + 1]};
        }
        HrtfPreparation preparation;
        preparation.sourceSampleRate = preparation.targetSampleRate = info.sampleRate;
        preparation.detectOnsets = false;
        preparation.maximumPoints = points;
        preparation.maximumTaps = taps;
        preparation.interpolationPoints = 4;
        HrtfSet<float> bank;
        ok(bank.prepare(hrir, preparation));
        constexpr std::size_t block = 128;
        const std::size_t frames = info.sampleRate <= 48000   ? 8192
                                   : info.sampleRate <= 96000 ? 16384
                                                              : 32768;
        NeuralBinaural<float> neural;
        ok(neural.prepare({info.sampleRate, block, 1}, model));
        AudioBuffer<float> input, output, bus;
        ok(input.resize(1, block));
        ok(output.resize(2, block));
        ok(bus.resize(2, block));
        std::vector<float> rendered(2 * frames);
        std::ofstream audio(argv[3], std::ios::binary);
        const std::array<std::uint32_t, 3> audioShape{tests, std::uint32_t(frames),
                                                      std::uint32_t(info.sectionsPerEar)};
        write(audio, audioShape.data(), audioShape.size());
        for (const auto index : indices) {
            if (index >= points)
                throw std::runtime_error("test index");
            NeuralHrtfParameters p;
            NeuralHrtfDesign design;
            ok(model.predict(directions[index], p));
            ok(designNeuralHrtf(p, design));
            ok(neural.setDesign(design, 0));
            std::vector<double> values;
            for (std::size_t ear = 0; ear < 2; ++ear) {
                for (std::size_t j = 0; j < p.sectionCount; ++j) {
                    const auto& v = p.sections[ear][j];
                    values.insert(values.end(), {v.frequencyHz, v.q, v.gainDb});
                }
                values.push_back(p.levelDb[ear]);
            }
            values.insert(values.end(), {p.itdMicroseconds, design.intrinsicItdMicroseconds,
                                         design.delayFrames[0], design.delayFrames[1]});
            write(audio, values.data(), values.size());
            for (std::size_t start = 0; start < frames; start += block) {
                std::fill(input.block().channel(0).begin(), input.block().channel(0).end(), 0.f);
                if (!start)
                    input.block().channel(0)[0] = 1;
                ok(neural.process(input.block(), output.block()));
                for (std::size_t ear = 0; ear < 2; ++ear)
                    std::copy(output.block().channel(ear).begin(),
                              output.block().channel(ear).end(),
                              rendered.begin() + std::ptrdiff_t(ear * frames + start));
            }
            write(audio, rendered.data(), rendered.size());
        }
        audio.close();
        for (std::size_t i = 0; i < block; ++i)
            input.block().channel(0)[i] =
                .05f * std::sin(float(i) * .13f) + .025f * std::cos(float(i) * .071f);
        std::array<NeuralHrtfDesign, 64> designs{};
        std::array<NeuralHrtfParameters, 64> parameters{};
        std::array<Vec3, 64> batchDirs{};
        for (std::size_t i = 0; i < 64; ++i) {
            batchDirs[i] = directions[indices[i % tests]];
            ok(model.predict(batchDirs[i], parameters[i]));
            ok(designNeuralHrtf(parameters[i], designs[i]));
        }
        // Real learned weights: verify public batch results against individual calls.
        std::array<NeuralHrtfParameters, 64> batchCheck;
        ok(model.predict(batchDirs, batchCheck));
        for (std::size_t i = 0; i < 64; ++i) {
            if (batchCheck[i].itdMicroseconds != parameters[i].itdMicroseconds ||
                batchCheck[i].levelDb != parameters[i].levelDb)
                throw std::runtime_error("batch prediction mismatch");
            for (std::size_t ear = 0; ear < 2; ++ear)
                for (std::size_t j = 0; j < info.sectionsPerEar; ++j) {
                    const auto& a = batchCheck[i].sections[ear][j];
                    const auto& b = parameters[i].sections[ear][j];
                    if (a.frequencyHz != b.frequencyHz || a.q != b.q || a.gainDb != b.gainDb)
                        throw std::runtime_error("batch section mismatch");
                }
        }
        VectorOps ops;
        ok(VectorOps::create(info.backend, ops));
        volatile double checksum = 0;
        std::cout << std::setprecision(10) << "{\"sampleRate\":" << info.sampleRate
                  << ",\"blockFrames\":" << block << ",\"backend\":" << int(info.backend)
                  << ",\"measurements\":[";
        timing("predict", 1, [&] {
            ok(model.predict(batchDirs[0], parameters[0]));
            checksum = parameters[0].itdMicroseconds;
        });
        std::cout << ',';
        timing("predictBatch", 64, [&] {
            ok(model.predict(batchDirs, parameters));
            checksum = parameters[63].itdMicroseconds;
        });
        std::cout << ',';
        timing("design", 1, [&] {
            ok(designNeuralHrtf(parameters[0], designs[0]));
            checksum = designs[0].delayFrames[0];
        });
        std::cout << ',';
        for (const std::size_t voices : {1u, 16u, 64u}) {
            std::vector<NeuralBinaural<float>> native(voices);
            std::vector<HrirBinaural<float>> measured(voices);
            for (std::size_t i = 0; i < voices; ++i) {
                ok(native[i].prepare({info.sampleRate, block, 1}, model));
                ok(native[i].setDesign(designs[i], 0));
                ok(measured[i].prepare({info.sampleRate, block, 1}, bank, info.backend));
                ok(measured[i].setDirection(batchDirs[i], 0));
            }
            auto mix = [&] {
                for (std::size_t e = 0; e < 2; ++e)
                    ok(ops.addScaled(output.block().channel(e), bus.block().channel(e), 1.f));
            };
            auto clear = [&] {
                for (std::size_t e = 0; e < 2; ++e)
                    std::fill(bus.block().channel(e).begin(), bus.block().channel(e).end(), 0.f);
            };
            timing("neuralStatic", voices, [&] {
                clear();
                for (auto& n : native) {
                    ok(n.process(input.block(), output.block()));
                    mix();
                }
                checksum = bus.block().channel(0)[127];
            });
            std::cout << ',';
            timing("measuredStatic", voices, [&] {
                clear();
                for (auto& m : measured) {
                    ok(m.process(input.block(), output.block()));
                    mix();
                }
                checksum = bus.block().channel(0)[127];
            });
            std::cout << ',';
            std::size_t tick = 0;
            timing("neuralContinuousFade", voices, [&] {
                clear();
                for (std::size_t i = 0; i < voices; ++i) {
                    if (tick % 2 == 0)
                        ok(native[i].setDesign(designs[(i + tick / 2) % 64], 256));
                    ok(native[i].process(input.block(), output.block()));
                    mix();
                }
                ++tick;
                checksum = bus.block().channel(0)[127];
            });
            std::cout << ',';
            tick = 0;
            timing("measuredContinuousFade", voices, [&] {
                clear();
                for (std::size_t i = 0; i < voices; ++i) {
                    if (tick % 2 == 0)
                        ok(measured[i].setDirection(batchDirs[(i + tick / 2) % 64], 256));
                    ok(measured[i].process(input.block(), output.block()));
                    mix();
                }
                ++tick;
                checksum = bus.block().channel(0)[127];
            });
            std::cout << ',';
            timing("measuredDirectionUpdate", voices, [&] {
                for (std::size_t i = 0; i < voices; ++i)
                    ok(measured[i].setDirection(batchDirs[(i + tick) % 64], 0));
                ++tick;
            });
            std::cout << ',';
        }
        timing("setPreparedDesign", 1, [&] {
            ok(neural.setDesign(designs[0], 0));
            checksum = double(neural.latencyFrames());
        });
        std::cout << "],\"checksum\":" << checksum << "}\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
