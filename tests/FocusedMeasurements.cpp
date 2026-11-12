// Explicit, batched public-API measurements; never run by a production build.
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
using namespace cydsp;
namespace {
volatile double sink = 0;
void ok(Status s) {
    if (s != Status::ok)
        throw std::runtime_error("focused measurement failed");
}
struct Case {
    std::string operation, type;
    CpuBackend backend;
    std::size_t frames, channels, offset, batch;
    std::function<void()> run;
};
std::vector<Case> cases;
template <class T> void vectors(CpuBackend backend) {
    VectorOps ops;
    if (VectorOps::create(backend, ops) != Status::ok)
        return;
    for (std::size_t n : {16u, 31u, 128u, 511u, 4096u}) {
        for (std::size_t offset : {0u, 1u}) {
            struct Data {
                std::vector<T> a, b, d;
                std::vector<std::complex<T>> ca, cb, cd;
            };
            auto data = std::make_shared<Data>();
            data->a.resize(n + offset, T(.125));
            data->b.resize(n + offset, T(.25));
            data->d.resize(n + offset, T(.5));
            data->ca.resize(n + offset, {T(.125), T(-.0625)});
            data->cb.resize(n + offset, {T(.25), T(.125)});
            data->cd.resize(n + offset);
            const auto a = std::span<const T>(data->a).subspan(offset);
            const auto b = std::span<const T>(data->b).subspan(offset);
            const auto d = std::span<T>(data->d).subspan(offset);
            const auto ca = std::span<const std::complex<T>>(data->ca).subspan(offset);
            const auto cb = std::span<const std::complex<T>>(data->cb).subspan(offset);
            const auto cd = std::span<std::complex<T>>(data->cd).subspan(offset);
            for (std::string op :
                 {"copy", "scale", "addScaled", "multiply", "fill", "dot", "complexMac"}) {
                cases.push_back({op, std::same_as<T, float> ? "float" : "double", backend, n, 1,
                                 offset, 256, [=] {
                                     // Retain owned buffers through the capture, outside the timed
                                     // allocations.
                                     (void)data;
                                     if (op == "copy")
                                         ok(ops.copy(a, d));
                                     else if (op == "scale")
                                         ok(ops.scale(a, d, T(.75)));
                                     else if (op == "addScaled")
                                         ok(ops.addScaled(a, d, T(.000001)));
                                     else if (op == "multiply")
                                         ok(ops.multiply(a, b, d));
                                     else if (op == "fill")
                                         ops.fill(d, T(.25));
                                     else if (op == "complexMac") {
                                         ok(ops.complexMultiplyAccumulate(ca, cb, cd));
                                         sink = cd.back().real();
                                         return;
                                     } else {
                                         T value{};
                                         ok(ops.dot(a, b, value));
                                         sink = value;
                                         return;
                                     }
                                     sink = d.back();
                                 }});
            }
        }
    }
}
template <class T> void processors() {
    const std::string type = std::same_as<T, float> ? "float" : "double";
    for (std::size_t frames : {32u, 128u, 512u}) {
        auto input = std::make_shared<AudioBuffer<T>>(),
             output = std::make_shared<AudioBuffer<T>>();
        ok(input->resize(2, frames));
        ok(output->resize(2, frames));
        for (std::size_t ch = 0; ch < 2; ++ch)
            for (std::size_t i = 0; i < frames; ++i)
                input->block().channel(ch)[i] =
                    T((i < frames / 2 ? .6 : .03) * std::cos(double(i) * .19 + double(ch)));
        for (auto mode : {DynamicsMode::compressor, DynamicsMode::expander, DynamicsMode::noiseGate,
                          DynamicsMode::limiter})
            for (auto detector : {DetectorMode::peak, DetectorMode::rms})
                for (double lookahead : {0., 5.}) {
                    auto processor = std::make_shared<Dynamics<T>>();
                    ok(processor->prepare({48000, frames, 2}, mode));
                    DynamicsParameters p;
                    p.attackMs = 2;
                    p.releaseMs = 10;
                    p.holdMs = 1;
                    p.hysteresisDb = 3;
                    p.lookAheadMs = lookahead;
                    p.detector = detector;
                    ok(processor->setParameters(p));
                    const std::array<const char*, 4> names{"compressor", "expander", "gate",
                                                           "limiter"};
                    const auto name = std::string(names[std::size_t(mode)]) +
                                      (detector == DetectorMode::peak ? "Peak" : "Rms") +
                                      (lookahead == 0 ? "0ms" : "5ms");
                    cases.push_back({name, type, CpuBackend::scalar, frames, 2, 0, 16, [=] {
                                         ok(processor->process(input->block(), output->block()));
                                         sink = output->block().channel(0).back();
                                     }});
                }
        auto table = std::make_shared<Wavetable<T>>();
        std::vector<T> cycle(2048);
        for (std::size_t i = 0; i < cycle.size(); ++i)
            cycle[i] = T(2. * double(i) / double(cycle.size()) - 1.);
        ok(table->prepare(cycle, CpuBackend::scalar));
        for (double frequency : {0., 55., 440., 12000.}) {
            auto oscillator = std::make_shared<WavetableOscillator<T>>();
            ok(oscillator->prepare({48000, frames, 2}, *table));
            ok(oscillator->setFrequency(frequency));
            cases.push_back({"wavetable" + std::to_string(int(frequency)), type, CpuBackend::scalar,
                             frames, 2, 0, 32, [=] {
                                 (void)table;
                                 ok(oscillator->process(output->block()));
                                 sink = output->block().channel(0).back();
                             }});
        }
    }
}
} // namespace
int main(int argc, char** argv) {
    try {
        const auto seed = argc > 1 ? unsigned(std::stoul(argv[1])) : 1u;
        for (auto b : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::avx512,
                       CpuBackend::neon, CpuBackend::sve}) {
            vectors<float>(b);
            vectors<double>(b);
        }
        processors<float>();
        processors<double>();
        std::mt19937 random(seed);
        std::shuffle(cases.begin(), cases.end(), random);
        std::cout << std::setprecision(12) << "[\n";
        bool first = true;
        for (const auto& c : cases) {
            for (std::size_t i = 0; i < c.batch * 2; ++i)
                c.run();
            std::array<double, 51> samples{};
            for (auto& t : samples) {
                const auto start = std::chrono::steady_clock::now();
                for (std::size_t i = 0; i < c.batch; ++i)
                    c.run();
                t = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() -
                                                             start)
                        .count() /
                    double(c.batch);
            }
            if (!std::isfinite(sink))
                throw std::runtime_error("nonfinite benchmark output");
            std::sort(samples.begin(), samples.end());
            if (!first)
                std::cout << ",\n";
            first = false;
            std::cout << "{\"operation\":\"" << c.operation << "\",\"sampleType\":\"" << c.type
                      << "\",\"backend\":\"" << backendName(c.backend)
                      << "\",\"frames\":" << c.frames << ",\"channels\":" << c.channels
                      << ",\"offset\":" << c.offset << ",\"batchSize\":" << c.batch
                      << ",\"samples\":" << samples.size() << ",\"seed\":" << seed
                      << ",\"medianNs\":" << samples[25] << ",\"p95BatchMeanNs\":" << samples[48]
                      << "}";
        }
        std::cout << "\n]\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
