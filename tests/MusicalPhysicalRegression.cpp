#include <array>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <numbers>
#include <stdexcept>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool v, const char* m) {
    if (!v)
        throw std::runtime_error(m);
}
void ok(Status s) {
    require(s == Status::ok, "music physical status");
}
template <class T> void test() {
    const ProcessSpec spec{8000, 128, 1};
    MusicalAnalysisConfig config;
    config.fftFrames = 1024;
    config.hopFrames = 128;
    config.melBands = 26;
    MusicalAnalyzer<T> analyzer;
    ok(analyzer.prepare(spec, config));
    AudioBuffer<T> in, out;
    ok(in.resize(1, 128));
    ok(out.resize(1, 128));
    for (unsigned block = 0; block < 30; ++block) {
        for (std::size_t i = 0; i < 128; ++i) {
            const double phase = 2 * std::numbers::pi * 220 * double(block * 128 + i) / 8000;
            in.block().channel(0)[i] =
                T(.5 * std::sin(phase) + .2 * std::sin(2 * phase) + .1 * std::sin(3 * phase));
        }
        ok(analyzer.process(in.block()));
    }
    auto f = analyzer.features();
    require(std::abs(f.fundamentalHz - 220) < .5 && f.pitchConfidence > .95,
            "YIN harmonic fundamental confidence");
    for (double c : f.mfcc)
        require(isFinite(c), "finite MFCC");
    double chroma = 0;
    for (double c : f.chroma)
        chroma += c;
    require(std::abs(chroma - 1) < 1e-12 && f.chroma[9] > .1, "normalized chroma A");
    analyzer.reset();
    for (unsigned b = 0; b < 500; ++b) {
        for (std::size_t i = 0; i < 128; ++i) {
            const auto t = (b * 128 + i) % 4000;
            in.block().channel(0)[i] =
                t < 80 ? T(std::sin(2 * std::numbers::pi * 1000 * double(t) / 8000) *
                           std::exp(-double(t) / 20))
                       : T{};
        }
        ok(analyzer.process(in.block()));
    }
    f = analyzer.features();
    require(f.tempoBpm > 115 && f.tempoBpm < 125 && f.beatConfidence > .5, "periodic onset tempo");
    analyzer.reset();
    for (auto& x : in.block().channel(0))
        x = 0;
    for (unsigned b = 0; b < 10; ++b)
        ok(analyzer.process(in.block()));
    require(analyzer.features().fundamentalHz == 0 && analyzer.features().pitchConfidence == 0,
            "silence unvoiced");
    PitchEffects<T> pitch;
    ok(pitch.prepare(spec, 4, config));
    const HarmonyVoice voice{12, 1, 0, 0};
    ok(pitch.setVoices({&voice, 1}));
    double first = 0, last = 0, count = 0;
    T previous = 0;
    for (unsigned b = 0; b < 100; ++b) {
        for (std::size_t i = 0; i < 128; ++i)
            in.block().channel(0)[i] =
                T(.5 * std::sin(2 * std::numbers::pi * 200 * double(b * 128 + i) / 8000));
        ok(pitch.process(in.block(), out.block()));
        for (std::size_t i = 0; i < 128; ++i) {
            const T x = out.block().channel(0)[i];
            if (b > 20 && previous < 0 && x >= 0) {
                const double time = double(b * 128 + i) - double(x) / (double(x) - previous);
                if (count == 0)
                    first = time;
                last = time;
                ++count;
            }
            previous = x;
        }
    }
    require(std::abs((count - 1) * 8000 / (last - first) - 400) < 1, "harmonizer octave frequency");
    ModalResonator<T> modal;
    ok(modal.prepare(spec, 1, 64));
    const ResonantMode mode{500, .1, 1, 0};
    ok(modal.setModes({&mode, 1}));
    for (auto& x : in.block().channel(0))
        x = 0;
    in.block().channel(0)[0] = 1;
    ok(modal.process(in.block(), out.block()));
    const double radius = std::pow(.001, 1 / (.1 * 8000));
    for (std::size_t i = 0; i < 128; ++i) {
        const double expected = std::pow(radius, double(i)) *
                                std::sin(2 * std::numbers::pi * 500 * double(i + 1) / 8000);
        require(std::abs(double(out.block().channel(0)[i]) - expected) < 2e-6,
                "modal closed-form impulse");
    }
    ok(modal.setSurface(ModalSurface::rectangularMembrane, 200));
    ok(modal.setSurface(ModalSurface::rectangularPlate, 200));
    Waveguide<T> wave;
    ok(wave.prepare(spec));
    for (auto type :
         {WaveguideModel::karplusStrong, WaveguideModel::string, WaveguideModel::tube}) {
        WaveguideParameters p;
        p.model = type;
        p.frequencyHz = 200;
        p.decaySeconds = .1;
        p.closedTube = type == WaveguideModel::tube;
        ok(wave.setParameters(p));
        wave.reset();
        wave.pluck(.2, 17);
        double energy = 0;
        for (unsigned b = 0; b < 20; ++b) {
            ok(wave.render(out.block()));
            for (T x : out.block().channel(0)) {
                require(isFinite(x) && std::abs(double(x)) < 4, "waveguide stability");
                energy += double(x) * x;
            }
        }
        require(energy > .001, "waveguide excitation");
    }
    struct Audit {
        MusicalAnalyzer<T>* a;
        PitchEffects<T>* p;
        ModalResonator<T>* m;
        Waveguide<T>* w;
        AudioBlock<const T> in;
        AudioBlock<T> out;
        bool pass = true;
    } audit{&analyzer, &pitch, &modal, &wave, in.block(), out.block()};
    runRealtimeAudit(
        [](void* raw) noexcept {
            auto& c = *static_cast<Audit*>(raw);
            c.pass = c.a->process(c.in) == Status::ok && c.p->process(c.in, c.out) == Status::ok &&
                     c.m->process(c.in, c.out) == Status::ok &&
                     c.w->process(c.in, c.out) == Status::ok;
            c.a->reset();
            c.p->reset();
            c.m->reset();
            c.w->reset();
        },
        &audit);
    require(audit.pass, "musical physical realtime");
}
} // namespace
void musicalPhysicalRegression() {
    test<float>();
    test<double>();
}
