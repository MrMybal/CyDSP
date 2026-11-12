#include <algorithm>
#include <complex>
#include <cydsp/CyDSP.h>
#include <iostream>
#include <numbers>
#include <source_location>
#include <stdexcept>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool v, const char* why) {
    if (!v)
        throw std::runtime_error(why);
}
void ok(Status r, std::source_location at = std::source_location::current()) {
    if (r != Status::ok)
        throw std::runtime_error("DAW expansion status " + std::to_string(unsigned(r)) + " line " +
                                 std::to_string(at.line()));
}
double magnitude(std::span<const BiquadCoefficients> coefficients, double hz, double rate) {
    auto z = std::polar(1.0, -2 * std::numbers::pi * hz / rate);
    std::complex<double> h = 1;
    for (const auto& c : coefficients)
        h *= (c.b0 + c.b1 * z + c.b2 * z * z) / (1.0 + c.a1 * z + c.a2 * z * z);
    return std::abs(h);
}
void designs() {
    std::array<BiquadCoefficients, 8> sections{};
    for (auto family :
         {IirFamily::chebyshevI, IirFamily::chebyshevII, IirFamily::elliptic, IirFamily::bessel})
        for (auto type : {BiquadType::lowPass, BiquadType::highPass})
            for (unsigned order = 1; order <= 16; ++order) {
                IirDesign d;
                d.family = family;
                d.response = type;
                d.order = order;
                d.cutoff = 3000;
                d.stopBandDb = 60;
                std::size_t n = 0;
                auto result = designIir(d, sections, n);
                if (result != Status::ok)
                    throw std::runtime_error("filter design family/order " +
                                             std::to_string(unsigned(family)) + "/" +
                                             std::to_string(order));
                require(n == (order + 1) / 2, "SOS count");
                const auto c = std::span(sections).first(n);
                for (auto v : c)
                    require(v.isStable(), "SOS poles");
                const auto edge = magnitude(c, 3000, 48000),
                           dc = magnitude(c, type == BiquadType::lowPass ? 0 : 24000, 48000);
                const auto expected = family == IirFamily::bessel        ? std::sqrt(.5)
                                      : family == IirFamily::chebyshevII ? .001
                                                                         : std::pow(10., -.05);
                require(std::abs(edge - expected) < (family == IirFamily::elliptic ? 2e-4 : 2e-7),
                        "independent cutoff attenuation");
                require(dc > .85 && dc < 1.000001, "pass-band normalization");
            }
    IirDesign invalid;
    invalid.order = 0;
    std::size_t n = 77;
    const auto before = sections;
    require(designIir(invalid, sections, n) == Status::invalid_argument && n == 77 &&
                sections[0].b0 == before[0].b0,
            "transactional coefficient rejection");
    for (auto type :
         {WindowType::blackmanHarris, WindowType::nuttall, WindowType::flatTop,
          WindowType::bartlett, WindowType::triangular, WindowType::cosine, WindowType::kaiser}) {
        std::array<double, 129> window{};
        WindowParameters p;
        p.type = type;
        p.normalization = WindowNormalization::coherentGain;
        ok(makeWindow(p, window));
        double sum = 0;
        for (std::size_t i = 0; i < window.size(); ++i) {
            sum += window[i];
            require(std::abs(window[i] - window[window.size() - 1 - i]) < 1e-12, "window symmetry");
        }
        require(std::abs(sum / 129 - 1) < 1e-12, "window coherent gain");
    }
}
template <Sample T> void interpolation() {
    AudioBuffer<T> source, out;
    ok(source.resize(1, 64));
    ok(out.resize(1, 16));
    for (std::size_t i = 0; i < 64; ++i)
        source.block().channel(0)[i] =
            static_cast<T>(double(i) * double(i) * double(i) - 2 * double(i) + 1);
    PolynomialInterpolator<T> interp;
    ok(interp.prepare({48000, 16, 1}, 24000, PolynomialInterpolation::lagrange, 3));
    ok(interp.process(source.block(), 4.25, out.block()));
    for (std::size_t i = 0; i < 16; ++i) {
        const auto x = 4.25 + double(i) * .5;
        require(std::abs(double(out.block().channel(0)[i]) - (x * x * x - 2 * x + 1)) <
                    (std::same_as<T, float> ? .001 : 1e-10),
                "cubic Lagrange oracle");
    }
    for (std::size_t i = 0; i < 64; ++i)
        source.block().channel(0)[i] = static_cast<T>(double(i) * 2 + 1);
    ok(interp.prepare({48000, 16, 1}, 24000, PolynomialInterpolation::catmullRom));
    ok(interp.process(source.block(), 4.25, out.block()));
    for (std::size_t i = 0; i < 16; ++i)
        require(std::abs(double(out.block().channel(0)[i]) - (9.5 + double(i))) < 1e-6,
                "Catmull-Rom linear oracle");
}
template <Sample T> void oversampling() {
    AudioBuffer<T> in, out;
    ok(in.resize(1, 2048));
    ok(out.resize(1, 2048));
    for (auto mode : {OversamplingMode::halfBandFir, OversamplingMode::iir})
        for (unsigned factor : {2u, 4u, 8u, 16u}) {
            Oversampler<T> over;
            ok(over.prepare({48000, 2048, 1}, factor, mode));
            std::fill(in.block().channel(0).begin(), in.block().channel(0).end(), T{});
            in.block().channel(0)[0] = 1;
            AudioBlock<T> high;
            ok(over.processUp(in.block(), high));
            ok(over.processDown(out.block()));
            if (mode == OversamplingMode::halfBandFir) {
                auto samples = out.block().channel(0);
                auto peak =
                    std::size_t(std::max_element(samples.begin(), samples.end()) - samples.begin());
                require(peak == over.latencyFrames() && over.linearPhase(), "half-band latency");
            } else
                require(!over.linearPhase() && over.latencyAtDcFrames() > 0, "IIR phase policy");
            over.reset();
            std::fill(in.block().channel(0).begin(), in.block().channel(0).end(), T{1});
            ok(over.processUp(in.block(), high));
            ok(over.processDown(out.block()));
            require(std::abs(double(out.block().channel(0)[1800]) - 1) < .001,
                    "staged oversampling DC");
            struct Audit {
                Oversampler<T>* processor;
                AudioBuffer<T>* input;
                AudioBuffer<T>* output;
                Status status = Status::ok;
            } audit{&over, &in, &out};
            runRealtimeAudit(
                [](void* ptr) noexcept {
                    auto& a = *static_cast<Audit*>(ptr);
                    a.processor->reset();
                    AudioBlock<T> high;
                    a.status = a.processor->processUp(a.input->block(), high);
                    if (a.status == Status::ok)
                        a.status = a.processor->processDown(a.output->block());
                },
                &audit);
            ok(audit.status);
        }
}
template <Sample T> void synthesis() {
    std::array<T, 256> cycle{};
    for (std::size_t i = 0; i < cycle.size(); ++i) {
        const auto phase = 2 * std::numbers::pi * double(i) / cycle.size();
        cycle[i] = static_cast<T>(std::sin(phase) + .5 * std::sin(6 * phase));
    }
    Wavetable<T> table;
    ok(table.prepare(cycle));
    require(table.levels() == 8, "harmonic mip levels");
    WavetableOscillator<T> oscillator;
    ok(oscillator.prepare({48000, 1024, 1}, table));
    ok(oscillator.setFrequency(6000));
    AudioBuffer<T> out;
    ok(out.resize(1, 1024));
    ok(oscillator.process(out.block()));
    std::complex<double> fundamental = 0, alias = 0;
    for (std::size_t i = 0; i < 1024; ++i) {
        fundamental += double(out.block().channel(0)[i]) *
                       std::polar(1.0, -2 * std::numbers::pi * double(i) / 8);
        alias += double(out.block().channel(0)[i]) *
                 std::polar(1.0, -2 * std::numbers::pi * double(i) / 4);
    }
    require(std::abs(std::abs(fundamental) * 2 / 1024 - 1) < 1e-5 &&
                std::abs(alias) * 2 / 1024 < 1e-5,
            "independent wavetable alias rejection");
    AudioBuffer<T> input;
    ok(input.resize(1, 4096));
    for (std::size_t i = 0; i < 4096; ++i)
        input.block().channel(0)[i] =
            static_cast<T>(std::sin(2 * std::numbers::pi * .35 * double(i)));
    SampleData<T> sample;
    ok(sample.prepare(input.block(), 48000));
    double linear = 0, filtered = 0;
    for (std::size_t i = 0; i < 512; ++i) {
        const auto position = 512 + 2 * double(i);
        const auto a = sample.value(0, position, 2, SampleInterpolation::linear),
                   b = sample.value(0, position, 2);
        linear += a * a;
        filtered += b * b;
    }
    require(linear > 200 && filtered < linear * 1e-5, "sampler antialias mip rejection");
    for (std::size_t i = 0; i < 128; ++i)
        require(sample.value(0, double(i), 1) == input.block().channel(0)[i],
                "native sampler exactness");
    std::array<T, 8> loop{1, 1, 1, 1, -1, -1, -1, -1};
    const T* channels[]{loop.data()};
    SampleData<T> loopSample;
    ok(loopSample.prepare({channels, 1, 8}, 48000));
    ok(loopSample.setLoop(0, 8, 2));
    require(loopSample.value(0, 6) == T{-1} && loopSample.value(0, 7) == T{0},
            "loop linear crossfade");
    Synthesizer<T> synth;
    ok(synth.prepare({48000, 1024, 1}, 4));
    ok(synth.setWavetable(&table));
    SynthParameters params;
    params.envelope = {0, 0, 1, 0};
    params.gain = 1;
    params.stealFadeFrames = 0;
    ok(synth.setParameters(params));
    ok(synth.setMpeZones(0, 0));
    MidiBuffer events;
    ok(events.prepare(4, 16));
    const auto note = midiNoteOn(1, 69, 127);
    ok(events.add(0, note.bytes()));
    struct Audit {
        Synthesizer<T>* synth;
        AudioBuffer<T>* out;
        MidiBuffer* events;
        Status status = Status::ok;
    } audit{&synth, &out, &events};
    runRealtimeAudit(
        [](void* ptr) noexcept {
            auto& a = *static_cast<Audit*>(ptr);
            a.status = a.synth->process(a.out->block(), a.events);
            a.synth->reset();
        },
        &audit);
    ok(audit.status);
}
#if defined(CYDSP_TEST_IO)
void instruments() {
    const std::string_view fixture =
        R"({"format":"CyDSPInstrument","version":1,"name":"Known","source":0,"voices":1,"mpeLower":0,"mpeUpper":0,"rootNote":60,"loopStart":0,"loopEnd":0,"crossfadeFrames":0,"sample":"","cycle":[],"parameters":{"waveform":0,"gain":0.25,"pan":0,"pressureToGain":0,"stealFadeFrames":0,"sampleInterpolation":1,"attack":0,"decay":0,"sustain":1,"release":0}})";
    InstrumentPreset known;
    ok(parseInstrumentPreset(fixture, known));
    require(known.name == "Known" && known.parameters.gain == .25 && known.voices == 1,
            "independent instrument fixture");
    InstrumentPreset preset;
    preset.name = "CyDSP piano";
    preset.voices = 8;
    preset.parameters.envelope = {0, 0, 1, .01};
    preset.source = InstrumentSource::wavetable;
    preset.cycle.resize(256);
    for (std::size_t i = 0; i < 256; ++i)
        preset.cycle[i] = std::sin(2 * std::numbers::pi * double(i) / 256);
    std::string json;
    ok(serializeInstrumentPreset(preset, json));
    InstrumentPreset copy;
    ok(parseInstrumentPreset(json, copy));
    require(copy.name == preset.name && copy.cycle == preset.cycle && copy.voices == 8,
            "versioned instrument roundtrip");
    auto unknown = json;
    const auto at = unknown.find(R"("version")");
    require(at != std::string::npos, "serialized version");
    const auto number = unknown.find_first_of("0123456789", at);
    unknown.replace(number, 1, "2");
    require(parseInstrumentPreset(unknown, copy) == Status::unsupported_format &&
                copy.name == preset.name,
            "version rejection preserves preset");
    std::string deep(40, '[');
    deep += "0";
    deep.append(40, ']');
    require(parseInstrumentPreset(deep, copy) == Status::invalid_argument, "bounded preset JSON");
    Instrument<float> instrument;
    ok(instrument.prepare({48000, 128, 2}, copy));
    auto* previous = instrument.synthesizer();
    copy.voices = 0;
    require(instrument.prepare({48000, 128, 2}, copy) == Status::invalid_argument &&
                instrument.synthesizer() == previous,
            "instrument transactional preparation");
    AudioBuffer<float> out;
    ok(out.resize(2, 128));
    MidiBuffer notes;
    ok(notes.prepare(4, 16));
    const auto on = midiNoteOn(1, 69, 127);
    ok(notes.add(0, on.bytes()));
    struct Audit {
        Instrument<float>* i;
        AudioBuffer<float>* out;
        MidiBuffer* notes;
        Status status = Status::ok;
    } audit{&instrument, &out, &notes};
    runRealtimeAudit(
        [](void* ptr) noexcept {
            auto& a = *static_cast<Audit*>(ptr);
            a.status = a.i->process(a.out->block(), a.notes);
            a.i->reset();
        },
        &audit);
    ok(audit.status);
}
#endif
void gates() {
    Dynamics<double> gate;
    ok(gate.prepare({1000, 16, 1}, DynamicsMode::noiseGate));
    DynamicsParameters p;
    p.thresholdDb = -20;
    p.attackMs = p.releaseMs = 0;
    p.holdMs = 3;
    p.hysteresisDb = 6;
    ok(gate.setParameters(p));
    std::array<double, 8> input{.2, .08, .06, .01, .01, .01, .01, .2}, output{};
    const double* a[]{input.data()};
    double* b[]{output.data()};
    ok(gate.process({a, 1, 8}, {b, 1, 8}));
    require(output[0] == .2 && output[1] == .08 && output[2] == .06 && output[5] > .009 &&
                output[6] < 1e-9 && output[7] == .2,
            "gate hold/hysteresis sample boundaries");
    Dynamics<double> delay;
    ok(delay.prepare({1000, 16, 1}));
    p = {};
    p.thresholdDb = 24;
    p.attackMs = p.releaseMs = 0;
    p.lookAheadMs = 3;
    ok(delay.setParameters(p));
    ok(delay.process({a, 1, 8}, {b, 1, 8}));
    require(delay.latencyFrames() == 3 && output[0] == 0 && output[2] == 0 &&
                output[3] == input[0] && output[7] == input[4],
            "look-ahead latency");
    struct Audit {
        Dynamics<double>* delay;
        const double** a;
        double** b;
        Status status = Status::ok;
    } audit{&delay, a, b};
    runRealtimeAudit(
        [](void* ptr) noexcept {
            auto& v = *static_cast<Audit*>(ptr);
            v.delay->reset();
            v.status = v.delay->process({v.a, 1, 8}, {v.b, 1, 8});
        },
        &audit);
    ok(audit.status);
}
} // namespace
void dawExpansionRegression() {
    designs();
    interpolation<float>();
    interpolation<double>();
    gates();
    oversampling<float>();
    oversampling<double>();
    synthesis<float>();
    synthesis<double>();
#if defined(CYDSP_TEST_IO)
    instruments();
#endif
    std::cout << "PASS DAW expansion DSP/wavetable/sampler/presets/realtime\n";
}
