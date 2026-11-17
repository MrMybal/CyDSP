// Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
// Standalone benchmark evaluator, not a public C++ renderer API.
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cydsp/math/VectorOps.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_MSC_VER)
#define CY_BENCH_NOINLINE __declspec(noinline)
#else
#define CY_BENCH_NOINLINE __attribute__((noinline))
#endif

namespace {
static_assert(std::endian::native == std::endian::little);
static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);
constexpr std::size_t maxWidth = 256, maxFeatures = 39, maxOutput = 195;

struct Reader {
    std::ifstream stream;
    explicit Reader(const std::filesystem::path& path) : stream(path, std::ios::binary) {
        if (!stream)
            throw std::runtime_error("Cannot open benchmark asset");
    }
    void bytes(void* destination, std::size_t size) {
        if (!stream.read(static_cast<char*>(destination), static_cast<std::streamsize>(size)))
            throw std::runtime_error("Truncated asset");
    }
    void magic(const char* expected) {
        std::array<char, 8> actual{};
        bytes(actual.data(), actual.size());
        if (!std::equal(actual.begin(), actual.end(), expected))
            throw std::runtime_error("Wrong benchmark asset magic");
    }
    std::uint32_t integer() {
        std::uint32_t result{};
        bytes(&result, sizeof(result));
        return result;
    }
    std::vector<float> floats(std::size_t count) {
        if (count > 16000000)
            throw std::runtime_error("Asset count limit");
        std::vector<float> result(count);
        bytes(result.data(), result.size() * sizeof(float));
        if (!std::all_of(result.begin(), result.end(), [](float v) { return std::isfinite(v); }))
            throw std::runtime_error("Nonfinite asset");
        return result;
    }
    void end() {
        if (stream.peek() != std::char_traits<char>::eof())
            throw std::runtime_error("Trailing asset bytes");
    }
};

struct Layer {
    std::size_t inputs{}, outputs{};
    std::vector<float> weights, bias;
    void read(Reader& reader, std::size_t in, std::size_t out) {
        inputs = in;
        outputs = out;
        weights = reader.floats(in * out);
        bias = reader.floats(out);
    }
    void apply(std::span<const float> input, std::span<float> output, const cydsp::VectorOps& ops,
               bool activate) const noexcept {
        for (std::size_t row = 0; row < outputs; ++row) {
            float dot{};
            const auto status = ops.dot(input, {weights.data() + row * inputs, inputs}, dot);
            if (status != cydsp::Status::ok)
                std::abort();
            const float value = dot + bias[row];
            output[row] = activate ? std::tanh(value) : value;
        }
    }
};

struct Scratch {
    alignas(64) std::array<float, maxFeatures> encoded{};
    alignas(64) std::array<float, maxWidth> first{}, second{};
    alignas(64) std::array<float, maxOutput> raw{}, prediction{};
};

struct Model {
    std::size_t inputs{}, width{}, outputs{}, filters{}, bands{};
    Layer first, second, last;
    std::vector<float> centers, shifts, phases;
    explicit Model(const std::filesystem::path& path) {
        Reader reader(path);
        reader.magic("CYHRTF01");
        inputs = reader.integer();
        width = reader.integer();
        outputs = reader.integer();
        filters = reader.integer();
        bands = reader.integer();
        if (bands < 1 || bands > 6 || inputs != 3 + 6 * bands || width < 16 || width > maxWidth ||
            filters < 4 || filters > 32 || outputs != 2 * (3 * filters + 1) + 1 ||
            outputs > maxOutput)
            throw std::runtime_error("Unsupported model dimensions");
        first.read(reader, inputs, width);
        second.read(reader, width, width);
        last.read(reader, width, outputs);
        centers = reader.floats(filters);
        shifts = reader.floats(filters);
        phases = reader.floats(bands);
        reader.end();
    }
    void encode(std::span<const float, 3> direction, Scratch& scratch) const noexcept {
        std::copy(direction.begin(), direction.end(), scratch.encoded.begin());
        for (std::size_t axis = 0; axis < 3; ++axis) {
            for (std::size_t band = 0; band < bands; ++band) {
                const auto index = axis * bands + band;
                const float phase = direction[axis] * phases[band];
                scratch.encoded[3 + index] = std::sin(phase);
                scratch.encoded[3 + 3 * bands + index] = std::cos(phase);
            }
        }
    }
    CY_BENCH_NOINLINE void network(std::span<const float> encoded, Scratch& scratch,
                                   const cydsp::VectorOps& ops) const noexcept {
        first.apply(encoded, {scratch.first.data(), width}, ops, true);
        second.apply({scratch.first.data(), width}, {scratch.second.data(), width}, ops, true);
        last.apply({scratch.second.data(), width}, {scratch.raw.data(), outputs}, ops, false);
    }
    CY_BENCH_NOINLINE void predict(std::span<const float, 3> direction, Scratch& scratch,
                                   const cydsp::VectorOps& ops) const noexcept {
        encode(direction, scratch);
        network({scratch.encoded.data(), inputs}, scratch, ops);
        for (std::size_t ear = 0; ear < 2; ++ear) {
            const auto base = ear * (filters * 3 + 1);
            for (std::size_t i = 0; i < filters; ++i) {
                const auto index = base + 3 * i;
                scratch.prediction[index] =
                    centers[i] * std::exp(shifts[i] * std::tanh(scratch.raw[index]));
                const float sigmoid = 1.0f / (1.0f + std::exp(-scratch.raw[index + 1]));
                scratch.prediction[index + 1] = .3f + 11.7f * sigmoid;
                scratch.prediction[index + 2] = 24.0f * std::tanh(scratch.raw[index + 2]);
            }
            const auto level = base + filters * 3;
            scratch.prediction[level] = 30.0f * std::tanh(scratch.raw[level]);
        }
        scratch.prediction[outputs - 1] = 1200.0f * std::tanh(scratch.raw[outputs - 1]);
    }
};

struct Reference {
    std::size_t count{};
    std::vector<float> directions, encoded, raw, prediction;
    Reference(const std::filesystem::path& path, const Model& model) {
        Reader reader(path);
        reader.magic("CYHRREF1");
        count = reader.integer();
        if (!count || count > 100000 || reader.integer() != model.inputs ||
            reader.integer() != model.outputs)
            throw std::runtime_error("Invalid reference dimensions");
        directions = reader.floats(count * 3);
        encoded = reader.floats(count * model.inputs);
        raw = reader.floats(count * model.outputs);
        prediction = reader.floats(count * model.outputs);
        reader.end();
    }
};

struct Errors {
    double features{}, raw{}, center{}, q{}, gain{}, level{}, itd{};
};

Errors validate(const Model& model, const Reference& reference, const cydsp::VectorOps& ops,
                const std::filesystem::path& output) {
    Scratch scratch;
    Errors errors;
    std::ofstream predictions(output, std::ios::binary);
    if (!predictions)
        throw std::runtime_error("Cannot create native prediction export");
    for (std::size_t row = 0; row < reference.count; ++row) {
        const auto difference = [](float actual, float expected) {
            if (!std::isfinite(actual))
                throw std::runtime_error("Nonfinite native prediction");
            return std::abs(static_cast<double>(actual) - expected);
        };
        model.network({reference.encoded.data() + row * model.inputs, model.inputs}, scratch, ops);
        for (std::size_t i = 0; i < model.outputs; ++i)
            errors.raw = std::max(
                errors.raw, difference(scratch.raw[i], reference.raw[row * model.outputs + i]));
        model.predict(std::span<const float, 3>{reference.directions.data() + row * 3, 3}, scratch,
                      ops);
        for (std::size_t i = 0; i < model.inputs; ++i)
            errors.features =
                std::max(errors.features,
                         difference(scratch.encoded[i], reference.encoded[row * model.inputs + i]));
        for (std::size_t i = 0; i < model.outputs; ++i) {
            const double error =
                difference(scratch.prediction[i], reference.prediction[row * model.outputs + i]);
            if (i == model.outputs - 1)
                errors.itd = std::max(errors.itd, error);
            else {
                const auto local = i % (model.filters * 3 + 1);
                double& target = local == model.filters * 3
                                     ? errors.level
                                     : (local % 3 == 0 ? errors.center
                                                       : (local % 3 == 1 ? errors.q : errors.gain));
                target = std::max(target, error);
            }
        }
        predictions.write(reinterpret_cast<const char*>(scratch.prediction.data()),
                          static_cast<std::streamsize>(model.outputs * sizeof(float)));
    }
    if (!predictions || errors.features > 2e-6 || errors.raw > .001 || errors.center > .1 ||
        errors.q > .001 || errors.gain > .005 || errors.level > .005 || errors.itd > .02)
        throw std::runtime_error("Native numerical equivalence failed");
    return errors;
}

volatile double observable{};
void benchmark(const Model& model, const Reference& reference, const cydsp::VectorOps& ops,
               const std::string& backend, bool full, std::size_t batch) {
    constexpr std::size_t warmup = 200, samples = 1500;
    Scratch scratch;
    double checksum{};
    const auto run = [&](std::size_t sample) {
        for (std::size_t j = 0; j < batch; ++j) {
            const auto row = (sample * batch + j) % reference.count;
            if (full)
                model.predict(std::span<const float, 3>{reference.directions.data() + row * 3, 3},
                              scratch, ops);
            else
                model.network({reference.encoded.data() + row * model.inputs, model.inputs},
                              scratch, ops);
            const auto index = (sample + j) % model.outputs;
            checksum += full ? scratch.prediction[index] : scratch.raw[index];
        }
    };
    for (std::size_t i = 0; i < warmup; ++i)
        run(i);
    std::vector<double> times(samples);
    for (std::size_t i = 0; i < samples; ++i) {
        const auto start = std::chrono::steady_clock::now();
        run(i + warmup);
        times[i] =
            std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start)
                .count();
    }
    observable = checksum;
    std::sort(times.begin(), times.end());
    const auto median = (times[samples / 2 - 1] + times[samples / 2]) * .5;
    std::cout << "{\"event\":\"timing\",\"backend\":\"" << backend << "\",\"operation\":\""
              << (full ? "directionToBoundedParameters" : "networkOnly")
              << "\",\"batchDirections\":" << batch << ",\"samples\":" << samples
              << ",\"medianUs\":" << median << ",\"p95Us\":" << times[1424]
              << ",\"minimumUs\":" << times.front() << ",\"maximumUs\":" << times.back()
              << ",\"amortizedMedianUsPerDirection\":" << median / batch << "}\n"
              << std::flush;
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 4)
            throw std::runtime_error("Usage: benchmark weights.bin reference.bin output-directory");
        const Model model(argv[1]);
        const Reference reference(argv[2], model);
        const std::filesystem::path output(argv[3]);
        std::filesystem::create_directories(output);
        cydsp::VectorOps automatic;
        if (cydsp::VectorOps::create(cydsp::CpuBackend::automatic, automatic) != cydsp::Status::ok)
            throw std::runtime_error("Cannot prepare CyDSP vector backend");
        std::cout << std::setprecision(12);
        std::cout << "{\"event\":\"environment\",\"automaticBackend\":\""
                  << cydsp::backendName(automatic.backend())
                  << "\",\"referenceDirections\":" << reference.count
                  << ",\"floatBytes\":" << sizeof(float) << "}\n";
        for (const auto requested :
             {cydsp::CpuBackend::scalar, cydsp::CpuBackend::sse2, cydsp::CpuBackend::avx2,
              cydsp::CpuBackend::avx512, cydsp::CpuBackend::neon, cydsp::CpuBackend::sve}) {
            cydsp::VectorOps ops;
            if (cydsp::VectorOps::create(requested, ops) != cydsp::Status::ok)
                continue;
            const std::string name(cydsp::backendName(ops.backend()));
            const auto errors =
                validate(model, reference, ops, output / (name + "-predictions.bin"));
            std::cout << "{\"event\":\"validation\",\"backend\":\"" << name
                      << "\",\"maximumFeatureError\":" << errors.features
                      << ",\"maximumRawError\":" << errors.raw
                      << ",\"maximumCenterErrorHz\":" << errors.center
                      << ",\"maximumQError\":" << errors.q
                      << ",\"maximumGainErrorDb\":" << errors.gain
                      << ",\"maximumLevelErrorDb\":" << errors.level
                      << ",\"maximumItdErrorUs\":" << errors.itd << "}\n"
                      << std::flush;
            for (const std::size_t batch : {1u, 64u}) {
                benchmark(model, reference, ops, name, false, batch);
                benchmark(model, reference, ops, name, true, batch);
            }
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
