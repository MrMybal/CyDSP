#include "cydsp/midi/CiProperty.h"
#include "detail/Alias.h"
#include "detail/CiJson.h"
#include <algorithm>
#include <stdexcept>
namespace cydsp {
namespace {
bool resourceName(std::string_view s) noexcept {
    return !s.empty() && s.size() <= 36 && std::all_of(s.begin(), s.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
               c == '-';
    });
}
bool instanceName(std::string_view s) noexcept {
    return !s.empty() && s.size() <= 36 && std::all_of(s.begin(), s.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
               c == '_';
    });
}
template <std::size_t N>
bool field(const CiPropertyHeader& h, std::string_view key, std::array<char, N>& buffer,
           std::string_view& out) noexcept {
    const auto v = h.find(key);
    std::size_t n = 0;
    if (!v) {
        out = {};
        return true;
    }
    if (v->string(buffer, n) != Status::ok)
        return false;
    out = {buffer.data(), n};
    return true;
}
void pair(detail::CiJsonWriter& w, std::string_view key, std::string_view value) noexcept {
    (void)w.string(key);
    w.character(':');
    if (!w.string(value))
        w.good = false;
}
Status channels(std::span<const CiChannelDescription> list, std::span<std::uint8_t> out,
                std::size_t& n) noexcept {
    n = 0;
    if (list.size() > 256)
        return Status::invalid_argument;
    for (std::size_t i = 0; i < list.size(); ++i) {
        const auto& c = list[i];
        if (c.title.empty() || c.title.size() > 512 || c.programTitle.size() > 512 || !c.channel ||
            c.channel > 256 || (c.clusterChannelStart == 0) != (c.clusterLength == 0) ||
            c.clusterChannelStart > 256 || c.clusterLength > 256 ||
            (c.clusterLength && c.clusterLength > 257 - c.clusterChannelStart) ||
            c.clusterMidiMode < 1 || c.clusterMidiMode > 4 ||
            (c.clusterType != "other" && c.clusterType != "profile" && c.clusterType != "mpe1") ||
            !std::all_of(c.bankPC.begin(), c.bankPC.end(), [](auto v) { return v < 128; }))
            return Status::invalid_argument;
        for (std::size_t j = 0; j < i; ++j)
            if (list[j].channel == c.channel)
                return Status::invalid_argument;
    }
    detail::CiJsonWriter w{out};
    w.character('[');
    for (std::size_t i = 0; i < list.size(); ++i) {
        const auto& c = list[i];
        if (i)
            w.character(',');
        w.character('{');
        pair(w, "title", c.title);
        w.text(",\"channel\":");
        w.number(c.channel);
        if (!c.programTitle.empty()) {
            w.character(',');
            pair(w, "programTitle", c.programTitle);
        }
        if (c.hasBankPC) {
            w.text(",\"bankPC\":[");
            for (unsigned j = 0; j < 3; ++j) {
                if (j)
                    w.character(',');
                w.number(c.bankPC[j]);
            }
            w.character(']');
        }
        if (c.clusterLength) {
            w.text(",\"clusterChannelStart\":");
            w.number(c.clusterChannelStart);
            w.text(",\"clusterLength\":");
            w.number(c.clusterLength);
            w.text(",\"clusterMidiMode\":");
            w.number(c.clusterMidiMode);
            w.character(',');
            pair(w, "clusterType", c.clusterType);
        }
        w.character('}');
    }
    w.character(']');
    if (!w.good)
        return Status::block_too_large;
    n = w.position;
    return Status::ok;
}
} // namespace
struct CiPropertyStore::State {
    struct Record {
        std::string id;
        std::vector<std::uint8_t> bytes, delta;
        std::size_t size = 0, deltaSize = 0;
        std::uint64_t revision = 1;
        bool deltaValid = false;
        std::span<const std::uint8_t> data() const noexcept {
            return {bytes.data(), size};
        }
    };
    struct Resource {
        std::string name, media, schema, schemaId;
        std::vector<Record> records;
        bool writable = false, subscribable = false, builtin = false, partial = false,
             paginated = false, requireId = false;
        std::uint32_t cacheTime = 0;
        CiSubscriptionDelivery delivery = CiSubscriptionDelivery::notify;
        bool json() const noexcept {
            return media == "application/json";
        }
        Record* select(std::string_view id) noexcept {
            for (auto& r : records)
                if (r.id == id)
                    return &r;
            return nullptr;
        }
    };
    struct Job {
        std::array<std::uint8_t, 512> header{};
        std::vector<std::uint8_t> data;
        std::size_t headerSize = 0, dataSize = 0;
        std::uint32_t peer = 0;
        std::uint8_t type = 0, request = 0;
        bool active = false;
    };
    struct Subscription {
        std::array<char, 8> id{};
        std::size_t resource = 0, record = 0;
        std::uint64_t observed = 0, sent = 0;
        std::uint32_t peer = 0;
        std::uint8_t request = 0;
        bool active = false, dirty = false, inFlight = false;
    };
    MidiCiDevice* transport = nullptr;
    CiPropertyStoreCallbacks callbacks;
    std::vector<Resource> resources;
    std::vector<Job> jobs;
    std::vector<Subscription> subscriptions;
    std::vector<std::uint8_t> scratch, edited, page;
    CiPropertyPatch workspace;
    std::size_t read = 0, count = 0;
    std::uint32_t nextSubscription = 0, muid = 0;
    Status error = Status::ok;
    bool processing = false;
    ~State() {
        for (const auto& sub : subscriptions)
            if (sub.active && sub.inFlight)
                transport->cancelPropertyRequest(sub.peer, sub.request);
    }
    void synchronize() noexcept {
        if (muid == transport->muid())
            return;
        muid = transport->muid();
        for (auto& s : subscriptions)
            s.active = false;
        for (auto& j : jobs)
            j.active = false;
        read = count = 0;
    }
    Resource* resource(std::string_view key) noexcept {
        for (auto& r : resources)
            if (r.name == key)
                return &r;
        return nullptr;
    }
    std::span<const std::uint8_t> schema(std::string_view id) const noexcept {
        for (const auto& r : resources)
            if (!r.schemaId.empty() && r.schemaId == id)
                return detail::ciBytes(r.schema);
        return {};
    }
    bool valid(const Resource& r, std::span<const std::uint8_t> data) noexcept {
        if (r.json() && validateCiPropertyJson(data) != Status::ok)
            return false;
        if (r.paginated) {
            std::size_t n = 0;
            std::uint32_t total = 0;
            if (workspace.list(data, 0, std::numeric_limits<std::uint32_t>::max(), page, n,
                               total) != Status::ok)
                return false;
        }
        return !callbacks.validate || callbacks.validate(callbacks.context, r.name, data);
    }
    void changed(Resource& r, Record& record, std::span<const std::uint8_t> data,
                 std::span<const std::uint8_t> delta = {}) noexcept {
        std::copy(data.begin(), data.end(), record.bytes.begin());
        record.size = data.size();
        ++record.revision;
        record.deltaValid = !delta.empty() && delta.size() <= record.delta.size();
        if (record.deltaValid) {
            std::copy(delta.begin(), delta.end(), record.delta.begin());
            record.deltaSize = delta.size();
        }
        const auto ri = static_cast<std::size_t>(&r - resources.data()),
                   di = static_cast<std::size_t>(&record - r.records.data());
        for (auto& sub : subscriptions)
            if (sub.active && sub.resource == ri && sub.record == di)
                sub.dirty = true;
        if (callbacks.changed)
            callbacks.changed(callbacks.context, r.name, record.data());
    }
    void flush() noexcept {
        for (std::size_t turn = 0; count && turn < jobs.size(); ++turn) {
            auto& job = jobs[read];
            if (job.active) {
                const auto r = transport->sendProperty(job.peer, job.type, job.request,
                                                       {job.header.data(), job.headerSize},
                                                       {job.data.data(), job.dataSize});
                if (r == Status::update_queue_full || r == Status::would_block ||
                    r == Status::transition_in_progress)
                    return;
                if (r != Status::ok)
                    error = r;
            }
            job.active = false;
            read = (read + 1) % jobs.size();
            --count;
        }
    }
    Status reply(const CiPropertyMessage& m, unsigned code, std::span<const std::uint8_t> data = {},
                 bool encoded = false, std::string_view media = {},
                 std::string_view subscription = {}, std::optional<std::uint32_t> total = {},
                 std::uint32_t cacheTime = 0) noexcept {
        flush();
        if (count == jobs.size())
            return Status::update_queue_full;
        auto& job = jobs[(read + count) % jobs.size()];
        job.dataSize = 0;
        if (encoded) {
            if (ciEncodeMcoded7(data, job.data, job.dataSize) != Status::ok)
                return reply(m, 413);
        } else {
            if (data.size() > job.data.size())
                return reply(m, 413);
            std::copy(data.begin(), data.end(), job.data.begin());
            job.dataSize = data.size();
        }
        detail::CiJsonWriter w{job.header};
        w.text("{\"status\":");
        w.number(code);
        if (encoded)
            w.text(",\"mutualEncoding\":\"Mcoded7\"");
        if (!media.empty() && media != "application/json") {
            w.character(',');
            pair(w, "mediaType", media);
        }
        if (!subscription.empty()) {
            w.character(',');
            pair(w, "subscribeId", subscription);
        }
        if (total) {
            w.text(",\"totalCount\":");
            w.number(*total);
        }
        if (cacheTime) {
            w.text(",\"cacheTime\":");
            w.number(cacheTime);
        }
        w.character('}');
        if (!w.good)
            return Status::block_too_large;
        job.headerSize = w.position;
        job.peer = m.source;
        job.type = static_cast<std::uint8_t>(m.type + 1);
        job.request = m.request;
        job.active = true;
        ++count;
        return Status::ok;
    }
    void id(Subscription& s) noexcept {
        do {
            ++nextSubscription;
            auto v = nextSubscription;
            s.id[0] = 's';
            for (unsigned i = 8; i-- > 1;) {
                s.id[i] = "0123456789abcdefghijklmnopqrstuvwxyz"[v % 36];
                v /= 36;
            }
        } while (std::any_of(subscriptions.begin(), subscriptions.end(),
                             [&](const auto& p) { return p.active && p.id == s.id; }));
    }
};
CiPropertyStore::CiPropertyStore() = default;
CiPropertyStore::~CiPropertyStore() = default;
CiPropertyStore::CiPropertyStore(CiPropertyStore&&) noexcept = default;
CiPropertyStore& CiPropertyStore::operator=(CiPropertyStore&&) noexcept = default;
Status CiPropertyStore::prepare(MidiCiDevice& transport, const CiDeviceInformation& info,
                                std::span<const CiPropertyDefinition> definitions,
                                const CiPropertyStoreOptions& options,
                                CiPropertyStoreCallbacks callbacks) noexcept {
    const auto capacity = transport.maximumPropertyBytes();
    const auto discovery = transport.localDiscovery();
    if (transport.muid() == ciBroadcastMuid || capacity < 128 || !(discovery.categories & 8) ||
        definitions.size() > 128 || options.subscriptions > 128 || !options.replySlots ||
        options.replySlots > 128)
        return Status::invalid_spec;
    if (info.manufacturer.empty() || info.family.empty() || info.model.empty() ||
        info.version.empty() || info.manufacturer.size() > 512 || info.family.size() > 512 ||
        info.model.size() > 512 || info.version.size() > 512 || info.serialNumber.size() > 512)
        return Status::invalid_argument;
    try {
        auto next = std::make_unique<State>();
        next->transport = &transport;
        next->muid = transport.muid();
        next->callbacks = callbacks;
        auto status =
            next->workspace.prepare(options.maximumJsonNodes, options.maximumPointerBytes);
        if (status != Status::ok)
            return status;
        next->scratch.resize(capacity);
        next->edited.resize(capacity);
        next->page.resize(capacity);
        next->jobs.resize(options.replySlots);
        next->subscriptions.resize(options.subscriptions);
        for (auto& j : next->jobs)
            j.data.resize(capacity);
        next->resources.resize(4 + definitions.size());
        const std::array<std::string_view, 4> names{"ResourceList", "DeviceInfo", "ChannelList",
                                                    "JSONSchema"};
        for (std::size_t i = 0; i < 4; ++i) {
            auto& r = next->resources[i];
            r.name = names[i];
            r.media = "application/json";
            r.builtin = true;
            r.records.resize(1);
            r.records[0].bytes.resize(capacity);
            r.schema = i == 0 || i == 2 ? "{\"type\":\"array\",\"items\":{\"type\":\"object\"}}"
                                        : "{\"type\":\"object\"}";
        }
        next->resources[0].paginated = next->resources[2].paginated = true;
        next->resources[2].subscribable = options.subscriptions != 0;
        next->resources[3].requireId = true;
        std::size_t totalInstances = 0;
        for (std::size_t i = 0; i < definitions.size(); ++i) {
            const auto& source = definitions[i];
            auto& r = next->resources[i + 4];
            if (!resourceName(source.resource) || !source.resource.starts_with("X-") ||
                source.resource.size() == 2 || !source.maximumBytes ||
                source.maximumBytes > capacity || source.mediaType.empty() ||
                source.mediaType.size() > 75 ||
                !std::all_of(source.mediaType.begin(), source.mediaType.end(),
                             [](unsigned char c) { return c > 32 && c < 127; }) ||
                (source.partial && (!source.writable || source.mediaType != "application/json")) ||
                (source.paginated &&
                 (source.mediaType != "application/json" || !source.resource.ends_with("List"))) ||
                (source.delivery != CiSubscriptionDelivery::notify &&
                 source.delivery != CiSubscriptionDelivery::full &&
                 source.delivery != CiSubscriptionDelivery::partial) ||
                (source.delivery == CiSubscriptionDelivery::partial &&
                 source.mediaType != "application/json") ||
                (!source.instances.empty() && !source.data.empty()) ||
                source.instances.size() > 256)
                return Status::invalid_argument;
            for (std::size_t j = 0; j < i; ++j)
                if (source.resource == definitions[j].resource)
                    return Status::invalid_argument;
            r.name = source.resource;
            r.media = source.mediaType;
            r.schema = source.schema;
            r.writable = source.writable;
            r.subscribable = source.subscribable && options.subscriptions;
            r.partial = source.partial;
            r.paginated = source.paginated;
            r.requireId = !source.instances.empty();
            r.cacheTime = source.cacheTimeSeconds;
            r.delivery = source.delivery;
            if (r.json()) {
                if (r.schema.empty())
                    r.schema = "{}";
                if (r.schema.front() != '{' ||
                    validateCiPropertyJson(detail::ciBytes(r.schema)) != Status::ok ||
                    (r.writable && r.schema != "{}" && !callbacks.validate))
                    return Status::invalid_argument;
            } else if (!r.schema.empty())
                return Status::invalid_argument;
            const auto count = r.requireId ? source.instances.size() : std::size_t{1};
            totalInstances += count;
            if (totalInstances > 256)
                return Status::invalid_argument;
            r.records.resize(count);
            for (std::size_t j = 0; j < count; ++j) {
                auto& record = r.records[j];
                const auto& data = r.requireId ? source.instances[j].data : source.data;
                if (r.requireId) {
                    record.id = source.instances[j].id;
                    if (!instanceName(record.id))
                        return Status::invalid_argument;
                    for (std::size_t k = 0; k < j; ++k)
                        if (r.records[k].id == record.id)
                            return Status::invalid_argument;
                }
                if (data.size() > source.maximumBytes || !next->valid(r, data))
                    return Status::invalid_argument;
                record.bytes.resize(source.maximumBytes);
                if (r.delivery == CiSubscriptionDelivery::partial)
                    record.delta.resize(capacity);
                std::copy(data.begin(), data.end(), record.bytes.begin());
                record.size = data.size();
            }
        }
        auto& device = next->resources[1].records[0];
        detail::CiJsonWriter d{device.bytes};
        d.character('{');
        auto numbers = [&](std::string_view key, auto values) {
            (void)d.string(key);
            d.text(":[");
            bool comma = false;
            for (auto value : values) {
                if (comma)
                    d.character(',');
                d.number(static_cast<std::uint32_t>(value));
                comma = true;
            }
            d.character(']');
        };
        numbers("manufacturerId", discovery.identity.manufacturer);
        d.character(',');
        numbers("familyId", std::array<unsigned, 2>{unsigned(discovery.identity.family & 127),
                                                    unsigned(discovery.identity.family >> 7)});
        d.character(',');
        numbers("modelId", std::array<unsigned, 2>{unsigned(discovery.identity.model & 127),
                                                   unsigned(discovery.identity.model >> 7)});
        d.character(',');
        numbers("versionId", discovery.identity.revision);
        for (auto [key, value] :
             {std::pair<std::string_view, std::string_view>{"manufacturer", info.manufacturer},
              {"family", info.family},
              {"model", info.model},
              {"version", info.version}}) {
            d.character(',');
            pair(d, key, value);
        }
        if (!info.serialNumber.empty()) {
            d.character(',');
            pair(d, "serialNumber", info.serialNumber);
        }
        d.character('}');
        if (!d.good)
            return Status::block_too_large;
        device.size = d.position;
        auto& channelData = next->resources[2].records[0];
        status = channels(options.channels, channelData.bytes, channelData.size);
        if (status != Status::ok)
            return status;
        auto& list = next->resources[0].records[0];
        detail::CiJsonWriter w{list.bytes};
        w.character('[');
        for (std::size_t i = 0; i < next->resources.size(); ++i)
            if (next->resources[i].json())
                next->resources[i].schemaId = "schema" + std::to_string(i);
        for (std::size_t i = 1; i < next->resources.size(); ++i) {
            const auto& r = next->resources[i];
            if (i != 1)
                w.character(',');
            w.character('{');
            pair(w, "resource", r.name);
            w.text(",\"canGet\":true,\"canSet\":");
            (void)w.string(r.partial ? "partial" : r.writable ? "full" : "none");
            w.text(",\"canSubscribe\":");
            w.text(r.subscribable ? "true" : "false");
            w.text(",\"canPaginate\":");
            w.text(r.paginated ? "true" : "false");
            w.text(",\"requireResId\":");
            w.text(r.requireId ? "true" : "false");
            w.text(",\"encodings\":[");
            if (r.json())
                w.text("\"ASCII\",");
            w.text("\"Mcoded7\"]");
            if (!r.json()) {
                w.text(",\"mediaTypes\":[");
                (void)w.string(r.media);
                w.character(']');
            }
            if (r.json()) {
                w.text(",\"schema\":{\"$ref\":");
                (void)w.string("midi+jsonschema://" + r.schemaId);
                w.character('}');
            }
            w.character('}');
        }
        w.character(']');
        if (!w.good)
            return Status::block_too_large;
        list.size = w.position;
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
}
Status CiPropertyStore::process(const CiPropertyMessage& m) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (s.processing)
        return Status::transition_in_progress;
    s.synchronize();
    if (!validCiMuid(m.source) || m.destination != s.transport->muid() || m.request > 127)
        return Status::invalid_argument;
    s.processing = true;
    struct Guard {
        bool& v;
        ~Guard() {
            v = false;
        }
    } guard{s.processing};
    CiPropertyHeader h;
    if (h.parse(m.header) != Status::ok)
        return m.type == 0x34 || m.type == 0x36 || m.type == 0x38 ? s.reply(m, 400)
                                                                  : Status::invalid_argument;
    if (m.type == 0x3f || m.type == 0x39) {
        const auto status = h.find("status");
        unsigned code = 0;
        if (h.firstKey() != "status" || !status || status->unsignedInteger(code) != Status::ok ||
            !m.data.empty())
            return Status::invalid_argument;
        if (m.type == 0x3f) {
            if (code != 100 && code != 144 && code != 408)
                return Status::invalid_argument;
            if (code != 100) {
                for (auto& j : s.jobs)
                    if (j.peer == m.source && j.request == m.request)
                        j.active = false;
                for (auto& sub : s.subscriptions)
                    if (sub.active && sub.inFlight && sub.peer == m.source &&
                        sub.request == m.request)
                        sub.active = false;
            }
        } else
            for (auto& sub : s.subscriptions)
                if (sub.active && sub.inFlight && sub.peer == m.source &&
                    sub.request == m.request) {
                    sub.inFlight = false;
                    if (code != 200 && code != 202)
                        sub.active = false;
                    else
                        sub.observed = sub.sent;
                }
        return Status::ok;
    }
    if (m.type != 0x34 && m.type != 0x36 && m.type != 0x38)
        return Status::unsupported_format;
    std::array<char, 36> resourceBuffer{}, resBuffer{};
    std::array<char, 16> encBuffer{}, cmdBuffer{};
    std::array<char, 75> mediaBuffer{};
    std::array<char, 8> idBuffer{};
    std::string_view name, resId, encoding, command, media, id;
    if (!field(h, "resource", resourceBuffer, name) || !field(h, "resId", resBuffer, resId) ||
        !field(h, "mutualEncoding", encBuffer, encoding) ||
        !field(h, "mediaType", mediaBuffer, media) || !field(h, "command", cmdBuffer, command) ||
        !field(h, "subscribeId", idBuffer, id) ||
        (m.type == 0x38 ? h.firstKey() != "command" : h.firstKey() != "resource") ||
        (h.find("resId") && !instanceName(resId)))
        return s.reply(m, 400);
    if (m.type == 0x38 && command == "end") {
        if (!m.data.empty() || id.empty())
            return s.reply(m, 400);
        auto sub = std::find_if(s.subscriptions.begin(), s.subscriptions.end(), [&](const auto& p) {
            return p.active && p.peer == m.source &&
                   std::string_view(p.id.data(), p.id.size()) == id;
        });
        if (sub == s.subscriptions.end())
            return s.reply(m, 404);
        const auto result = s.reply(m, 200);
        if (result == Status::ok) {
            if (sub->inFlight)
                s.transport->cancelPropertyRequest(sub->peer, sub->request);
            sub->active = false;
        }
        return result;
    }
    if (!resourceName(name))
        return s.reply(m, 400);
    auto* r = s.resource(name);
    if (!r)
        return s.reply(m, 404);
    if (s.callbacks.authorize &&
        !s.callbacks.authorize(s.callbacks.context, m.source, name, m.type == 0x36))
        return s.reply(m, 403);
    if (encoding.empty())
        encoding = "ASCII";
    const bool encoded = encoding == "Mcoded7";
    if ((!encoded && encoding != "ASCII") || (!encoded && !r->json()) ||
        (!media.empty() && media != r->media))
        return s.reply(m, 415);
    const auto offsetValue = h.find("offset"), limitValue = h.find("limit");
    std::uint32_t offset = 0, limit = std::numeric_limits<std::uint32_t>::max();
    if (bool(offsetValue) != bool(limitValue) ||
        ((offsetValue || limitValue) &&
         (m.type != 0x34 || !r->paginated || offsetValue->unsignedInteger(offset) != Status::ok ||
          limitValue->unsignedInteger(limit) != Status::ok || !limit)))
        return s.reply(m, 400);
    if (name == "JSONSchema") {
        if (m.type != 0x34)
            return s.reply(m, 405);
        if (!m.data.empty() || resId.empty())
            return s.reply(m, 400);
        const auto schema = s.schema(resId);
        return schema.empty() ? s.reply(m, 404) : s.reply(m, 200, schema, encoded);
    }
    if (r->requireId != !resId.empty())
        return s.reply(m, 400);
    auto* record = r->select(resId);
    if (!record)
        return s.reply(m, 404);
    if (m.type == 0x34) {
        if (!m.data.empty())
            return s.reply(m, 400);
        if (r->paginated) {
            std::size_t n = 0;
            std::uint32_t total = 0;
            const auto result = s.workspace.list(record->data(), offset, limit, s.page, n, total);
            if (result != Status::ok)
                return s.reply(m, result == Status::block_too_large ? 413 : 400);
            return s.reply(m, 200, std::span<const std::uint8_t>(s.page).first(n), encoded,
                           r->media, {}, total, r->cacheTime);
        }
        return s.reply(m, 200, record->data(), encoded, r->media, {}, {}, r->cacheTime);
    }
    if (m.type == 0x36) {
        if (!r->writable)
            return s.reply(m, 405);
        bool partial = false;
        if (const auto value = h.find("setPartial"); value && value->boolean(partial) != Status::ok)
            return s.reply(m, 400);
        if (partial && !r->partial)
            return s.reply(m, 405);
        if (!r->json() && media.empty())
            return s.reply(m, 415);
        auto data = m.data;
        if (encoded) {
            std::size_t n = 0;
            const auto result = ciDecodeMcoded7(data, s.scratch, n);
            if (result != Status::ok)
                return s.reply(m, result == Status::block_too_large ? 413 : 400);
            data = std::span<const std::uint8_t>(s.scratch).first(n);
        }
        const auto delta = partial ? data : std::span<const std::uint8_t>{};
        if (partial) {
            std::size_t n = 0;
            const auto result = s.workspace.apply(record->data(), data, s.edited, n);
            if (result != Status::ok)
                return s.reply(m, result == Status::block_too_large ? 413 : 400);
            data = std::span<const std::uint8_t>(s.edited).first(n);
        }
        if (data.size() > record->bytes.size())
            return s.reply(m, 413);
        if (!s.valid(*r, data))
            return s.reply(m, 400);
        const auto result = s.reply(m, 200);
        if (result == Status::ok)
            s.changed(*r, *record, data, delta);
        return result;
    }
    if (command != "start" || !m.data.empty() || !id.empty())
        return s.reply(m, 400);
    if (!r->subscribable)
        return s.reply(m, 405);
    const auto ri = static_cast<std::size_t>(r - s.resources.data()),
               di = static_cast<std::size_t>(record - r->records.data());
    auto where = std::find_if(s.subscriptions.begin(), s.subscriptions.end(), [&](const auto& p) {
        return p.active && p.peer == m.source && p.resource == ri && p.record == di;
    });
    if (where == s.subscriptions.end()) {
        where = std::find_if(s.subscriptions.begin(), s.subscriptions.end(),
                             [](const auto& p) { return !p.active; });
        if (where == s.subscriptions.end())
            return s.reply(m, 343);
        s.id(*where);
    }
    const auto result =
        s.reply(m, 200, {}, false, {}, std::string_view(where->id.data(), where->id.size()));
    if (result == Status::ok && !where->active) {
        where->active = true;
        where->peer = m.source;
        where->resource = ri;
        where->record = di;
        where->observed = record->revision;
        where->dirty = where->inFlight = false;
    }
    return result;
}
Status CiPropertyStore::publish(std::string_view name, std::span<const std::uint8_t> data,
                                std::string_view id) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (s.processing)
        return Status::transition_in_progress;
    auto* r = s.resource(name);
    if (!r || r->builtin)
        return Status::invalid_argument;
    auto* record = r->select(id);
    if (!record)
        return Status::invalid_argument;
    if (detail::overlaps(data.data(), data.size(), record->bytes.data(), record->bytes.size()))
        return Status::partial_overlap;
    if (data.size() > record->bytes.size() || !s.valid(*r, data))
        return Status::invalid_argument;
    s.processing = true;
    s.changed(*r, *record, data);
    s.processing = false;
    return Status::ok;
}
Status CiPropertyStore::publishChannels(std::span<const CiChannelDescription> list) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (s.processing)
        return Status::transition_in_progress;
    std::size_t n = 0;
    const auto result = channels(list, s.scratch, n);
    if (result != Status::ok)
        return result;
    auto& r = s.resources[2];
    const auto data = std::span<const std::uint8_t>(s.scratch).first(n);
    if (!s.valid(r, data))
        return Status::invalid_argument;
    s.processing = true;
    s.changed(r, r.records[0], data);
    s.processing = false;
    return Status::ok;
}
std::span<const std::uint8_t> CiPropertyStore::data(std::string_view name,
                                                    std::string_view id) const noexcept {
    if (!state_)
        return {};
    if (name == "JSONSchema")
        return state_->schema(id);
    auto* r = state_->resource(name);
    const auto* record = r ? r->select(id) : nullptr;
    return record ? record->data() : std::span<const std::uint8_t>{};
}
void CiPropertyStore::poll(std::uint64_t now) noexcept {
    if (!state_ || state_->processing)
        return;
    auto& s = *state_;
    s.synchronize();
    s.transport->poll(now);
    s.flush();
    for (auto& sub : s.subscriptions) {
        if (!sub.active)
            continue;
        if (sub.inFlight) {
            if (!s.transport->propertyRequestActive(sub.peer, sub.request))
                sub.active = false;
            continue;
        }
        if (!sub.dirty)
            continue;
        auto& r = s.resources[sub.resource];
        auto& record = r.records[sub.record];
        const bool delta = r.delivery == CiSubscriptionDelivery::partial && record.deltaValid &&
                           sub.observed + 1 == record.revision;
        const bool notify = r.delivery == CiSubscriptionDelivery::notify;
        std::array<std::uint8_t, 256> header{};
        detail::CiJsonWriter w{header};
        w.text("{\"command\":");
        (void)w.string(notify ? "notify" : delta ? "partial" : "full");
        w.text(",\"subscribeId\":");
        (void)w.string({sub.id.data(), sub.id.size()});
        std::span<const std::uint8_t> payload =
            notify  ? std::span<const std::uint8_t>{}
            : delta ? std::span<const std::uint8_t>(record.delta).first(record.deltaSize)
                    : record.data();
        if (!notify && !r.json()) {
            std::size_t n = 0;
            const auto result = ciEncodeMcoded7(payload, s.scratch, n);
            if (result != Status::ok) {
                s.error = result;
                sub.active = false;
                continue;
            }
            payload = std::span<const std::uint8_t>(s.scratch).first(n);
            w.text(",\"mutualEncoding\":\"Mcoded7\",");
            pair(w, "mediaType", r.media);
        }
        w.character('}');
        const auto result = s.transport->requestProperty(
            sub.peer, 0x38, {header.data(), w.position}, payload, now, sub.request);
        if (result == Status::update_queue_full || result == Status::would_block)
            break;
        if (result != Status::ok) {
            s.error = result;
            sub.active = false;
        } else {
            sub.inFlight = true;
            sub.sent = record.revision;
            sub.dirty = false;
        }
    }
}
void CiPropertyStore::invalidate(std::uint32_t peer) noexcept {
    if (!state_)
        return;
    for (auto& s : state_->subscriptions)
        if (s.peer == peer) {
            if (s.inFlight)
                state_->transport->cancelPropertyRequest(s.peer, s.request);
            s.active = false;
        }
    for (auto& j : state_->jobs)
        if (j.peer == peer)
            j.active = false;
}
void CiPropertyStore::reset() noexcept {
    if (!state_)
        return;
    for (auto& s : state_->subscriptions) {
        if (s.active && s.inFlight)
            state_->transport->cancelPropertyRequest(s.peer, s.request);
        s.active = false;
    }
    for (auto& j : state_->jobs)
        j.active = false;
    state_->read = state_->count = 0;
    state_->error = Status::ok;
}
std::size_t CiPropertyStore::activeSubscriptions() const noexcept {
    return state_ ? static_cast<std::size_t>(std::count_if(state_->subscriptions.begin(),
                                                           state_->subscriptions.end(),
                                                           [](const auto& s) { return s.active; }))
                  : 0;
}
Status CiPropertyStore::lastError() const noexcept {
    return state_ ? state_->error : Status::not_prepared;
}
} // namespace cydsp
