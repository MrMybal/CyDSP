#include "detail/StretchFft.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <complex>
#include <cydsp/CyDSP.h>
#include <numbers>
#include <stdexcept>
#include <thread>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool v, const char* m) {
    if (!v)
        throw std::runtime_error(m);
}
void ok(Status s) {
    require(s == Status::ok, "DAW status");
}
void ump() {
    auto on = umpNote(true, 3, 2, 60, 0xabcd);
    require(on.valid() && on.words[0] == 0x43913c00 && on.words[1] == 0xabcd0000,
            "UMP independent note word layout");
    require(midiScale7To16(10) == 0x1400 && midiScale7To16(64) == 0x8000 &&
                midiScale7To16(87) == 0xaeba && midiScale7To16(127) == 0xffff,
            "UMP official min/center/max examples");
    require(midiScale7To32(64) == 0x80000000u && midiScale7To32(127) == 0xffffffffu,
            "UMP 32-bit scaling");
    for (unsigned v = 0; v < 16384; ++v) {
        auto message = midiPitchBend(4, v);
        UmpPacket modern;
        ok(midi1ToMidi2(umpMidi1(7, MidiMessageView(message.bytes())), modern));
        ShortMidiMessage back;
        ok(midi2ToMidi1(modern, back));
        require(std::equal(back.bytes().begin(), back.bytes().end(), message.bytes().begin()),
                "14-bit pitch roundtrip exhaustive");
    }
    std::array<std::uint8_t, 16> bytes{};
    std::size_t n = 0;
    ok(on.toBytes(bytes, n));
    require(n == 8 && bytes[0] == 0x43 && bytes[4] == 0xab, "UMP byte endianness");
    struct Packets {
        std::array<UmpPacket, 32> values{};
        std::size_t count = 0;
    };
    Packets captured;
    UmpStreamParser parser;
    auto callback = [](void* c, const UmpPacket& p) noexcept {
        auto& a = *static_cast<Packets*>(c);
        if (a.count < a.values.size())
            a.values[a.count++] = p;
    };
    for (std::size_t i = 0; i < n; ++i)
        ok(parser.feedBytes({bytes.data() + i, 1}, callback, &captured));
    require(captured.count == 1 && captured.values[0].words == on.words && !parser.incomplete(),
            "fragmented UMP bytes");
    for (unsigned t = 0; t < 16; ++t) {
        UmpPacket p;
        p.count = static_cast<std::uint8_t>(umpWordCount(t));
        p.words[0] = t << 28;
        if (t == 1)
            p.words[0] |= 0x00f80000;
        if (t == 2)
            p.words[0] |= 0x00900000;
        if (t == 5)
            p.words[0] |= 0x00010000;
        for (unsigned i = 0; i < p.count; ++i)
            ok(parser.feed({p.words.data() + i, 1}, callback, &captured));
    }
    require(captured.count == 17, "UMP reserved-type framing sizes");
    UmpSysExAssembler sysex;
    ok(sysex.prepare(16));
    const std::array<std::uint8_t, 4> payload{1, 2, 3, 4};
    ok(sysex.append(umpSysEx7(2, 1, payload)));
    ok(sysex.append(umpSysEx7(2, 3, payload)));
    require(sysex.message().size() == 8 && sysex.message()[4] == 1, "UMP SysEx7 assembly");
    const std::array<std::uint8_t, 3> binary{0, 128, 255};
    ok(sysex.append(umpSysEx8(15, 0, 42, binary)));
    require(sysex.message().size() == 3 && sysex.message()[2] == 255, "UMP SysEx8 binary payload");
    auto invalid = on;
    invalid.count = 4;
    require(!invalid.valid(), "UMP size validation");
    UmpBuffer buffer;
    ok(buffer.prepare(4));
    ok(buffer.add(4, on));
    ok(buffer.add(1, umpController(0, 1, 7, 0xffffffff)));
    require(buffer.events()[0].frame == 1, "UMP stable sample order");
    struct Audit {
        UmpStreamParser& parser;
        UmpSysExAssembler& sysex;
        UmpBuffer& buffer;
        UmpPacket p;
    };
    Audit audit{parser, sysex, buffer, on};
    runRealtimeAudit(
        [](void* p) noexcept {
            auto& a = *static_cast<Audit*>(p);
            a.parser.reset();
            (void)a.parser.feed(
                {a.p.words.data(), a.p.count}, [](void*, const UmpPacket&) noexcept {}, nullptr);
            a.buffer.reset();
            (void)a.buffer.add(0, a.p);
            a.sysex.reset();
            const std::array<std::uint8_t, 1> data{0x7e};
            (void)a.sysex.append(umpSysEx7(0, 0, data));
        },
        &audit);
}
void mpe() {
    MpeInstrument instrument;
    ok(instrument.setZones(3, 2));
    struct State {
        std::array<MpeNote, 64> notes{};
        std::array<MpeChange, 64> changes{};
        std::size_t count = 0;
    };
    State state;
    auto callback = [](void* c, MpeChange change, const MpeNote& note, std::uint32_t) noexcept {
        auto& s = *static_cast<State*>(c);
        if (s.count < s.notes.size()) {
            s.notes[s.count] = note;
            s.changes[s.count++] = change;
        }
    };
    auto send = [&](ShortMidiMessage m) {
        ok(instrument.process(MidiMessageView(m.bytes()), 0, callback, &state));
    };
    send(midiNoteOn(2, 60, 127));
    send(midiPitchBend(2, 16383));
    send(midiPitchBend(1, 16383));
    require(instrument.note(2, 60) && std::abs(instrument.note(2, 60)->pitchSemitones - 50) < 1e-12,
            "MPE member plus manager bend");
    send(midiController(1, 64, 127));
    send(midiNoteOff(2, 60));
    require(instrument.note(2, 60) && !instrument.note(2, 60)->keyDown, "MPE master sustain");
    send(midiController(1, 64, 0));
    require(!instrument.note(2, 60), "MPE sustain release");
    send(midiNoteOn(2, 60, 100));
    send(midiController(1, 66, 127));
    send(midiNoteOn(3, 60, 100));
    send(midiNoteOff(2, 60));
    send(midiNoteOff(3, 60));
    require(instrument.note(2, 60) && !instrument.note(3, 60),
            "MPE sostenuto captures individual notes");
    send(midiController(1, 66, 0));
    require(!instrument.note(2, 60), "MPE sostenuto release");
    send(midiController(2, 101, 0));
    send(midiController(2, 100, 0));
    send(midiController(2, 6, 12));
    send(midiNoteOn(2, 64, 127));
    send(midiPitchBend(1, 8192));
    send(midiPitchBend(2, 16383));
    require(std::abs(instrument.note(2, 64)->pitchSemitones - 12) < 1e-12, "MPE RPN pitch range");
    require(instrument.setZones(2, 0) == Status::transition_in_progress,
            "MPE live configuration transaction");
    struct Audit {
        MpeInstrument& instrument;
    };
    Audit audit{instrument};
    runRealtimeAudit(
        [](void* p) noexcept {
            auto& a = *static_cast<Audit*>(p);
            const auto message = midiController(1, 120, 0);
            (void)a.instrument.process(
                MidiMessageView(message.bytes()), 0,
                [](void*, MpeChange, const MpeNote&, std::uint32_t) noexcept {}, nullptr);
            a.instrument.reset();
        },
        &audit);
}
template <Sample T> void fft() {
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::avx512,
                         CpuBackend::neon, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        for (std::size_t n : {4u, 8u, 12u, 24u, 32u, 96u, 128u, 192u}) {
            detail::StretchFft<T> fft;
            fft.select(backend);
            fft.resize(n);
            std::vector<T> input(n), output(n);
            std::vector<std::complex<T>> spectrum(n / 2);
            for (std::size_t i = 0; i < n; ++i)
                input[i] = static_cast<T>(std::sin(.37 * static_cast<double>(i)) * .2);
            fft.fft(input.data(), spectrum.data());
            for (std::size_t k = 0; k < n / 2; ++k) {
                std::complex<double> reference{};
                for (std::size_t i = 0; i < n; ++i)
                    reference +=
                        static_cast<double>(input[i]) *
                        std::polar(1.0, -2 * std::numbers::pi * (static_cast<double>(k) + .5) *
                                            static_cast<double>(i) / static_cast<double>(n));
                require(std::abs(reference - static_cast<std::complex<double>>(spectrum[k])) <
                            (std::same_as<T, float> ? 2e-5 : 1e-11),
                        "modified FFT independent DFT");
            }
            fft.ifft(spectrum.data(), output.data());
            for (std::size_t i = 0; i < n; ++i)
                require(std::abs(output[i] / static_cast<T>(n) - input[i]) <
                            (std::same_as<T, float> ? 1e-6 : 1e-13),
                        "modified FFT roundtrip");
        }
    }
}
template <Sample T> void synth() {
    const ProcessSpec spec{48000, 128, 1};
    Synthesizer<T> synth;
    ok(synth.prepare(spec, 4));
    SynthParameters parameters;
    parameters.envelope = {0, 0, 1, 0};
    parameters.gain = 1;
    parameters.stealFadeFrames = 0;
    ok(synth.setParameters(parameters));
    ok(synth.setMpeZones(0, 0));
    AudioBuffer<T> out;
    ok(out.resize(1, 128));
    MidiBuffer events;
    ok(events.prepare(8, 64));
    auto on = midiNoteOn(1, 69, 127), off = midiNoteOff(1, 69);
    ok(events.add(17, on.bytes()));
    ok(events.add(81, off.bytes()));
    ok(synth.process(out.block(), &events));
    for (std::size_t i = 0; i < 128; ++i) {
        const auto expected =
            i < 17 || i >= 81
                ? 0
                : std::sin(2 * std::numbers::pi * 440 * static_cast<double>(i - 17) / 48000);
        require(std::abs(out.block().channel(0)[i] - expected) <
                    (std::same_as<T, float> ? 1e-6 : 1e-13),
                "sample accurate synth gate/frequency");
    }
    require(synth.activeVoices() == 0, "synth release termination");
    UmpBuffer modern;
    ok(modern.prepare(8));
    ok(modern.add(0,
                  umpNote(true, 4, 3, 60, 65535, 3, static_cast<std::uint16_t>(69 * 512 + 256))));
    ok(synth.process(out.block(), nullptr, &modern));
    auto info = synth.voice(0);
    require(info.group == 4 && info.channel == 3 &&
                std::abs(info.frequency - 440 * std::exp2(.5 / 12)) < 1e-10,
            "MIDI2 note pitch attribute");
    modern.reset();
    ok(modern.add(0, umpPitch(4, 3, 0xc0000000u, 60)));
    ok(synth.process(out.block(), nullptr, &modern));
    require(synth.voice(0).frequency > 1000, "MIDI2 per-note pitch");
    synth.reset();
    AudioBuffer<T> sample;
    ok(sample.resize(2, 8));
    for (std::size_t i = 0; i < 8; ++i) {
        sample.block().channel(0)[i] = static_cast<T>(i) / 16;
        sample.block().channel(1)[i] = -static_cast<T>(i) / 16;
    }
    SampleData<T> data;
    ok(data.prepare(sample.block(), 48000));
    // A finite sample shorter than the callback must render its prefix before releasing.
    ok(synth.prepare({48000, 128, 2}, 2));
    ok(synth.setParameters(parameters));
    ok(synth.setMpeZones(0, 0));
    ok(synth.setSample(&data));
    ok(out.resize(2, 128));
    events.reset();
    on = midiNoteOn(1, 60, 127);
    ok(events.add(0, on.bytes()));
    ok(synth.process(out.block(), &events));
    require(std::abs(out.block().channel(0)[3] - 3.0 / 16 * std::sqrt(.5)) < 1e-6 &&
                out.block().channel(0)[8] == 0 && synth.activeVoices() == 0,
            "finite sampler sample-accurate envelope");
    ok(data.setLoop(2, 5));
    ok(synth.prepare({48000, 128, 2}, 2));
    ok(synth.setParameters(parameters));
    ok(synth.setMpeZones(0, 0));
    ok(synth.setSample(&data));
    ok(out.resize(2, 128));
    events.reset();
    on = midiNoteOn(1, 60, 127);
    ok(events.add(0, on.bytes()));
    ok(synth.process(out.block(), &events));
    for (std::size_t i = 0; i < 128; ++i) {
        const auto index = i < 5 ? i : 2 + (i - 5) % 3;
        const auto expected = static_cast<double>(index) / 16 * std::sqrt(.5);
        require(std::abs(out.block().channel(0)[i] - expected) < 1e-6 &&
                    std::abs(out.block().channel(1)[i] + expected) < 1e-6,
                "stereo sampler loop");
    }
    synth.reset();
    ok(synth.setSample(nullptr));
    events.reset();
    for (unsigned note : {60u, 64u, 67u}) {
        on = midiNoteOn(1, note, 127);
        ok(events.add(0, on.bytes()));
    }
    ok(synth.process(out.block(), &events));
    require(synth.activeVoices() == 2 && synth.stolenVoices() == 1,
            "deterministic polyphonic voice stealing");
    struct Audit {
        Synthesizer<T>& synth;
        AudioBuffer<T>& out;
    };
    Audit audit{synth, out};
    runRealtimeAudit(
        [](void* p) noexcept {
            auto& a = *static_cast<Audit*>(p);
            for (unsigned n = 0; n < 4; ++n)
                (void)a.synth.process(a.out.block());
            a.synth.reset();
        },
        &audit);
    struct Custom {
        unsigned starts = 0, stops = 0;
    } custom;
    SynthVoiceRenderer<T> renderer;
    renderer.context = &custom;
    renderer.start = [](void* p, const SynthVoiceInfo&) noexcept {
        ++static_cast<Custom*>(p)->starts;
    };
    renderer.stop = [](void* p, const SynthVoiceInfo&) noexcept {
        ++static_cast<Custom*>(p)->stops;
    };
    renderer.render = [](void*, const SynthVoiceInfo&, AudioBlock<T> block) noexcept {
        for (auto& value : block.channel(0))
            value = T{1};
        return Status::ok;
    };
    ok(synth.setRenderer(renderer));
    for (std::size_t ch = 0; ch < 2; ++ch)
        std::fill(out.block().channel(ch).begin(), out.block().channel(ch).end(), T{.25});
    events.reset();
    on = midiNoteOn(1, 60, 127);
    const auto mute = midiController(1, 7, 0);
    off = midiNoteOff(1, 60);
    ok(events.add(0, on.bytes()));
    ok(events.add(48, mute.bytes()));
    ok(events.add(96, off.bytes()));
    ok(synth.process(out.block(), &events, nullptr, true));
    for (std::size_t i = 0; i < 128; ++i)
        require(std::abs(out.block().channel(0)[i] - (.25 + (i < 48 ? std::sqrt(.5) : 0))) < 1e-6,
                "custom renderer/additive mix/sample-accurate controller");
    require(custom.starts == 1 && custom.stops == 1, "custom voice lifecycle callbacks");
    synth.reset();
}
template <Sample T> void transport() {
    AudioBuffer<T> data, out;
    ok(data.resize(1, 20));
    ok(out.resize(1, 16));
    for (std::size_t i = 0; i < 20; ++i)
        data.block().channel(0)[i] = static_cast<T>(i);
    MemoryAudioSource<T> source;
    ok(source.prepare(data.block(), 48000));
    AudioTransport<T> player;
    ok(player.prepare({48000, 16, 1}, source));
    ok(player.enqueue({TransportAction::setLoop, 3, 7, true}));
    ok(player.enqueue({TransportAction::play}));
    ok(player.process(out.block()));
    for (std::size_t i = 0; i < 16; ++i)
        require(out.block().channel(0)[i] == static_cast<T>(3 + i % 4),
                "transport loop exact frames");
    TransportPosition position;
    ok(player.position(position));
    require(position.frame == 3 && position.playing, "transport snapshot");
    ok(player.enqueue({TransportAction::pause}));
    ok(player.process(out.block()));
    for (auto sample : out.block().channel(0))
        require(sample == 0, "paused transport silence");
    const auto before = data.block().channel(0)[0];
    std::size_t n = 0;
    require(source.readAt(0, data.block(), n) == Status::partial_overlap &&
                data.block().channel(0)[0] == before,
            "source alias rejected before writes");
    struct Audit {
        AudioTransport<T>& player;
        AudioBuffer<T>& out;
    };
    Audit audit{player, out};
    runRealtimeAudit(
        [](void* p) noexcept {
            auto& a = *static_cast<Audit*>(p);
            (void)a.player.enqueue({TransportAction::play});
            (void)a.player.process(a.out.block());
            a.player.reset();
        },
        &audit);
    struct Missing final : AudioSource<T> {
        AudioSourceInfo info() const noexcept override {
            return {48000, 1000, 1};
        }
        Status readAt(std::uint64_t, AudioBlock<T> block, std::size_t& count) noexcept override {
            count = std::min(std::size_t{2}, block.frames());
            for (std::size_t i = 0; i < block.frames(); ++i)
                block.channel(0)[i] = i < count ? T{1} : T{};
            return Status::would_block;
        }
    } missing;
    ok(player.prepare({48000, 16, 1}, missing));
    ok(player.enqueue({TransportAction::play}));
    ok(player.process(out.block(), UnderrunPolicy::hold));
    ok(player.position(position));
    require(position.frame == 2 && position.underruns == 1, "transport underrun hold timeline");
    player.reset();
    ok(player.enqueue({TransportAction::play}));
    ok(player.process(out.block(), UnderrunPolicy::silenceAndAdvance));
    ok(player.position(position));
    require(position.frame == 16 && position.underruns == 1 && out.block().channel(0)[2] == 0,
            "transport underrun advances silent timeline");
}
void tempo() {
    TempoMap map;
    const std::array<TempoPoint, 3> points{{{0, 120, 4, 4}, {2, 60, 4, 4}, {8, 90, 3, 4}}};
    ok(map.prepare(48000, points));
    std::uint64_t frame = 0;
    ok(map.frameAtBeat(4, frame));
    require(frame == 144000, "tempo integrated frame clock");
    const auto p = map.position(96000);
    require(p.bar == 0 && std::abs(p.beatInBar - 3) < 1e-12, "tempo change preserves bar phase");
    ok(map.frameAtBeat(8, frame));
    require(map.position(frame).numerator == 3 && map.position(frame).bar == 2,
            "time-signature boundary");
}
} // namespace
void dawRegression() {
    ump();
    mpe();
    fft<float>();
    fft<double>();
    synth<float>();
    synth<double>();
    transport<float>();
    transport<double>();
    tempo();
}
