void variableSincRealtimeRegression();
void dawExpansionRegression();
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cydsp/CyDSP.h>
#include <iostream>
#include <limits>
#include <numbers>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>
namespace {
void check(bool value) {
    if (!value)
        throw std::runtime_error("CyDSP regression failed");
}
using cydsp::Status;
template <class T> void vectors() {
    for (auto backend :
         {cydsp::CpuBackend::scalar, cydsp::CpuBackend::sse2, cydsp::CpuBackend::avx2,
          cydsp::CpuBackend::neon, cydsp::CpuBackend::avx512, cydsp::CpuBackend::sve}) {
        cydsp::VectorOps ops;
        const auto status = cydsp::VectorOps::create(backend, ops);
        if (status == Status::unsupported_backend)
            continue;
        check(status == Status::ok && ops.backend() == backend);
        for (std::size_t n = 0; n < 66; ++n)
            for (std::size_t offset = 0; offset < 4; ++offset) {
                std::array<T, 72> a{}, b{};
                for (std::size_t i = 0; i < n; ++i)
                    a[offset + i] = static_cast<T>(i) - T{17};
                const std::span<const T> source{a.data() + offset, n};
                const std::span<T> destination{b.data() + offset, n};
                check(ops.copy(source, destination) == Status::ok);
                for (std::size_t i = 0; i < n; ++i)
                    check(destination[i] == source[i]);
                check(ops.scale(source, destination, T{0.5}) == Status::ok);
                check(ops.addScaled(source, destination, T{-0.25}) == Status::ok);
                for (std::size_t i = 0; i < n; ++i)
                    check(destination[i] == source[i] * T{0.25});
                check(ops.scale(destination, destination, T{2}) == Status::ok);
                for (std::size_t i = 0; i < n; ++i)
                    check(destination[i] == source[i] * T{0.5});
                ops.fill(destination, T{3});
                for (std::size_t i = 0; i < n; ++i)
                    check(destination[i] == T{3});
                if (offset)
                    check(b[offset - 1] == 0);
                check(b[offset + n] == 0);
            }
        std::array<T, 8> shared{1, 2, 3, 4, 5, 6, 7, 8};
        const auto before = shared;
        check(ops.copy(std::span<const T>{shared.data(), 7}, std::span<T>{shared.data() + 1, 7}) ==
              Status::partial_overlap);
        check(shared == before);
        check(ops.scale(shared, shared, std::numeric_limits<T>::quiet_NaN()) ==
              Status::invalid_argument);
        check(shared == before);
    }
}
void buffersAndGain() {
    cydsp::AudioBuffer<float> buffer;
    check(buffer.resize(2, 17) == Status::ok);
    auto audio = buffer.block();
    for (std::size_t ch = 0; ch < 2; ++ch)
        check(reinterpret_cast<std::uintptr_t>(audio.channel(ch).data()) % 64 == 0);
    check(buffer.resize(2, std::numeric_limits<std::size_t>::max()) == Status::invalid_argument);
    check(buffer.block().frames() == 17);
    cydsp::AudioBuffer<float> moved = std::move(buffer);
    check(buffer.block().channels() == 0 && moved.block().frames() == 17);
    cydsp::Gain<float> gain;
    check(gain.prepare({48000, 4, 2}, cydsp::CpuBackend::scalar) == Status::ok);
    check(gain.setGain(0) == Status::ok && gain.setGain(1, 4) == Status::ok);
    std::array<float, 4> a{1, 1, 1, 1}, b = a;
    float* channels[]{a.data(), b.data()};
    cydsp::AudioBlock<float> block{channels, 2, 4};
    check(gain.process(block, block) == Status::ok);
    for (std::size_t i = 0; i < 4; ++i)
        check(a[i] == static_cast<float>(i + 1) / 4 && a[i] == b[i]);
    check(gain.currentGain() == 1);
    // Tiny float ramps must accumulate below float epsilon instead of stalling.
    check(gain.setGain(0.999f) == Status::ok && gain.setGain(1.0f, 48000) == Status::ok);
    const auto initial = gain.currentGain();
    for (int i = 0; i < 8; ++i)
        check(gain.process(block, block) == Status::ok);
    check(gain.currentGain() > initial);
    gain.reset();
    float* crossed[]{b.data(), a.data()};
    const auto saved = a;
    check(gain.process(block, {crossed, 2, 4}) == Status::partial_overlap && a == saved);
    check(gain.process({channels, 2, 0}, {channels, 2, 0}) == Status::ok &&
          gain.currentGain() == 1);
    check(gain.prepare({0, 4, 2}) == Status::invalid_spec && gain.currentGain() == 1);
    check(gain.process(block, block) == Status::ok);
}
void filters() {
    cydsp::BiquadCoefficients low, high;
    check(cydsp::designBiquad(cydsp::BiquadType::lowPass, 48000, 1000, 0.707, low) == Status::ok);
    check(cydsp::designBiquad(cydsp::BiquadType::highPass, 48000, 1000, 0.707, high) == Status::ok);
    check(std::abs((low.b0 + low.b1 + low.b2) / (1 + low.a1 + low.a2) - 1) < 1e-12);
    check(std::abs(high.b0 + high.b1 + high.b2) < 1e-12);
    const auto saved = low;
    check(cydsp::designBiquad(cydsp::BiquadType::lowPass, 48000, 24000, 0.707, low) ==
          Status::invalid_argument);
    check(low.b0 == saved.b0 && low.a1 == saved.a1);
    cydsp::Biquad<double> whole, split;
    for (auto* filter : {&whole, &split}) {
        check(filter->prepare({48000, 16, 1}) == Status::ok);
        check(filter->setCoefficients(low) == Status::ok);
    }
    std::array<double, 16> a{}, b{};
    a[0] = b[0] = 1;
    double* ap[]{a.data()};
    double* bp[]{b.data()};
    check(whole.process({ap, 1, 16}, {ap, 1, 16}) == Status::ok);
    check(split.process({bp, 1, 5}, {bp, 1, 5}) == Status::ok);
    double* tail[]{b.data() + 5};
    check(split.process({tail, 1, 11}, {tail, 1, 11}) == Status::ok);
    check(a == b);
    check(split.setCoefficients({1, 0, 0, 0, 1}) == Status::invalid_argument);
    cydsp::Biquad<float> narrow;
    check(narrow.setCoefficients({std::numeric_limits<double>::max(), 0, 0, 0, 0}) ==
          Status::invalid_argument);
    auto moved = std::move(split);
    check(split.process({bp, 1, 16}, {bp, 1, 16}) == Status::not_prepared);
    moved.reset();
    check(moved.process({bp, 1, 16}, {bp, 1, 16}) == Status::ok);
}
template <class T> void fft() {
    for (std::size_t n : {1u, 2u, 8u, 16u, 32u}) {
        cydsp::ComplexFft<T> plan;
        check(plan.prepare(n) == Status::ok);
        std::vector<std::complex<T>> input(n), output(n), restored(n);
        for (std::size_t i = 0; i < n; ++i)
            input[i] = {static_cast<T>(static_cast<int>(i % 7) - 3), T{0.25}};
        check(plan.process(input, output, cydsp::FftDirection::forward) == Status::ok);
        const double tolerance = std::same_as<T, float> ? 1e-4 : 1e-10;
        // Independent direct DFT oracle, not another radix-2 implementation.
        for (std::size_t k = 0; k < n; ++k) {
            std::complex<double> expected{};
            for (std::size_t j = 0; j < n; ++j) {
                const double angle =
                    -2 * std::numbers::pi * static_cast<double>(k * j) / static_cast<double>(n);
                expected += std::complex<double>(input[j]) *
                            std::complex<double>(std::cos(angle), std::sin(angle));
            }
            check(std::abs(std::complex<double>(output[k]) - expected) <
                  tolerance * static_cast<double>(n));
        }
        check(plan.process(output, restored, cydsp::FftDirection::inverse) == Status::ok);
        for (std::size_t i = 0; i < n; ++i)
            check(std::abs(restored[i] - input[i]) < tolerance);
        auto inplace = input;
        check(plan.process(inplace, inplace, cydsp::FftDirection::forward) == Status::ok);
        for (std::size_t i = 0; i < n; ++i)
            check(std::abs(inplace[i] - output[i]) < tolerance);
        check(plan.prepare(3) == Status::invalid_argument && plan.size() == n);
        auto moved = std::move(plan);
        check(plan.size() == 0 && moved.size() == n &&
              plan.process(input, output, cydsp::FftDirection::forward) == Status::not_prepared);
    }
}
} // namespace
// This source is compiled as an OBJECT by --compile-checks; it is not linked/run.
void advancedRegression();
void filtersRegression();
void routingRegression();
void completionRegression();
void realtimeRegression();
void optimizationRegression();
void schedulingRegression();
void matrixSchedulingRegression();
void expandedRegression();
void stretchFftRegression();
int main() {
 try {
  dawExpansionRegression(); stretchFftRegression(); expandedRegression();
  variableSincRealtimeRegression(); realtimeRegression(); schedulingRegression();
  matrixSchedulingRegression(); optimizationRegression(); completionRegression();
  routingRegression(); filtersRegression(); advancedRegression();
  vectors<float>(); vectors<double>(); buffersAndGain(); filters(); fft<float>(); fft<double>();
  std::cout << "PASS independent CyDSP DSP/SIMD/FFT/convolution/resampling/sampler/realtime\n";
  return 0;
 } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
