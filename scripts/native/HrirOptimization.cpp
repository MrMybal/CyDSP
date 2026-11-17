// Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
// Explicit CPU-only comparison. Reads the corpus created by validate_hrtf_rendering.py.
#include <algorithm>
#include <chrono>
#include <cydsp/CyDSP.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace cydsp;
void ok(Status s) {
    if (s != Status::ok)
        throw std::runtime_error("DSP status " + std::to_string(int(s)));
}
template <class T> std::vector<T> read(std::ifstream& stream, std::size_t n) {
    if (n > 32000000)
        throw std::runtime_error("corpus bound");
    std::vector<T> out(n);
    if (!stream.read(reinterpret_cast<char*>(out.data()), std::streamsize(n * sizeof(T))))
        throw std::runtime_error("truncated corpus");
    return out;
}
template <class F> double median(F&& f, unsigned iterations = 256) {
    for (unsigned i = 0; i < std::min(64u, iterations); ++i)
        f();
    std::vector<double> result;
    for (unsigned pass = 0; pass < 11; ++pass) {
        const auto start = std::chrono::steady_clock::now();
        for (unsigned i = 0; i < iterations; ++i)
            f();
        const auto stop = std::chrono::steady_clock::now();
        result.push_back(std::chrono::duration<double, std::micro>(stop - start).count() /
                         double(iterations));
    }
    std::sort(result.begin(), result.end());
    return result[result.size() / 2];
}
int main(int argc, char** argv) {
    try {
        if (argc < 2 || argc > 4)
            throw std::runtime_error(
                "usage: hrir-optimization corpus.bin [neighbours=0] [fractionalTaps=0]");
        std::ifstream stream(argv[1], std::ios::binary);
        const auto magic = read<char>(stream, 8);
        if (std::string(magic.begin(), magic.end()) != "CYHRQA02")
            throw std::runtime_error("corpus magic");
        const auto shape = read<std::uint32_t>(stream, 3);
        const auto m = shape[0], n = shape[1];
        if (!m || m > 100000 || !n || n > 65536 || std::uint64_t(m) * 2 * n > 32000000)
            throw std::runtime_error("corpus shape bound");
        const auto xyz = read<float>(stream, 3 * m), ir = read<float>(stream, 2 * m * n);
        const auto delay = read<double>(stream, 2 * m);
        std::vector<HrirPoint<float>> points;
        for (std::size_t i = 0; i < m; ++i)
            points.push_back({{xyz[3 * i], xyz[3 * i + 1], xyz[3 * i + 2]},
                              {ir.data() + 2 * i * n, n},
                              {ir.data() + (2 * i + 1) * n, n},
                              delay[2 * i],
                              delay[2 * i + 1]});
        HrtfSet<float> bank;
        HrtfPreparation p;
        p.maximumPoints = 16384;
        p.interpolationPoints = argc > 2 ? std::stoul(argv[2]) : 0;
        p.fractionalDelayTaps = argc > 3 ? std::stoul(argv[3]) : 0;
        p.detectOnsets = false;
        ok(bank.prepare(points, p));
        HrirBinaural<float> direct, spectral, automatic;
        HrirRenderConfig c;
        c.convolution = HrirConvolution::direct;
        ok(direct.prepare({48000, 128, 1}, bank, c));
        c.convolution = HrirConvolution::fft;
        ok(spectral.prepare({48000, 128, 1}, bank, c));
        ok(automatic.prepare({48000, 128, 1}, bank));
        AudioBuffer<float> input, a, b, d;
        ok(input.resize(1, 128));
        ok(a.resize(2, 128));
        ok(b.resize(2, 128));
        ok(d.resize(2, 128));
        for (unsigned j = 0; j < 128; ++j)
            input.block().channel(0)[j] = float(.2 * std::sin(double(j) * .37));
        double maxError = 0, signal = 0, error = 0;
        for (std::size_t i = 0; i < 32; ++i) {
            const auto dir = points[(i * 271) % m].direction;
            const auto fade = i ? (i % 2 ? 37u : 513u) : 0u;
            for (auto* render : {&direct, &spectral, &automatic}) {
                render->reset();
                ok(render->setDirection(dir, fade));
            }
            for (unsigned block = 0; block < 16; ++block) {
                ok(direct.process(input.block(), a.block()));
                ok(spectral.process(input.block(), b.block()));
                ok(automatic.process(input.block(), d.block()));
                for (unsigned ear = 0; ear < 2; ++ear)
                    for (unsigned j = 0; j < 128; ++j) {
                        const double x = a.block().channel(ear)[j],
                                     delta = x - b.block().channel(ear)[j];
                        maxError = std::max(maxError, std::abs(delta));
                        signal += x * x;
                        error += delta * delta;
                    }
            }
        }
        if (maxError > 2e-6)
            throw std::runtime_error("measured HRIR FFT/direct disagreement");
        std::cout << std::setprecision(12)
                  << "{\n  \"sampleRate\":48000, \"blockFrames\":128, \"taps\":" << n
                  << ", \"interpolationPoints\":" << p.interpolationPoints
                  << ", \"fractionalDelayTaps\":" << p.fractionalDelayTaps << ", \"points\":" << m
                  << ", \"maxAbsError\":" << maxError
                  << ", \"relativeRmsError\":" << std::sqrt(error / signal)
                  << ",\n  \"measurements\":[\n";
        bool first = true;
        for (auto mode :
             {HrirConvolution::direct, HrirConvolution::fft, HrirConvolution::automatic}) {
            auto* render = mode == HrirConvolution::direct ? &direct
                           : mode == HrirConvolution::fft  ? &spectral
                                                           : &automatic;
            const auto name = mode == HrirConvolution::direct ? "direct"
                              : mode == HrirConvolution::fft  ? "fft"
                                                              : "automatic";
            render->reset();
            ok(render->setDirection(points[0].direction, 0));
            const double stationary =
                median([&] { ok(render->process(input.block(), b.block())); });
            std::size_t pose = 0;
            const double moving = median([&] {
                ok(render->setDirection(points[((++pose % 2) * 271) % m].direction, 128));
                ok(render->process(input.block(), b.block()));
            });
            render->reset();
            const double update = median(
                [&] { ok(render->setDirection(points[((++pose % 2) * 271) % m].direction, 0)); });
            const double interpolatedUpdate =
                median([&] { ok(render->setDirection({.932, .217, .301}, 0)); }, 16);
            if (!first)
                std::cout << ",\n";
            first = false;
            std::cout << "    {\"mode\":\"" << name << "\",\"staticUsPerBlock\":" << stationary
                      << ",\"movingIncludingUpdateUsPerBlock\":" << moving
                      << ",\"updateUs\":" << update
                      << ",\"interpolatedUpdateUs\":" << interpolatedUpdate
                      << ",\"staticMsPerSecond\":" << stationary * 375. / 1000.
                      << ",\"latencyFrames\":" << render->latencyFrames() << "}";
        }
        std::cout << "\n  ]\n}\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
