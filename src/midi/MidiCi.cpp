#include "cydsp/midi/MidiCi.h"
#include "cydsp/midi/CiProperty.h"
#include "detail/Alias.h"
#include <algorithm>
#include <random>
#include <stdexcept>
namespace cydsp {
namespace {
bool seven(std::span<const std::uint8_t> b) noexcept {
    return std::all_of(b.begin(), b.end(), [](auto v) { return v < 128; });
}
bool address(unsigned a) noexcept {
    return a < 16 || a == 0x7e || a == 0x7f;
}
std::uint32_t get(std::span<const std::uint8_t> b) noexcept {
    std::uint32_t v = 0;
    for (std::size_t i = 0; i < b.size(); ++i)
        v |= std::uint32_t(b[i]) << (7 * i);
    return v;
}
void put(std::uint32_t v, std::span<std::uint8_t> b) noexcept {
    for (auto& c : b) {
        c = static_cast<std::uint8_t>(v & 127);
        v >>= 7;
    }
}
bool propertyType(unsigned t) noexcept {
    return t >= 0x34 && t <= 0x3f && t != 0x3a && t != 0x3b && t != 0x3c && t != 0x3d && t != 0x3e;
}
bool broadcastAllowed(unsigned t) noexcept {
    return t == 0x70 || t == 0x7e || t == 0x24 || t == 0x25 || t == 0x26 || t == 0x27 || t == 0x2f;
}
} // namespace
bool validCiMuid(std::uint32_t v) noexcept {
    return v < 0x0fffff00;
}
Status parseCiMessage(std::span<const std::uint8_t> b, CiMessageView& out) noexcept {
    if (b.size() >= 2 && b.front() == 0xf0 && b.back() == 0xf7)
        b = b.subspan(1, b.size() - 2);
    if (b.size() < 13 || b[0] != 0x7e || b[2] != 0x0d || !seven(b) || !address(b[1]) || !b[4] ||
        (b[4] & 0x70))
        return Status::invalid_argument;
    CiMessageView m{b[1], b[3], b[4], get(b.subspan(5, 4)), get(b.subspan(9, 4)), b.subspan(13)};
    if (!validCiMuid(m.source) || (!validCiMuid(m.destination) && m.destination != ciBroadcastMuid))
        return Status::invalid_argument;
    if (m.destination == ciBroadcastMuid && !broadcastAllowed(m.type))
        return Status::invalid_argument;
    out = m;
    return Status::ok;
}
Status makeCiMessage(const CiMessageView& m, std::span<std::uint8_t> out, std::size_t& n) noexcept {
    n = 0;
    if (!address(m.address) || m.type > 127 || !m.version || m.version > 15 ||
        !validCiMuid(m.source) ||
        (!validCiMuid(m.destination) && m.destination != ciBroadcastMuid) || !seven(m.body) ||
        (m.destination == ciBroadcastMuid && !broadcastAllowed(m.type)))
        return Status::invalid_argument;
    if (out.size() < 13 || m.body.size() > out.size() - 13)
        return Status::block_too_large;
    if (detail::overlaps(m.body.data(), m.body.size(), out.data(), 13 + m.body.size()))
        return Status::partial_overlap;
    out[0] = 0x7e;
    out[1] = m.address;
    out[2] = 0x0d;
    out[3] = m.type;
    out[4] = m.version;
    put(m.source, out.subspan(5, 4));
    put(m.destination, out.subspan(9, 4));
    std::copy(m.body.begin(), m.body.end(), out.begin() + 13);
    n = 13 + m.body.size();
    return Status::ok;
}
Status ciEncodeMcoded7(std::span<const std::uint8_t> in, std::span<std::uint8_t> out,
                       std::size_t& n) noexcept {
    n = 0;
    const auto groups = in.size() / 7 + (in.size() % 7 != 0);
    if (in.size() > out.size() || groups > out.size() - in.size())
        return Status::block_too_large;
    if (detail::overlaps(in.data(), in.size(), out.data(), in.size() + groups))
        return Status::partial_overlap;
    for (std::size_t p = 0; p < in.size(); p += 7) {
        const auto count = std::min(std::size_t{7}, in.size() - p);
        std::uint8_t hi = 0;
        for (std::size_t i = 0; i < count; ++i)
            hi |= static_cast<std::uint8_t>((in[p + i] >> 7) << (6 - i));
        out[n++] = hi;
        for (std::size_t i = 0; i < count; ++i)
            out[n++] = in[p + i] & 127;
    }
    return Status::ok;
}
Status ciDecodeMcoded7(std::span<const std::uint8_t> in, std::span<std::uint8_t> out,
                       std::size_t& n) noexcept {
    n = 0;
    if (!seven(in) || in.size() % 8 == 1)
        return Status::invalid_argument;
    const auto size = in.size() - in.size() / 8 - (in.size() % 8 != 0);
    if (out.size() < size)
        return Status::block_too_large;
    if (detail::overlaps(in.data(), in.size(), out.data(), size))
        return Status::partial_overlap;
    // Reject unused low bits before writes.
    if (in.size() % 8 && (in[in.size() / 8 * 8] & ((1u << (8 - in.size() % 8)) - 1)))
        return Status::invalid_argument;
    for (std::size_t p = 0; p < in.size(); p += 8) {
        auto high = in[p];
        const auto count = std::min(std::size_t{7}, in.size() - p - 1);
        for (std::size_t i = 0; i < count; ++i)
            out[n++] = static_cast<std::uint8_t>(in[p + 1 + i] | (((high >> (6 - i)) & 1) << 7));
    }
    return Status::ok;
}
Status parseCiDiscovery(const CiMessageView& m, CiDiscovery& out) noexcept {
    const bool reply = m.type == 0x71;
    const auto need = m.version >= 2 ? (reply ? 18u : 17u) : 16u;
    if ((m.type != 0x70 && !reply) || m.address != 0x7f || m.body.size() < need || !seven(m.body))
        return Status::invalid_argument;
    const auto b = m.body;
    CiDiscovery d;
    std::copy_n(b.begin(), 3, d.identity.manufacturer.begin());
    d.identity.family = static_cast<std::uint16_t>(get(b.subspan(3, 2)));
    d.identity.model = static_cast<std::uint16_t>(get(b.subspan(5, 2)));
    std::copy_n(b.begin() + 7, 4, d.identity.revision.begin());
    d.categories = b[11];
    d.maximumSysExBytes = get(b.subspan(12, 4));
    if (d.maximumSysExBytes < 128 || (d.categories & 0x61))
        return Status::invalid_argument;
    if (m.version >= 2) {
        d.outputPath = b[16];
        if (reply)
            d.functionBlock = b[17];
    }
    out = d;
    return Status::ok;
}
Status makeCiDiscoveryBody(const CiDiscovery& d, bool reply, unsigned version,
                           std::span<std::uint8_t> b, std::size_t& n) noexcept {
    n = 0;
    if (!version || version > 15 || !seven(d.identity.manufacturer) ||
        !seven(d.identity.revision) || d.identity.family > 16383 || d.identity.model > 16383 ||
        d.maximumSysExBytes < 128 || d.maximumSysExBytes > ciBroadcastMuid ||
        (d.categories & 0x61) || d.outputPath > 127 || d.functionBlock > 127)
        return Status::invalid_argument;
    const auto count = version >= 2 ? (reply ? 18u : 17u) : 16u;
    if (b.size() < count)
        return Status::block_too_large;
    std::copy(d.identity.manufacturer.begin(), d.identity.manufacturer.end(), b.begin());
    put(d.identity.family, b.subspan(3, 2));
    put(d.identity.model, b.subspan(5, 2));
    std::copy(d.identity.revision.begin(), d.identity.revision.end(), b.begin() + 7);
    b[11] = d.categories;
    put(d.maximumSysExBytes, b.subspan(12, 4));
    if (version >= 2) {
        b[16] = d.outputPath;
        if (reply)
            b[17] = d.functionBlock;
    }
    n = count;
    return Status::ok;
}
Status parseCiProperty(const CiMessageView& m, CiPropertyChunk& out) noexcept {
    if (!propertyType(m.type) || m.address != 0x7f || m.body.size() < 9 || !seven(m.body))
        return Status::invalid_argument;
    const auto b = m.body;
    const auto h = get(b.subspan(1, 2));
    if (h > b.size() - 9)
        return Status::invalid_argument;
    const auto p = 3 + h;
    const auto d = get(b.subspan(p + 4, 2));
    if (d != b.size() - p - 6)
        return Status::invalid_argument;
    CiPropertyChunk c{b[0], static_cast<std::uint16_t>(get(b.subspan(p, 2))),
                      static_cast<std::uint16_t>(get(b.subspan(p + 2, 2))), b.subspan(3, h),
                      b.subspan(p + 6, d)};
    if (c.number > 1 && !c.header.empty())
        return Status::invalid_argument;
    if (c.total && c.number > c.total)
        return Status::invalid_argument;
    if ((m.type == 0x34 || m.type == 0x37 || m.type == 0x3f) &&
        (!c.data.empty() || c.total != 1 || c.number != 1))
        return Status::invalid_argument;
    out = c;
    return Status::ok;
}
Status makeCiPropertyBody(const CiPropertyChunk& c, std::span<std::uint8_t> b,
                          std::size_t& n) noexcept {
    n = 0;
    if (c.request > 127 || c.total > 16383 || c.number > 16383 || c.header.size() > 16383 ||
        c.data.size() > 16383 || !seven(c.header) || !seven(c.data) ||
        (c.number > 1 && !c.header.empty()) || (c.total && c.number > c.total))
        return Status::invalid_argument;
    if (b.size() < 9 || c.header.size() > b.size() - 9 ||
        c.data.size() > b.size() - 9 - c.header.size())
        return Status::block_too_large;
    if (detail::overlaps(c.header.data(), c.header.size(), b.data(),
                         9 + c.header.size() + c.data.size()) ||
        detail::overlaps(c.data.data(), c.data.size(), b.data(),
                         9 + c.header.size() + c.data.size()))
        return Status::partial_overlap;
    b[0] = c.request;
    put(static_cast<std::uint32_t>(c.header.size()), b.subspan(1, 2));
    std::copy(c.header.begin(), c.header.end(), b.begin() + 3);
    const auto p = 3 + c.header.size();
    put(c.total, b.subspan(p, 2));
    put(c.number, b.subspan(p + 2, 2));
    put(static_cast<std::uint32_t>(c.data.size()), b.subspan(p + 4, 2));
    std::copy(c.data.begin(), c.data.end(), b.begin() + static_cast<std::ptrdiff_t>(p + 6));
    n = p + 6 + c.data.size();
    return Status::ok;
}
Status CiPropertyAssembler::prepare(std::size_t concurrent, std::size_t header,
                                    std::size_t data) noexcept {
    if (!concurrent || concurrent > 128 || !header || header > 16383 || data > 16777216)
        return Status::invalid_argument;
    try {
        std::vector<Slot> next(concurrent);
        for (auto& s : next) {
            s.header.resize(header);
            s.data.resize(data);
        }
        slots_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
}
void CiPropertyAssembler::reset() noexcept {
    for (auto& s : slots_)
        s.active = false;
}
void CiPropertyAssembler::expire(std::uint64_t now, std::uint64_t timeout) noexcept {
    for (auto& s : slots_)
        if (s.active && now >= s.last && now - s.last >= timeout)
            s.active = false;
}
void CiPropertyAssembler::invalidate(std::uint32_t id) noexcept {
    for (auto& s : slots_)
        if (s.source == id || s.destination == id)
            s.active = false;
}
void CiPropertyAssembler::keepAlive(std::uint32_t source, std::uint32_t destination,
                                    std::uint8_t request, std::uint64_t now) noexcept {
    for (auto& slot : slots_)
        if (slot.active && slot.source == source && slot.destination == destination &&
            slot.request == request)
            slot.last = now;
}
void CiPropertyAssembler::cancel(std::uint32_t source, std::uint32_t destination,
                                 std::uint8_t request) noexcept {
    for (auto& slot : slots_)
        if (slot.source == source && slot.destination == destination && slot.request == request)
            slot.active = false;
}
Status CiPropertyAssembler::append(const CiMessageView& m, std::uint64_t now,
                                   CiPropertyMessage& out, bool& complete) noexcept {
    complete = false;
    if (slots_.empty())
        return Status::not_prepared;
    CiPropertyChunk c;
    auto status = parseCiProperty(m, c);
    if (status != Status::ok)
        return status;
    // Header-only Notify must remain usable even when every assembly slot is occupied.
    if (m.type == 0x3f) {
        out = {m.source, m.destination, m.type, c.request, c.header, {}};
        complete = true;
        return Status::ok;
    }
    auto where = std::find_if(slots_.begin(), slots_.end(), [&](const Slot& s) {
        return s.active && s.source == m.source && s.destination == m.destination &&
               s.type == m.type && s.request == c.request;
    });
    if (!c.number) {
        if (where != slots_.end())
            where->active = false;
        return Status::end_of_stream;
    }
    if (c.number == 1) {
        if (where == slots_.end())
            where =
                std::find_if(slots_.begin(), slots_.end(), [](const Slot& s) { return !s.active; });
        if (where == slots_.end())
            return Status::update_queue_full;
        where->source = m.source;
        where->destination = m.destination;
        where->type = m.type;
        where->request = c.request;
        where->headerSize = where->dataSize = 0;
        where->next = 1;
        where->total = c.total;
        where->active = true;
    } else if (where == slots_.end())
        return Status::invalid_argument;
    auto& s = *where;
    if (c.number != s.next || (s.total && c.total && c.total != s.total && c.total != c.number) ||
        c.header.size() > s.header.size() - s.headerSize ||
        c.data.size() > s.data.size() - s.dataSize) {
        s.active = false;
        return Status::invalid_argument;
    }
    std::copy(c.header.begin(), c.header.end(),
              s.header.begin() + static_cast<std::ptrdiff_t>(s.headerSize));
    std::copy(c.data.begin(), c.data.end(),
              s.data.begin() + static_cast<std::ptrdiff_t>(s.dataSize));
    s.headerSize += c.header.size();
    s.dataSize += c.data.size();
    s.last = now;
    ++s.next;
    if (c.total)
        s.total = c.total;
    if (c.total && c.number == c.total) {
        out = {m.source,
               m.destination,
               m.type,
               c.request,
               {s.header.data(), s.headerSize},
               {s.data.data(), s.dataSize}};
        s.active = false;
        complete = true;
    }
    return Status::ok;
}
struct MidiCiDevice::State {
    struct Pending {
        std::uint32_t destination = 0;
        std::uint64_t started = 0;
        std::uint8_t request = 0, type = 0;
        bool active = false;
    };
    CiConfiguration config;
    CiCallbacks callbacks;
    std::uint32_t muid = 0;
    std::uint64_t random = 1, clock = 0;
    std::vector<CiPeer> peers;
    std::size_t peerCount = 0;
    std::vector<CiProfile> profiles;
    std::vector<Pending> pending;
    struct Transfer {
        std::vector<std::uint8_t> header, data;
        std::size_t headerSize = 0, dataSize = 0, offset = 0, firstCapacity = 0, capacity = 0;
        std::uint32_t destination = 0;
        std::uint16_t total = 1, number = 1;
        std::uint8_t type = 0, request = 0;
        bool active = false;
    };
    std::vector<Transfer> transfers;
    std::size_t transferCursor = 0;
    Status lastSend = Status::ok;
    bool pumping = false;
    std::vector<std::uint8_t> tx, body;
    CiPropertyAssembler assembler;
    UmpSysExAssembler sysex;
    bool processing = false;
    std::uint32_t fresh() noexcept {
        std::uint32_t id;
        do {
            random ^= random << 13;
            random ^= random >> 7;
            random ^= random << 17;
            id = static_cast<std::uint32_t>(random) & ciBroadcastMuid;
        } while (!validCiMuid(id) || id == muid);
        return id;
    }
    Status emit(std::uint32_t dest, std::uint8_t type, std::span<const std::uint8_t> data = {},
                std::uint8_t addr = 0x7f) noexcept {
        std::size_t n = 0;
        auto result = makeCiMessage({addr, type, 2, muid, dest, data}, tx, n);
        return result == Status::ok
                   ? callbacks.send(callbacks.context, config.group, {tx.data(), n})
                   : result;
    }
    Status pump() noexcept {
        if (pumping)
            return Status::ok;
        pumping = true;
        struct Guard {
            bool& value;
            ~Guard() {
                value = false;
            }
        } guard{pumping};
        for (unsigned iteration = 0; iteration < 32; ++iteration) {
            Transfer* transfer = nullptr;
            for (std::size_t i = 0; i < transfers.size(); ++i) {
                auto& next = transfers[transferCursor++ % transfers.size()];
                if (next.active) {
                    transfer = &next;
                    break;
                }
            }
            if (!transfer)
                return Status::ok;
            auto& t = *transfer;
            const auto size =
                std::min(t.number == 1 ? t.firstCapacity : t.capacity, t.dataSize - t.offset);
            CiPropertyChunk chunk{t.request, t.total, t.number,
                                  t.number == 1
                                      ? std::span<const std::uint8_t>(t.header.data(), t.headerSize)
                                      : std::span<const std::uint8_t>{},
                                  std::span<const std::uint8_t>(t.data).subspan(t.offset, size)};
            std::size_t n = 0;
            auto status = makeCiPropertyBody(chunk, body, n);
            if (status == Status::ok)
                status = emit(t.destination, t.type, {body.data(), n});
            if (status == Status::would_block || status == Status::update_queue_full)
                return Status::ok;
            if (status != Status::ok) {
                lastSend = status;
                t.active = false;
                return status;
            }
            for (auto& pendingRequest : pending)
                if (pendingRequest.active && pendingRequest.destination == t.destination &&
                    pendingRequest.request == t.request && pendingRequest.type == t.type)
                    pendingRequest.started = clock;
            t.offset += size;
            if (t.number++ == t.total)
                t.active = false;
        }
        return Status::ok;
    }
    Status discovery(bool reply, std::uint32_t dest, unsigned path = 0) noexcept {
        auto d = config.discovery;
        if (reply)
            d.outputPath = static_cast<std::uint8_t>(path);
        std::size_t n = 0;
        auto s = makeCiDiscoveryBody(d, reply, 2, body, n);
        return s == Status::ok ? emit(dest, reply ? 0x71 : 0x70, {body.data(), n}) : s;
    }
    Status nak(const CiMessageView& m, std::uint8_t code = 1) noexcept {
        std::array<std::uint8_t, 10> b{};
        b[0] = m.type;
        b[1] = code;
        return emit(m.source, 0x7f,
                    m.version >= 2 ? std::span<const std::uint8_t>(b)
                                   : std::span<const std::uint8_t>{},
                    m.address);
    }
    void remove(std::uint32_t id) noexcept {
        if (callbacks.invalidated)
            callbacks.invalidated(callbacks.context, id);
        for (std::size_t i = 0; i < peerCount;)
            if (peers[i].muid == id) {
                if (callbacks.peer)
                    callbacks.peer(callbacks.context, peers[i], true);
                peers[i] = peers[--peerCount];
            } else
                ++i;
        assembler.invalidate(id);
        for (auto& t : transfers)
            if (t.destination == id)
                t.active = false;
        for (auto& p : pending)
            if (p.destination == id)
                p.active = false;
    }
    void cancelRequest(std::uint32_t peer, std::uint8_t id) noexcept {
        for (auto& p : pending)
            if (p.destination == peer && p.request == id)
                p.active = false;
        for (auto& t : transfers)
            if (t.destination == peer && t.request == id && t.type != 0x3f)
                t.active = false;
        assembler.cancel(peer, muid, id);
    }
    std::size_t maxMessage(std::uint32_t id) const noexcept {
        for (std::size_t i = 0; i < peerCount; ++i)
            if (peers[i].muid == id)
                return std::min<std::size_t>(config.discovery.maximumSysExBytes,
                                             peers[i].discovery.maximumSysExBytes);
        return std::min(std::size_t{512},
                        static_cast<std::size_t>(config.discovery.maximumSysExBytes));
    }
};
MidiCiDevice::MidiCiDevice() = default;
MidiCiDevice::~MidiCiDevice() = default;
MidiCiDevice::MidiCiDevice(MidiCiDevice&&) noexcept = default;
MidiCiDevice& MidiCiDevice::operator=(MidiCiDevice&&) noexcept = default;
Status MidiCiDevice::prepare(const CiConfiguration& c, CiCallbacks callbacks,
                             std::span<const CiProfile> profiles) noexcept {
    if (!callbacks.send || !c.propertyTransactions || c.propertyTransactions > 128 ||
        c.maximumPropertyBytes > 16777216 || c.group > 15 || !c.peers || c.peers > 1024 ||
        c.maximumProfiles > 1024 || profiles.size() > c.maximumProfiles ||
        c.discovery.maximumSysExBytes < 512 || c.discovery.maximumSysExBytes > 65536 ||
        c.productInstanceId.size() > 16 || (!validCiMuid(c.muid) && c.muid != ciBroadcastMuid) ||
        !std::all_of(c.productInstanceId.begin(), c.productInstanceId.end(),
                     [](unsigned char v) { return v >= 32 && v <= 126; }) ||
        (c.propertyExchange && !callbacks.property))
        return Status::invalid_argument;
    std::array<std::uint8_t, 18> check{};
    std::size_t n;
    auto s = makeCiDiscoveryBody(c.discovery, true, 2, check, n);
    if (s != Status::ok)
        return s;
    for (std::size_t i = 0; i < profiles.size(); ++i) {
        const auto& p = profiles[i];
        if (!address(p.address) || !seven(p.id) || p.channels > 16)
            return Status::invalid_argument;
        for (std::size_t j = 0; j < i; ++j)
            if (p.id == profiles[j].id && p.address == profiles[j].address)
                return Status::invalid_argument;
    }
    try {
        auto next = std::make_unique<State>();
        next->config = c;
        next->config.discovery.categories =
            static_cast<std::uint8_t>((c.maximumProfiles ? 4 : 0) | (c.propertyExchange ? 8 : 0));
        next->callbacks = callbacks;
        std::random_device random;
        next->random = (std::uint64_t(random()) << 32) ^ random();
        if (!next->random)
            next->random = 1;
        next->muid = validCiMuid(c.muid) ? c.muid : next->fresh();
        next->peers.resize(c.peers);
        next->profiles.assign(profiles.begin(), profiles.end());
        next->tx.resize(c.discovery.maximumSysExBytes - 2);
        next->body.resize(c.discovery.maximumSysExBytes - 15);
        next->pending.resize(c.propertyTransactions);
        next->transfers.resize(c.propertyTransactions);
        for (auto& t : next->transfers) {
            t.header.resize(4096);
            t.data.resize(c.maximumPropertyBytes);
        }
        s = next->assembler.prepare(c.propertyTransactions, 4096, c.maximumPropertyBytes);
        if (s != Status::ok)
            return s;
        s = next->sysex.prepare(c.discovery.maximumSysExBytes - 2);
        if (s != Status::ok)
            return s;
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::invalid_argument;
    }
}
Status MidiCiDevice::discover() noexcept {
    return state_ ? state_->discovery(false, ciBroadcastMuid) : Status::not_prepared;
}
Status MidiCiDevice::send(std::uint32_t dest, std::uint8_t type, std::span<const std::uint8_t> body,
                          std::uint8_t addr) noexcept {
    return state_ ? state_->emit(dest, type, body, addr) : Status::not_prepared;
}
Status MidiCiDevice::requestProfiles(std::uint32_t dest, std::uint8_t addr) noexcept {
    return send(dest, 0x20, {}, addr);
}
Status MidiCiDevice::setRemoteProfile(std::uint32_t dest, const CiProfile& p,
                                      bool enabled) noexcept {
    if (!seven(p.id) || !address(p.address) || p.channels > 16)
        return Status::invalid_argument;
    std::array<std::uint8_t, 7> b{};
    std::copy(p.id.begin(), p.id.end(), b.begin());
    put(enabled && p.address < 16 ? p.channels : 0, std::span(b).subspan(5, 2));
    return send(dest, enabled ? 0x22 : 0x23, b, p.address);
}
Status MidiCiDevice::requestPropertyCapabilities(std::uint32_t dest) noexcept {
    const std::array<std::uint8_t, 3> b{1, 0, 0};
    return send(dest, 0x30, b);
}
Status MidiCiDevice::sendProperty(std::uint32_t dest, std::uint8_t type, std::uint8_t request,
                                  std::span<const std::uint8_t> header,
                                  std::span<const std::uint8_t> data) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (!validCiMuid(dest) || !propertyType(type) || request > 127 || header.size() > 4096 ||
        data.size() > s.config.maximumPropertyBytes || !seven(header) || !seven(data) ||
        ((type == 0x34 || type == 0x37 || type == 0x3f) && !data.empty()))
        return Status::invalid_argument;
    const auto limit = s.maxMessage(dest);
    if (limit < 24 || header.size() > limit - 24)
        return Status::block_too_large;
    const auto firstCapacity = std::min(std::size_t{16383}, limit - 24 - header.size()),
               capacity = std::min(std::size_t{16383}, limit - 24);
    const auto first = std::min(firstCapacity, data.size()), remaining = data.size() - first;
    const auto count = 1 + remaining / capacity + (remaining % capacity != 0);
    if (count > 16383)
        return Status::block_too_large;
    auto slot = std::find_if(s.transfers.begin(), s.transfers.end(),
                             [](const auto& t) { return !t.active; });
    if (slot == s.transfers.end())
        return Status::update_queue_full;
    for (const auto& t : s.transfers)
        if (t.active && t.destination == dest && t.type == type && t.request == request)
            return Status::transition_in_progress;
    std::copy(header.begin(), header.end(), slot->header.begin());
    std::copy(data.begin(), data.end(), slot->data.begin());
    slot->headerSize = header.size();
    slot->dataSize = data.size();
    slot->destination = dest;
    slot->type = type;
    slot->request = request;
    slot->offset = 0;
    slot->number = 1;
    slot->total = static_cast<std::uint16_t>(count);
    slot->firstCapacity = firstCapacity;
    slot->capacity = capacity;
    slot->active = true;
    return s.pump();
}

Status MidiCiDevice::requestProperty(std::uint32_t dest, std::uint8_t type,
                                     std::span<const std::uint8_t> h,
                                     std::span<const std::uint8_t> d, std::uint64_t now,
                                     std::uint8_t& id) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (type != 0x34 && type != 0x36 && type != 0x38)
        return Status::invalid_argument;
    auto& s = *state_;
    s.clock = now;
    auto slot =
        std::find_if(s.pending.begin(), s.pending.end(), [](const auto& p) { return !p.active; });
    if (slot == s.pending.end())
        return Status::update_queue_full;
    unsigned chosen = 0;
    for (; chosen < 128; ++chosen)
        if (std::none_of(s.pending.begin(), s.pending.end(), [&](const auto& p) {
                return p.active && p.destination == dest && p.request == chosen;
            }))
            break;
    if (chosen == 128)
        return Status::update_queue_full;
    *slot = {dest, now, static_cast<std::uint8_t>(chosen), type, true};
    auto r = sendProperty(dest, type, static_cast<std::uint8_t>(chosen), h, d);
    if (r != Status::ok)
        slot->active = false;
    else
        id = static_cast<std::uint8_t>(chosen);
    return r;
}
Status MidiCiDevice::process(std::span<const std::uint8_t> bytes, std::uint64_t now) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (s.processing)
        return Status::transition_in_progress;
    CiMessageView m;
    auto r = parseCiMessage(bytes, m);
    if (r != Status::ok)
        return r;
    if (m.body.size() > s.config.discovery.maximumSysExBytes - 15)
        return Status::block_too_large;
    if (m.destination != s.muid && m.destination != ciBroadcastMuid)
        return Status::ok;
    s.clock = now;
    s.processing = true;
    struct Guard {
        bool& b;
        ~Guard() {
            b = false;
        }
    } guard{s.processing};
    if (m.type == 0x7e) {
        if (m.body.size() < 4)
            return Status::invalid_argument;
        auto target = get(m.body.first(4));
        s.remove(target);
        if (target == s.muid) {
            for (std::size_t i = 0; i < s.peerCount; ++i)
                if (s.callbacks.peer)
                    s.callbacks.peer(s.callbacks.context, s.peers[i], true);
            s.peerCount = 0;
            s.assembler.reset();
            for (auto& p : s.pending)
                p.active = false;
            for (auto& t : s.transfers)
                t.active = false;
            s.muid = s.fresh();
            return s.discovery(false, ciBroadcastMuid);
        }
        return Status::ok;
    }
    if (m.source == s.muid) {
        std::array<std::uint8_t, 4> b{};
        put(s.muid, b);
        r = s.emit(ciBroadcastMuid, 0x7e, b);
        if (r != Status::ok)
            return r;
        s.muid = s.fresh();
        for (auto& t : s.transfers)
            t.active = false;
        s.peerCount = 0;
        s.assembler.reset();
        for (auto& p : s.pending)
            p.active = false;
        return s.discovery(false, ciBroadcastMuid);
    }
    if (m.type == 0x70 || m.type == 0x71) {
        CiDiscovery d;
        r = parseCiDiscovery(m, d);
        if (r != Status::ok)
            return r;
        auto found = std::find_if(s.peers.begin(),
                                  s.peers.begin() + static_cast<std::ptrdiff_t>(s.peerCount),
                                  [&](const auto& p) { return p.muid == m.source; });
        if (found == s.peers.begin() + static_cast<std::ptrdiff_t>(s.peerCount)) {
            if (s.peerCount == s.peers.size())
                return Status::update_queue_full;
            found = s.peers.begin() + static_cast<std::ptrdiff_t>(s.peerCount++);
        }
        *found = {m.source, d, m.version};
        if (s.callbacks.peer)
            s.callbacks.peer(s.callbacks.context, *found, false);
        return m.type == 0x70 ? s.discovery(true, m.source, d.outputPath) : Status::ok;
    }
    if (m.type == 0x72) {
        if (m.body.empty())
            return Status::invalid_argument;
        if (m.body[0] || s.config.productInstanceId.empty())
            return s.nak(m);
        s.body[0] = 0;
        put(static_cast<std::uint32_t>(s.config.productInstanceId.size()),
            std::span(s.body).subspan(1, 2));
        std::copy(s.config.productInstanceId.begin(), s.config.productInstanceId.end(),
                  s.body.begin() + 3);
        return s.emit(m.source, 0x73, {s.body.data(), 3 + s.config.productInstanceId.size()});
    }
    if (m.type == 0x20) {
        if (!s.config.maximumProfiles)
            return s.nak(m);
        std::size_t enabled = 0, disabled = 0;
        for (const auto& p : s.profiles)
            if (p.address == m.address)
                (p.enabled ? enabled : disabled)++;
        const auto size = 4 + 5 * (enabled + disabled);
        if (size > s.body.size() || size + 15 > s.maxMessage(m.source))
            return s.nak(m, 0x40);
        put(static_cast<std::uint32_t>(enabled), std::span(s.body).first(2));
        std::size_t pos = 2;
        for (bool active : {true, false}) {
            if (!active) {
                put(static_cast<std::uint32_t>(disabled), std::span(s.body).subspan(pos, 2));
                pos += 2;
            }
            for (const auto& p : s.profiles)
                if (p.address == m.address && p.enabled == active) {
                    std::copy(p.id.begin(), p.id.end(),
                              s.body.begin() + static_cast<std::ptrdiff_t>(pos));
                    pos += 5;
                }
        }
        return s.emit(m.source, 0x21, {s.body.data(), pos}, m.address);
    }
    if (m.type == 0x22 || m.type == 0x23) {
        if (m.body.size() < (m.version >= 2 ? 7u : 5u))
            return Status::invalid_argument;
        auto p = std::find_if(s.profiles.begin(), s.profiles.end(), [&](const CiProfile& p) {
            return p.address == m.address && std::equal(p.id.begin(), p.id.end(), m.body.begin());
        });
        if (p == s.profiles.end())
            return s.nak(m, 4);
        const auto wanted = m.version >= 2 ? get(m.body.subspan(5, 2)) : 1;
        if (m.version >= 2 && ((m.address >= 16 && wanted) || (m.type == 0x23 && wanted)))
            return s.nak(m, 0x41);
        auto next = *p;
        next.enabled = m.type == 0x22;
        next.channels = static_cast<std::uint16_t>(
            next.enabled ? (m.address < 16 ? std::min(16u - m.address, std::max(1u, wanted)) : 16u)
                         : 0u);
        std::array<std::uint8_t, 7> b{};
        std::copy(next.id.begin(), next.id.end(), b.begin());
        put(next.channels, std::span(b).subspan(5, 2));
        r = s.emit(ciBroadcastMuid, next.enabled ? 0x24 : 0x25, b, m.address);
        if (r == Status::ok) {
            *p = next;
            if (s.callbacks.profile)
                s.callbacks.profile(s.callbacks.context, m.source, *p);
        }
        return r;
    }
    if (m.type == 0x21) {
        const auto b = m.body;
        if (b.size() < 4)
            return Status::invalid_argument;
        const auto enabled = get(b.first(2));
        if (enabled > (b.size() - 4) / 5)
            return Status::invalid_argument;
        const auto middle = 2 + enabled * 5;
        const auto disabled = get(b.subspan(middle, 2));
        if (disabled > (b.size() - middle - 2) / 5 || middle + 2 + disabled * 5 != b.size())
            return Status::invalid_argument;
        if (s.callbacks.profile)
            for (unsigned i = 0; i < enabled + disabled; ++i) {
                CiProfile p;
                p.address = m.address;
                p.enabled = i < enabled;
                const auto pos = 2 + i * 5 + (i >= enabled ? 2 : 0);
                std::copy_n(b.begin() + pos, 5, p.id.begin());
                s.callbacks.profile(s.callbacks.context, m.source, p);
            }
    } else if (m.type == 0x24 || m.type == 0x25) {
        if (m.body.size() < (m.version >= 2 ? 7u : 5u))
            return Status::invalid_argument;
        CiProfile p;
        std::copy_n(m.body.begin(), 5, p.id.begin());
        p.address = m.address;
        p.enabled = m.type == 0x24;
        p.channels = static_cast<std::uint16_t>(m.version >= 2 ? get(m.body.subspan(5, 2)) : 0);
        if (s.callbacks.profile)
            s.callbacks.profile(s.callbacks.context, m.source, p);
    }
    if (m.type == 0x28 && !s.callbacks.message)
        return s.nak(m);
    if (m.type >= 0x21 && m.type <= 0x2f) {
        if (s.callbacks.message)
            s.callbacks.message(s.callbacks.context, m);
        return Status::ok;
    }
    if (m.type == 0x30) {
        if (m.body.size() < (m.version >= 2 ? 3u : 1u))
            return Status::invalid_argument;
        if (!s.config.propertyExchange)
            return s.nak(m);
        const std::array<std::uint8_t, 3> b{
            static_cast<std::uint8_t>(std::min(std::size_t{127}, s.pending.size())), 0, 0};
        return s.emit(m.source, 0x31, b);
    }
    if (propertyType(m.type)) {
        if (!s.config.propertyExchange)
            return s.nak(m);
        CiPropertyMessage complete;
        bool done = false;
        r = s.assembler.append(m, now, complete, done);
        if (r != Status::ok)
            return r == Status::update_queue_full ? s.nak(m, 0x43) : r;
        for (auto& p : s.pending)
            if (p.active && p.destination == m.source && !m.body.empty() &&
                p.request == m.body[0] && m.type == p.type + 1)
                p.started = now;
        if (done && m.type == 0x3f) {
            CiPropertyHeader header;
            unsigned code = 0;
            if (header.parse(complete.header) != Status::ok || header.firstKey() != "status" ||
                !complete.data.empty())
                return Status::invalid_argument;
            const auto value = header.find("status");
            if (!value || value->unsignedInteger(code) != Status::ok ||
                (code != 100 && code != 144 && code != 408))
                return Status::invalid_argument;
            if (code == 100) {
                s.assembler.keepAlive(m.source, s.muid, complete.request, now);
                for (auto& pending : s.pending)
                    if (pending.active && pending.destination == m.source &&
                        pending.request == complete.request)
                        pending.started = now;
            } else {
                const bool active = propertyRequestActive(m.source, complete.request);
                s.cancelRequest(m.source, complete.request);
                if (active && code == 408 && s.callbacks.timeout)
                    s.callbacks.timeout(s.callbacks.context, m.source, complete.request);
            }
        }
        if (done) {
            for (auto& p : s.pending)
                if (p.active && p.destination == m.source && p.request == complete.request &&
                    m.type == p.type + 1)
                    p.active = false;
            return s.callbacks.property(s.callbacks.context, complete);
        }
        return Status::ok;
    }
    if (m.type == 0x40) {
        const std::array<std::uint8_t, 1> b{0};
        return s.emit(m.source, 0x41, b);
    }
    if (m.type == 0x31 || m.type == 0x41 || m.type == 0x73 || m.type == 0x7d || m.type == 0x7f) {
        if (s.callbacks.message)
            s.callbacks.message(s.callbacks.context, m);
        return Status::ok;
    }
    return s.nak(m);
}
Status MidiCiDevice::processUmp(const UmpPacket& p, std::uint64_t now) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (p.type() != 3 || p.group() != state_->config.group)
        return Status::unsupported_format;
    auto r = state_->sysex.append(p);
    if (r != Status::ok)
        return r;
    const auto bytes = state_->sysex.message();
    return bytes.empty() ? Status::ok : process(bytes, now);
}
Status MidiCiDevice::notifyProperty(std::uint32_t destination, std::uint8_t request,
                                    unsigned code) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (code != 100 && code != 144 && code != 408)
        return Status::invalid_argument;
    const auto text = code == 100   ? std::string_view("{\"status\":100}")
                      : code == 144 ? std::string_view("{\"status\":144}")
                                    : std::string_view("{\"status\":408}");
    if (!validCiMuid(destination) || request > 127)
        return Status::invalid_argument;
    if (code != 100)
        state_->cancelRequest(destination, request);
    const auto r =
        sendProperty(destination, 0x3f, request,
                     {reinterpret_cast<const std::uint8_t*>(text.data()), text.size()}, {});
    return r;
}
void MidiCiDevice::cancelPropertyRequest(std::uint32_t destination, std::uint8_t request) noexcept {
    if (state_)
        state_->cancelRequest(destination, request);
}
bool MidiCiDevice::propertyRequestActive(std::uint32_t destination,
                                         std::uint8_t request) const noexcept {
    return state_ &&
           std::any_of(state_->pending.begin(), state_->pending.end(), [&](const auto& p) {
               return p.active && p.destination == destination && p.request == request;
           });
}
CiDiscovery MidiCiDevice::localDiscovery() const noexcept {
    return state_ ? state_->config.discovery : CiDiscovery{};
}
std::size_t MidiCiDevice::maximumPropertyBytes() const noexcept {
    return state_ ? state_->config.maximumPropertyBytes : 0;
}
void MidiCiDevice::poll(std::uint64_t now) noexcept {
    if (!state_)
        return;
    auto& s = *state_;
    s.clock = now;
    s.assembler.expire(now);
    for (auto& p : s.pending)
        if (p.active && now >= p.started && now - p.started >= 3000) {
            p.active = false;
            for (auto& t : s.transfers)
                if (t.destination == p.destination && t.request == p.request && t.type == p.type)
                    t.active = false;
            if (s.callbacks.timeout)
                s.callbacks.timeout(s.callbacks.context, p.destination, p.request);
        }
    (void)s.pump();
}
void MidiCiDevice::reset() noexcept {
    if (state_) {
        state_->peerCount = 0;
        state_->assembler.reset();
        state_->sysex.reset();
        state_->lastSend = Status::ok;
        for (auto& t : state_->transfers)
            t.active = false;
        for (auto& p : state_->pending)
            p.active = false;
    }
}
Status MidiCiDevice::lastSendError() const noexcept {
    return state_ ? state_->lastSend : Status::not_prepared;
}
std::uint32_t MidiCiDevice::muid() const noexcept {
    return state_ ? state_->muid : ciBroadcastMuid;
}
std::span<const CiPeer> MidiCiDevice::peers() const noexcept {
    return state_ ? std::span<const CiPeer>(state_->peers.data(), state_->peerCount)
                  : std::span<const CiPeer>{};
}
} // namespace cydsp
