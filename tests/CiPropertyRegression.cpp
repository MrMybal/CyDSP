#include <algorithm>
#include <cydsp/CyDSP.h>
#include <iostream>
#include <stdexcept>
#include <string_view>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
void ok(Status status) {
    require(status == Status::ok, "CI property status");
}
std::span<const std::uint8_t> bytes(std::string_view s) {
    return {reinterpret_cast<const std::uint8_t*>(s.data()), s.size()};
}
std::string_view text(std::span<const std::uint8_t> s) {
    return {reinterpret_cast<const char*>(s.data()), s.size()};
}
void json() {
    for (auto s : {"null", "true", "false", "0", "-0", "1.23e-4", " [1,{\"a\":[true,null,\"x\"]}] ",
                   "\"\\ud83c\\udfb5\""})
        ok(validateCiPropertyJson(bytes(s)));
    for (auto s : {"", "01", "+1", "NaN", "1.", "1e", "true false", "[1,]", "{\"a\":}",
                   "\"\\ud800\"", "\"\\udc00\"", "\"\\ud800\\u0041\"", "\"\\x00\""})
        require(validateCiPropertyJson(bytes(s)) == Status::invalid_argument,
                "invalid JSON accepted");
    CiPropertyHeader h;
    ok(h.parse(bytes("{\"resource\":\"X-Test\",\"setPartial\":false,\"limit\":20}")));
    require(h.firstKey() == "resource" && !h.find("missing"), "header lookup/first key");
    std::array<char, 64> out{};
    std::size_t n = 0;
    auto value = h.find("resource");
    ok(value->string(out, n));
    require(std::string_view(out.data(), n) == "X-Test", "decoded header text");
    std::uint32_t limit = 0;
    ok(h.find("limit")->unsignedInteger(limit));
    require(limit == 20, "header integer");
    for (auto s : {"{} ", "{ \"resource\":\"X-Test\"}", "{\"a\":null}", "{\"a\":{}}", "{\"a\":[]}",
                   "{\"a\":1,\"a\":2}", "{\"a\":1,\"\\u0061\":2}", "{\"a\":01}"})
        require(h.parse(bytes(s)) != Status::ok, "malformed PE header accepted");
    require(h.firstKey() == "resource", "failed parse preserves previous view");
    std::string many = "{";
    for (unsigned i = 0; i < 64; ++i) {
        if (i)
            many += ',';
        many += "\"field" + std::to_string(i) + "\":0";
    }
    many += '}';
    ok(h.parse(bytes(many)));
    require(h.find("field63").has_value(), "bounded header field capacity");
    many.insert(many.size() - 1, ",\"extra\":1");
    require(h.parse(bytes(many)) == Status::block_too_large, "excess header fields rejected");
    const std::string_view utf8 = "Cl\xc3\xa9 \xf0\x9f\x8e\xb5";
    std::array<std::uint8_t, 128> escaped{};
    ok(ciPropertyJsonString(utf8, escaped, n));
    require(text(std::span(escaped).first(n)) == "\"Cl\\u00e9 \\ud83c\\udfb5\"",
            "independent Unicode escape result");
    CiPropertyValue string{std::span(escaped).first(n)};
    ok(string.string(out, n));
    require(std::string_view(out.data(), n) == utf8, "surrogate pair UTF8 decode");
    escaped.fill(55);
    const auto original = escaped;
    require(ciPropertyJsonString(std::string_view("\xc0\x80", 2), escaped, n) ==
                    Status::invalid_argument &&
                escaped == original,
            "invalid UTF8 rejected before output writes");
    std::array<char, 2> shortBuffer{'a', 'b'};
    CiPropertyValue large{bytes("\"abcd\"")};
    require(large.string(shortBuffer, n) == Status::block_too_large && n == 4 &&
                shortBuffer[0] == 'a',
            "short decode is transactional");
    std::string deep(34, '[');
    deep += '0';
    deep.append(34, ']');
    require(validateCiPropertyJson(bytes(deep)) == Status::invalid_argument,
            "bounded JSON recursion");
}
struct Link {
    struct Packet {
        std::array<std::uint8_t, 4096> bytes{};
        std::size_t size = 0;
    };
    std::vector<Packet> output = std::vector<Packet>(64);
    std::vector<std::uint8_t> received = std::vector<std::uint8_t>(65536);
    std::size_t head = 0, count = 0, limit = 2, receivedSize = 0;
    MidiCiDevice* device = nullptr;
    CiPropertyStore* store = nullptr;
    unsigned replies = 0, status = 0, notifications = 0, changed = 0, timeouts = 0;
    bool deny = false;
    std::array<char, 8> subscription{};
    static Status send(void* p, unsigned group, std::span<const std::uint8_t> data) noexcept {
        auto& l = *static_cast<Link*>(p);
        if (group != 0 || data.size() > 4096)
            return Status::invalid_argument;
        if (l.count == l.limit)
            return Status::update_queue_full;
        auto& packet = l.output[(l.head + l.count) % l.output.size()];
        std::copy(data.begin(), data.end(), packet.bytes.begin());
        packet.size = data.size();
        ++l.count;
        return Status::ok;
    }
    static Status property(void* p, const CiPropertyMessage& m) noexcept {
        auto& l = *static_cast<Link*>(p);
        if (l.store && (m.type == 0x34 || m.type == 0x36 || m.type == 0x38 || m.type == 0x39 ||
                        m.type == 0x3f))
            return l.store->process(m);
        if (m.type == 0x3f)
            return Status::ok;
        CiPropertyHeader h;
        if (h.parse(m.header) != Status::ok)
            return Status::invalid_argument;
        if (m.type == 0x38) {
            ++l.notifications;
            return l.device->sendProperty(m.source, 0x39, m.request, bytes("{\"status\":200}"), {});
        }
        const auto code = h.find("status");
        if (!code || code->unsignedInteger(l.status) != Status::ok)
            return Status::invalid_argument;
        ++l.replies;
        if (const auto sub = h.find("subscribeId")) {
            std::size_t n;
            if (sub->string(l.subscription, n) != Status::ok || n != 8)
                return Status::invalid_argument;
        }
        l.receivedSize = 0;
        if (h.find("mutualEncoding"))
            return ciDecodeMcoded7(m.data, l.received, l.receivedSize);
        if (m.data.size() > l.received.size())
            return Status::block_too_large;
        std::copy(m.data.begin(), m.data.end(), l.received.begin());
        l.receivedSize = m.data.size();
        return Status::ok;
    }
    CiCallbacks callbacks() {
        CiCallbacks c;
        c.context = this;
        c.send = &send;
        c.property = &property;
        c.timeout = [](void* p, std::uint32_t, std::uint8_t) noexcept {
            ++static_cast<Link*>(p)->timeouts;
        };
        c.invalidated = [](void* p, std::uint32_t id) noexcept {
            auto& l = *static_cast<Link*>(p);
            if (l.store)
                l.store->invalidate(id);
        };
        return c;
    }
    bool deliver(MidiCiDevice& other, std::uint64_t now) {
        if (!count)
            return false;
        auto& packet = output[head];
        ok(other.process({packet.bytes.data(), packet.size}, now));
        head = (head + 1) % output.size();
        --count;
        return true;
    }
    void clear() {
        head = count = 0;
    }
};
void store() {
    MidiCiDevice a, b;
    CiPropertyStore store;
    Link x, y;
    x.device = &a;
    y.device = &b;
    y.store = &store;
    CiConfiguration config;
    config.muid = 1;
    config.propertyExchange = true;
    config.discovery.maximumSysExBytes = 512;
    config.discovery.identity.family = 0x1234;
    config.discovery.identity.model = 0x2345;
    config.discovery.identity.revision = {1, 2, 3, 4};
    config.propertyTransactions = 32;
    ok(a.prepare(config, x.callbacks()));
    config.muid = 2;
    config.propertyTransactions = 1;
    ok(b.prepare(config, y.callbacks()));
    std::array<CiPropertyDefinition, 2> definitions;
    definitions[0].resource = "X-Gain";
    definitions[0].data.assign(bytes("{\"value\":0.25}").begin(), bytes("{\"value\":0.25}").end());
    definitions[0].maximumBytes = 128;
    definitions[0].writable = definitions[0].subscribable = true;
    definitions[0].schema = "{\"type\":\"object\"}";
    definitions[1].resource = "X-Binary";
    definitions[1].mediaType = "application/octet-stream";
    definitions[1].data = {0, 1, 255, 128, 13};
    definitions[1].maximumBytes = 32;
    definitions[1].writable = true;
    CiPropertyStoreCallbacks callbacks;
    callbacks.context = &y;
    callbacks.authorize = [](void* p, std::uint32_t, std::string_view, bool) noexcept {
        return !static_cast<Link*>(p)->deny;
    };
    callbacks.validate = [](void*, std::string_view name,
                            std::span<const std::uint8_t> data) noexcept {
        return name != "X-Gain" || text(data).find("100") == std::string_view::npos;
    };
    callbacks.changed = [](void* p, std::string_view, std::span<const std::uint8_t>) noexcept {
        ++static_cast<Link*>(p)->changed;
    };
    ok(store.prepare(b, {"CyDSP", "Audio", "Cl\xc3\xa9 \xf0\x9f\x8e\xb5", "1.0", "serial"},
                     definitions, {4, 2}, callbacks));
    auto invalidDefinitions = definitions;
    invalidDefinitions[1].resource = "X-Gain";
    require(store.prepare(b, {}, invalidDefinitions, {4, 2}, callbacks) ==
                    Status::invalid_argument &&
                text(store.data("X-Gain")) == "{\"value\":0.25}",
            "failed store preparation preserves previous resources");
    auto drain = [&](std::uint64_t now = 0) {
        for (unsigned i = 0; i < 1024; ++i) {
            a.poll(now);
            store.poll(now);
            const bool left = x.deliver(b, now), right = y.deliver(a, now);
            if (!left && !right && !x.count && !y.count)
                return;
        }
        throw std::runtime_error("property store failed to quiesce");
    };
    auto request = [&](std::uint8_t type, std::string_view header,
                       std::span<const std::uint8_t> data = {}) {
        std::uint8_t id;
        ok(a.requestProperty(2, type, bytes(header), data, 0, id));
        drain();
        return id;
    };
    request(0x34, "{\"resource\":\"ResourceList\"}");
    auto result = text(std::span(x.received).first(x.receivedSize));
    require(x.status == 200 && result.starts_with("[{\"resource\":\"DeviceInfo\"") &&
                result.find("X-Binary") != result.npos &&
                result.find("ResourceList") == result.npos,
            "generated ResourceList discovery");
    request(0x34, "{\"resource\":\"DeviceInfo\"}");
    result = text(std::span(x.received).first(x.receivedSize));
    require(x.status == 200 && result.find("\"familyId\":[52,36]") != result.npos &&
                result.find("\"modelId\":[69,70]") != result.npos &&
                result.find("Cl\\u00e9 \\ud83c\\udfb5") != result.npos,
            "DeviceInfo identity/Unicode");
    request(0x34, "{\"resource\":\"X-Gain\"}");
    require(x.status == 200 &&
                text(std::span(x.received).first(x.receivedSize)) == "{\"value\":0.25}",
            "GET original resource");
    request(0x36, "{\"resource\":\"X-Gain\"}", bytes("{\"value\":0.5}"));
    require(x.status == 200 && y.changed == 1 && text(store.data("X-Gain")) == "{\"value\":0.5}",
            "SET accepted once");
    request(0x36, "{\"resource\":\"X-Gain\"}", bytes("{\"value\":NaN}"));
    require(x.status == 400 && y.changed == 1, "malformed SET rejected");
    request(0x36, "{\"resource\":\"X-Gain\"}", bytes("{\"value\":100}"));
    require(x.status == 400 && y.changed == 1, "application validation before mutation");
    request(0x36, "{\"resource\":\"DeviceInfo\"}", bytes("{}"));
    require(x.status == 405, "readonly built-in resource");
    request(0x34, "{\"resource\":\"X-Missing\"}");
    require(x.status == 404, "unknown resource status");
    request(0x34, "{\"resource\":\"X-Gain\",\"mutualEncoding\":\"zlib+Mcoded7\"}");
    require(x.status == 415, "unadvertised encoding rejected");
    y.deny = true;
    request(0x34, "{\"resource\":\"X-Gain\"}");
    require(x.status == 403, "access callback");
    y.deny = false;
    request(0x34, "{\"resource\":\"X-Binary\",\"mutualEncoding\":\"Mcoded7\"}");
    require(x.status == 200 && x.receivedSize == 5 && x.received[2] == 255, "binary GET decoding");
    const std::array<std::uint8_t, 4> binary{128, 255, 0, 3};
    std::array<std::uint8_t, 64> encoded{};
    std::size_t n;
    ok(ciEncodeMcoded7(binary, encoded, n));
    request(0x36,
            "{\"resource\":\"X-Binary\",\"mutualEncoding\":\"Mcoded7\",\"mediaType\":\"application/"
            "octet-stream\"}",
            std::span(encoded).first(n));
    require(x.status == 200 &&
                std::equal(binary.begin(), binary.end(), store.data("X-Binary").begin()),
            "binary SET decoded state");
    const auto before = x.replies;
    y.limit = 0;
    for (unsigned i = 0; i < 3; ++i) {
        std::uint8_t id;
        ok(a.requestProperty(2, 0x34, bytes("{\"resource\":\"X-Gain\"}"), {}, 0, id));
        ok(x.deliver(b, 0) ? Status::ok : Status::invalid_argument);
        store.poll(0);
    }
    require(x.replies == before, "replies held while physical queue full");
    require(store.process({1, 2, 0x36, 99, bytes("{\"resource\":\"X-Gain\"}"),
                           bytes("{\"value\":0.4}")}) == Status::update_queue_full &&
                text(store.data("X-Gain")) == "{\"value\":0.5}",
            "full reply queue rejects SET before state mutation");
    y.limit = 2;
    drain();
    require(x.replies == before + 3 && store.lastError() == Status::ok,
            "retained replies across CI and transport saturation");
    const auto cancellationBaseline = x.replies;
    y.limit = 0;
    std::array<std::uint8_t, 2> ids{};
    for (auto& id : ids) {
        ok(a.requestProperty(2, 0x34, bytes("{\"resource\":\"X-Gain\"}"), {}, 0, id));
        require(x.deliver(b, 0), "stalled service GET delivery");
        store.poll(0);
    }
    ok(a.notifyProperty(2, ids[1], 144));
    require(x.deliver(b, 0), "retained service reply cancellation");
    y.limit = 2;
    drain();
    require(x.replies == cancellationBaseline + 1,
            "Notify cancellation removes the store's retained reply");
    request(0x38, "{\"command\":\"start\",\"resource\":\"X-Gain\"}");
    require(x.status == 200 && store.activeSubscriptions() == 1, "subscription start");
    ok(store.publish("X-Gain", bytes("{\"value\":0.6}")));
    ok(store.publish("X-Gain", bytes("{\"value\":0.7}")));
    drain();
    require(x.notifications == 1 && store.activeSubscriptions() == 1,
            "coalesced acknowledged subscription notify");
    ok(store.publish("X-Gain", bytes("{\"value\":0.8}")));
    drain();
    require(x.notifications == 2, "subsequent notification after acknowledgement");
    const std::string end = "{\"command\":\"end\",\"subscribeId\":\"" +
                            std::string(x.subscription.data(), x.subscription.size()) + "\"}";
    request(0x38, end);
    require(x.status == 200 && store.activeSubscriptions() == 0, "subscription end");
    request(0x38, "{\"command\":\"start\",\"resource\":\"X-Gain\"}");
    const std::array<std::uint8_t, 4> invalidated{1, 0, 0, 0};
    ok(a.send(ciBroadcastMuid, 0x7e, invalidated));
    drain();
    require(store.activeSubscriptions() == 0, "uncached MUID invalidates subscriptions");
    // No dynamic resource operation, callback, retained reply or reset may allocate/free.
    struct Audit {
        CiPropertyStore& store;
        bool good = true;
    } audit{store};
    runRealtimeAudit(
        [](void* p) noexcept {
            auto& a = *static_cast<Audit*>(p);
            a.good &= a.store.process({1, 2, 0x34, 10, bytes("{\"resource\":\"X-Gain\"}"), {}}) ==
                      Status::ok;
            a.good &= a.store.process({1, 2, 0x36, 11, bytes("{\"resource\":\"X-Gain\"}"),
                                       bytes("{\"value\":0.9}")}) == Status::ok;
            a.store.poll(100);
            a.store.reset();
        },
        &audit);
    require(audit.good && text(store.data("X-Gain")) == "{\"value\":0.9}",
            "property service retained-memory audit");
    drain();
    request(0x38, "{\"command\":\"start\",\"resource\":\"X-Gain\"}");
    ok(store.publish("X-Gain", bytes("{\"value\":0.8}")));
    store.poll(1000);
    require(store.activeSubscriptions() == 1 && y.count,
            "subscription update waits for acknowledgement");
    store.poll(4000);
    y.clear();
    require(store.activeSubscriptions() == 0,
            "unacknowledged subscription closes on transaction expiry");
    request(0x38, "{\"command\":\"start\",\"resource\":\"X-Gain\"}");
    const auto oldMuid = b.muid();
    const std::array<std::uint8_t, 4> localInvalidation{2, 0, 0, 0};
    ok(a.send(ciBroadcastMuid, 0x7e, localInvalidation));
    drain();
    require(b.muid() != oldMuid && store.activeSubscriptions() == 0,
            "local MUID replacement clears resource subscriptions");
}
void notify() {
    MidiCiDevice a, b;
    Link x, y;
    x.device = &a;
    y.device = &b;
    CiConfiguration c;
    c.muid = 1;
    c.propertyExchange = true;
    ok(a.prepare(c, x.callbacks()));
    c.muid = 2;
    ok(b.prepare(c, y.callbacks()));
    std::uint8_t id;
    ok(a.requestProperty(2, 0x34, bytes("{\"resource\":\"X-Slow\"}"), {}, 0, id));
    x.clear();
    ok(b.notifyProperty(1, id, 100));
    require(y.deliver(a, 2500), "keepalive delivery");
    a.poll(3000);
    require(a.propertyRequestActive(2, id) && x.timeouts == 0,
            "Notify 100 extends pending deadline");
    ok(b.notifyProperty(1, id, 144));
    require(y.deliver(a, 5000), "cancel delivery");
    a.poll(9000);
    require(!a.propertyRequestActive(2, id) && x.timeouts == 0,
            "Notify 144 cancels without timeout");
    ok(a.requestProperty(2, 0x34, bytes("{\"resource\":\"X-Slow\"}"), {}, 10000, id));
    x.clear();
    ok(b.notifyProperty(1, id, 408));
    require(y.deliver(a, 10500), "timeout delivery");
    require(!a.propertyRequestActive(2, id) && x.timeouts == 1,
            "Notify 408 reports remote timeout once");
    require(b.notifyProperty(1, id, 101) == Status::invalid_argument, "unsupported Notify code");
    c.muid = 2;
    c.propertyTransactions = 1;
    ok(b.prepare(c, y.callbacks()));
    y.clear();
    y.limit = 0;
    ok(b.requestProperty(1, 0x36, bytes("{\"resource\":\"X-Slow\"}"), bytes("{}"), 11000, id));
    require(b.propertyRequestActive(1, id), "stalled local request active");
    ok(b.notifyProperty(1, id, 144));
    require(!b.propertyRequestActive(1, id),
            "local cancellation frees a saturated CI transfer slot");
    y.limit = 2;
    b.poll(11001);
    y.clear();
    c.muid = 1;
    c.propertyTransactions = 1;
    ok(a.prepare(c, x.callbacks()));
    x.clear();
    y.clear();
    ok(a.requestProperty(2, 0x34, bytes("{\"resource\":\"X-Slow\"}"), {}, 20000, id));
    x.clear();
    std::array<std::uint8_t, 128> fragment{};
    std::size_t length;
    ok(makeCiPropertyBody({id, 2, 1, bytes("{\"status\":200}"), bytes("one")}, fragment, length));
    ok(b.send(1, 0x35, std::span(fragment).first(length)));
    require(y.deliver(a, 20010), "partial reply delivery");
    ok(b.notifyProperty(1, id, 100));
    require(y.deliver(a, 22500), "keepalive with all assembly slots occupied");
    a.poll(23500);
    require(a.propertyRequestActive(2, id), "keepalive extends full assembler and pending request");
    ok(makeCiPropertyBody({id, 2, 2, {}, bytes("two")}, fragment, length));
    ok(b.send(1, 0x35, std::span(fragment).first(length)));
    require(y.deliver(a, 24000), "continued partial reply");
    require(!a.propertyRequestActive(2, id) &&
                text(std::span(x.received).first(x.receivedSize)) == "onetwo",
            "full assembler retains data across Notify 100");
    CiPropertyAssembler assembler;
    ok(assembler.prepare(1, 64, 64));
    std::array<std::uint8_t, 128> body{};
    std::size_t n;
    CiPropertyMessage message;
    bool complete;
    ok(makeCiPropertyBody({3, 2, 1, bytes("{\"status\":200}"), bytes("one")}, body, n));
    ok(assembler.append({0x7f, 0x35, 2, 2, 1, std::span(body).first(n)}, 0, message, complete));
    assembler.keepAlive(2, 1, 3, 2500);
    assembler.expire(3000);
    ok(makeCiPropertyBody({3, 2, 2, {}, bytes("two")}, body, n));
    ok(assembler.append({0x7f, 0x35, 2, 2, 1, std::span(body).first(n)}, 4000, message, complete));
    require(complete && text(message.data) == "onetwo", "Notify keepalive retains partial reply");
}
} // namespace
void ciPropertyRegression() {
    json();
    store();
    notify();
    std::cout << "PASS CI resources/GET/SET/subscriptions/Notify/realtime\n";
}
