#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cydsp/CyDSP.h>
#include <numbers>
#include <stdexcept>
#include <vector>
namespace {
using namespace cydsp;
void require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
template <class T> constexpr double tolerance = std::same_as<T, float> ? 2e-4 : 1e-10;
template <class T> void complexKernels() {
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::neon,
                         CpuBackend::avx512, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) == Status::unsupported_backend)
            continue;
        for (std::size_t n = 0; n < 34; ++n)
            for (std::size_t offset = 0; offset < 3; ++offset) {
                std::array<std::complex<T>, 38> a{}, b{}, sum{}, even{}, odd{};
                for (std::size_t i = 0; i < n; ++i) {
                    a[offset + i] = {static_cast<T>(i) - 7, T{0.25}};
                    b[offset + i] = {T{0.5}, T{-1}};
                    sum[offset + i] = {T{0.25}, T{-0.5}};
                    even[offset + i] = {T{2}, T{3}};
                    odd[offset + i] = a[offset + i];
                }
                auto av = std::span{a}.subspan(offset, n), bv = std::span{b}.subspan(offset, n),
                     sv = std::span{sum}.subspan(offset, n);
                require(ops.complexMultiplyAccumulate(av, bv, sv) == Status::ok,
                        "complex MAC status");
                for (std::size_t i = 0; i < n; ++i)
                    require(sv[i] == std::complex<T>{T{0.25}, T{-0.5}} + av[i] * bv[i],
                            "complex MAC oracle");
                require(ops.complexMultiplyAccumulate(av, bv, av) == Status::ok, "exact MAC alias");
                for (std::size_t i = 0; i < n; ++i)
                    require(av[i] == odd[offset + i] + odd[offset + i] * bv[i],
                            "complex MAC alias oracle");
                auto ev = std::span{even}.subspan(offset, n),
                     ov = std::span{odd}.subspan(offset, n);
                const auto originals = odd;
                require(ops.complexButterfly(ev, ov, bv, true) == Status::ok,
                        "conjugate butterfly status");
                for (std::size_t i = 0; i < n; ++i) {
                    const auto p = originals[offset + i] * std::conj(bv[i]);
                    require(ev[i] == std::complex<T>{2, 3} + p &&
                                ov[i] == std::complex<T>{2, 3} - p,
                            "butterfly oracle");
                }
            }
        std::array<T, 5> a{1, 2, 3, 4, 5}, b{2, 3, 4, 5, 6}, d{};
        require(ops.multiply(a, b, d) == Status::ok && d[4] == 30, "multiply tail");
    }
}
template <class T> void realFft() {
    for (const auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2,
                               CpuBackend::neon, CpuBackend::avx512, CpuBackend::sve})
        for (std::size_t n : {1u, 2u, 4u, 8u, 16u, 32u, 64u}) {
            RealFft<T> fft;
            const auto status = fft.prepare(n, backend);
            if (status == Status::unsupported_backend)
                continue;
            require(status == Status::ok && fft.backend() == backend, "real FFT prepare/backend");
            std::vector<T> input(n), restored(n);
            std::vector<std::complex<T>> spectrum(n / 2 + 1);
            for (std::size_t i = 0; i < n; ++i)
                input[i] = static_cast<T>(static_cast<int>(i % 5) - 2);
            require(fft.forward(input, spectrum) == Status::ok, "real FFT forward");
            for (std::size_t k = 0; k < spectrum.size(); ++k) {
                std::complex<double> expected{};
                for (std::size_t j = 0; j < n; ++j) {
                    const auto a =
                        -2 * std::numbers::pi * static_cast<double>(j * k) / static_cast<double>(n);
                    expected += static_cast<double>(input[j]) *
                                std::complex<double>{std::cos(a), std::sin(a)};
                }
                require(std::abs(std::complex<double>{spectrum[k]} - expected) < tolerance<T> * n,
                        "real DFT oracle");
            }
            require(fft.inverse(spectrum, restored) == Status::ok, "real FFT inverse");
            for (std::size_t i = 0; i < n; ++i)
                require(std::abs(restored[i] - input[i]) < tolerance<T>, "real FFT round trip");
            spectrum[0].imag(T{1});
            const auto saved = restored;
            require(fft.inverse(spectrum, restored) == Status::invalid_argument &&
                        saved == restored,
                    "invalid DC preserves output");
            auto moved = std::move(fft);
            require(fft.forward(input, spectrum) == Status::not_prepared && moved.size() == n,
                    "real FFT move");
        }
}
template <class T> void convolution() {
    const std::array<T, 5> ir{1, T{0.5}, T{-0.25}, 0, T{0.125}};
    for (std::size_t partition : {1u, 2u, 4u, 8u}) {
        const auto total = 13 + ir.size() + partition + 3;
        std::vector<T> a(total), b(total), oa(total), ob(total);
        for (std::size_t i = 0; i < 13; ++i) {
            a[i] = static_cast<T>(static_cast<int>(i % 5) - 2);
            b[i] = static_cast<T>(i + 1) * T{0.25};
        }
        PartitionedConvolver<T> conv;
        require(conv.prepare({48000, 7, 2}, ir, partition) == Status::ok, "convolver prepare");
        require(conv.latencyFrames() == partition && conv.tailFrames() == partition + ir.size() - 1,
                "convolver latency/tail");
        T* ip[]{a.data(), b.data()};
        T* op[]{oa.data(), ob.data()};
        AudioBlock<T> in{ip, 2, total}, out{op, 2, total};
        std::size_t offset = 0;
        const std::array<std::size_t, 4> pattern{3, 1, 7, 2};
        std::size_t step = 0;
        while (offset < total) {
            auto n = std::min(pattern[step++ % pattern.size()], total - offset);
            require(conv.process(in.subBlock(offset, n), out.subBlock(offset, n)) == Status::ok,
                    "convolver split process");
            offset += n;
        }
        for (std::size_t i = 0; i < total; ++i) {
            double expectedA = 0, expectedB = 0;
            for (std::size_t k = 0; k < ir.size(); ++k)
                if (i >= partition + k) {
                    const auto index = i - partition - k;
                    if (index < a.size()) {
                        expectedA += a[index] * ir[k];
                        expectedB += b[index] * ir[k];
                    }
                }
            require(std::abs(oa[i] - expectedA) < tolerance<T> &&
                        std::abs(ob[i] - expectedB) < tolerance<T>,
                    "direct convolution oracle");
        }
        require(conv.prepare({48000, 7, 2}, {}, partition) == Status::invalid_argument,
                "invalid IR");
        conv.reset();
        auto ia = a, ib = b;
        T* pp[]{ia.data(), ib.data()};
        AudioBlock<T> both{pp, 2, total};
        offset = 0;
        while (offset < total) {
            const auto n = std::min(std::size_t{7}, total - offset);
            require(conv.process(both.subBlock(offset, n), both.subBlock(offset, n)) == Status::ok,
                    "in-place convolution");
            offset += n;
        }
        for (std::size_t i = 0; i < total; ++i)
            require(std::abs(ia[i] - oa[i]) < tolerance<T> &&
                        std::abs(ib[i] - ob[i]) < tolerance<T>,
                    "convolver reset/in-place oracle");
        auto moved = std::move(conv);
        require(conv.process(in.subBlock(0, 0), out.subBlock(0, 0)) == Status::not_prepared,
                "convolver moved-from state");
    }
}
template <class T> void resampling() {
    for (const auto& rates :
         {std::pair{48000u, 44100u}, std::pair{44100u, 48000u}, std::pair{48000u, 16000u},
          std::pair{12000u, 48000u}, std::pair{48000u, 48000u}}) {
        constexpr std::size_t n = 97;
        auto run = [&](bool split, bool constant, CpuBackend backend) {
            ResampleSpec spec{rates.first, rates.second, 2, n, 512, 32, 256};
            Resampler<T> src;
            require(src.prepare(spec, backend) == Status::ok && src.backend() == backend,
                    "resampler prepare/backend");
            std::array<T, n> a{}, b{};
            for (std::size_t i = 0; i < n; ++i) {
                a[i] = constant ? T{1} : static_cast<T>(static_cast<int>(i % 17) - 8) * T{0.125};
                b[i] = T{-0.5} * a[i];
            }
            T* ip[]{a.data(), b.data()};
            AudioBlock<T> input{ip, 2, n};
            std::array<T, 512> oa{}, ob{};
            T* op[]{oa.data(), ob.data()};
            AudioBlock<T> output{op, 2, 512};
            const auto backpressure = src.process(input, output.subBlock(0, 0));
            require(backpressure.status == Status::ok && !backpressure.inputConsumed &&
                        !backpressure.outputProduced,
                    "zero output backpressure");
            std::array<std::vector<T>, 2> collected;
            std::size_t consumed = 0, calls = 0;
            for (; calls < 2000; ++calls) {
                const auto available = std::min(n - consumed, split ? 1 + calls % 7 : n),
                           capacity = split ? 1 + calls % 11 : std::size_t{512};
                const auto result =
                    src.process(input.subBlock(consumed, available), output.subBlock(0, capacity));
                require(result.status == Status::ok && result.inputConsumed <= available &&
                            result.outputProduced <= capacity,
                        "resampler counts/bounds");
                consumed += result.inputConsumed;
                for (std::size_t i = 0; i < result.outputProduced; ++i) {
                    collected[0].push_back(oa[i]);
                    collected[1].push_back(ob[i]);
                }
                if (!result.inputConsumed && !result.outputProduced) {
                    require(consumed == n, "resampler progress");
                    break;
                }
            }
            require(calls < 2000 && consumed == n, "resampler bounded drain");
            const auto expected =
                (n * static_cast<std::uint64_t>(rates.second) + rates.first - 1) / rates.first;
            require(collected[0].size() == expected, "resampler exact rational count");
            for (std::size_t i = 0; i < collected[0].size(); ++i) {
                require(std::abs(collected[1][i] + T{0.5} * collected[0][i]) < tolerance<T>,
                        "resampler channel isolation");
                if (constant &&
                    i * static_cast<std::uint64_t>(rates.first) / rates.second >= spec.taps)
                    require(std::abs(collected[0][i] - T{1}) < tolerance<T>, "resampler steady DC");
            }
            require(src.prepare({0, 44100}, backend) == Status::invalid_spec &&
                        src.backend() == backend,
                    "resampler failed prepare preserves plan");
            auto moved = std::move(src);
            require(src.process(input.subBlock(0, 0), output.subBlock(0, 0)).status ==
                        Status::not_prepared,
                    "resampler moved-from state");
            return collected;
        };
        for (bool constant : {false, true}) {
            const auto reference = run(false, constant, CpuBackend::scalar);
            for (const auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2,
                                       CpuBackend::neon, CpuBackend::avx512, CpuBackend::sve}) {
                VectorOps ops;
                if (VectorOps::create(backend, ops) == Status::unsupported_backend)
                    continue;
                const auto whole = run(false, constant, backend),
                           split = run(true, constant, backend);
                require(whole == split, "resampler block-boundary invariance");
                require(whole[0].size() == reference[0].size(), "resampler backend output count");
                for (std::size_t ch = 0; ch < 2; ++ch)
                    for (std::size_t i = 0; i < whole[ch].size(); ++i)
                        require(std::abs(whole[ch][i] - reference[ch][i]) < tolerance<T>,
                                "resampler scalar/SIMD comparison");
            }
        }
    }
}

template <class T> void delays() {
    for (double delay : {0.0, 1.0, 3.0, 1.5, 3.25}) {
        DelayLine<T> line;
        require(line.prepare({48000, 7, 1}, 8) == Status::ok && line.setDelay(delay) == Status::ok,
                "delay prepare/set");
        std::array<T, 31> input{}, output{};
        for (std::size_t i = 0; i < input.size(); ++i)
            input[i] = static_cast<T>(i + 1);
        T* ip[]{input.data()};
        T* op[]{output.data()};
        AudioBlock<T> in{ip, 1, input.size()}, out{op, 1, output.size()};
        std::size_t offset = 0;
        while (offset < input.size()) {
            const auto count = std::min(std::size_t{7}, input.size() - offset);
            require(line.process(in.subBlock(offset, count), out.subBlock(offset, count)) ==
                        Status::ok,
                    "delay process");
            offset += count;
        }
        const auto integer = static_cast<std::size_t>(delay);
        const auto fraction = delay - static_cast<double>(integer);
        for (std::size_t i = 0; i < input.size(); ++i) {
            const double a = i >= integer ? input[i - integer] : 0,
                         b = i > integer ? input[i - integer - 1] : 0;
            require(std::abs(output[i] - (a * (1 - fraction) + b * fraction)) < tolerance<T>,
                    "fractional delay oracle");
        }
        require(line.setDelay(9) == Status::invalid_argument && line.delayFrames() == delay,
                "invalid delay preserves parameter");
    }
}
template <class T> void firFiltering() {
    const std::array<T, 5> h{T{0.5}, T{-0.25}, T{0.125}, 0, T{0.0625}};
    for (const auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2,
                               CpuBackend::neon, CpuBackend::avx512, CpuBackend::sve}) {
        Fir<T> fir;
        const auto status = fir.prepare({48000, 7, 2}, h, backend);
        if (status == Status::unsupported_backend)
            continue;
        require(status == Status::ok, "FIR prepare");
        std::array<T, 31> a{}, b{}, oa{}, ob{};
        for (std::size_t i = 0; i < a.size(); ++i) {
            a[i] = static_cast<T>(i % 7);
            b[i] = -a[i];
        }
        T* ip[]{a.data(), b.data()};
        T* op[]{oa.data(), ob.data()};
        AudioBlock<T> input{ip, 2, a.size()}, output{op, 2, oa.size()};
        std::size_t offset = 0;
        while (offset < a.size()) {
            const auto n = std::min(std::size_t{7}, a.size() - offset);
            require(fir.process(input.subBlock(offset, n), output.subBlock(offset, n)) ==
                        Status::ok,
                    "FIR process");
            offset += n;
        }
        for (std::size_t i = 0; i < a.size(); ++i) {
            double expected = 0;
            for (std::size_t k = 0; k < h.size(); ++k)
                if (i >= k)
                    expected += a[i - k] * h[k];
            require(std::abs(oa[i] - expected) < tolerance<T> &&
                        std::abs(ob[i] + expected) < tolerance<T>,
                    "FIR direct convolution oracle");
        }
        fir.reset();
        offset = 0;
        while (offset < a.size()) {
            const auto n = std::min(std::size_t{7}, a.size() - offset);
            require(fir.process(input.subBlock(offset, n), input.subBlock(offset, n)) == Status::ok,
                    "FIR in-place");
            offset += n;
        }
        require(a == oa && b == ob, "FIR reset/in-place oracle");
        VectorOps ops;
        require(VectorOps::create(backend, ops) == Status::ok, "dot backend");
        for (std::size_t n = 0; n < a.size(); ++n) {
            T value{}, expected{};
            for (std::size_t i = 0; i < n; ++i)
                expected += a[i] * b[i];
            require(ops.dot(std::span<const T>{a}.first(n), std::span<const T>{b}.first(n),
                            value) == Status::ok &&
                        std::abs(value - expected) < tolerance<T>,
                    "dot size/tail oracle");
        }
        T value{77};
        require(ops.dot(std::span<const T>{a}.first(1), std::span<const T>{b}.first(2), value) ==
                        Status::invalid_argument &&
                    value == 77,
                "dot error preserves result");
        auto moved = std::move(fir);
        require(!fir.taps() && moved.taps() == h.size(), "FIR move");
    }
}
void mixedKernels() {
    for (const auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2,
                               CpuBackend::neon, CpuBackend::avx512, CpuBackend::sve}) {
        VectorOps ops;
        const auto status = VectorOps::create(backend, ops);
        if (status == Status::unsupported_backend)
            continue;
        require(status == Status::ok, "mixed dot backend");
        for (std::size_t count = 0; count < 66; ++count)
            for (std::size_t offset = 0; offset < 4; ++offset) {
                std::array<float, 72> a{};
                std::array<double, 72> b{};
                double expected = 0, absoluteSum = 0;
                for (std::size_t i = 0; i < count; ++i) {
                    a[offset + i] = static_cast<float>(static_cast<int>(i % 13) - 6) * 0.125f;
                    b[offset + i] = 1.0 / static_cast<double>(i + 1);
                    const auto product = static_cast<double>(a[offset + i]) * b[offset + i];
                    expected += product;
                    absoluteSum += std::abs(product);
                }
                double result = 77;
                require(ops.dot(std::span<const float>{a}.subspan(offset, count),
                                std::span<const double>{b}.subspan(offset, count),
                                result) == Status::ok,
                        "mixed dot status");
                require(std::abs(result - expected) <= 1e-13 * (1 + absoluteSum),
                        "mixed double precision oracle");
            }
        const std::array<float, 1> input{1};
        const std::array<double, 2> weights{2, 3};
        double result = 77;
        require(ops.dot(input, weights, result) == Status::invalid_argument && result == 77,
                "mixed dot error preservation");
    }
}
template <class T> void hybridConvolution() {
    for (const auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2,
                               CpuBackend::neon, CpuBackend::avx512, CpuBackend::sve})
        for (std::size_t partition : {1u, 2u, 4u, 8u})
            for (std::size_t length : {1u, 3u, 5u, 9u}) {
                std::vector<T> ir(length);
                for (std::size_t k = 0; k < length; ++k)
                    ir[k] = static_cast<T>(static_cast<int>(k % 5) - 2) * T{0.125};
                HybridConvolver<T> convolver;
                const auto status = convolver.prepare({48000, 7, 2}, ir, partition, backend);
                if (status == Status::unsupported_backend)
                    continue;
                require(status == Status::ok, "hybrid prepare");
                require(convolver.latencyFrames() == 0 && convolver.tailFrames() == length - 1 &&
                            convolver.partitionFrames() == partition,
                        "hybrid latency/tail");
                const auto total = 13 + length + 2;
                std::vector<T> a(total), b(total), outA(total), outB(total);
                for (std::size_t i = 0; i < 13; ++i) {
                    a[i] = static_cast<T>(static_cast<int>(i % 7) - 3);
                    b[i] = static_cast<T>(i + 1) * T{0.25};
                }
                T* ip[]{a.data(), b.data()};
                T* op[]{outA.data(), outB.data()};
                AudioBlock<T> input{ip, 2, total}, output{op, 2, total};
                const std::array<std::size_t, 4> pattern{3, 1, 7, 2};
                std::size_t offset = 0, step = 0;
                while (offset < total) {
                    const auto count = std::min(pattern[step++ % pattern.size()], total - offset);
                    require(convolver.process(input.subBlock(offset, count),
                                              output.subBlock(offset, count)) == Status::ok,
                            "hybrid process");
                    offset += count;
                }
                for (std::size_t i = 0; i < total; ++i) {
                    double expectedA = 0, expectedB = 0;
                    for (std::size_t k = 0; k < length; ++k)
                        if (i >= k) {
                            expectedA += a[i - k] * ir[k];
                            expectedB += b[i - k] * ir[k];
                        }
                    require(std::abs(outA[i] - expectedA) < tolerance<T> &&
                                std::abs(outB[i] - expectedB) < tolerance<T>,
                            "hybrid causal convolution oracle");
                }
                require(convolver.prepare({48000, 7, 2}, {}, partition) == Status::invalid_argument,
                        "hybrid invalid IR");
                convolver.reset();
                offset = 0;
                while (offset < total) {
                    const auto count = std::min(std::size_t{7}, total - offset);
                    require(convolver.process(input.subBlock(offset, count),
                                              input.subBlock(offset, count)) == Status::ok,
                            "hybrid in-place");
                    offset += count;
                }
                for (std::size_t i = 0; i < total; ++i)
                    require(std::abs(a[i] - outA[i]) < tolerance<T> &&
                                std::abs(b[i] - outB[i]) < tolerance<T>,
                            "hybrid reset/split/in-place oracle");
                auto moved = std::move(convolver);
                require(convolver.process(input.subBlock(0, 0), output.subBlock(0, 0)) ==
                                Status::not_prepared &&
                            moved.tailFrames() == length - 1,
                        "hybrid move");
            }
}
template <class T> void delayRamps() {
    auto run = [](bool split) {
        DelayLine<T> line;
        require(line.prepare({48000, 16, 2}, 8) == Status::ok && line.setDelay(4, 8) == Status::ok,
                "delay ramp prepare");
        std::array<std::array<T, 16>, 2> input{}, output{};
        for (std::size_t i = 0; i < 16; ++i) {
            input[0][i] = static_cast<T>(i + 1);
            input[1][i] = -2 * input[0][i];
        }
        T* ip[]{input[0].data(), input[1].data()};
        T* op[]{output[0].data(), output[1].data()};
        AudioBlock<T> in{ip, 2, 16}, out{op, 2, 16};
        require(line.process(in.subBlock(0, 0), out.subBlock(0, 0)) == Status::ok &&
                    line.delayFrames() == 0 && line.targetDelayFrames() == 4,
                "empty block preserves delay ramp");
        require(line.setDelay(9, 3) == Status::invalid_argument && line.targetDelayFrames() == 4,
                "bad ramp preserves target");
        std::size_t offset = 0, step = 0;
        const std::array<std::size_t, 3> pattern{3, 2, 7};
        while (offset < 16) {
            const auto count =
                std::min(split ? pattern[step++ % pattern.size()] : std::size_t{16}, 16 - offset);
            require(line.process(in.subBlock(offset, count), out.subBlock(offset, count)) ==
                        Status::ok,
                    "ramped delay process");
            offset += count;
        }
        for (std::size_t i = 0; i < 16; ++i) {
            const auto expected =
                i < 8 ? 0.5 * static_cast<double>(i + 1) : static_cast<double>(i - 3);
            require(output[0][i] == static_cast<T>(expected) &&
                        output[1][i] == static_cast<T>(-2 * expected),
                    "delay slew/channel oracle");
        }
        require(line.delayFrames() == 4 && line.setDelay(2, 4) == Status::ok,
                "delay ramp completion");
        line.reset();
        require(line.delayFrames() == 2 && line.targetDelayFrames() == 2,
                "delay reset snaps target");
        return output;
    };
    require(run(false) == run(true), "delay ramp block-boundary invariance");
}
void windowsAndEq() {
    std::array<double, 9> w{};
    require(makeWindow(WindowType::hann, w) == Status::ok, "window prepare");
    require(std::abs(w.front()) < 1e-14 && std::abs(w[4] - 1) < 1e-14, "Hann endpoints/center");
    for (std::size_t i = 0; i < w.size(); ++i)
        require(std::abs(w[i] - w[w.size() - 1 - i]) < 1e-14, "symmetric Hann");
    auto response = [](const BiquadCoefficients& c, double angle) {
        const auto z = std::polar(1.0, -angle);
        return std::abs((c.b0 + c.b1 * z + c.b2 * z * z) / (1.0 + c.a1 * z + c.a2 * z * z));
    };
    BiquadCoefficients peak, low, high;
    require(designPeakingBiquad(48000, 1000, 0.707, 6, peak) == Status::ok, "peaking EQ");
    require(designShelfBiquad(false, 48000, 1000, 1, 6, low) == Status::ok &&
                designShelfBiquad(true, 48000, 1000, 1, 6, high) == Status::ok,
            "shelving EQ");
    const auto gain = std::pow(10.0, 6.0 / 20);
    require(std::abs(response(peak, 2 * std::numbers::pi * 1000 / 48000) - gain) < 1e-10,
            "peaking center gain");
    require(std::abs(response(low, 0) - gain) < 1e-10 &&
                std::abs(response(high, std::numbers::pi) - gain) < 1e-10,
            "shelf gains");
    BiquadCascade<double> bypass;
    require(bypass.prepare({48000, 4, 1}, {}) == Status::ok, "empty cascade prepare");
    std::array<double, 4> a{1, 2, 3, 4}, b{};
    double* ip[]{a.data()};
    double* op[]{b.data()};
    require(bypass.process({ip, 1, 4}, {op, 1, 4}) == Status::ok && a == b, "empty cascade bypass");
}
} // namespace
void advancedRegression() {
    complexKernels<float>();
    complexKernels<double>();
    realFft<float>();
    realFft<double>();
    convolution<float>();
    convolution<double>();
    resampling<float>();
    resampling<double>();
    delays<float>();
    delays<double>();
    firFiltering<float>();
    firFiltering<double>();
    mixedKernels();
    hybridConvolution<float>();
    hybridConvolution<double>();
    delayRamps<float>();
    delayRamps<double>();
    windowsAndEq();
}
