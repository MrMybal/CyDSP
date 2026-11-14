// Explicit opt-in measurements. Fixtures and parameter preparation are outside timing.
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>
#ifndef CYDSP_HAS_PREPARED_OPTIMIZATIONS
#define CYDSP_HAS_PREPARED_OPTIMIZATIONS 0
#endif
namespace {
using namespace cydsp;
volatile double sink = 0;
void ok(Status s) {
    if (s != Status::ok)
        throw std::runtime_error("optimization measurement status");
}
struct Case {
    std::string name;
    std::size_t frames;
    std::function<void()> run, reset;
};
std::vector<Case> cases;
template <class T> std::string suffix() {
    return std::same_as<T, float> ? ".f32" : ".f64";
}
template <class T> struct Data {
    AudioBuffer<T> in, out;
    Data() {
        ok(in.resize(2, 256));
        ok(out.resize(2, 256));
        for (std::size_t c = 0; c < 2; ++c)
            for (std::size_t i = 0; i < 256; ++i)
                in.block().channel(c)[i] = T(.42 * std::sin(.17 * double(i) + double(c)) +
                                             .08 * std::cos(1.13 * double(i)));
    }
};
template <class T, class P>
void processor(std::string name, std::shared_ptr<P> p, std::shared_ptr<Data<T>> d) {
    cases.push_back({name + suffix<T>(), 256,
                     [p, d] {
                         ok(p->process(d->in.block(), d->out.block()));
                         sink = d->out.block().channel(0).back();
                     },
                     [p] { p->reset(); }});
}
template <class T> void common() {
    struct FftData {
        RealFft<T> fft;
        std::vector<T> in, out;
        std::vector<std::complex<T>> bins;
    };
    for (std::size_t n : {64, 128, 256, 512, 2048}) {
        auto d = std::make_shared<FftData>();
        ok(d->fft.prepare(n));
        d->in.resize(n);
        d->out.resize(n);
        d->bins.resize(n / 2 + 1);
        for (std::size_t i = 0; i < n; ++i)
            d->in[i] = T(.3 * std::sin(.17 * double(i)) + .2 * std::cos(.83 * double(i)));
        ok(d->fft.forward(d->in, d->bins));
        cases.push_back({"fft.forward." + std::to_string(n) + suffix<T>(), n,
                         [d] {
                             ok(d->fft.forward(d->in, d->bins));
                             sink = d->bins.back().real();
                         },
                         [] {}});
        cases.push_back({"fft.inverse." + std::to_string(n) + suffix<T>(), n,
                         [d] {
                             ok(d->fft.inverse(d->bins, d->out));
                             sink = d->out.back();
                         },
                         [] {}});
    }
    auto transient = std::make_shared<TransientShaper<T>>();
    ok(transient->prepare({48000, 256, 2}));
    TransientParameters t;
    t.attack = .7;
    t.sustain = -.3;
    ok(transient->setParameters(t));
    processor<T>("transient.audio", transient, std::make_shared<Data<T>>());
    auto clip = std::make_shared<AdaaSaturator<T>>();
    ok(clip->prepare({48000, 256, 2}));
    SaturationParameters p;
    p.shape = AdaaShape::hardClip;
    p.antialias = false;
    ok(clip->setParameters(p));
    processor<T>("clip.identity.noAA", clip, std::make_shared<Data<T>>());
}
void commonControls() {
    for (double scale : {1., 1.5, 2.}) {
        AcousticMaterial material;
        material.absorption.fill(.3);
        material.transmissionLossDb.fill(12);
        auto output = std::make_shared<MaterialResponse>();
        cases.push_back({"material.equal64.scale" + std::to_string(scale), 0,
                         [=] {
                             for (int i = 0; i < 64; ++i) {
                                 ok(materialResponse(material, .2 * scale, *output));
                                 sink = output->transmission[0];
                             }
                         },
                         [] {}});
    }
}
#if CYDSP_HAS_PREPARED_OPTIMIZATIONS
template <class T> void advanced() {
    for (auto math : {MathAccuracy::precise, MathAccuracy::fast}) {
        auto p = std::make_shared<TransientShaper<T>>();
        ok(p->prepare({48000, 256, 2}));
        TransientParameters parameters;
        parameters.attack = .7;
        parameters.sustain = -.3;
        parameters.math = math;
        ok(p->setParameters(parameters));
        processor<T>(math == MathAccuracy::fast ? "transient.fast" : "transient.precise", p,
                     std::make_shared<Data<T>>());
    }

    for (bool fast : {false, true}) {
        auto d = std::make_shared<Data<T>>();
        auto p = std::make_shared<AdaaSaturator<T>>();
        ok(p->prepare({48000, 256, 2}));
        SaturationParameters settings;
        settings.shape = AdaaShape::atan;
        settings.drive = 3;
        settings.math = fast ? MathAccuracy::fast : MathAccuracy::precise;
        ok(p->setParameters(settings));
        processor<T>(fast ? "atan.fast" : "atan.precise", p, d);
        for (std::size_t c = 0; c < 2; ++c)
            for (std::size_t i = 0; i < 256; ++i)
                d->in.block().channel(c)[i] = T(.0001 + .000001 * double(i));
        // Own a second fixture so the ordinary case keeps its original stimulus.
        auto ordinary = std::make_shared<Data<T>>();
        cases[cases.size() - 1].run = [p, ordinary] {
            ok(p->process(ordinary->in.block(), ordinary->out.block()));
            sink = ordinary->out.block().channel(0).back();
        };
        auto slow = std::make_shared<AdaaSaturator<T>>();
        ok(slow->prepare({48000, 256, 2}));
        ok(slow->setParameters(settings));
        processor<T>(fast ? "atan.slow.fast" : "atan.slow.precise", slow, d);
    }
    for (bool fused : {false, true}) {
        auto d = std::make_shared<Data<T>>();
        auto p = std::make_shared<Dynamics<T>>();
        ok(p->prepare({48000, 256, 2}, DynamicsMode::compressor, 0));
        DynamicsParameters settings;
        settings.ratio = 6;
        ok(p->setParameters(settings));
        auto mix = std::make_shared<DryWetMixer<T>>();
        ok(mix->prepare({48000, 256, 2}));
        ok(mix->setMix(.7, MixLaw::sine3dB));
        const double dry = std::cos(.7 * std::numbers::pi / 2),
                     wet = std::sin(.7 * std::numbers::pi / 2);
        cases.push_back({std::string(fused ? "dynamics.mixed" : "dynamics.separate") + suffix<T>(),
                         256,
                         [=] {
                             if (fused)
                                 ok(p->processMixed(d->in.block(), d->out.block(), dry, wet));
                             else {
                                 ok(mix->pushDrySamples(d->in.block()));
                                 ok(p->process(d->in.block(), d->out.block()));
                                 ok(mix->mixWetSamples(d->out.block()));
                             }
                             sink = d->out.block().channel(0).back();
                         },
                         [=] {
                             p->reset();
                             mix->reset();
                         }});
        auto transient = std::make_shared<TransientShaper<T>>();
        ok(transient->prepare({48000, 256, 2}));
        TransientParameters tp;
        tp.attack = .7;
        tp.sustain = -.3;
        ok(transient->setParameters(tp));
        cases.push_back(
            {std::string(fused ? "transient.gainOnly" : "transient.recoverGain") + suffix<T>(), 256,
             [=] {
                 if (fused)
                     ok(transient->processGain(d->in.block(), d->out.block()));
                 else {
                     ok(transient->process(d->in.block(), d->out.block()));
                     for (std::size_t c = 0; c < 2; ++c)
                         for (std::size_t i = 0; i < 256; ++i) {
                             auto& y = d->out.block().channel(c)[i];
                             const auto x = d->in.block().channel(c)[i];
                             y = std::abs(x) > T(1e-12) ? y / x : T(1);
                         }
                 }
                 sink = d->out.block().channel(0).back();
             },
             [=] { transient->reset(); }});
    }
    for (bool fused : {false, true}) {
        auto d = std::make_shared<Data<T>>();
        auto p = std::make_shared<AdaaSaturator<T>>();
        ok(p->prepare({48000, 256, 2}));
        SaturationParameters settings;
        settings.shape = AdaaShape::hardClip;
        ok(p->setParameters(settings));
        auto transformed = std::make_shared<AudioBuffer<T>>();
        ok(transformed->resize(2, 256));
        const SaturationInputStage stage{4, .2, 2.5};
        cases.push_back(
            {std::string(fused ? "saturation.mixed" : "saturation.separate") + suffix<T>(), 256,
             [=] {
                 if (fused)
                     ok(p->processMixed(d->in.block(), d->out.block(), stage, .3, .7));
                 else {
                     for (std::size_t c = 0; c < 2; ++c)
                         for (std::size_t i = 0; i < 256; ++i)
                             transformed->block().channel(c)[i] =
                                 T((double(d->in.block().channel(c)[i]) * stage.gain + stage.bias) *
                                   stage.scale);
                     ok(p->process(transformed->block(), d->out.block()));
                     for (std::size_t c = 0; c < 2; ++c)
                         for (std::size_t i = 0; i < 256; ++i)
                             d->out.block().channel(c)[i] =
                                 T(.3 * double(d->in.block().channel(c)[i]) +
                                   .7 * double(d->out.block().channel(c)[i]));
                 }
                 sink = d->out.block().channel(0).back();
             },
             [=] { p->reset(); }});
    }
}
void controls() {
    for (bool prepared : {false, true}) {
        auto eq = std::make_shared<PeakingBiquadDesigner>();
        ok(eq->prepare(48000, 2800, .8));
        auto cursor = std::make_shared<unsigned>(0);
        auto coefficients = std::make_shared<BiquadCoefficients>();
        cases.push_back({prepared ? "eq.prepared.gain34" : "eq.raw.gain34", 0,
                         [=] {
                             for (int i = 0; i < 34; ++i) {
                                 const auto gain = -9. * double((*cursor)++ % 257) / 256;
                                 if (prepared)
                                     ok(eq->design(gain, *coefficients));
                                 else
                                     ok(designPeakingBiquad(48000, 2800, .8, gain, *coefficients));
                                 sink = coefficients->b0;
                             }
                         },
                         [=] { *cursor = 0; }});
        AcousticMaterial m;
        m.absorption.fill(.3);
        m.transmissionLossDb.fill(12);
        auto material = std::make_shared<PreparedAcousticMaterial>();
        ok(material->prepare(m));
        auto response = std::make_shared<MaterialResponse>();
        cases.push_back({prepared ? "material.prepared.equal64" : "material.raw.equal64", 0,
                         [=] {
                             for (int i = 0; i < 64; ++i) {
                                 if (prepared)
                                     ok(material->response(.3, *response));
                                 else
                                     ok(materialResponse(m, .3, *response));
                                 sink = response->transmission[0];
                             }
                         },
                         [] {}});
        AcousticMaterialBand band{.3, 12, .2, .2};
        auto mono = std::make_shared<PreparedMaterialBand>();
        ok(mono->prepare(band));
        auto monoResponse = std::make_shared<MaterialBandResponse>();
        cases.push_back({prepared ? "material.prepared.mono64" : "material.raw.mono64", 0,
                         [=] {
                             for (int i = 0; i < 64; ++i) {
                                 if (prepared)
                                     ok(mono->response(.3, *monoResponse));
                                 else
                                     ok(materialBandResponse(band, .3, *monoResponse));
                                 sink = monoResponse->transmission;
                             }
                         },
                         [] {}});
        auto doppler = std::make_shared<DopplerPlan>();
        ok(doppler->prepare());
        auto ratio = std::make_shared<double>(1);
        cases.push_back({prepared ? "doppler.prepared64" : "doppler.raw64", 0,
                         [=] {
                             for (int i = 0; i < 64; ++i) {
                                 if (prepared)
                                     ok(doppler->ratio(-30. + i, 3, *ratio));
                                 else {
                                     RelativeSource relative;
                                     relative.sourceRadialVelocity = -30. + i;
                                     relative.listenerRadialVelocity = 3;
                                     ok(dopplerRatio(relative, {}, *ratio));
                                 }
                                 sink = *ratio;
                             }
                         },
                         [] {}});
    }
}
#endif
} // namespace
int main(int argc, char**) {
    try {
        common<float>();
        common<double>();
        commonControls();
#if CYDSP_HAS_PREPARED_OPTIMIZATIONS
        if (argc > 1) {
            advanced<float>();
            advanced<double>();
            controls();
        }
#else
        (void)argc;
#endif
        std::cout << std::setprecision(10) << "[\n";
        bool first = true;
        for (auto& c : cases) {
            c.reset();
            for (int i = 0; i < 256; ++i)
                c.run();
            std::array<double, 9> means;
            for (auto& value : means) {
                const auto start = std::chrono::steady_clock::now();
                for (int i = 0; i < 128; ++i)
                    c.run();
                value = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() -
                                                                  start)
                            .count() /
                        128;
            }
            std::sort(means.begin(), means.end());
            if (!std::isfinite(sink))
                throw std::runtime_error("nonfinite measurement output");
            if (!first)
                std::cout << ",\n";
            first = false;
            std::cout << "{\"case\":\"" << c.name << "\",\"frames\":" << c.frames
                      << ",\"medianBatchMeanUs\":" << means[4]
                      << ",\"worstBatchMeanUs\":" << means[8] << "}";
        }
        std::cout << "\n]\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
