#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cydsp/CyDSP.h>
#include <numbers>
#include <stdexcept>
#include <vector>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool v, const char* text) {
    if (!v)
        throw std::runtime_error(text);
}
void ok(Status s) {
    require(s == Status::ok, "advanced foundation status");
}
std::complex<double> transfer(std::span<const BiquadCoefficients> sections,
                              std::complex<double> z) {
    std::complex<double> value = 1;
    for (const auto& c : sections)
        value *= (c.b0 + c.b1 * z + c.b2 * z * z) / (1. + c.a1 * z + c.a2 * z * z);
    return value;
}
void bandDesign() {
    for (auto family : {IirFamily::butterworth, IirFamily::chebyshevI, IirFamily::chebyshevII,
                        IirFamily::elliptic, IirFamily::bessel})
        for (unsigned order : {1u, 3u, 4u, 8u, 16u})
            for (bool stop : {false, true}) {
                IirBandDesign d;
                d.prototype.family = family;
                d.prototype.order = order;
                d.lowHz = 1000;
                d.highHz = 8000;
                d.bandStop = stop;
                std::array<BiquadCoefficients, 16> sections{};
                std::size_t count = 0;
                ok(designBandIir(d, sections, count));
                require(count == order, "band filter doubled order");
                auto p = d.prototype;
                p.response = BiquadType::lowPass;
                p.cutoff = 12000;
                std::array<BiquadCoefficients, 8> prototype{};
                std::size_t lowCount = 0;
                ok(designIir(p, prototype, lowCount));
                const double w1 = std::tan(std::numbers::pi * 1000 / 48000),
                             w2 = std::tan(std::numbers::pi * 8000 / 48000);
                for (double frequency : {100., 700., 1300., 3300., 7000., 10000., 17000., 22000.}) {
                    const std::complex<double> s(0, std::tan(std::numbers::pi * frequency / 48000));
                    const auto mapped = stop ? (w2 - w1) * s / (s * s + w1 * w2)
                                             : (s * s + w1 * w2) / ((w2 - w1) * s);
                    const auto lowZ = (1. - mapped) / (1. + mapped);
                    const auto expected =
                        std::abs(transfer(std::span(prototype).first(lowCount), lowZ));
                    const auto measured = std::abs(
                        transfer(std::span(sections).first(count),
                                 std::polar(1., -2 * std::numbers::pi * frequency / 48000)));
                    require(std::abs(measured - expected) < 2e-6,
                            "IIR analog substitution magnitude oracle");
                }
            }
    IirBandDesign d;
    d.lowHz = 0;
    std::array<BiquadCoefficients, 16> sections;
    std::size_t count = 117;
    require(designBandIir(d, sections, count) == Status::invalid_argument && count == 117,
            "band design output transaction");
}
void firDesign() {
    std::array<double, 2> source{.2, 1}, minimum{};
    ok(minimumPhaseFir(source, minimum));
    require(std::abs(minimum[0] - 1) < 1e-8 && std::abs(minimum[1] - .2) < 1e-8,
            "minimum phase known root reflection oracle");
    for (double frequency : {0., .12, .27, .49}) {
        std::complex<double> a{}, b{};
        for (std::size_t i = 0; i < 2; ++i) {
            const auto angle = std::polar(1., -2 * std::numbers::pi * frequency * double(i));
            a += source[i] * angle;
            b += minimum[i] * angle;
        }
        require(std::abs(std::abs(a) - std::abs(b)) < 1e-8, "minimum phase magnitude preservation");
    }
    std::vector<double> taps(129);
    const std::array<FirEqPoint, 2> points{{{0, 6}, {24000, 6}}};
    ok(designFirEqualizer(48000, points, taps));
    const auto gain = std::pow(10., 6. / 20);
    for (std::size_t i = 0; i < taps.size(); ++i)
        require(std::abs(taps[i] - (i == 64 ? gain : 0)) < 1e-12,
                "linear phase EQ flat response impulse");
    ok(designFirEqualizer(48000, points, taps, FirPhase::minimum));
    require(std::abs(taps[0] - gain) < 1e-10, "minimum phase EQ flat impulse");
}
template <class T> void bank(CpuBackend backend) {
    FirFilterBank<T> bank;
    const std::array<double, 2> crossovers{1000, 6000};
    ok(bank.prepare({48000, 128, 2}, crossovers, 65, backend));
    AudioBuffer<T> input, bands, sum;
    ok(input.resize(2, 1024));
    ok(bands.resize(6, 128));
    ok(sum.resize(2, 128));
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t i = 0; i < 1024; ++i)
            input.block().channel(ch)[i] = T(.4 * std::sin(.13 * double(i) + double(ch)));
    for (std::size_t at = 0; at < 1024; at += 128) {
        ok(bank.process(input.block().subBlock(at, 128), bands.block()));
        ok(bank.sumBands(bands.block(), sum.block()));
        for (std::size_t ch = 0; ch < 2; ++ch)
            for (std::size_t i = 0; i < 128; ++i)
                require(std::abs(double(sum.block().channel(ch)[i]) -
                                 (at + i >= 32 ? double(input.block().channel(ch)[at + i - 32])
                                               : 0)) < 2e-6,
                        "N-band exact delayed reconstruction");
    }
    require(bank.latencyFrames() == 32 && bank.bands() == 3, "filterbank timing");
    struct C {
        FirFilterBank<T>* p;
        AudioBlock<const T> in;
        AudioBlock<T> out;
        bool passed = true;
    } c{&bank, input.block(128), bands.block()};
    runRealtimeAudit(
        [](void* raw) noexcept {
            auto& c = *static_cast<C*>(raw);
            c.passed = c.p->process(c.in, c.out) == Status::ok;
            c.p->reset();
        },
        &c);
    require(c.passed, "filterbank realtime");
}
template <class T> void hilbert(CpuBackend backend) {
    HilbertTransform<T> transform;
    ok(transform.prepare({48000, 128, 1}, 129, backend));
    AudioBuffer<T> input, real, imag;
    ok(input.resize(1, 512));
    ok(real.resize(1, 128));
    ok(imag.resize(1, 128));
    for (std::size_t i = 0; i < 512; ++i)
        input.block().channel(0)[i] = T(.25 * std::cos(2 * std::numbers::pi * .125 * double(i)));
    for (std::size_t at = 0; at < 512; at += 128) {
        ok(transform.process(input.block().subBlock(at, 128), real.block(), imag.block()));
        if (at >= 128)
            for (std::size_t i = 0; i < 128; ++i) {
                const double phase = 2 * std::numbers::pi * .125 * double(at + i - 64);
                require(std::abs(double(real.block().channel(0)[i]) - .25 * std::cos(phase)) < 1e-7,
                        "Hilbert real delay");
                require(std::abs(double(imag.block().channel(0)[i]) - .25 * std::sin(phase)) < 2e-5,
                        "Hilbert analytic sinusoid");
            }
    }
    struct C {
        HilbertTransform<T>* p;
        AudioBlock<const T> in;
        AudioBlock<T> r, i;
        bool passed = true;
    } c{&transform, input.block(128), real.block(), imag.block()};
    runRealtimeAudit(
        [](void* raw) noexcept {
            auto& c = *static_cast<C*>(raw);
            c.passed = c.p->process(c.in, c.r, c.i) == Status::ok;
            c.p->reset();
        },
        &c);
    require(c.passed, "Hilbert realtime");
}
template <class T> void diffusion() {
    for (auto kind : {OrthogonalMix::hadamard, OrthogonalMix::householder}) {
        std::array<T, 8> input{T(1), T(-.5), T(.1)}, output{};
        ok(orthogonalMix<T>(input, output, kind));
        double a = 0, b = 0;
        for (std::size_t i = 0; i < 8; ++i) {
            a += double(input[i]) * input[i];
            b += double(output[i]) * output[i];
        }
        require(std::abs(a - b) < 1e-6, "orthogonal matrix energy");
    }
    Diffuser<T> diffuser;
    const std::array<DiffusionStage, 2> stages{{{7, .6}, {11, -.4}}};
    ok(diffuser.prepare({48000, 128, 1}, stages));
    AudioBuffer<T> input, output;
    ok(input.resize(1, 128));
    ok(output.resize(1, 128));
    double energy = 0;
    for (int block = 0; block < 64; ++block) {
        std::fill(input.block().channel(0).begin(), input.block().channel(0).end(), T{});
        if (!block)
            input.block().channel(0)[0] = 1;
        ok(diffuser.process(input.block(), output.block()));
        for (T value : output.block().channel(0))
            energy += double(value) * value;
    }
    require(std::abs(energy - 1) < 1e-6, "allpass diffuser impulse energy");
}
} // namespace
void advancedFoundationRegression() {
    bandDesign();
    firDesign();
    diffusion<float>();
    diffusion<double>();
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::avx512,
                         CpuBackend::neon, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        bank<float>(backend);
        bank<double>(backend);
        hilbert<float>(backend);
        hilbert<double>(backend);
    }
}
