// Opt-in component measurements for real integration regressions; no production startup work.
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
using namespace cydsp;
namespace {
volatile double sink = 0;
void ok(Status s) {
    if (s != Status::ok)
        throw std::runtime_error("hotspot status " + std::to_string(int(s)));
}
struct Case {
    std::string name;
    std::function<void()> run, reset;
    std::function<void(std::ofstream&)> dump;
};
std::vector<Case> cases;
template <class T> struct Data {
    AudioBuffer<T> in, out;
    Data(std::size_t ch) {
        ok(in.resize(ch, 256));
        ok(out.resize(ch, 256));
        for (std::size_t c = 0; c < ch; ++c)
            for (std::size_t i = 0; i < 256; ++i)
                in.block().channel(c)[i] = T(.42 * std::sin(.17 * double(i) + .9 * double(c)) +
                                             .08 * std::cos(1.13 * double(i)));
    }
};
template <class T, class P>
void add(std::string name, std::shared_ptr<P> p, std::shared_ptr<Data<T>> d) {
    name += std::same_as<T, float> ? ".f32" : ".f64";
    cases.push_back({name,
                     [p, d] {
                         ok(p->process(d->in.block(), d->out.block()));
                         sink = d->out.block().channel(0).back();
                     },
                     [p] { p->reset(); },
                     [d](std::ofstream& f) {
                         for (std::size_t c = 0; c < d->out.block().channels(); ++c)
                             for (T x : d->out.block().channel(c)) {
                                 double v = x;
                                 f.write(reinterpret_cast<const char*>(&v), sizeof(v));
                             }
                     }});
}
template <class T> void make() {
    for (std::size_t channels : {1u, 2u, 8u}) {
        auto d = std::make_shared<Data<T>>(channels);
        const ProcessSpec spec{48000, 256, channels};
        for (bool linked : {false, true})
            for (auto detector : {DetectorMode::peak, DetectorMode::rms}) {
                auto p = std::make_shared<Dynamics<T>>();
                ok(p->prepare(spec, DynamicsMode::compressor, 0));
                DynamicsParameters settings;
                settings.thresholdDb = -18;
                settings.ratio = 4;
                settings.kneeDb = 6;
                settings.linkedChannels = linked;
                settings.detector = detector;
                ok(p->setParameters(settings));
                add("compressor." + std::to_string(channels) +
                        (linked ? ".linked" : ".independent") +
                        (detector == DetectorMode::peak ? ".peak" : ".rms"),
                    p, d);
            }
        if (channels != 2)
            continue;
        for (double ratio : {2., 4., 6., 12.})
            for (int level : {-40, -24, -8}) {
                auto d = std::make_shared<Data<T>>(2);
                const auto amplitude = std::pow(10., level / 20.);
                for (std::size_t c = 0; c < 2; ++c)
                    std::fill(d->in.block().channel(c).begin(), d->in.block().channel(c).end(),
                              T(amplitude * (c ? .7 : 1.)));
                auto p = std::make_shared<Dynamics<T>>();
                ok(p->prepare(spec, DynamicsMode::compressor, 0));
                DynamicsParameters settings;
                settings.thresholdDb = -24;
                settings.ratio = ratio;
                settings.kneeDb = 6;
                settings.attackMs = 5;
                settings.releaseMs = 100;
                ok(p->setParameters(settings));
                add("compressor.r" + std::to_string(int(ratio)) + ".level" + std::to_string(level),
                    p, d);
            }
        for (auto shape : {AdaaShape::tanh, AdaaShape::atan, AdaaShape::cubic, AdaaShape::hardClip,
                           AdaaShape::wavefolder})
            for (bool aa : {false, true}) {
                auto p = std::make_shared<AdaaSaturator<T>>();
                ok(p->prepare(spec));
                SaturationParameters settings;
                settings.shape = shape;
                settings.antialias = aa;
                settings.drive = 3;
                ok(p->setParameters(settings));
                add("saturator." + std::to_string(int(shape)) + (aa ? ".adaa" : ".direct"), p, d);
            }
        for (auto shape : {AdaaShape::tanh, AdaaShape::atan, AdaaShape::cubic, AdaaShape::hardClip,
                           AdaaShape::wavefolder})
            for (bool silence : {false, true}) {
                auto d = std::make_shared<Data<T>>(2);
                for (std::size_t c = 0; c < 2; ++c)
                    for (std::size_t i = 0; i < 256; ++i)
                        d->in.block().channel(c)[i] =
                            silence ? T{} : T(.42 * std::sin(.0033 * double(i) + .2 * double(c)));
                auto p = std::make_shared<AdaaSaturator<T>>();
                ok(p->prepare(spec));
                SaturationParameters settings;
                settings.shape = shape;
                settings.drive = 3;
                ok(p->setParameters(settings));
                add("saturator." + std::to_string(int(shape)) + (silence ? ".silence" : ".slow"), p,
                    d);
            }
        for (std::size_t bands : {4u, 8u}) {
            std::vector<BiquadCoefficients> coefficients(bands);
            for (std::size_t b = 0; b < bands; ++b)
                ok(designPeakingBiquad(48000, 100 * std::pow(1.8, double(b)), .8, b % 2 ? -3 : 4,
                                       coefficients[b]));
            auto p = std::make_shared<SmoothedBiquadCascade<T>>();
            ok(p->prepare(spec, bands));
            ok(p->setCoefficients(coefficients, 0));
            add("eq.smoothed." + std::to_string(bands), p, d);
            auto plain = std::make_shared<BiquadCascade<T>>();
            ok(plain->prepare(spec, coefficients));
            add("eq.plain." + std::to_string(bands), plain, d);
        }
        for (auto backend : {CpuBackend::scalar, CpuBackend::automatic}) {
            auto p = std::make_shared<BiquadBank<T>>();
            ok(p->prepare(spec, backend));
            BiquadCoefficients c;
            ok(designBiquad(BiquadType::lowPass, 48000, 1800, .7, c));
            ok(p->setAllCoefficients(c));
            add(std::string("biquad.stereo.") + backendName(p->backend()), p, d);
        }
        for (std::size_t length : {512u, 8192u, 16384u, 65536u})
            for (std::size_t early : {32u, 64u, 128u, 256u})
                for (auto mode : {ConvolutionMode::nonuniform, ConvolutionMode::scheduled}) {
                    if constexpr (std::same_as<T, double>) {
                        if (length != 8192 || early != 128)
                            continue;
                    }
                    auto p = std::make_shared<MatrixConvolver<T>>();
                    std::vector<T> ir(length), ir2(length);
                    for (std::size_t i = 0; i < length; ++i) {
                        ir[i] = T(std::exp(-6. * double(i) / double(length)) * .01 *
                                  std::cos(.3 * double(i)));
                        ir2[i] = T(.8 * double(ir[i]));
                    }
                    ir[0] += T(1);
                    std::array<ConvolutionRoute<T>, 4> r{
                        {{0, 0, ir}, {1, 1, ir2}, {0, 1, ir2}, {1, 0, ir}}};
                    const bool mimo = length == 16384;
                    ok(p->prepare(spec, 2, std::span(r).first(mimo ? 4 : 2), {mode, early, 1024}));
                    add("matrix." + std::to_string(length) + ".p" + std::to_string(early) +
                            (mode == ConvolutionMode::scheduled ? ".scheduled" : ".nonuniform"),
                        p, d);
                }
    }
}
void controlCases() {
    for (bool moving : {false, true}) {
        auto coefficients = std::make_shared<BiquadCoefficients>();
        auto cursor = std::make_shared<unsigned>(0);
        cases.push_back({moving ? "design.peaking.changing34" : "design.peaking.fixed34",
                         [coefficients, cursor, moving] {
                             for (int i = 0; i < 34; ++i) {
                                 const auto n = (*cursor)++ % 257;
                                 ok(designPeakingBiquad(48000, moving ? 500 + n * 20 : 2800, .8,
                                                        moving ? -9. * n / 256 : -3,
                                                        *coefficients));
                                 sink = coefficients->b0;
                             }
                         },
                         [cursor] { *cursor = 0; }, [](std::ofstream&) {}});
    }
    auto gains = std::make_shared<std::array<double, 3>>();
    Directivity directivity;
    AcousticMaterial material;
    material.absorption = {.2, .4, .7};
    material.transmissionLossDb = {6, 12, 24};
    auto response = std::make_shared<MaterialResponse>();
    cases.push_back({"spatial.directivity.64calls",
                     [gains, directivity] {
                         for (int i = 0; i < 64; ++i) {
                             ok(directivityGains(.25, directivity, *gains));
                             sink = (*gains)[0];
                         }
                     },
                     [] {}, [](std::ofstream&) {}});
    cases.push_back({"spatial.material.64calls",
                     [response, material] {
                         for (int i = 0; i < 64; ++i) {
                             ok(materialResponse(material, .2, *response));
                             sink = response->transmission[0];
                         }
                     },
                     [] {}, [](std::ofstream&) {}});
}
} // namespace
int main(int argc, char** argv) {
    try {
        make<float>();
        make<double>();
        controlCases();
        // Capture only reset deterministic output, independent of timing/warmup history.
        if (argc > 1) {
            std::ofstream f(argv[1], std::ios::binary);
            for (auto& c : cases) {
                c.reset();
                for (int i = 0; i < 20; ++i) {
                    c.run();
                    c.dump(f);
                }
            }
            if (!f)
                throw std::runtime_error("dump failed");
        }
        std::cout << std::setprecision(10) << "[\n";
        bool first = true;
        for (auto& c : cases) {
            c.reset();
            for (int i = 0; i < 256; ++i)
                c.run();
            std::array<double, 9> trials;
            for (auto& t : trials) {
                const auto start = std::chrono::steady_clock::now();
                for (int i = 0; i < 128; ++i)
                    c.run();
                t = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() -
                                                              start)
                        .count() /
                    128;
            }
            std::sort(trials.begin(), trials.end());
            if (!std::isfinite(sink))
                throw std::runtime_error("nonfinite output");
            if (!first)
                std::cout << ",\n";
            first = false;
            std::cout << "{\"case\":\"" << c.name
                      << "\",\"frames\":256,\"medianBatchMeanUs\":" << trials[4]
                      << ",\"worstBatchMeanUs\":" << trials[8] << "}";
        }
        std::cout << "\n]\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
