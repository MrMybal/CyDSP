#include "cydsp/midi/CiPropertyClient.h"
#include "detail/CiJson.h"
#include <algorithm>
namespace cydsp {
namespace {
bool validQuery(const CiPropertyQuery& q) noexcept {
    if (!validCiMuid(q.peer) || q.resource.empty() || q.resource.size() > 36 ||
        q.resId.size() > 36 || q.mediaType.size() > 75 || (q.page && !q.page->limit) ||
        (q.encoding != CiPropertyEncoding::ascii && q.encoding != CiPropertyEncoding::mcoded7))
        return false;
    const auto ascii = [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
    };
    return std::all_of(q.resource.begin(), q.resource.end(),
                       [&](auto c) { return ascii(static_cast<unsigned char>(c)) || c == '-'; }) &&
           std::all_of(q.resId.begin(), q.resId.end(),
                       [&](auto c) { return ascii(static_cast<unsigned char>(c)) || c == '_'; }) &&
           std::all_of(q.mediaType.begin(), q.mediaType.end(),
                       [](unsigned char c) { return c > 32 && c < 127; });
}
template <std::size_t N>
std::string_view view(const std::array<char, N>& b, std::size_t n) noexcept {
    return {b.data(), n};
}
template <std::size_t N>
void copy(std::string_view s, std::array<char, N>& out, std::size_t& n) noexcept {
    std::copy(s.begin(), s.end(), out.begin());
    n = s.size();
}
template <std::size_t N>
bool field(const CiPropertyHeader& h, std::string_view key, std::array<char, N>& out,
           std::string_view& result) noexcept {
    const auto value = h.find(key);
    std::size_t n = 0;
    if (!value) {
        result = {};
        return true;
    }
    if (value->string(out, n) != Status::ok)
        return false;
    result = view(out, n);
    return true;
}
bool subscriptionId(std::string_view id) noexcept {
    return !id.empty() && id.size() <= 8 && std::all_of(id.begin(), id.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
    });
}
} // namespace
struct CiPropertyClient::State {
    static constexpr std::size_t none = std::numeric_limits<std::size_t>::max();
    struct Entry {
        std::array<char, 36> resource{}, resId{};
        std::array<char, 75> requestedMedia{}, media{};
        std::size_t resourceSize = 0, resIdSize = 0, requestedMediaSize = 0, mediaSize = 0,
                    size = 0;
        std::uint32_t peer = 0;
        CiPropertyEncoding encoding = CiPropertyEncoding::ascii;
        std::optional<CiPropertyPage> page;
        std::optional<std::uint32_t> totalCount;
        std::uint64_t expires = 0, generation = 0, use = 0;
        bool active = false, hasData = false, fresh = false, subscribed = false, refresh = false,
             baseline = false;
        std::vector<std::uint8_t> data;
        CiPropertyQuery query() const noexcept {
            return {peer,
                    view(resource, resourceSize),
                    view(resId, resIdSize),
                    view(requestedMedia, requestedMediaSize),
                    encoding,
                    page};
        }
        bool matches(const CiPropertyQuery& q) const noexcept {
            return active && peer == q.peer && view(resource, resourceSize) == q.resource &&
                   view(resId, resIdSize) == q.resId &&
                   view(requestedMedia, requestedMediaSize) == q.mediaType &&
                   bool(page) == bool(q.page) &&
                   (!page || (page->offset == q.page->offset && page->limit == q.page->limit));
        }
        bool isFresh(std::uint64_t now) const noexcept {
            return hasData && fresh && !refresh && (subscribed || now < expires);
        }
        CiPropertyCacheView snapshot(std::uint64_t now) const noexcept {
            return {{data.data(), size}, view(media, mediaSize), totalCount, expires, isFresh(now)};
        }
    };
    enum class Operation { get, set, start, end };
    struct Pending {
        std::size_t entry = 0, sub = none;
        std::uint8_t id = 0, type = 0;
        std::uint64_t generation = 0;
        Operation op = Operation::get;
        bool active = false;
    };
    struct Subscription {
        std::size_t entry = 0, idSize = 0;
        std::array<char, 8> id{};
        bool active = false, starting = false;
    };
    struct Ack {
        std::uint32_t peer = 0;
        std::uint8_t id = 0;
        unsigned code = 200;
        bool active = false;
    };
    MidiCiDevice* transport = nullptr;
    CiPropertyClientCallbacks callbacks;
    std::vector<Entry> entries;
    std::vector<Pending> pending;
    std::vector<Subscription> subscriptions;
    std::vector<Ack> acks;
    std::vector<std::uint8_t> scratch, edited;
    CiPropertyPatch patch;
    std::uint32_t muid = 0;
    std::uint64_t uses = 0;
    std::size_t ackRead = 0, ackCount = 0;
    Status error = Status::ok;
    bool processing = false;
    ~State() {
        clear();
    }
    void clear() noexcept {
        for (auto& p : pending)
            if (p.active) {
                transport->cancelPropertyRequest(entries[p.entry].peer, p.id);
                p.active = false;
            }
        for (auto& e : entries) {
            e.active = e.hasData = e.fresh = e.subscribed = e.refresh = e.baseline = false;
            ++e.generation;
        }
        for (auto& sub : subscriptions)
            sub.active = sub.starting = false;
        for (auto& ack : acks)
            ack.active = false;
        ackRead = ackCount = 0;
    }
    void synchronize() noexcept {
        if (muid != transport->muid()) {
            clear();
            muid = transport->muid();
        }
    }
    Entry* find(const CiPropertyQuery& q) noexcept {
        for (auto& e : entries)
            if (e.matches(q))
                return &e;
        return nullptr;
    }
    bool busy(std::size_t i) const noexcept {
        return entries[i].subscribed ||
               std::any_of(pending.begin(), pending.end(),
                           [&](const auto& p) { return p.active && p.entry == i; });
    }
    Entry* ensure(const CiPropertyQuery& q) noexcept {
        if (auto* e = find(q)) {
            e->encoding = q.encoding;
            e->use = ++uses;
            return e;
        }
        Entry* result = nullptr;
        for (std::size_t i = 0; i < entries.size(); ++i) {
            auto& e = entries[i];
            if (!e.active) {
                result = &e;
                break;
            }
            if (!busy(i) && (!result || e.use < result->use))
                result = &e;
        }
        if (!result)
            return nullptr;
        auto& e = *result;
        e.peer = q.peer;
        copy(q.resource, e.resource, e.resourceSize);
        copy(q.resId, e.resId, e.resIdSize);
        copy(q.mediaType, e.requestedMedia, e.requestedMediaSize);
        e.encoding = q.encoding;
        e.page = q.page;
        e.active = true;
        e.hasData = e.fresh = e.subscribed = e.refresh = e.baseline = false;
        e.totalCount.reset();
        e.size = 0;
        e.use = ++uses;
        ++e.generation;
        return &e;
    }
    bool getting(std::size_t entry) const noexcept {
        return std::any_of(pending.begin(), pending.end(), [&](const auto& p) {
            return p.active && p.entry == entry && p.op == Operation::get;
        });
    }
    void complete(Pending& p, unsigned code) noexcept {
        p.active = false;
        auto& e = entries[p.entry];
        if (callbacks.completed)
            callbacks.completed(callbacks.context, e.query(), p.type, code);
    }
    void subChanged(Subscription& sub, bool active) noexcept {
        auto& e = entries[sub.entry];
        sub.active = active;
        sub.starting = false;
        e.subscribed = active;
        if (!active)
            e.fresh = false;
        if (callbacks.subscription)
            callbacks.subscription(callbacks.context, e.query(), active);
    }
    Status request(Entry& e, Operation operation, std::span<const std::uint8_t> data,
                   std::uint64_t now, bool partial = false, std::size_t sub = none) noexcept {
        auto where =
            std::find_if(pending.begin(), pending.end(), [](const auto& p) { return !p.active; });
        if (where == pending.end())
            return Status::update_queue_full;
        const auto q = e.query();
        std::array<std::uint8_t, 512> header{};
        detail::CiJsonWriter w{header};
        auto pair = [&](std::string_view k, std::string_view v) {
            (void)w.string(k);
            w.character(':');
            if (!w.string(v))
                w.good = false;
        };
        w.character('{');
        if (operation == Operation::end) {
            pair("command", "end");
            w.character(',');
            pair("subscribeId", view(subscriptions[sub].id, subscriptions[sub].idSize));
        } else {
            if (operation == Operation::start) {
                pair("command", "start");
                w.character(',');
            }
            pair("resource", q.resource);
            if (!q.resId.empty()) {
                w.character(',');
                pair("resId", q.resId);
            }
            if (q.encoding == CiPropertyEncoding::mcoded7) {
                w.character(',');
                pair("mutualEncoding", "Mcoded7");
            }
            if (!q.mediaType.empty()) {
                w.character(',');
                pair("mediaType", q.mediaType);
            }
            if (q.page) {
                w.text(",\"offset\":");
                w.number(q.page->offset);
                w.text(",\"limit\":");
                w.number(q.page->limit);
            }
            if (partial)
                w.text(",\"setPartial\":true");
        }
        w.character('}');
        if (!w.good)
            return Status::block_too_large;
        if (operation == Operation::set && q.encoding == CiPropertyEncoding::mcoded7) {
            std::size_t n = 0;
            const auto r = ciEncodeMcoded7(data, scratch, n);
            if (r != Status::ok)
                return r;
            data = std::span<const std::uint8_t>(scratch).first(n);
        }
        const std::uint8_t type = operation == Operation::get   ? 0x34
                                  : operation == Operation::set ? 0x36
                                                                : 0x38;
        std::uint8_t id = 0;
        const auto result =
            transport->requestProperty(q.peer, type, {header.data(), w.position}, data, now, id);
        if (result == Status::ok)
            *where = {static_cast<std::size_t>(&e - entries.data()),
                      sub,
                      id,
                      type,
                      e.generation,
                      operation,
                      true};
        return result;
    }
    void flush() noexcept {
        for (std::size_t i = 0; ackCount && i < acks.size(); ++i) {
            auto& ack = acks[ackRead];
            if (ack.active) {
                std::array<std::uint8_t, 32> header{};
                detail::CiJsonWriter w{header};
                w.text("{\"status\":");
                w.number(ack.code);
                w.character('}');
                const auto r = transport->sendProperty(ack.peer, 0x39, ack.id,
                                                       {header.data(), w.position}, {});
                if (r == Status::update_queue_full || r == Status::would_block ||
                    r == Status::transition_in_progress)
                    return;
                if (r != Status::ok)
                    error = r;
            }
            ack.active = false;
            ackRead = (ackRead + 1) % acks.size();
            --ackCount;
        }
    }
    Status ack(const CiPropertyMessage& m, unsigned code) noexcept {
        flush();
        if (ackCount == acks.size())
            return Status::update_queue_full;
        acks[(ackRead + ackCount) % acks.size()] = {m.source, m.request, code, true};
        ++ackCount;
        return Status::ok;
    }
    Status decode(const CiPropertyMessage& m, const CiPropertyHeader& h,
                  std::span<const std::uint8_t>& data, std::string_view& media,
                  std::array<char, 75>& mediaBuffer) noexcept {
        std::array<char, 16> encodingBuffer{};
        std::string_view encoding;
        if (!field(h, "mutualEncoding", encodingBuffer, encoding) ||
            !field(h, "mediaType", mediaBuffer, media))
            return Status::invalid_argument;
        data = m.data;
        if (encoding == "Mcoded7") {
            std::size_t n = 0;
            const auto r = ciDecodeMcoded7(data, scratch, n);
            if (r != Status::ok)
                return r;
            data = std::span<const std::uint8_t>(scratch).first(n);
        } else if (!encoding.empty() && encoding != "ASCII")
            return Status::unsupported_format;
        if (media.empty())
            media = "application/json";
        if (media == "application/json" && validateCiPropertyJson(data) != Status::ok)
            return Status::invalid_argument;
        if (data.size() > entries[0].data.size())
            return Status::block_too_large;
        return Status::ok;
    }
    void update(Entry& e, std::span<const std::uint8_t> data, std::string_view media,
                std::uint64_t now, std::uint32_t cacheTime, bool partial) noexcept {
        std::copy(data.begin(), data.end(), e.data.begin());
        e.size = data.size();
        copy(media, e.media, e.mediaSize);
        const auto duration = std::uint64_t(cacheTime) * 1000;
        e.expires = duration > std::numeric_limits<std::uint64_t>::max() - now
                        ? std::numeric_limits<std::uint64_t>::max()
                        : now + duration;
        e.hasData = e.fresh = e.baseline = true;
        e.refresh = false;
        e.use = ++uses;
        if (callbacks.updated)
            callbacks.updated(callbacks.context, e.query(), e.snapshot(now), partial);
    }
};
CiPropertyClient::CiPropertyClient() = default;
CiPropertyClient::~CiPropertyClient() = default;
CiPropertyClient::CiPropertyClient(CiPropertyClient&&) noexcept = default;
CiPropertyClient& CiPropertyClient::operator=(CiPropertyClient&&) noexcept = default;
Status CiPropertyClient::prepare(MidiCiDevice& ci, const CiPropertyClientOptions& options,
                                 CiPropertyClientCallbacks callbacks) noexcept {
    if (ci.muid() == ciBroadcastMuid || !(ci.localDiscovery().categories & 8) || !options.entries ||
        options.entries > 256 || !options.transactions || options.transactions > 128 ||
        options.subscriptions > 128 || !options.acknowledgements ||
        options.acknowledgements > 256 || !options.maximumBytes ||
        options.maximumBytes > ci.maximumPropertyBytes())
        return Status::invalid_spec;
    try {
        auto next = std::make_unique<State>();
        next->transport = &ci;
        next->muid = ci.muid();
        next->callbacks = callbacks;
        auto r = next->patch.prepare(options.maximumJsonNodes, options.maximumPointerBytes);
        if (r != Status::ok)
            return r;
        next->entries.resize(options.entries);
        for (auto& e : next->entries)
            e.data.resize(options.maximumBytes);
        next->pending.resize(options.transactions);
        next->subscriptions.resize(options.subscriptions);
        next->acks.resize(options.acknowledgements);
        next->scratch.resize(ci.maximumPropertyBytes());
        next->edited.resize(options.maximumBytes);
        state_.swap(next);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status CiPropertyClient::fetch(const CiPropertyQuery& q, std::uint64_t now, bool force) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!validQuery(q))
        return Status::invalid_argument;
    auto& s = *state_;
    if (s.processing)
        return Status::transition_in_progress;
    s.synchronize();
    auto* e = s.ensure(q);
    if (!e)
        return Status::update_queue_full;
    if (!force && e->isFresh(now))
        return Status::ok;
    if (s.getting(static_cast<std::size_t>(e - s.entries.data())))
        return Status::transition_in_progress;
    return s.request(*e, State::Operation::get, {}, now);
}
Status CiPropertyClient::set(const CiPropertyQuery& q, std::span<const std::uint8_t> data,
                             std::uint64_t now, bool partial) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!validQuery(q) || q.page ||
        (partial && !q.mediaType.empty() && q.mediaType != "application/json"))
        return Status::invalid_argument;
    auto& s = *state_;
    if (s.processing)
        return Status::transition_in_progress;
    s.synchronize();
    auto* e = s.ensure(q);
    if (!e)
        return Status::update_queue_full;
    if (data.size() > e->data.size())
        return Status::block_too_large;
    if ((q.mediaType.empty() || q.mediaType == "application/json") &&
        validateCiPropertyJson(data) != Status::ok)
        return Status::invalid_argument;
    ++e->generation;
    const auto r = s.request(*e, State::Operation::set, data, now, partial);
    if (r == Status::ok) {
        e->fresh = false;
        if (e->subscribed)
            e->refresh = true;
    }
    return r;
}
Status CiPropertyClient::subscribe(const CiPropertyQuery& q, std::uint64_t now) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!validQuery(q) || q.page)
        return Status::invalid_argument;
    auto& s = *state_;
    if (s.processing)
        return Status::transition_in_progress;
    s.synchronize();
    auto* e = s.ensure(q);
    if (!e)
        return Status::update_queue_full;
    const auto index = static_cast<std::size_t>(e - s.entries.data());
    for (auto& sub : s.subscriptions)
        if ((sub.active || sub.starting) && sub.entry == index)
            return sub.active ? Status::ok : Status::transition_in_progress;
    auto sub = std::find_if(s.subscriptions.begin(), s.subscriptions.end(),
                            [](const auto& p) { return !p.active && !p.starting; });
    if (sub == s.subscriptions.end())
        return Status::update_queue_full;
    const auto r = s.request(*e, State::Operation::start, {}, now, false,
                             static_cast<std::size_t>(sub - s.subscriptions.begin()));
    if (r == Status::ok) {
        sub->entry = index;
        sub->starting = true;
    }
    return r;
}
Status CiPropertyClient::unsubscribe(const CiPropertyQuery& q, std::uint64_t now) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!validQuery(q) || q.page)
        return Status::invalid_argument;
    auto& s = *state_;
    if (s.processing)
        return Status::transition_in_progress;
    s.synchronize();
    auto* e = s.find(q);
    if (!e)
        return Status::invalid_argument;
    const auto index = static_cast<std::size_t>(e - s.entries.data());
    for (std::size_t i = 0; i < s.subscriptions.size(); ++i)
        if (s.subscriptions[i].entry == index) {
            auto& sub = s.subscriptions[i];
            if (sub.starting)
                return Status::transition_in_progress;
            if (sub.active)
                return s.request(*e, State::Operation::end, {}, now, false, i);
        }
    return Status::invalid_argument;
}
Status CiPropertyClient::process(const CiPropertyMessage& m, std::uint64_t now) noexcept {
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
        bool& value;
        ~Guard() {
            value = false;
        }
    } guard{s.processing};
    CiPropertyHeader h;
    if (h.parse(m.header) != Status::ok)
        return Status::invalid_argument;
    if (m.type == 0x38) {
        std::array<char, 16> commandBuffer{};
        std::array<char, 8> idBuffer{};
        std::string_view command, id;
        if (h.firstKey() != "command" || !field(h, "command", commandBuffer, command) ||
            !field(h, "subscribeId", idBuffer, id) || !subscriptionId(id))
            return s.ack(m, 400);
        auto sub = std::find_if(s.subscriptions.begin(), s.subscriptions.end(), [&](const auto& p) {
            return p.active && s.entries[p.entry].peer == m.source && view(p.id, p.idSize) == id;
        });
        if (sub == s.subscriptions.end())
            return s.ack(m, 404);
        auto& e = s.entries[sub->entry];
        auto reject = [&](unsigned code) noexcept {
            const auto r = s.ack(m, code);
            if (r == Status::ok) {
                ++e.generation;
                e.fresh = e.baseline = false;
                e.refresh = true;
                s.subChanged(*sub, false);
            }
            return r;
        };
        if (command == "notify" || command == "end") {
            if (!m.data.empty())
                return reject(400);
            const auto r = s.ack(m, 200);
            if (r != Status::ok)
                return r;
            ++e.generation;
            e.fresh = false;
            if (command == "notify") {
                e.refresh = true;
                e.baseline = false;
            } else {
                e.refresh = false;
                s.subChanged(*sub, false);
            }
            return Status::ok;
        }
        if (command != "full" && command != "partial")
            return reject(400);
        std::span<const std::uint8_t> data;
        std::string_view media;
        std::array<char, 75> mediaBuffer{};
        auto result = s.decode(m, h, data, media, mediaBuffer);
        if (result != Status::ok)
            return reject(result == Status::block_too_large      ? 413
                          : result == Status::unsupported_format ? 415
                                                                 : 400);
        if (!e.query().mediaType.empty() && e.query().mediaType != media)
            return reject(415);
        const bool partial = command == "partial";
        if (partial) {
            if (!e.hasData || !e.baseline || view(e.media, e.mediaSize) != "application/json" ||
                media != "application/json") {
                return reject(400);
            }
            std::size_t n = 0;
            result = s.patch.apply({e.data.data(), e.size}, data, s.edited, n);
            if (result != Status::ok)
                return reject(result == Status::block_too_large ? 413 : 400);
            data = std::span<const std::uint8_t>(s.edited).first(n);
        }
        const auto r = s.ack(m, 200);
        if (r != Status::ok)
            return r;
        ++e.generation;
        if (!partial)
            e.totalCount.reset();
        s.update(e, data, media, now, 0, partial);
        return Status::ok;
    }
    if (m.type == 0x3f) {
        unsigned code = 0;
        const auto status = h.find("status");
        if (h.firstKey() != "status" || !status || status->unsignedInteger(code) != Status::ok ||
            !m.data.empty() || (code != 100 && code != 144 && code != 408))
            return Status::invalid_argument;
        if (code != 100)
            for (auto& p : s.pending)
                if (p.active && p.id == m.request && s.entries[p.entry].peer == m.source) {
                    if (p.sub != State::none)
                        s.subChanged(s.subscriptions[p.sub], false);
                    s.entries[p.entry].fresh = s.entries[p.entry].refresh = false;
                    s.complete(p, code);
                }
        return Status::ok;
    }
    auto p = std::find_if(s.pending.begin(), s.pending.end(), [&](const auto& value) {
        return value.active && value.id == m.request && value.type + 1 == m.type &&
               s.entries[value.entry].peer == m.source;
    });
    if (p == s.pending.end())
        return Status::ok;
    auto& e = s.entries[p->entry];
    unsigned code = 0;
    const auto status = h.find("status");
    if (h.firstKey() != "status" || !status || status->unsignedInteger(code) != Status::ok)
        return Status::invalid_argument;
    if (code != 200 && code != 202) {
        if (p->sub != State::none)
            s.subChanged(s.subscriptions[p->sub], false);
        if (p->generation == e.generation)
            e.refresh = false;
        s.complete(*p, code);
        return Status::ok;
    }
    if (p->op == State::Operation::get && code == 200) {
        std::span<const std::uint8_t> data;
        std::string_view media;
        std::array<char, 75> mediaBuffer{};
        const auto result = s.decode(m, h, data, media, mediaBuffer);
        if (result != Status::ok)
            return result;
        if (!e.query().mediaType.empty() && e.query().mediaType != media)
            return Status::unsupported_format;
        unsigned cacheTime = 0;
        if (const auto value = h.find("cacheTime");
            value && value->unsignedInteger(cacheTime) != Status::ok)
            return Status::invalid_argument;
        std::optional<std::uint32_t> total;
        if (const auto value = h.find("totalCount")) {
            unsigned n;
            if (value->unsignedInteger(n) != Status::ok)
                return Status::invalid_argument;
            total = n;
        }
        if (p->generation == e.generation) {
            e.totalCount = total;
            s.update(e, data, media, now, cacheTime, false);
        }
    } else if (p->op == State::Operation::get) {
        if (p->generation == e.generation)
            e.refresh = e.fresh = false;
    } else if (p->op == State::Operation::start) {
        std::array<char, 8> buffer{};
        std::string_view id;
        if (!m.data.empty() || !field(h, "subscribeId", buffer, id) || !subscriptionId(id))
            return Status::invalid_argument;
        for (const auto& sub : s.subscriptions)
            if (sub.active && s.entries[sub.entry].peer == m.source &&
                view(sub.id, sub.idSize) == id)
                return Status::invalid_argument;
        auto& sub = s.subscriptions[p->sub];
        copy(id, sub.id, sub.idSize);
        s.subChanged(sub, true);
        e.refresh = true;
        e.baseline = false;
        ++e.generation;
    } else if (p->op == State::Operation::end) {
        if (!m.data.empty())
            return Status::invalid_argument;
        s.subChanged(s.subscriptions[p->sub], false);
        e.refresh = false;
    } else if (p->op == State::Operation::set) {
        if (!m.data.empty())
            return Status::invalid_argument;
        if (p->generation == e.generation) {
            e.fresh = false;
            if (e.subscribed)
                e.refresh = true;
        }
    }
    s.complete(*p, code);
    return Status::ok;
}
void CiPropertyClient::poll(std::uint64_t now) noexcept {
    if (!state_ || state_->processing)
        return;
    auto& s = *state_;
    s.synchronize();
    s.transport->poll(now);
    s.flush();
    for (auto& p : s.pending)
        if (p.active && !s.transport->propertyRequestActive(s.entries[p.entry].peer, p.id)) {
            if (p.sub != State::none)
                s.subChanged(s.subscriptions[p.sub], false);
            s.entries[p.entry].fresh = s.entries[p.entry].refresh = false;
            s.error = Status::timed_out;
            s.complete(p, 408);
        }
    for (auto& e : s.entries)
        if (e.active && e.refresh && !s.getting(static_cast<std::size_t>(&e - s.entries.data()))) {
            const auto r = s.request(e, State::Operation::get, {}, now);
            if (r == Status::update_queue_full || r == Status::would_block)
                break;
            if (r != Status::ok) {
                s.error = r;
                e.refresh = false;
            }
        }
}
std::optional<CiPropertyCacheView> CiPropertyClient::cached(const CiPropertyQuery& q,
                                                            std::uint64_t now,
                                                            bool allowStale) const noexcept {
    if (!state_ || state_->muid != state_->transport->muid())
        return {};
    const auto* e = state_->find(q);
    return e && e->hasData && (allowStale || e->isFresh(now))
               ? std::optional<CiPropertyCacheView>(e->snapshot(now))
               : std::nullopt;
}
void CiPropertyClient::invalidate(std::uint32_t peer) noexcept {
    if (!state_)
        return;
    auto& s = *state_;
    for (auto& p : s.pending)
        if (p.active && s.entries[p.entry].peer == peer) {
            s.transport->cancelPropertyRequest(peer, p.id);
            p.active = false;
        }
    for (auto& sub : s.subscriptions)
        if ((sub.active || sub.starting) && s.entries[sub.entry].peer == peer)
            sub.active = sub.starting = false;
    for (auto& e : s.entries)
        if (e.peer == peer) {
            e.active = e.hasData = e.fresh = e.subscribed = e.refresh = e.baseline = false;
            ++e.generation;
        }
    for (auto& ack : s.acks)
        if (ack.peer == peer)
            ack.active = false;
}
void CiPropertyClient::reset() noexcept {
    if (state_) {
        state_->clear();
        state_->error = Status::ok;
    }
}
std::size_t CiPropertyClient::activeSubscriptions() const noexcept {
    return state_ ? static_cast<std::size_t>(std::count_if(state_->subscriptions.begin(),
                                                           state_->subscriptions.end(),
                                                           [](const auto& p) { return p.active; }))
                  : 0;
}
Status CiPropertyClient::lastError() const noexcept {
    return state_ ? state_->error : Status::not_prepared;
}
} // namespace cydsp
