#include <cmath>
#include <cydsp/CyDSP.h>
#include <iostream>
#include <stdexcept>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool v, const char* why) {
    if (!v)
        throw std::runtime_error(why);
}
void ok(Status r) {
    require(r == Status::ok, "DAW MIDI status");
}
void clocks() {
    MidiEventCollector collector;
    MidiCollectorOptions options;
    options.messages = 8;
    options.maximumMessageBytes = 16;
    ok(collector.prepare(options));
    MidiBuffer midi;
    UmpBuffer ump;
    ok(midi.prepare(8, 128));
    ok(ump.prepare(8));
    auto on = midiNoteOn(1, 60, 100), off = midiNoteOff(1, 60);
    ok(collector.pushMidi(1003000000, on.bytes()));
    ok(collector.pushMidi(999000000, off.bytes()));
    ok(collector.pushUmp(1002000000, umpNote(true, 0, 1, 64, 65535)));
    ok(collector.pushMidi(1008000000, off.bytes()));
    ok(collector.collect({1000000000, 100, 1000}, 8, midi, ump));
    require(midi.size() == 2 && midi.event(0).frame == 0 && midi.event(1).frame == 3 &&
                ump.events().size() == 1 && ump.events()[0].frame == 2,
            "arrival sort/late clamp");
    require(collector.lateMessages() == 1, "late counter");
    midi.reset();
    ump.reset();
    ok(collector.collect({1008000000, 108, 1000}, 8, midi, ump));
    require(midi.size() == 1 && midi.event(0).frame == 0, "future event boundary retained");
    collector.reset();
    midi.reset();
    ump.reset();
    for (unsigned i = 0; i < 8; ++i)
        ok(collector.pushMidi(1000000000, on.bytes()));
    require(collector.pushMidi(1000000000, on.bytes()) == Status::update_queue_full &&
                collector.overflows() == 1,
            "collector bounded slots");
    MidiBuffer small;
    ok(small.prepare(1, 16));
    ok(collector.collect({1000000000, 0, 1000}, 1, small, ump) == Status::update_queue_full
           ? Status::ok
           : Status::invalid_argument);
    require(small.size() == 1, "saturation retains remainder");
    small.reset();
    ok(collector.collect({1000000000, 0, 1000}, 1, midi, ump));
    require(midi.size() == 7, "retained events after output saturation");
    collector.reset();
    midi.reset();
    ump.reset();
    ok(collector.pushMidi(9000000000, on.bytes()));
    ok(collector.collect({1000000000, 0, 48000}, 1, midi, ump));
    collector.invalidate();
    ok(collector.pushMidi(1000000000, off.bytes()));
    ok(collector.collect({1000000000, 0, 48000}, 1, midi, ump));
    require(midi.size() == 1 && midi.event(0).bytes[0] == 0x80, "old future epoch discarded");
    collector.reset();
    midi.reset();
    ump.reset();
    struct Audit {
        MidiEventCollector* c;
        MidiBuffer* m;
        UmpBuffer* u;
        Status status = Status::ok;
    } audit{&collector, &midi, &ump};
    runRealtimeAudit(
        [](void* ptr) noexcept {
            auto& a = *static_cast<Audit*>(ptr);
            const auto on = midiNoteOn(1, 60, 100);
            a.status = a.c->pushMidi(2000000000, on.bytes());
            if (a.status == Status::ok)
                a.status = a.c->collect({2000000000, 0, 48000}, 128, *a.m, *a.u);
            a.c->reset();
        },
        &audit);
    ok(audit.status);
}

void extensions() {
    UmpFunctionBlockInfo info;
    info.number = 2;
    info.uiHint = 1;
    info.direction = 3;
    info.firstGroup = 4;
    auto packet = umpFunctionBlockInfo(info);
    require(packet.words == std::array<std::uint32_t, 4>{0xf0118213, 0x04010201, 0, 0},
            "Function Block literal");
    UmpFunctionBlockInfo parsed;
    ok(parseUmpFunctionBlockInfo(packet, parsed));
    require(parsed.number == 2 && parsed.firstGroup == 4, "Function Block parse");
    require(umpFunctionBlockDiscovery().words[0] == 0xf010ff03, "discovery literal");
    require(umpTempo(2, 50000000).words == std::array<std::uint32_t, 4>{0xd2100000, 50000000, 0, 0},
            "tempo literal");
    std::uint32_t tempo = 0;
    ok(parseUmpTempo(umpTempo(2, 50000000), tempo));
    require(tempo == 50000000, "tempo parse");
    require(umpTimeSignature(0, {7, 3, 8}).words[1] == 0x07030800, "signature literal");
    require(umpKeySignature(0, 1, {-2, 3}).words[1] == 0xe3000000, "key literal");
    UmpChord chord;
    chord.tonic = 3;
    chord.type = 1;
    auto ch = umpChord(0, 1, chord);
    UmpChord decoded;
    ok(parseUmpChord(ch, decoded));
    require(ch.words[1] == 0x03010000 && ch.words[2] == 0x00008000 && decoded.tonic == 3,
            "chord literal");
    CiIdentity identity;
    identity.manufacturer = {0, 0x20, 0x33};
    identity.family = 0x1234;
    identity.model = 0x321;
    auto id = umpDeviceIdentity(identity);
    require(id.words[1] == 0x002033 && id.words[2] == 0x34242106, "identity literal");
    CiIdentity copy;
    ok(parseUmpDeviceIdentity(id, copy));
    require(copy.family == identity.family, "identity parse");
    std::array<UmpPacket, 8> text;
    std::size_t used = 0;
    const std::string title = "CyDSP \xc3\xa9\xc3\xa9 audio metadata crossing packets";
    UmpTextHeader h;
    h.kind = UmpTextKind::metadata;
    h.status = 2;
    ok(makeUmpText(h, title, text, used));
    require(used > 1, "fragmented text");
    UmpTextAssembler a;
    ok(a.prepare(128));
    for (std::size_t i = 0; i < used; ++i)
        ok(a.append(text[i]));
    require(a.complete() && a.text() == title, "UTF8 fragmented assembly");
    auto saved = text;
    const auto count = used;
    require(makeUmpText(h, "\xc0\x80", text, used) == Status::invalid_argument &&
                text[0].words == saved[0].words,
            "invalid UTF8 transactional");
    used = count;
    struct Sender {
        std::array<UmpPacket, 64> packets;
        std::size_t size = 0;
        bool blocked = true;
    } sink;
    auto send = [](void* p, const UmpPacket& v) noexcept {
        auto& x = *static_cast<Sender*>(p);
        if (x.blocked)
            return Status::would_block;
        if (x.size == x.packets.size())
            return Status::update_queue_full;
        x.packets[x.size++] = v;
        return Status::ok;
    };
    UmpEndpointDescription d;
    d.name = "CyDSP";
    d.productInstanceId = "original-test";
    d.capabilities.staticFunctionBlocks = false;
    d.blocks.push_back({{}, "Synth"});
    UmpEndpointService service;
    ok(service.prepare(d, send, &sink));
    ok(service.process(umpEndpointDiscovery(31), 0));
    const auto queued = service.queuedReplies();
    require(queued >= 5 && service.poll(0) == Status::would_block &&
                service.queuedReplies() == queued,
            "reply retained on full transport");
    sink.blocked = false;
    ok(service.poll(1));
    UmpEndpointCapabilities caps;
    ok(parseUmpEndpointInfo(sink.packets[0], caps));
    require(caps.functionBlocks == 1 && !caps.staticFunctionBlocks, "endpoint advertises blocks");
    ok(service.process(umpFunctionBlockDiscovery(), 2));
    ok(service.poll(2));
    require(sink.packets[queued].words[0] == 0xf0118003, "block reply");
    auto changed = d.blocks[0].info;
    changed.active = false;
    ok(service.updateFunctionBlock(changed));
    ok(service.poll(3));
    struct Audit {
        UmpTextAssembler* a;
        std::array<UmpPacket, 8>* p;
        std::size_t n;
        UmpEndpointService* service;
        Status r = Status::ok;
    } audit{&a, &text, used, &service};
    runRealtimeAudit(
        [](void* ptr) noexcept {
            auto& x = *static_cast<Audit*>(ptr);
            x.a->reset();
            for (std::size_t i = 0; i < x.n; ++i)
                x.r = x.a->append((*x.p)[i]);
            x.service->reset();
            if (x.r == Status::ok)
                x.r = x.service->process(umpFunctionBlockDiscovery(), 4);
        },
        &audit);
    ok(audit.r);
}
void controllers() {
    Midi2Controllers c;
    ok(c.prepare(2));
    require(umpParameter(true, 1, 2, 0, 0, 12u << 25).words[0] == 0x41210000, "RPN literal");
    ok(c.process(umpParameter(true, 1, 2, 0, 0, 12u << 25)));
    require(c.channel(1, 2)->pitchRange == 12, "registered pitch range");
    ok(c.process(umpParameter(false, 1, 2, 1, 2, 100)));
    ok(c.process(umpParameter(false, 1, 2, 1, 2, 0xfffffff6u, true)));
    std::uint32_t v = 0;
    ok(c.parameter(false, 1, 2, 1, 2, v));
    require(v == 90, "relative NRPN signed");
    require(c.process(umpParameter(false, 1, 2, 1, 3, 100)) == Status::update_queue_full,
            "bounded parameter storage");
    ok(c.process(umpController(1, 2, 7, 123)));
    ok(c.process(umpController(1, 2, 0, 456)));
    require(c.channel(1, 2)->cc[7] == 123 && c.channel(1, 2)->cc[0] == 0, "atomic CC restrictions");
    unsigned applied = 0;
    CiProfileBinding binding;
    binding.id = {0x7d, 1, 2, 3, 4};
    binding.context = &applied;
    binding.apply = [](void* p, unsigned g, const CiProfile& profile) noexcept {
        if (g == 2 && profile.enabled)
            ++*static_cast<unsigned*>(p);
    };
    CiProfileDispatcher profiles;
    ok(profiles.prepare({&binding, 1}, 2));
    CiProfile profile;
    profile.id = binding.id;
    profile.enabled = true;
    profile.address = 1;
    profile.channels = 1;
    ok(profiles.notify(2, profile));
    require(!applied, "profile deferred to audio boundary");
    ok(profiles.applyPending());
    require(applied == 1, "profile behavior dispatched");
    Synthesizer<double> synth;
    ok(synth.prepare({48000, 32, 1}, 4));
    AudioBuffer<double> out;
    ok(out.resize(1, 32));
    UmpBuffer events;
    ok(events.prepare(16));
    ok(events.add(0, umpNote(true, 0, 1, 69, 65535)));
    ok(events.add(0, umpParameter(true, 0, 1, 0, 7, 12u << 25)));
    ok(events.add(0, umpPitch(0, 1, 0xc0000000u, 69)));
    ok(synth.process(out.block(), nullptr, &events));
    require(std::abs(synth.voice(0).frequency - 440 * std::sqrt(2.0)) < 1e-7,
            "synth per-note range");
    events.reset();
    ok(events.add(0, umpPerNoteManagement(0, 1, 69, true, true)));
    ok(events.add(0, umpPitch(0, 1, 0x80000000u, 69)));
    ok(synth.process(out.block(), nullptr, &events));
    require(std::abs(synth.voice(0).frequency - 440 * std::sqrt(2.0)) < 1e-7,
            "detach before reset freezes per-note state");
    struct Audit {
        Midi2Controllers* c;
        CiProfileDispatcher* p;
        CiProfile profile;
        Status r = Status::ok;
    } audit{&c, &profiles, profile};
    runRealtimeAudit(
        [](void* ptr) noexcept {
            auto& x = *static_cast<Audit*>(ptr);
            x.r = x.c->process(umpParameter(false, 1, 2, 1, 2, 2, true));
            if (x.r == Status::ok)
                x.r = x.p->notify(2, x.profile);
            if (x.r == Status::ok)
                x.r = x.p->applyPending();
            x.c->reset();
        },
        &audit);
    ok(audit.r);
}
#if defined(CYDSP_TEST_IO)
void midiReconnect() {
    struct Fake {
        bool present = true, opened = false, readOne = false, block = false;
        unsigned sent = 0, opens = 0;
        MidiConnectionDriver driver() {
            MidiConnectionDriver d;
            d.context = this;
            d.enumerate = [](void* p, std::vector<MidiConnectionPort>& v) noexcept {
                try {
                    v.clear();
                    if (static_cast<Fake*>(p)->present)
                        v.push_back({"stable", "Device", true, true});
                    return Status::ok;
                } catch (...) {
                    return Status::allocation_failed;
                }
            };
            d.open = [](void* p, const MidiConnectionSelection&) noexcept {
                auto& s = *static_cast<Fake*>(p);
                s.opened = true;
                ++s.opens;
                s.readOne = false;
                return Status::ok;
            };
            d.close = [](void* p) noexcept { static_cast<Fake*>(p)->opened = false; };
            d.health = [](void*) noexcept { return Status::ok; };
            d.read = [](void* p, MidiConnectionMessage& v) noexcept {
                auto& s = *static_cast<Fake*>(p);
                if (s.readOne)
                    return Status::would_block;
                s.readOne = true;
                v = {};
                v.ump = true;
                v.packet = umpNote(true, 0, 1, 60, 65535);
                v.timestampNs = 1000000000;
                return Status::ok;
            };
            d.send = [](void* p, const MidiConnectionMessage&) noexcept {
                auto& s = *static_cast<Fake*>(p);
                if (s.block)
                    return Status::would_block;
                ++s.sent;
                return Status::ok;
            };
            return d;
        }
    } fake;
    MidiConnection connection;
    MidiReconnectOptions o;
    o.scanIntervalMs = 10;
    o.retryIntervalMs = 100;
    o.queuedMessages = 8;
    ok(connection.prepare(fake.driver(), o));
    ok(connection.open({"stable", "stable"}, 0));
    MidiBuffer m;
    UmpBuffer u;
    ok(m.prepare(8, 128));
    ok(u.prepare(8));
    ok(connection.collect({1000000000, 0, 48000}, 128, m, u));
    require(u.events().size() == 1, "device collector");
    MidiConnectionMessage output;
    output.ump = true;
    output.packet = umpNote(false, 0, 1, 60, 0);
    ok(connection.send(output));
    fake.block = true;
    require(connection.poll(1) == Status::would_block, "output backpressure");
    fake.present = false;
    require(connection.poll(10) == Status::device_unavailable && !connection.connected(),
            "MIDI unplug");
    fake.present = true;
    require(connection.poll(20) == Status::device_unavailable && fake.opens == 1,
            "MIDI retry backoff");
    fake.block = false;
    ok(connection.poll(100));
    require(connection.connected() && fake.opens == 2 && fake.sent == 0,
            "stale output discarded after reconnect");
    // Old future events never survive reconnection.
    m.reset();
    u.reset();
    ok(connection.collect({1000000000, 0, 48000}, 128, m, u));
    require(u.events().size() == 1, "new epoch event");
    connection.close();
    ok(connection.poll(1000));
    require(!connection.connected(), "explicit MIDI close");
}
void devices() {
    struct Fake {
        bool present = true, running = false;
        unsigned opens = 0, closes = 0;
        std::uint32_t flags = 0;
        static Status list(void* p, std::vector<AudioDeviceInfo>& out) noexcept {
            try {
                out.clear();
                if (static_cast<Fake*>(p)->present)
                    out.push_back({"output", "Fake", false, true});
                return Status::ok;
            } catch (...) {
                return Status::allocation_failed;
            }
        }
        AudioDeviceDriver driver() {
            AudioDeviceDriver d;
            d.context = this;
            d.enumerate = list;
            d.open = [](void* p, const AudioDeviceConfig&, AudioDeviceCallback, void*) noexcept {
                auto& s = *static_cast<Fake*>(p);
                ++s.opens;
                return s.present ? Status::ok : Status::device_unavailable;
            };
            d.start = [](void* p) noexcept {
                static_cast<Fake*>(p)->running = true;
                return Status::ok;
            };
            d.close = [](void* p) noexcept {
                auto& s = *static_cast<Fake*>(p);
                s.running = false;
                ++s.closes;
            };
            d.running = [](void* p) noexcept { return static_cast<Fake*>(p)->running; };
            d.sampleRate = [](void*) noexcept { return std::uint32_t(48000); };
            d.notifications = [](void* p) noexcept {
                auto& s = *static_cast<Fake*>(p);
                return std::exchange(s.flags, 0);
            };
            return d;
        }
    } driver;
    struct Changes {
        unsigned connected = 0, disconnected = 0, interrupted = 0;
    } changes;
    AudioManagerCallbacks callback;
    callback.context = &changes;
    callback.changed = [](void* p, AudioManagerEvent event, Status) noexcept {
        auto& c = *static_cast<Changes*>(p);
        if (event == AudioManagerEvent::connected)
            ++c.connected;
        if (event == AudioManagerEvent::disconnected)
            ++c.disconnected;
        if (event == AudioManagerEvent::interrupted)
            ++c.interrupted;
    };
    AudioDeviceManager manager;
    ok(manager.prepare(driver.driver(), callback));
    auto render = [](void*, AudioBlock<const float>, AudioBlock<float>, std::uint64_t) noexcept {
        return Status::ok;
    };
    AudioReconnectOptions options;
    options.scanIntervalMs = 10;
    options.retryIntervalMs = 100;
    ok(manager.open({}, render, nullptr, options, 0));
    require(manager.running() && manager.generation() == 1 && manager.sampleRate() == 48000,
            "manager initial stream");
    driver.present = false;
    require(manager.poll(10) == Status::device_unavailable && !manager.running() &&
                changes.disconnected == 1,
            "manager disconnect");
    driver.present = true;
    require(manager.poll(50) == Status::device_unavailable && driver.opens == 1,
            "manager retry backoff");
    ok(manager.poll(100));
    require(manager.running() && manager.generation() == 2 && changes.connected == 2,
            "manager automatic reconnect");
    driver.flags = 8;
    ok(manager.poll(101));
    require(changes.interrupted == 1, "manager serialized interruption callback");
    manager.close();
    ok(manager.poll(1000));
    require(!manager.running(), "manager explicit close stops reconnection");
    MidiInput input;
    MidiOutput output;
    require(input.openVirtual("") == Status::invalid_argument &&
                output.openVirtual("") == Status::invalid_argument,
            "virtual port names validated before backend use");
}
#endif
} // namespace
void dawMidiRegression() {
    clocks();
    extensions();
    controllers();
#if defined(CYDSP_TEST_IO)
    devices();
    midiReconnect();
#endif
    std::cout << "PASS DAW MIDI clock/reconnect/Flex/Stream/controllers/profiles/realtime\n";
}
