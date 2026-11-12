#include <algorithm>
#include <cydsp/CyDSP.h>
#include <iostream>
#include <stdexcept>
#include <string_view>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool value, const char* reason) {
    if (!value)
        throw std::runtime_error(reason);
}
void ok(Status value) {
    require(value == Status::ok, "advanced CI status");
}
std::span<const std::uint8_t> bytes(std::string_view s) {
    return {reinterpret_cast<const std::uint8_t*>(s.data()), s.size()};
}
std::string_view text(std::span<const std::uint8_t> s) {
    return {reinterpret_cast<const char*>(s.data()), s.size()};
}
void patches() {
    CiPropertyPatch patch;
    std::array<std::uint8_t, 1024> output{};
    std::size_t n = 0;
    require(patch.apply(bytes("{}"), bytes("{}"), output, n) == Status::not_prepared,
            "unprepared patch");
    ok(patch.prepare(128, 128));
    const auto doc = bytes(R"({"a":[1,{"x/y":2}],"~":true,"":0,"\u00e9":3})");
    const auto changes = bytes(R"({"/a/1/x~1y":7,"/~0":false,"/":4,"/\u00e9":"yes"})");
    ok(patch.apply(doc, changes, output, n));
    require(text(std::span(output).first(n)) ==
                R"({"a":[1,{"x/y":7}],"~":false,"":4,"\u00e9":"yes"})",
            "RFC6901 independent expected result");
    ok(patch.apply(doc, bytes(R"({"":"root"})"), output, n));
    require(text(std::span(output).first(n)) == R"("root")", "empty pointer replaces root");
    for (auto bad : {R"({"/missing":1})", R"({"/a/01":1})", R"({"/a/-":1})", R"({"/a/9":1})",
                     R"({"/a~2":1})", R"({"a":1})", R"({"/a":null})", R"({"/a":[]})",
                     R"({"/a":{}})", R"({"/a":0,"/a/0":1})", R"({"/~0":1,"/\u007e0":2})"}) {
        output.fill(91);
        const auto before = output;
        require(patch.apply(doc, bytes(bad), output, n) == Status::invalid_argument &&
                    output == before,
                "invalid patch must not write");
    }
    require(patch.apply(bytes(R"({"x":1,"\u0078":2})"), bytes("{}"), output, n) ==
                Status::invalid_argument,
            "ambiguous original keys rejected");
    output.fill(33);
    require(patch.apply(doc, changes, std::span(output).first(2), n) == Status::block_too_large &&
                n > 2 && output[0] == 33,
            "sizing without partial write");
    auto alias = output;
    std::copy(doc.begin(), doc.end(), alias.begin());
    const auto before = alias;
    require(patch.apply(std::span(alias).first(doc.size()), changes, alias, n) ==
                    Status::partial_overlap &&
                alias == before,
            "patch alias rejection");
    CiPropertyPatch small;
    ok(small.prepare(2, 8));
    require(small.apply(doc, changes, output, n) == Status::block_too_large,
            "bounded node workspace");
    ok(small.prepare(64, 2));
    require(small.apply(bytes(R"({"long":1})"), bytes("{}"), output, n) == Status::block_too_large,
            "bounded decoded keys");
    std::uint32_t total = 0;
    const auto list = bytes(R"([{"id":0},{"id":1},{"id":2}])");
    ok(patch.list(list, 1, 1, output, n, total));
    require(total == 3 && text(std::span(output).first(n)) == R"([{"id":1}])",
            "pagination exact slice");
    ok(patch.list(list, 0xffffffffu, 0xffffffffu, output, n, total));
    require(total == 3 && text(std::span(output).first(n)) == "[]", "pagination large offset");
    require(patch.list(list, 0, 0, output, n, total) == Status::invalid_argument,
            "zero page limit");
    require(patch.list(bytes("[{},0]"), 0, 1, output, n, total) == Status::invalid_argument,
            "list objects only");
    struct Audit {
        CiPropertyPatch* patch;
        std::array<std::uint8_t, 1024> out{};
        Status status = Status::ok;
    } audit{&patch};
    runRealtimeAudit(
        [](void* p) noexcept {
            auto& a = *static_cast<Audit*>(p);
            std::size_t written = 0;
            a.status =
                a.patch->apply(bytes(R"({"x":[1,2]})"), bytes(R"({"/x/1":4})"), a.out, written);
        },
        &audit);
    ok(audit.status);
}
struct Wire {
    struct Packet {
        std::array<std::uint8_t, 4096> data{};
        std::size_t size = 0;
    };
    std::vector<Packet> packets = std::vector<Packet>(128);
    std::size_t read = 0, count = 0, limit = 2;
    CiPropertyClient* client = nullptr;
    CiPropertyStore* store = nullptr;
    std::uint64_t now = 0;
    unsigned sent = 0, completed = 0, status = 0, updated = 0, partials = 0, fulls = 0;
    std::array<char, 8> subId{};
    std::size_t subSize = 0;
    static Status send(void* p, unsigned group, std::span<const std::uint8_t> data) noexcept {
        auto& w = *static_cast<Wire*>(p);
        if (group || data.size() > 4096)
            return Status::invalid_argument;
        if (w.count >= w.limit)
            return Status::update_queue_full;
        auto& packet = w.packets[(w.read + w.count) % w.packets.size()];
        std::copy(data.begin(), data.end(), packet.data.begin());
        packet.size = data.size();
        ++w.count;
        ++w.sent;
        return Status::ok;
    }
    static Status property(void* p, const CiPropertyMessage& m) noexcept {
        auto& w = *static_cast<Wire*>(p);
        CiPropertyHeader h;
        if (h.parse(m.header) != Status::ok)
            return Status::invalid_argument;
        if (m.type == 0x39)
            if (auto id = h.find("subscribeId")) {
                if (id->string(w.subId, w.subSize) != Status::ok)
                    return Status::invalid_argument;
            }
        if (w.client)
            return w.client->process(m, w.now);
        if (w.store)
            return w.store->process(m);
        return Status::ok;
    }
    CiCallbacks callbacks() {
        CiCallbacks c;
        c.context = this;
        c.send = send;
        c.property = property;
        c.invalidated = [](void* p, std::uint32_t peer) noexcept {
            auto& w = *static_cast<Wire*>(p);
            if (w.client)
                w.client->invalidate(peer);
            if (w.store)
                w.store->invalidate(peer);
        };
        return c;
    }
    CiPropertyClientCallbacks clientCallbacks() {
        CiPropertyClientCallbacks c;
        c.context = this;
        c.completed = [](void* p, const CiPropertyQuery&, std::uint8_t, unsigned code) noexcept {
            auto& w = *static_cast<Wire*>(p);
            ++w.completed;
            w.status = code;
        };
        c.updated = [](void* p, const CiPropertyQuery&, CiPropertyCacheView,
                       bool partial) noexcept {
            auto& w = *static_cast<Wire*>(p);
            ++w.updated;
            if (partial)
                ++w.partials;
            else
                ++w.fulls;
        };
        return c;
    }
    Packet pop() {
        auto packet = packets[read];
        read = (read + 1) % packets.size();
        --count;
        return packet;
    }
    Status deliver(MidiCiDevice& peer) noexcept {
        if (!count)
            return Status::ok;
        auto& p = packets[read];
        const auto r = peer.process({p.data.data(), p.size}, now);
        read = (read + 1) % packets.size();
        --count;
        return r;
    }
};
struct Session {
    MidiCiDevice a, b;
    CiPropertyStore store;
    CiPropertyClient client;
    Wire x, y;
    std::uint64_t now = 10;
    Session() {
        x.client = &client;
        y.store = &store;
        CiConfiguration config;
        config.muid = 1;
        config.propertyExchange = true;
        config.propertyTransactions = 16;
        config.maximumPropertyBytes = 8192;
        config.discovery.maximumSysExBytes = 512;
        ok(a.prepare(config, x.callbacks()));
        config.muid = 2;
        ok(b.prepare(config, y.callbacks()));
        std::array<CiPropertyDefinition, 5> definitions;
        auto& gain = definitions[0];
        gain.resource = "X-Gain";
        gain.data.assign(bytes(R"({"value":1,"name":"a"})").begin(),
                         bytes(R"({"value":1,"name":"a"})").end());
        gain.maximumBytes = 256;
        gain.writable = gain.partial = gain.subscribable = true;
        gain.delivery = CiSubscriptionDelivery::partial;
        gain.cacheTimeSeconds = 2;
        auto& list = definitions[1];
        list.resource = "X-PresetList";
        list.maximumBytes = 256;
        list.paginated = true;
        const auto listData = bytes(R"([{"id":0},{"id":1},{"id":2}])");
        list.data.assign(listData.begin(), listData.end());
        list.cacheTimeSeconds = 2;
        auto& inst = definitions[2];
        inst.resource = "X-Instance";
        inst.maximumBytes = 128;
        inst.writable = inst.partial = inst.subscribable = true;
        inst.cacheTimeSeconds = 2;
        inst.delivery = CiSubscriptionDelivery::full;
        inst.instances = {{"left", {123, 34, 118, 34, 58, 49, 125}},
                          {"right", {123, 34, 118, 34, 58, 50, 125}}};
        auto& binary = definitions[3];
        binary.resource = "X-Binary";
        binary.mediaType = "application/octet-stream";
        binary.data = {0, 127, 128, 255};
        binary.maximumBytes = 128;
        binary.cacheTimeSeconds = 2;
        auto& notify = definitions[4];
        notify.resource = "X-Notify";
        notify.maximumBytes = 128;
        notify.data = {123, 34, 110, 34, 58, 48, 125};
        notify.subscribable = true;
        std::array<CiChannelDescription, 2> channels;
        channels[0].title = "Piano";
        channels[0].channel = 1;
        channels[0].hasBankPC = true;
        channels[0].bankPC = {1, 2, 3};
        channels[1].title = "MPE";
        channels[1].channel = 17;
        channels[1].clusterChannelStart = 17;
        channels[1].clusterLength = 16;
        channels[1].clusterType = "mpe1";
        CiPropertyStoreOptions options;
        options.channels = channels;
        CiPropertyStoreCallbacks callbacks;
        callbacks.validate = [](void*, std::string_view name,
                                std::span<const std::uint8_t> data) noexcept {
            return name != "X-Gain" || text(data).find("999") == std::string_view::npos;
        };
        ok(store.prepare(b, {"CyDSP", "audio", "test", "1", ""}, definitions, options, callbacks));
        CiPropertyClientOptions clientOptions;
        clientOptions.maximumBytes = 8192;
        ok(client.prepare(a, clientOptions, x.clientCallbacks()));
    }
    Status cycle() noexcept {
        x.now = y.now = now;
        client.poll(now);
        store.poll(now);
        auto r = x.deliver(b);
        if (r != Status::ok)
            return r;
        return y.deliver(a);
    }
    void drain() {
        for (unsigned i = 0; i < 128; ++i)
            ok(cycle());
        require(!x.count && !y.count, "CI session quiescence");
    }
    void expect(const CiPropertyQuery& q, std::string_view expected) {
        const auto v = client.cached(q, now, true);
        if (!v || text(v->data) != expected)
            throw std::runtime_error(std::string("CI cache ") + std::string(q.resource) + ": " +
                                     (v ? std::string(text(v->data)) : "empty") + " expected " +
                                     std::string(expected));
    }
};
void service() {
    Session s;
    CiPropertyQuery gain{2, "X-Gain"};
    ok(s.client.fetch(gain, s.now));
    s.drain();
    s.expect(gain, R"({"value":1,"name":"a"})");
    require(s.client.cached(gain, s.now)->expiresAtMs == 2010, "cache TTL milliseconds");
    const auto sent = s.x.sent;
    ok(s.client.fetch(gain, 2009));
    require(s.x.sent == sent, "fresh cache sends no GET");
    require(!s.client.cached(gain, 2010) && s.client.cached(gain, 2010, true),
            "TTL boundary stale access");
    s.now = 2010;
    ok(s.client.fetch(gain, s.now));
    s.drain();

    CiPropertyQuery page{
        2, "X-PresetList", "", "", CiPropertyEncoding::ascii, CiPropertyPage{1, 1}};
    ok(s.client.fetch(page, s.now));
    s.drain();
    s.expect(page, R"([{"id":1}])");
    require(s.client.cached(page, s.now)->totalCount == 3u, "totalCount cache");
    auto page2 = page;
    page2.page->offset = 0;
    ok(s.client.fetch(page2, s.now));
    s.drain();
    s.expect(page2, R"([{"id":0}])");
    s.expect(page, R"([{"id":1}])");
    CiPropertyQuery list{2, "ResourceList"};
    ok(s.client.fetch(list, s.now));
    s.drain();
    auto resources = s.client.cached(list, s.now, true);
    require(resources && resources->totalCount == 8u &&
                text(resources->data).find("midi+jsonschema://schema4") != std::string_view::npos &&
                text(resources->data).find(R"("requireResId":true)") != std::string_view::npos,
            "ResourceList capabilities and schema URI");
    CiPropertyQuery schema{2, "JSONSchema", "schema4"};
    ok(s.client.fetch(schema, s.now));
    s.drain();
    s.expect(schema, "{}");
    CiPropertyQuery channels{2, "ChannelList"};
    ok(s.client.subscribe(channels, s.now));
    s.drain();
    const auto channelView = s.client.cached(channels, s.now);
    require(channelView && channelView->totalCount == 2u &&
                text(channelView->data).find(R"("channel":17)") != std::string_view::npos,
            "function block channel mapping");
    CiChannelDescription replacement;
    replacement.title = "Bass";
    replacement.channel = 256;
    ok(s.store.publishChannels(std::span(&replacement, 1)));
    s.drain();
    require(text(s.client.cached(channels, s.now)->data).find(R"("channel":256)") !=
                std::string_view::npos,
            "ChannelList notification refresh");
    replacement.channel = 257;
    require(s.store.publishChannels(std::span(&replacement, 1)) == Status::invalid_argument &&
                text(s.store.data("ChannelList")).find(R"("channel":256)") !=
                    std::string_view::npos,
            "invalid channels preserve data");

    CiPropertyQuery left{2, "X-Instance", "left"}, right{2, "X-Instance", "right"};
    ok(s.client.subscribe(left, s.now));
    s.drain();
    ok(s.client.fetch(right, s.now));
    s.drain();
    ok(s.store.publish("X-Instance", bytes(R"({"v":9})"), "right"));
    s.drain();
    s.expect(left, R"({"v":1})");
    s.expect(right, R"({"v":2})");
    ok(s.client.set(left, bytes(R"({"/v":7})"), s.now, true));
    s.drain();
    s.expect(left, R"({"v":7})");
    require(text(s.store.data("X-Instance", "right")) == R"({"v":9})", "instance SET isolation");
    CiPropertyQuery missing{2, "X-Instance"};
    ok(s.client.fetch(missing, s.now));
    s.drain();
    require(s.x.status == 400, "resId mandatory");
    missing.resId = "unknown";
    ok(s.client.fetch(missing, s.now));
    s.drain();
    require(s.x.status == 404, "unknown instance");

    CiPropertyQuery binary{2, "X-Binary", "", "application/octet-stream",
                           CiPropertyEncoding::mcoded7};
    ok(s.client.fetch(binary, s.now));
    s.drain();
    const auto data = s.client.cached(binary, s.now);
    require(data && data->data.size() == 4 && data->data[2] == 128 && data->data[3] == 255,
            "client binary decoding");
    require(s.client.set(binary, bytes("{}"), s.now, true) == Status::invalid_argument,
            "binary partial rejected");

    ok(s.client.subscribe(gain, s.now));
    s.drain();
    const auto beforePartial = s.x.partials;
    std::uint8_t id = 0;
    ok(s.a.requestProperty(2, 0x36, bytes(R"({"resource":"X-Gain","setPartial":true})"),
                           bytes(R"({"/value":3})"), s.now, id));
    s.drain();
    s.expect(gain, R"({"value":3,"name":"a"})");
    require(s.x.partials == beforePartial + 1, "subscription partial delivered and patched");
    ok(s.client.set(gain, bytes(R"({"/name":"b"})"), s.now, true));
    s.drain();
    s.expect(gain, R"({"value":3,"name":"b"})");
    ok(s.client.set(gain, bytes(R"({"/value":999})"), s.now, true));
    s.drain();
    require(text(s.store.data("X-Gain")) == R"({"value":3,"name":"b"})",
            "validation after patch before commit");

    // Two changes coalesce before poll: the responder must send a complete baseline.
    const auto fulls = s.x.fulls;
    ok(s.store.publish("X-Gain", bytes(R"({"value":4,"name":"c"})")));
    ok(s.store.publish("X-Gain", bytes(R"({"value":5,"name":"d"})")));
    s.drain();
    s.expect(gain, R"({"value":5,"name":"d"})");
    require(s.x.fulls > fulls, "coalescing falls back to full snapshot");

    // Hold an older GET response while a later subscription full update arrives.
    s.x.now = s.y.now = s.now;
    ok(s.client.fetch(gain, s.now, true));
    ok(s.x.deliver(s.b));
    s.store.poll(s.now);
    require(s.y.count == 1, "GET response held independently");
    auto held = s.y.pop();
    ok(s.store.publish("X-Gain", bytes(R"({"value":6,"name":"new"})")));
    s.drain();
    s.expect(gain, R"({"value":6,"name":"new"})");
    ok(s.a.process({held.data.data(), held.size}, s.now));
    s.expect(gain, R"({"value":6,"name":"new"})");
    s.drain();

    CiPropertyQuery notify{2, "X-Notify"};
    ok(s.client.subscribe(notify, s.now));
    s.drain();
    ok(s.store.publish("X-Notify", bytes(R"({"n":8})")));
    s.drain();
    s.expect(notify, R"({"n":8})");
    // Force bounded transport backpressure; accepted work is retained and drained.
    s.x.limit = s.y.limit = 0;
    const auto result = s.client.fetch(page, s.now, true);
    require(result == Status::ok || result == Status::update_queue_full, "backpressure acceptance");
    s.x.limit = s.y.limit = 1;
    if (result != Status::ok)
        ok(s.client.fetch(page, s.now, true));
    s.drain();
    s.expect(page, R"([{"id":1}])");

    struct Audit {
        Session* s;
        CiPropertyQuery query;
        Status status = Status::ok;
    } audit{&s, gain};
    runRealtimeAudit(
        [](void* p) noexcept {
            auto& a = *static_cast<Audit*>(p);
            a.status = a.s->client.fetch(a.query, a.s->now, true);
            for (unsigned i = 0; i < 128 && a.status == Status::ok; ++i)
                a.status = a.s->cycle();
            if (a.status == Status::ok) {
                std::uint8_t id = 0;
                a.status = a.s->a.requestProperty(
                    2, 0x36, bytes(R"({"resource":"X-Gain","setPartial":true})"),
                    bytes(R"({"/value":7})"), a.s->now, id);
                for (unsigned i = 0; i < 128 && a.status == Status::ok; ++i)
                    a.status = a.s->cycle();
            }
        },
        &audit);
    ok(audit.status);
    s.expect(gain, R"({"value":7,"name":"new"})");
    ok(s.client.unsubscribe(gain, s.now));
    s.drain();
    require(s.client.activeSubscriptions() == 3, "subscription end removes exactly one entry");
    s.client.invalidate(2);
    s.store.invalidate(1);
    require(!s.client.cached(left, s.now, true) && !s.client.activeSubscriptions() &&
                !s.store.activeSubscriptions(),
            "peer invalidation clears sessions/cache");
    ok(s.client.fetch(gain, s.now));
    s.x.pop();
    s.now += 4000;
    s.client.poll(s.now);
    require(s.client.lastError() == Status::timed_out && s.x.status == 408,
            "client timeout completion");
    s.client.reset();
}
void clientLimits() {
    Session s;
    CiPropertyClientOptions opts;
    opts.entries = opts.transactions = opts.subscriptions = opts.acknowledgements = 1;
    opts.maximumBytes = 8192;
    ok(s.client.prepare(s.a, opts, s.x.clientCallbacks()));
    const CiPropertyQuery gain{2, "X-Gain"}, other{2, "DeviceInfo"};
    ok(s.client.fetch(gain, s.now));
    require(s.client.fetch(other, s.now) == Status::update_queue_full,
            "pending cache entry cannot be evicted");
    require(s.client.fetch(gain, s.now, true) == Status::transition_in_progress,
            "coalesced GET reports pending");
    s.drain();
    ok(s.client.subscribe(gain, s.now));
    s.drain();
    require(s.client.fetch(other, s.now) == Status::update_queue_full,
            "subscribed cache entry cannot be evicted");
    // Invalid partial: acknowledge failure, close local subscription, fetch a safe baseline.
    const auto id = std::string_view(s.x.subId.data(), s.x.subSize);
    require(!id.empty(), "captured subscription identity");
    std::string header = R"({"command":"partial","subscribeId":")" + std::string(id) + R"("})";
    ok(s.client.process({2, 1, 0x38, 91, bytes(header), bytes(R"({"/missing":3})")}, s.now));
    require(!s.client.activeSubscriptions() && !s.client.cached(gain, s.now),
            "invalid partial cannot leave cache fresh or subscription active");
    s.drain();
    s.expect(gain, R"({"value":1,"name":"a"})");
    ok(s.client.subscribe(gain, s.now));
    s.drain();
    header = R"({"command":"full","subscribeId":")" + std::string(s.x.subId.data(), s.x.subSize) +
             R"("})";
    const auto state = s.client.cached(gain, s.now);
    require(state.has_value(), "full baseline before backpressure");
    // Occupy both the core outgoing transfers and client ACK slot.
    s.x.limit = 0;
    for (unsigned i = 0; i < 64; ++i) {
        const auto result = s.client.process(
            {2, 1, 0x38, 92, bytes(header), bytes(R"({"value":8,"name":"ok"})")}, s.now);
        if (result == Status::update_queue_full) {
            const auto snapshot = s.client.cached(gain, s.now, true);
            require(snapshot && text(snapshot->data) == R"({"value":8,"name":"ok"})",
                    "retained full update");
            require(s.client.process({2, 1, 0x38, 93, bytes(header), bytes(R"({"value":9})")},
                                     s.now) == Status::update_queue_full,
                    "bounded ACK FIFO");
            require(text(s.client.cached(gain, s.now, true)->data) == R"({"value":8,"name":"ok"})",
                    "ACK saturation rejects before mutating cache");
            break;
        }
        ok(result);
        require(i != 63, "ACK backpressure exercised");
    }
    s.x.limit = 2;
    s.drain();
    s.client.reset();
    s.store.reset();
    ok(s.client.fetch(other, s.now));
    s.drain();
    require(!s.client.cached(gain, s.now, true) && s.client.cached(other, s.now, true),
            "LRU reuse after reset");
}
} // namespace
void ciAdvancedRegression() {
    patches();
    service();
    clientLimits();
    std::cout
        << "PASS CI partial/instances/pagination/ChannelList/JSONSchema/client-cache/realtime\n";
}
