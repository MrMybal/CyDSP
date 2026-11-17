// Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
// Optional offline listening harness: actual streaming CyDSP renderers on moving scenes.
#include <algorithm>
#include <array>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace cydsp;
namespace {
void ok(Status s) {
    if (s != Status::ok)
        throw std::runtime_error("CyDSP status " + std::to_string(int(s)));
}
template <class T> std::vector<T> read(std::ifstream& s, std::size_t n) {
    if (n > 32000000)
        throw std::runtime_error("input bound");
    std::vector<T> v(n);
    if (!s.read(reinterpret_cast<char*>(v.data()), std::streamsize(n * sizeof(T))))
        throw std::runtime_error("truncated input");
    return v;
}
template <class T> void write(std::ofstream& s, const T* data, std::size_t count) {
    if (!s.write(reinterpret_cast<const char*>(data), std::streamsize(count * sizeof(T))))
        throw std::runtime_error("output write");
}
void load(const char* path, NeuralHrtfModel& model) {
    std::ifstream s(path, std::ios::binary | std::ios::ate);
    const auto bytes = s.tellg();
    if (bytes <= 0 || bytes > 2000000)
        throw std::runtime_error("model size");
    s.seekg(0);
    ok(model.load(read<std::byte>(s, std::size_t(bytes))));
    if (model.info().sampleRate != 48000)
        throw std::runtime_error("48 kHz model required");
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc != 6)
            throw std::runtime_error("before.cyhrtf after.cyhrtf corpus.bin scenes.bin output.bin");
        NeuralHrtfModel beforeModel, afterModel;
        load(argv[1], beforeModel);
        load(argv[2], afterModel);
        std::ifstream corpus(argv[3], std::ios::binary);
        const auto magic = read<char>(corpus, 8);
        if (std::string(magic.begin(), magic.end()) != "CYHRQA02")
            throw std::runtime_error("corpus magic");
        const auto shape = read<std::uint32_t>(corpus, 3);
        const std::size_t points = shape[0], taps = shape[1];
        if (!points || points > 100000 || !taps || taps > 65536 || points * 2 * taps > 32000000)
            throw std::runtime_error("corpus shape");
        const auto xyz = read<float>(corpus, points * 3);
        const auto impulses = read<float>(corpus, points * 2 * taps);
        const auto delays = read<double>(corpus, points * 2);
        std::vector<HrirPoint<float>> hrir;
        for (std::size_t i = 0; i < points; ++i)
            hrir.push_back({{xyz[3 * i], xyz[3 * i + 1], xyz[3 * i + 2]},
                            {impulses.data() + 2 * i * taps, taps},
                            {impulses.data() + (2 * i + 1) * taps, taps},
                            delays[2 * i],
                            delays[2 * i + 1]});
        HrtfPreparation prep;
        ok(hrtfPreparation(HrtfQuality::realtimeHigh, 48000, prep));
        HrtfSet<float> bank;
        ok(bank.prepare(hrir, prep));
        constexpr std::size_t block = 128, updateFrames = 768, fadeFrames = 256;
        ProcessSpec spec{48000, block, 1};
        HrirBinaural<float> measured;
        ok(measured.prepare(spec, bank));
        NeuralBinaural<float> before, after;
        ok(before.prepare(spec, beforeModel));
        ok(after.prepare(spec, afterModel));
        std::ifstream source(argv[4], std::ios::binary);
        const auto sourceMagic = read<char>(source, 8);
        if (std::string(sourceMagic.begin(), sourceMagic.end()) != "CYHRAUD1")
            throw std::runtime_error("scene magic");
        const auto sizes = read<std::uint32_t>(source, 4);
        const std::size_t frames = sizes[1], scenes = sizes[2], ticks = sizes[3];
        if (sizes[0] != 48000 || !frames || frames > 48000 * 60 || !scenes || scenes > 8 ||
            ticks != (frames + updateFrames - 1) / updateFrames)
            throw std::runtime_error("scene shape");
        const auto poses = read<double>(source, ticks * 3);
        const auto signals = read<float>(source, scenes * frames);
        if (source.peek() != std::char_traits<char>::eof())
            throw std::runtime_error("scene trailing data");
        for (float v : signals)
            if (!std::isfinite(v))
                throw std::runtime_error("nonfinite source");
        const auto latency = measured.latencyFrames();
        const auto padded = ((frames + latency + block - 1) / block) * block;
        std::array<std::vector<float>, 3> rendered;
        for (auto& v : rendered)
            v.resize(2 * padded);
        AudioBuffer<float> mono, stereo;
        ok(mono.resize(1, block));
        ok(stereo.resize(2, block));
        std::ofstream output(argv[5], std::ios::binary);
        const std::array<char, 8> outputMagic{'C', 'Y', 'H', 'R', 'D', 'E', 'M', '1'};
        const std::array<std::uint32_t, 4> outputShape{48000, std::uint32_t(frames),
                                                       std::uint32_t(scenes), 3};
        write(output, outputMagic.data(), outputMagic.size());
        write(output, outputShape.data(), outputShape.size());
        std::vector<float> aligned(2 * frames);
        for (std::size_t scene = 0; scene < scenes; ++scene) {
            measured.reset();
            before.reset();
            after.reset();
            for (std::size_t start = 0; start < padded; start += block) {
                if (start < frames && start % updateFrames == 0) {
                    const auto i = (start / updateFrames) * 3;
                    const Vec3 direction{poses[i], poses[i + 1], poses[i + 2]};
                    const auto fade = start ? fadeFrames : 0;
                    ok(measured.setDirection(direction, fade));
                    ok(before.setDirection(direction, fade));
                    ok(after.setDirection(direction, fade));
                }
                for (std::size_t i = 0; i < block; ++i)
                    mono.block().channel(0)[i] =
                        start + i < frames ? signals[scene * frames + start + i] : 0;
                for (unsigned mode = 0; mode < 3; ++mode) {
                    if (mode == 0)
                        ok(measured.process(mono.block(), stereo.block()));
                    else if (mode == 1)
                        ok(before.process(mono.block(), stereo.block()));
                    else
                        ok(after.process(mono.block(), stereo.block()));
                    for (std::size_t i = 0; i < block; ++i)
                        for (unsigned ear = 0; ear < 2; ++ear) {
                            const float value = stereo.block().channel(ear)[i];
                            if (!std::isfinite(value))
                                throw std::runtime_error("nonfinite renderer output");
                            rendered[mode][2 * (start + i) + ear] = value;
                        }
                }
            }
            for (unsigned mode = 0; mode < 3; ++mode) {
                const auto offset = mode == 0 ? 2 * latency : 0;
                std::copy_n(rendered[mode].begin() + offset, 2 * frames, aligned.begin());
                write(output, aligned.data(), aligned.size());
            }
        }
        if (!output.flush())
            throw std::runtime_error("output flush");
        std::cout << "{\"sampleRate\":48000,\"framesPerScene\":" << frames
                  << ",\"scenes\":" << scenes
                  << ",\"controlIntervalFrames\":768,\"crossfadeFrames\":256"
                  << ",\"measuredInterpolationPoints\":8,\"fractionalDelayTaps\":17,"
                     "\"alignedMeasuredLatencyFrames\":"
                  << latency << ",\"backend\":\"" << backendName(afterModel.info().backend)
                  << "\"}\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
