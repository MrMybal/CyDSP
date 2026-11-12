#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <cydsp/CyDSP.h>
#include <iostream>
#include <stdexcept>
#include <thread>
#if defined(CYDSP_TEST_IO) && !defined(_WIN32)
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool v, const char* message) {
    if (!v)
        throw std::runtime_error(message);
}
void ok(Status s) {
    require(s == Status::ok, "MIDI2 status");
}
std::span<const std::uint8_t> bytes(std::string_view s) {
    return {reinterpret_cast<const std::uint8_t*>(s.data()), s.size()};
}
void codecs() {
    std::array<std::uint8_t, 128> output{};
    std::size_t count = 0;
    const CiMessageView msg{0x7f, 0x20, 2, 0x0204081, 0x0306081, {}};
    ok(makeCiMessage(msg, output, count));
    // Independent 7-bit little-endian MUID representation.
    require(count == 13 && output[5] == 1 && output[6] == 1 && output[7] == 1 && output[8] == 1,
            "CI source MUID wire order");
    CiMessageView decoded;
    ok(parseCiMessage({output.data(), count}, decoded));
    require(decoded.source == msg.source && decoded.destination == msg.destination,
            "CI header roundtrip");
    const auto before = output;
    auto bad = msg;
    bad.source = 0x0fffff00;
    require(makeCiMessage(bad, output, count) == Status::invalid_argument && output == before,
            "CI reserved MUID rejects before writes");
    const std::array<std::uint8_t, 7> binary{0x80, 0x01, 0xff, 0x7f, 0xaa, 0x55, 0xcc};
    const std::array<std::uint8_t, 8> packed{0x55, 0, 1, 0x7f, 0x7f, 0x2a, 0x55, 0x4c};
    ok(ciEncodeMcoded7(binary, output, count));
    require(count == packed.size() && std::equal(packed.begin(), packed.end(), output.begin()),
            "independent Mcoded7 sign-bit order");
    std::array<std::uint8_t, 16> restored{};
    ok(ciDecodeMcoded7(packed, restored, count));
    require(count == 7 && std::equal(binary.begin(), binary.end(), restored.begin()),
            "Mcoded7 decode");
    ok(ciEncodeMcoded7(std::span(binary).first(3), output, count));
    require(count == 4 && output[0] == 0x50, "partial Mcoded7 high sign bits");
    output[0] |= 1;
    require(ciDecodeMcoded7({output.data(), 4}, restored, count) == Status::invalid_argument,
            "Mcoded7 unused bits rejected");
    CiDiscovery discovered;
    discovered.identity.family = 0x1234;
    discovered.maximumSysExBytes = 512;
    discovered.categories = 0x0c;
    discovered.outputPath = 5;
    ok(makeCiDiscoveryBody(discovered, true, 2, output, count));
    require(count == 18 && output[3] == 0x34 && output[4] == 0x24 && output[12] == 0 &&
                output[13] == 4 && output[16] == 5 && output[17] == 0x7f,
            "CI discovery independent fields");
    CiMessageView reply{0x7f, 0x71, 2, 1, 2, {output.data(), count}};
    CiDiscovery parsed;
    ok(parseCiDiscovery(reply, parsed));
    require(parsed.identity.family == 0x1234 && parsed.maximumSysExBytes == 512,
            "CI discovery parse");
    CiPropertyAssembler assembler;
    ok(assembler.prepare(2, 128, 64));
    const auto header = bytes("{\"status\":200}");
    const std::array<std::uint8_t, 3> a{1, 2, 3}, b{4, 5, 6};
    CiPropertyMessage whole;
    bool complete = false;
    auto append = [&](unsigned id, unsigned total, unsigned number,
                      std::span<const std::uint8_t> data) {
        CiPropertyChunk c{static_cast<std::uint8_t>(id), static_cast<std::uint16_t>(total),
                          static_cast<std::uint16_t>(number),
                          number == 1 ? header : std::span<const std::uint8_t>{}, data};
        ok(makeCiPropertyBody(c, output, count));
        return assembler.append({0x7f, 0x35, 2, 1, 2, {output.data(), count}}, 100, whole,
                                complete);
    };
    ok(append(1, 0, 1, a));
    require(!complete, "unknown property chunk count");
    ok(append(2, 1, 1, b));
    require(complete && whole.request == 2, "interleaved PE request isolation");
    ok(append(1, 2, 2, b));
    require(complete && whole.data.size() == 6 && whole.data[5] == 6,
            "PE unknown final chunk completion");
    ok(append(1, 3, 1, a));
    require(append(1, 3, 3, b) == Status::invalid_argument, "out-of-order PE invalidates assembly");
}
struct CiWire {
    struct Packet {
        std::array<std::uint8_t, 4096> data{};
        std::size_t size = 0;
    };
    std::vector<Packet> packets = std::vector<Packet>(16);
    std::size_t read = 0, write = 0, count = 0, limit = 2;
    MidiCiDevice* owner = nullptr;
    std::vector<std::uint8_t> reply = std::vector<std::uint8_t>(4096),
                              received = std::vector<std::uint8_t>(8192);
    std::size_t receivedSize = 0;
    unsigned properties = 0, profiles = 0, timeouts = 0, lastType = 0;
    bool server = false;
    static Status send(void* ptr, unsigned group, std::span<const std::uint8_t> data) noexcept {
        auto& s = *static_cast<CiWire*>(ptr);
        if (group != 3 || data.size() > 4096)
            return Status::invalid_argument;
        if (s.count == s.limit)
            return Status::update_queue_full;
        auto& p = s.packets[s.write++ % s.packets.size()];
        p.size = data.size();
        std::copy(data.begin(), data.end(), p.data.begin());
        ++s.count;
        return Status::ok;
    }
    static Status property(void* ptr, const CiPropertyMessage& m) noexcept {
        auto& s = *static_cast<CiWire*>(ptr);
        ++s.properties;
        if (s.server && m.type == 0x34)
            return s.owner->sendProperty(m.source, 0x35, m.request, bytes("{\"status\":200}"),
                                         s.reply);
        if (m.data.size() > s.received.size())
            return Status::block_too_large;
        s.receivedSize = m.data.size();
        std::copy(m.data.begin(), m.data.end(), s.received.begin());
        return Status::ok;
    }
    CiCallbacks callbacks() {
        return {this,
                &send,
                nullptr,
                [](void* p, std::uint32_t, const CiProfile&) noexcept {
                    ++static_cast<CiWire*>(p)->profiles;
                },
                &property,
                [](void* p, const CiMessageView& m) noexcept {
                    static_cast<CiWire*>(p)->lastType = m.type;
                },
                [](void* p, std::uint32_t, std::uint8_t) noexcept {
                    ++static_cast<CiWire*>(p)->timeouts;
                }};
    }
    bool deliver(MidiCiDevice& other, std::uint64_t now) {
        if (!count)
            return false;
        const auto& p = packets[read % packets.size()];
        ok(other.process({p.data.data(), p.size}, now));
        ++read;
        --count;
        return true;
    }
    void clear() {
        read = write = count = 0;
    }
};
void ciDevice() {
    MidiCiDevice a, b;
    CiWire x, y;
    x.owner = &a;
    y.owner = &b;
    y.server = true;
    for (std::size_t i = 0; i < y.reply.size(); ++i)
        y.reply[i] = static_cast<std::uint8_t>(i % 128);
    CiConfiguration config;
    config.group = 3;
    config.muid = 1;
    config.discovery.maximumSysExBytes = 512;
    config.propertyExchange = true;
    config.productInstanceId = "CyDSP test";
    CiProfile profile{{0x7e, 1, 1, 1, 0}, 0x7f, false, 0};
    ok(a.prepare(config, x.callbacks()));
    config.muid = 2;
    ok(b.prepare(config, y.callbacks(), {&profile, 1}));
    auto drain = [&](std::uint64_t now) {
        for (unsigned i = 0; i < 2048; ++i) {
            a.poll(now);
            b.poll(now);
            const bool sentLeft = x.deliver(b, now);
            const bool sentRight = y.deliver(a, now);
            const bool sent = sentLeft || sentRight;
            if (!sent && !x.count && !y.count)
                return;
        }
        throw std::runtime_error("CI pump did not quiesce");
    };
    ok(a.discover());
    drain(0);
    require(a.peers().size() == 1 && b.peers().size() == 1 && a.peers()[0].muid == 2 &&
                a.peers()[0].discovery.maximumSysExBytes == 512,
            "bidirectional CI discovery");
    ok(a.requestProfiles(2));
    drain(0);
    require(x.lastType == 0x21, "CI profile lists");
    ok(a.setRemoteProfile(2, profile, true));
    drain(0);
    require(y.profiles == 1 && x.profiles == 2 && x.lastType == 0x24,
            "CI profile enable and broadcast report");
    ok(a.requestPropertyCapabilities(2));
    drain(0);
    require(x.lastType == 0x31, "CI PE capabilities");
    std::uint8_t id = 127;
    ok(a.requestProperty(2, 0x34, bytes("{\"resource\":\"DeviceInfo\"}"), {}, 100, id));
    drain(100);
    require(x.properties == 1 && x.receivedSize == y.reply.size() &&
                std::equal(y.reply.begin(), y.reply.end(), x.received.begin()),
            "CI chunked PE reply survives bounded transport backpressure");
    require(a.lastSendError() == Status::ok && b.lastSendError() == Status::ok,
            "CI retained transfer success");
    ok(a.send(2, 0x10));
    drain(200);
    require(x.lastType == 0x7f, "deprecated/unsupported CI request NAK");
    ok(a.requestProperty(2, 0x34, bytes("{\"resource\":\"DeviceInfo\"}"), {}, 1000, id));
    a.poll(3999);
    require(!x.timeouts, "PE timeout lower bound");
    a.poll(4000);
    require(x.timeouts == 1, "PE timeout and request reclamation");
    x.clear();
    // A retained transfer must not allocate while accepted by a saturated transport.
    struct PropertyAudit {
        MidiCiDevice& device;
        CiWire& wire;
        std::span<const std::uint8_t> data;
        bool good = false;
    } propertyAudit{a, x, y.reply};
    runRealtimeAudit(
        [](void* ptr) noexcept {
            auto& t = *static_cast<PropertyAudit*>(ptr);
            t.good = t.device.sendProperty(2, 0x36, 1, bytes("{\"resource\":\"test\"}"), t.data) ==
                     Status::ok;
            t.device.poll(4000);
            t.device.reset();
        },
        &propertyAudit);
    require(propertyAudit.good, "PE backpressure retains memory");
    x.clear();
    std::array<std::uint8_t, 64> packet{};
    std::size_t n = 0;
    ok(makeCiMessage({0x7f, 0x20, 2, 2, 1, {}}, packet, n));
    struct Audit {
        MidiCiDevice& device;
        CiWire& wire;
        std::span<const std::uint8_t> message;
        bool good = true;
    };
    Audit audit{a, x, {packet.data(), n}};
    runRealtimeAudit(
        [](void* p) noexcept {
            auto& t = *static_cast<Audit*>(p);
            t.wire.clear();
            t.good = t.device.process(t.message, 5000) == Status::ok;
            t.device.poll(5000);
            t.device.reset();
        },
        &audit);
    require(audit.good, "CI processing/reset retains memory");
    // Colliding session MUIDs must invalidate and regenerate, not cache themselves.
    MidiCiDevice c, d;
    CiWire u, v;
    u.owner = &c;
    v.owner = &d;
    u.limit = v.limit = 16;
    config.muid = 7;
    ok(c.prepare(config, u.callbacks()));
    ok(d.prepare(config, v.callbacks()));
    ok(c.discover());
    for (unsigned i = 0; i < 100; ++i) {
        const bool sentLeft = u.deliver(d, 0);
        const bool sentRight = v.deliver(c, 0);
        const bool sent = sentLeft || sentRight;
        if (!sent)
            break;
    }
    require(c.muid() != 7 && d.muid() != 7 && c.muid() != d.muid() && validCiMuid(c.muid()) &&
                validCiMuid(d.muid()),
            "MUID collision recovery");
}
void ciProgressTimeout() {
    MidiCiDevice a, b;
    CiWire x, y;
    x.owner = &a;
    y.owner = &b;
    CiConfiguration config;
    config.group = 3;
    config.muid = 1;
    config.propertyExchange = true;
    config.discovery.maximumSysExBytes = 512;
    ok(a.prepare(config, x.callbacks()));
    config.muid = 2;
    ok(b.prepare(config, y.callbacks()));
    std::uint8_t id = 0;
    ok(a.requestProperty(2, 0x36, bytes("{\"resource\":\"test\"}"), x.reply, 0, id));
    std::uint64_t now = 0;
    for (; x.count && now < 20000; now += 1000) {
        a.poll(now);
        require(x.timeouts == 0, "outgoing PE progress refreshes the reply deadline");
        x.deliver(b, now);
    }
    require(y.properties == 1 && y.receivedSize == x.reply.size() && now > 3000,
            "long PE request survives progressing backpressure");
    a.poll(now + 3000);
    require(x.timeouts == 1, "stalled PE reply expires after last outgoing progress");
}
void endpoint() {
    const auto discovery = umpEndpointDiscovery();
    require(discovery.words[0] == 0xf0000101 && discovery.words[1] == 0x11,
            "UMP discovery independent words");
    const auto info = umpEndpointInfo({true, true, false, false});
    require(info.words[0] == 0xf0010101 && info.words[1] == 0x80000300,
            "UMP endpoint info independent words");
    const auto request = umpStreamConfiguration({2, false, false});
    require(request.words[0] == 0xf0050200, "UMP stream configuration independent word");
    struct Queue {
        std::array<UmpPacket, 16> packets{};
        std::size_t size = 0;
        static Status send(void* p, const UmpPacket& packet) noexcept {
            auto& q = *static_cast<Queue*>(p);
            if (q.size == q.packets.size())
                return Status::update_queue_full;
            q.packets[q.size++] = packet;
            return Status::ok;
        }
    } x, y;
    UmpEndpointNegotiator a, b;
    ok(a.prepare({}, &Queue::send, &x));
    ok(b.prepare({}, &Queue::send, &y));
    ok(a.negotiate(2, 0));
    ok(b.process(x.packets[0], 0));
    require(y.size == 2, "discovery filter replies");
    ok(a.process(y.packets[0], 0));
    ok(a.process(y.packets[1], 0));
    require(a.pending() && a.localConfiguration().protocol == 1 && x.size == 2,
            "initiator waits for protocol confirmation");
    ok(b.process(x.packets[1], 0));
    ok(a.process(y.packets[2], 0));
    require(!a.pending() && a.localConfiguration().protocol == 2 &&
                b.localConfiguration().protocol == 2,
            "UMP negotiated MIDI2");
    ok(a.negotiate(1, 100));
    require(a.poll(3100) == Status::timed_out && !a.pending(), "UMP negotiation timeout");
    runRealtimeAudit(
        [](void* p) noexcept {
            auto& n = *static_cast<UmpEndpointNegotiator*>(p);
            n.reset();
            (void)n.poll(0);
        },
        &a);
}
#if defined(CYDSP_TEST_IO)
// Two explicit native loopbacks stand in for a physical duplex link. The host routes
// each received stream to the other endpoint, including complete CI fragmentation.
void nativeCiRoundtrip() {
    Midi2Device x, y;
    ok(x.openLoopback());
    ok(y.openLoopback());
    MidiCiDevice a, b;
    auto callbacks = [](Midi2Device& io) {
        CiCallbacks c;
        c.context = &io;
        c.send = [](void* ptr, unsigned group, std::span<const std::uint8_t> payload) noexcept {
            return static_cast<Midi2Device*>(ptr)->sendSysEx7(group, payload);
        };
        return c;
    };
    CiConfiguration config;
    config.muid = 1;
    ok(a.prepare(config, callbacks(x)));
    config.muid = 2;
    ok(b.prepare(config, callbacks(y)));
    ok(a.discover());
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while ((a.peers().empty() || b.peers().empty()) &&
           std::chrono::steady_clock::now() < deadline) {
        Midi2TimedPacket p;
        while (x.read(p) == Status::ok)
            ok(b.processUmp(p.packet, 0));
        while (y.read(p) == Status::ok)
            ok(a.processUmp(p.packet, 0));
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    require(a.peers().size() == 1 && b.peers().size() == 1 && a.peers()[0].muid == 2 &&
                b.peers()[0].muid == 1,
            "native CI discover/sendSysEx7/framing roundtrip");
    x.close();
    y.close();
}
void nativeQueues() {
    Midi2Device device;
    Midi2DeviceOptions options;
    options.incomingPackets = 4096;
    options.outgoingPackets = 4096;
    options.queuedSysEx = 32;
    ok(device.openLoopback(options));
    std::thread producer([&] {
        for (unsigned m = 0; m < 20; ++m) {
            std::array<std::uint8_t, 257> data{};
            for (std::size_t i = 0; i < data.size(); ++i)
                data[i] = static_cast<std::uint8_t>((i + m) % 128);
            while (device.sendSysEx7(3, data) == Status::update_queue_full)
                std::this_thread::yield();
        }
    });
    for (unsigned n = 0; n < 1000; ++n)
        ok(device.send(umpController(0, 1, 7, n)));
    unsigned notes = 0, messages = 0;
    UmpSysExAssembler assembler;
    ok(assembler.prepare(512));
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while ((notes < 1000 || messages < 20) && std::chrono::steady_clock::now() < deadline) {
        Midi2TimedPacket p;
        const auto status = device.read(p);
        if (status == Status::would_block) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        ok(status);
        if (p.packet.type() == 4) {
            require(p.packet.words[1] == notes++, "native outgoing single-producer order");
        } else if (p.packet.type() == 3) {
            ok(assembler.append(p.packet));
            if (!assembler.message().empty()) {
                const auto body = assembler.message();
                require(body.size() == 257, "atomic queued SysEx length");
                for (std::size_t i = 0; i < body.size(); ++i)
                    require(body[i] == (i + messages) % 128,
                            "SysEx payload across interleaved notes");
                ++messages;
            }
        }
    }
    producer.join();
    require(notes == 1000 && messages == 20 && device.droppedPackets() == 0,
            "parallel note/CI producers and shared UMP consumer");
    device.close();
    options.incomingPackets = 4;
    options.outgoingPackets = 1;
    options.queuedSysEx = 1;
    ok(device.openLoopback(options));
    const auto future = midi2TimestampNow() + 60000000000ull;
    const auto note = umpNote(true, 0, 1, 60, 65535);
    struct Audit {
        Midi2Device& device;
        UmpPacket note;
        std::uint64_t time;
        bool good = false;
    };
    Audit audit{device, note, future};
    runRealtimeAudit(
        [](void* p) noexcept {
            auto& a = *static_cast<Audit*>(p);
            Midi2TimedPacket packet;
            const std::array<std::uint8_t, 3> bytes{1, 2, 3};
            a.good = a.device.send(a.note, a.time) == Status::ok &&
                     a.device.send(a.note, a.time) == Status::update_queue_full &&
                     a.device.sendSysEx7(0, bytes, a.time) == Status::ok &&
                     a.device.sendSysEx7(0, bytes, a.time) == Status::update_queue_full &&
                     a.device.read(packet) == Status::would_block;
        },
        &audit);
    require(audit.good, "native bounded realtime queues and backpressure");
    device.close();
    std::vector<Midi2EndpointInfo> endpoints;
    const auto availability = enumerateMidi2Endpoints(endpoints);
    require(availability == Status::ok || availability == Status::unsupported_backend ||
                availability == Status::device_unavailable,
            "native MIDI2 discovery status");
    std::cout << "MIDI2 native enumeration status=" << static_cast<int>(availability)
              << " endpoints=" << endpoints.size() << " (no hardware opened)\n";
}
#if !defined(_WIN32)
void descriptors() {
    struct Sockets {
        int pair[2]{-1, -1};
        ~Sockets() {
            for (auto fd : pair)
                if (fd >= 0)
                    ::close(fd);
        }
    } s;
    require(socketpair(AF_UNIX, SOCK_STREAM, 0, s.pair) == 0, "UMP socket pair");
    for (auto fd : s.pair) {
        const auto flags = fcntl(fd, F_GETFL);
        require(flags >= 0 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0,
                "private fixture nonblocking descriptors");
#if defined(SO_NOSIGPIPE)
        const int one = 1;
        setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif
    }
    const int smallBuffer = 1024;
    require(setsockopt(s.pair[0], SOL_SOCKET, SO_SNDBUF, &smallBuffer, sizeof(smallBuffer)) == 0,
            "bounded private socket output forces native backpressure");
    Midi2Device device;
    Midi2DeviceOptions options;
    options.incomingPackets = 128;
    options.outgoingPackets = 4096;
    ok(device.openFileDescriptors(s.pair[0], s.pair[0], options));
    const auto input = umpNote(true, 9, 3, 62, 0x1234);
    const auto* data = reinterpret_cast<const std::uint8_t*>(input.words.data());
    for (std::size_t pos = 0; pos < 8;) {
        const auto n = std::min(std::size_t{3}, 8 - pos);
        require(::write(s.pair[1], data + pos, n) == static_cast<ssize_t>(n),
                "fragmented native input write");
        pos += n;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    Midi2TimedPacket received;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    Status status;
    do {
        status = device.read(received);
        if (status == Status::would_block)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (status == Status::would_block && std::chrono::steady_clock::now() < deadline);
    ok(status);
    require(received.packet.words == input.words && received.timestampNs > 0,
            "native-endian UMP fd ingress and fragmented words");
    for (unsigned i = 0; i < 2000; ++i)
        ok(device.send(umpController(0, 1, 7, i)));
    std::array<std::uint8_t, 4096> payload{};
    for (std::size_t i = 0; i < payload.size(); ++i)
        payload[i] = static_cast<std::uint8_t>(i % 128);
    ok(device.sendSysEx7(3, payload));
    // Force OS backpressure before draining; notes and CI must never share partial packet offsets.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    struct Receiver {
        unsigned notes = 0, sysex = 0;
        bool good = true;
        UmpSysExAssembler assembler;
    };
    Receiver result;
    ok(result.assembler.prepare(4096));
    UmpStreamParser parser;
    std::array<std::uint8_t, 4> partial{};
    std::size_t have = 0;
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while ((result.notes < 2000 || !result.sysex) && std::chrono::steady_clock::now() < deadline) {
        std::array<std::uint8_t, 257> buffer{};
        const auto n = ::read(s.pair[1], buffer.data(), buffer.size());
        if (n <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        for (ssize_t i = 0; i < n; ++i) {
            partial[have++] = buffer[static_cast<std::size_t>(i)];
            if (have == 4) {
                std::uint32_t word;
                std::memcpy(&word, partial.data(), 4);
                ok(parser.feed(
                    {&word, 1},
                    [](void* p, const UmpPacket& packet) noexcept {
                        auto& r = *static_cast<Receiver*>(p);
                        if (packet.type() == 4)
                            r.good &= packet.words[1] == r.notes++;
                        else if (packet.type() == 3) {
                            r.good &= r.assembler.append(packet) == Status::ok;
                            if (!r.assembler.message().empty()) {
                                const auto b = r.assembler.message();
                                r.good &= b.size() == 4096;
                                for (std::size_t j = 0; j < b.size(); ++j)
                                    r.good &= b[j] == j % 128;
                                ++r.sysex;
                            }
                        } else
                            r.good = false;
                    },
                    &result));
                have = 0;
            }
        }
    }
    require(result.good && result.notes == 2000 && result.sysex == 1,
            "native fd output/backpressure/CI framing");
    device.close();
    require(fcntl(s.pair[0], F_GETFD) >= 0, "native descriptors remain caller-owned");
    ok(device.openFileDescriptors(s.pair[0], s.pair[0]));
    ::close(s.pair[1]);
    s.pair[1] = -1;
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (device.lastError() == Status::ok && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    require(device.lastError() != Status::ok, "native disconnect error propagation");
    device.close();
}
#endif
#endif
} // namespace
void midi2ProtocolRegression() {
    codecs();
    ciDevice();
    ciProgressTimeout();
    endpoint();
}
#if defined(CYDSP_TEST_IO)
void midi2NativeRegression() {
    nativeCiRoundtrip();
    nativeQueues();
#if !defined(_WIN32)
    descriptors();
#endif
}
#endif
