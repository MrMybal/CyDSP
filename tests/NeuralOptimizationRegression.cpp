// Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>
using namespace cydsp;
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
void check(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
void ok(Status s) {
    check(s == Status::ok, "neural optimization status");
}
void integer(std::vector<std::byte>& x, std::uint32_t v) {
    for (unsigned i = 0; i < 4; ++i)
        x.push_back(std::byte((v >> (8 * i)) & 255));
}
void number(std::vector<std::byte>& x, float v) {
    integer(x, std::bit_cast<std::uint32_t>(v));
}
std::vector<std::byte> modelBytes() {
    std::vector<std::byte> x;
    for (char c : std::string_view("CYHRTF02"))
        x.push_back(std::byte(c));
    for (auto n : {9u, 16u, 195u, 32u, 1u, 48000u})
        integer(x, n);
    number(x, 0);
    for (unsigned i = 0; i < 9 * 16 + 16 + 16 * 16 + 16 + 16 * 195 + 195; ++i)
        number(x, 0);
    for (unsigned i = 0; i < 32; ++i)
        number(x, float(100 * std::pow(180., double(i) / 31)));
    for (unsigned i = 0; i < 32; ++i)
        number(x, .01f);
    number(x, float(std::acos(-1)));
    return x;
}
NeuralHrtfDesign design(std::size_t sections, unsigned variant = 0) {
    NeuralHrtfDesign d;
    d.sampleRate = 48000;
    d.sectionCount = sections;
    d.gain = {.63 + .01 * variant, .81 - .02 * variant};
    d.delayFrames = {3.25 + .11 * variant, 5.75 - .13 * variant};
    for (std::size_t e = 0; e < 2; ++e)
        for (std::size_t j = 0; j < sections; ++j) {
            const double hz = 100 * std::pow(160., double(j) / std::max(1., double(sections - 1)));
            const double gain = (int((j + e + variant) % 5) - 2) * 1.5;
            ok(designPeakingBiquad(48000, hz, .7 + .07 * double((j + variant) % 9), gain,
                                   d.sections[e][j]));
        }
    return d;
}
template <class T> void near(T actual, double expected, const char* why) {
    const double tolerance = (std::same_as<T, float> ? 3e-7 : 2e-12) * (1 + std::abs(expected));
    check(std::abs(double(actual) - expected) <= tolerance, why);
}
// Independent public Biquad processors plus the explicit integer/all-pass delay oracle.
template <class T> void staticOracle(const NeuralHrtfModel& model) {
    constexpr std::size_t frames = 257;
    const auto d = design(32);
    NeuralBinaural<T> renderer;
    NeuralBinauralOptions options;
    options.transitionHistoryFrames = 0;
    ok(renderer.prepare({48000, frames, 1}, model, options));
    ok(renderer.setDesign(d, 0));
    AudioBuffer<T> input, output;
    ok(input.resize(1, frames));
    ok(output.resize(2, frames));
    AudioBuffer<double> reference;
    ok(reference.resize(2, frames));
    std::array<std::vector<Biquad<double>>, 2> filter;
    std::array<ThiranDelay<double>, 2> fractional;
    std::array<std::array<double, 16>, 2> delay{};
    std::size_t write = 0, position = 0;
    for (std::size_t e = 0; e < 2; ++e) {
        filter[e].resize(32);
        for (std::size_t j = 0; j < 32; ++j) {
            ok(filter[e][j].prepare({48000, frames, 1}));
            ok(filter[e][j].setCoefficients(d.sections[e][j]));
        }
        ok(fractional[e].setDelay(d.delayFrames[e] - std::floor(d.delayFrames[e])));
    }
    for (auto n : {1u, 7u, 31u, 128u, 257u, 13u, 257u}) {
        for (std::size_t i = 0; i < n; ++i) {
            const T x = T(.13 * std::sin(.117 * double(position + i)) +
                          .03 * std::cos(.017 * double(position + i)));
            input.block().channel(0)[i] = x;
            for (std::size_t e = 0; e < 2; ++e)
                reference.block().channel(e)[i] = double(x) * d.gain[e];
        }
        for (std::size_t e = 0; e < 2; ++e) {
            double* p[]{reference.block().channel(e).data()};
            for (auto& f : filter[e])
                ok(f.process({p, 1, n}, {p, 1, n}));
        }
        ok(renderer.process(input.block(n), output.block(n)));
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t e = 0; e < 2; ++e) {
                delay[e][write] = reference.block().channel(e)[i];
                const auto index = (write + 16 - std::size_t(d.delayFrames[e])) % 16;
                near(output.block().channel(e)[i], fractional[e].processSample(delay[e][index]),
                     "fused cascade vs independent biquad/delay oracle");
            }
            write = (write + 1) % 16;
        }
        position += n;
    }
}
template <class T> void warmOracle(const NeuralHrtfModel& model, std::size_t history) {
    constexpr std::size_t prefix = 701;
    NeuralBinaural<T> renderer, oldReference, newReference;
    NeuralBinauralOptions config;
    config.transitionHistoryFrames = history;
    NeuralBinauralOptions cold;
    cold.transitionHistoryFrames = 0;
    for (auto* r : {&renderer, &oldReference, &newReference})
        ok(r->prepare({48000, 64, 1}, model, r == &renderer ? config : cold));
    const auto a = design(7), b = design(11, 1);
    ok(renderer.setDesign(a, 0));
    ok(oldReference.setDesign(a, 0));
    ok(newReference.setDesign(b, 0));
    AudioBuffer<T> input, output, oldOut, newOut;
    ok(input.resize(1, 64));
    ok(output.resize(2, 64));
    ok(oldOut.resize(2, 64));
    ok(newOut.resize(2, 64));
    std::array<T, prefix> past;
    for (std::size_t i = 0; i < prefix; ++i)
        past[i] = T(.17 * std::sin(.071 * i) + .04 * std::cos(.029 * i));
    for (std::size_t first = 0; first < prefix;) {
        const auto n = std::min(std::size_t{64}, prefix - first);
        std::copy_n(past.data() + first, n, input.block().channel(0).data());
        ok(renderer.process(input.block(n), output.block(n)));
        ok(oldReference.process(input.block(n), oldOut.block(n)));
        first += n;
    }
    const auto retained = std::min(history, prefix);
    for (std::size_t first = prefix - retained; first < prefix;) {
        const auto n = std::min(std::size_t{64}, prefix - first);
        std::copy_n(past.data() + first, n, input.block().channel(0).data());
        ok(newReference.process(input.block(n), newOut.block(n)));
        first += n;
    }
    ok(renderer.setDesign(b, 79));
    std::size_t rendered = 0;
    for (auto n : {1u, 7u, 31u, 64u, 25u}) {
        for (std::size_t i = 0; i < n; ++i)
            input.block().channel(0)[i] = T(.14 * std::cos(.081 * (prefix + rendered + i)));
        ok(renderer.process(input.block(n), output.block(n)));
        ok(oldReference.process(input.block(n), oldOut.block(n)));
        ok(newReference.process(input.block(n), newOut.block(n)));
        for (std::size_t i = 0; i < n; ++i)
            for (std::size_t e = 0; e < 2; ++e) {
                const auto blend = std::min(1., double(rendered + i + 1) / 79);
                const double from = oldOut.block().channel(e)[i], to = newOut.block().channel(e)[i];
                near(output.block().channel(e)[i], from + (to - from) * blend,
                     "new design replays the retained input before crossfade");
            }
        rendered += n;
    }
    check(renderer.transitionRemainingFrames() == 0, "warm fade completes at exact frame");
    renderer.reset();
    newReference.reset();
    ok(renderer.setDesign(b, 0));
    for (std::size_t i = 0; i < 64; ++i)
        input.block().channel(0)[i] = T(i == 0);
    ok(renderer.process(input.block(), output.block()));
    ok(newReference.process(input.block(), newOut.block()));
    for (std::size_t e = 0; e < 2; ++e)
        for (std::size_t i = 0; i < 64; ++i)
            near(output.block().channel(e)[i], double(newOut.block().channel(e)[i]),
                 "reset clears replay and filter histories");
}
template <class T>
void batchOracle(const NeuralHrtfModel& model, std::size_t count, std::size_t history) {
    std::vector<NeuralBinaural<T>> batch(count), individual(count);
    std::vector<NeuralHrtfDesign> designs(count);
    NeuralBinauralOptions options;
    options.transitionHistoryFrames = history;
    for (std::size_t v = 0; v < count; ++v) {
        ok(batch[v].prepare({48000, 257, 1}, model, options));
        ok(individual[v].prepare({48000, 257, 1}, model, options));
        designs[v] = design(1 + (v * 7) % 32, unsigned(v % 3));
        ok(individual[v].setDesign(designs[v], 0));
    }
    ok(NeuralBinaural<T>::setDesigns(batch, designs, 0));
    AudioBuffer<T> input, output, expected;
    ok(input.resize(count, 263));
    ok(output.resize(2 * count, 267));
    ok(expected.resize(2 * count, 267));
    std::size_t absolute = 0, cycle = 0;
    for (auto frames : {31u, 7u, 257u, 1u, 128u, 64u, 9u, 13u, 256u}) {
        if (cycle == 2 || cycle == 5) {
            for (std::size_t v = 0; v < count; ++v) {
                designs[v] = design(1 + (v * 5 + cycle) % 32, unsigned((v + cycle) % 3));
                ok(individual[v].setDesign(designs[v], 79));
            }
            ok(NeuralBinaural<T>::setDesigns(batch, designs, 79));
        }
        if (cycle == 3) {
            batch.back().reset();
            individual.back().reset();
        }
        if (cycle == 4)
            for (std::size_t v = 0; v < count; v += 2) {
                designs[v] = design(2 + (v * 3) % 30, 2);
                ok(batch[v].setDesign(designs[v], 37 + v));
                ok(individual[v].setDesign(designs[v], 37 + v));
            }
        for (std::size_t v = 0; v < count; ++v)
            for (std::size_t i = 0; i < frames; ++i)
                input.block().channel(v)[i + 3] =
                    T(.13 * std::sin(.101 * (absolute + i) + .31 * v));
        const auto in = input.block().subBlock(3, frames), out = output.block().subBlock(5, frames);
        ok(NeuralBinaural<T>::processBatch(batch, in, out));
        for (std::size_t v = 0; v < count; ++v) {
            const T* a[]{in.channel(v).data()};
            T* b[]{expected.block().channel(2 * v).data() + 5,
                   expected.block().channel(2 * v + 1).data() + 5};
            ok(individual[v].process({a, 1, frames}, {b, 2, frames}));
            check(batch[v].transitionRemainingFrames() == individual[v].transitionRemainingFrames(),
                  "asynchronous batch fade clocks");
            for (std::size_t e = 0; e < 2; ++e)
                for (std::size_t i = 0; i < frames; ++i)
                    near(out.channel(2 * v + e)[i], double(b[e][i]),
                         "SIMD voice group matches independent renderers, offsets and remainder");
        }
        absolute += frames;
        ++cycle;
    }
    // A late invalid design/voice/alias must not change any earlier voice or output.
    auto bad = designs;
    bad.back().sections[0][0].a2 = 1;
    check(NeuralBinaural<T>::setDesigns(batch, bad, 79) == Status::invalid_argument,
          "batch design whole-call preflight");
    for (auto& v : batch)
        check(v.transitionRemainingFrames() == 0, "failed late design changes no fade");
    std::vector<T*> aliases(2 * count);
    for (std::size_t c = 0; c < 2 * count; ++c)
        aliases[c] = output.block().channel(c).data();
    aliases.back() = input.block().channel(0).data();
    check(NeuralBinaural<T>::processBatch(batch, input.block().subBlock(3, 31),
                                          {aliases.data(), 2 * count, 31}) ==
              Status::partial_overlap,
          "late batch alias rejected");
    auto moved = std::move(batch.back());
    check(NeuralBinaural<T>::processBatch(batch, input.block().subBlock(3, 31),
                                          output.block().subBlock(5, 31)) == Status::not_prepared,
          "late unprepared batch voice rejected");
    batch.back() = std::move(moved);
    ok(NeuralBinaural<T>::processBatch(batch, input.block().subBlock(3, 31),
                                       output.block().subBlock(5, 31)));
    for (std::size_t v = 0; v < count; ++v) {
        const T* a[]{input.block().channel(v).data() + 3};
        T* b[]{expected.block().channel(2 * v).data() + 5,
               expected.block().channel(2 * v + 1).data() + 5};
        ok(individual[v].process({a, 1, 31}, {b, 2, 31}));
        for (std::size_t e = 0; e < 2; ++e)
            for (std::size_t i = 0; i < 31; ++i)
                near(output.block().channel(2 * v + e)[i + 5], double(b[e][i]),
                     "failed batch preflight preserves every state");
    }
    ok(batch.back().setDesign(designs.back(), 79));
    check(NeuralBinaural<T>::setDesigns(batch, designs, 79) == Status::transition_in_progress,
          "late busy voice rejects the whole design batch");
    for (std::size_t v = 0; v + 1 < count; ++v)
        check(batch[v].transitionRemainingFrames() == 0, "busy preflight starts no other fade");
    struct Context {
        std::vector<NeuralBinaural<T>>* voices;
        std::vector<NeuralHrtfDesign>* designs;
        AudioBuffer<T>* in;
        AudioBuffer<T>* out;
        bool pass = true;
    } context{&batch, &designs, &input, &output};
    runRealtimeAudit(
        [](void* raw) noexcept {
            auto& c = *static_cast<Context*>(raw);
            for (auto& v : *c.voices)
                v.reset();
            c.pass &= NeuralBinaural<T>::setDesigns(*c.voices, *c.designs, 0) == Status::ok;
            for (unsigned i = 0; i < 4; ++i)
                c.pass &=
                    NeuralBinaural<T>::processBatch(*c.voices, c.in->block().subBlock(3, 128),
                                                    c.out->block().subBlock(5, 128)) == Status::ok;
            c.pass &= NeuralBinaural<T>::setDesigns(*c.voices, *c.designs, 79) == Status::ok;
            c.pass &=
                NeuralBinaural<T>::processBatch(*c.voices, c.in->block().subBlock(3, 128),
                                                c.out->block().subBlock(5, 128)) == Status::ok;
        },
        &context);
    check(context.pass, "batch replay/process/reset allocate and free no memory");
}
} // namespace
void neuralOptimizationRegression() {
    const auto bytes = modelBytes();
    unsigned backends = 0;
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::avx512,
                         CpuBackend::neon, CpuBackend::sve}) {
        NeuralHrtfModel model;
        const auto status = model.load(bytes, backend);
        if (status == Status::unsupported_backend)
            continue;
        ok(status);
        ++backends;
        staticOracle<float>(model);
        staticOracle<double>(model);
        for (auto history : {0u, 127u, 512u, 2048u}) {
            warmOracle<float>(model, history);
            warmOracle<double>(model, history);
            for (auto n : {1u, 3u, 4u, 5u, 9u}) {
                batchOracle<float>(model, n, history);
                batchOracle<double>(model, n, history);
            }
        }
        NeuralBinaural<float> limit;
        NeuralBinauralOptions options;
        options.transitionHistoryFrames = 65537;
        check(limit.prepare({48000, 128, 1}, model, options) == Status::invalid_argument,
              "bounded replay capacity");
    }
    check(backends > 0, "neural optimization executed a backend");
    std::cout << "QUALITY neural cascade/oracles/replay/voice groups float+double backends="
              << backends << "\n";
}
