#include "cydsp/midi/UmpEndpointService.h"
#include <algorithm>
namespace cydsp {
struct UmpEndpointService::State {
    struct Block {
        UmpFunctionBlockInfo info;
        std::vector<UmpPacket> name;
    };
    UmpEndpointCapabilities caps;
    CiIdentity identity;
    std::vector<UmpPacket> name, product, queue;
    std::vector<Block> blocks;
    std::size_t head = 0, size = 0;
    Sender send = nullptr;
    void* context = nullptr;
    UmpEndpointNegotiator negotiator;
    Status add(const UmpPacket& p) noexcept {
        if (size == queue.size())
            return Status::update_queue_full;
        queue[(head + size++) % queue.size()] = p;
        return Status::ok;
    }
    void add(std::span<const UmpPacket> p) noexcept {
        for (auto& v : p)
            (void)add(v);
    }
    static Status queued(void* ptr, const UmpPacket& p) noexcept {
        return static_cast<State*>(ptr)->add(p);
    }
};
UmpEndpointService::UmpEndpointService() = default;
UmpEndpointService::~UmpEndpointService() = default;
UmpEndpointService::UmpEndpointService(UmpEndpointService&&) noexcept = default;
UmpEndpointService& UmpEndpointService::operator=(UmpEndpointService&&) noexcept = default;
Status UmpEndpointService::prepare(const UmpEndpointDescription& d, Sender send, void* ctx,
                                   std::size_t replies) noexcept {
    if (!send || replies < 4 || replies > 65536 || d.blocks.size() > 32 ||
        (!d.capabilities.midi1 && !d.capabilities.midi2) || !umpDeviceIdentity(d.identity).valid())
        return Status::invalid_spec;
    try {
        auto n = std::make_unique<State>();
        n->caps = d.capabilities;
        n->caps.functionBlocks = unsigned(d.blocks.size());
        n->identity = d.identity;
        n->send = send;
        n->context = ctx;
        n->queue.resize(replies);
        auto text = [](UmpTextHeader h, const std::string& s, std::vector<UmpPacket>& out) {
            out.resize(8);
            std::size_t used = 0;
            auto r = makeUmpText(h, s, out, used);
            if (r == Status::ok)
                out.resize(used);
            return r;
        };
        auto r = text({}, d.name, n->name);
        if (r != Status::ok)
            return r;
        UmpTextHeader h;
        h.kind = UmpTextKind::productInstance;
        r = text(h, d.productInstanceId, n->product);
        if (r != Status::ok)
            return r;
        unsigned occupied = 0;
        for (std::size_t i = 0; i < d.blocks.size(); ++i) {
            auto b = d.blocks[i].info;
            if (b.number != i || !umpFunctionBlockInfo(b).valid() ||
                (n->caps.staticFunctionBlocks && !b.active))
                return Status::invalid_argument;
            const auto mask = ((1u << b.groups) - 1) << b.firstGroup;
            if (occupied & mask)
                return Status::invalid_argument;
            occupied |= mask;
            n->blocks.push_back({b, {}});
            h = {};
            h.kind = UmpTextKind::functionBlockName;
            h.functionBlock = unsigned(i);
            r = text(h, d.blocks[i].name, n->blocks.back().name);
            if (r != Status::ok)
                return r;
        }
        auto minimal = n->caps;
        minimal.functionBlocks = 0;
        r = n->negotiator.prepare(minimal, &State::queued, n.get());
        if (r != Status::ok)
            return r;
        state_.swap(n);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status UmpEndpointService::process(const UmpPacket& p, std::uint64_t now) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!p.valid())
        return Status::invalid_argument;
    auto& s = *state_;
    const auto code = (p.words[0] >> 16) & 0xfff;
    if (p.type() == 15 && code == 0) {
        if ((p.words[0] & 0xff00) != 0x0100 || (p.words[1] & ~31u) || p.words[2] || p.words[3])
            return Status::invalid_argument;
        const auto f = p.words[1];
        const auto count = std::size_t(bool(f & 1)) + bool(f & 2) + ((f & 4) ? s.name.size() : 0) +
                           ((f & 8) ? s.product.size() : 0) + bool(f & 16);
        if (count > s.queue.size() - s.size)
            return Status::update_queue_full;
        if (f & 1)
            (void)s.add(umpEndpointInfo(s.caps));
        if (f & 2)
            (void)s.add(umpDeviceIdentity(s.identity));
        if (f & 4)
            s.add(s.name);
        if (f & 8)
            s.add(s.product);
        if (f & 16)
            (void)s.add(umpStreamConfiguration(s.negotiator.localConfiguration(), true));
        return Status::ok;
    }
    if (p.type() == 15 && code == 0x10) {
        const auto number = (p.words[0] >> 8) & 255, filter = p.words[0] & 255;
        if (filter > 3 || p.words[1] || p.words[2] || p.words[3] ||
            (number != 255 && number >= s.blocks.size()))
            return Status::invalid_argument;
        std::size_t count = 0;
        for (auto& b : s.blocks)
            if (number == 255 || b.info.number == number)
                count += bool(filter & 1) + ((filter & 2) ? b.name.size() : 0);
        if (count > s.queue.size() - s.size)
            return Status::update_queue_full;
        for (auto& b : s.blocks)
            if (number == 255 || b.info.number == number) {
                if (filter & 1)
                    (void)s.add(umpFunctionBlockInfo(b.info));
                if (filter & 2)
                    s.add(b.name);
            }
        return Status::ok;
    }
    return s.negotiator.process(p, now);
}
Status UmpEndpointService::poll(std::uint64_t now) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto r = s.negotiator.poll(now);
    if (r != Status::ok && r != Status::update_queue_full)
        return r;
    while (s.size) {
        r = s.send(s.context, s.queue[s.head]);
        if (r != Status::ok)
            return r;
        s.head = (s.head + 1) % s.queue.size();
        --s.size;
    }
    return s.negotiator.poll(now);
}
Status UmpEndpointService::negotiate(unsigned protocol, std::uint64_t now) noexcept {
    return state_ ? state_->negotiator.negotiate(protocol, now) : Status::not_prepared;
}
Status UmpEndpointService::updateFunctionBlock(const UmpFunctionBlockInfo& b) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (s.caps.staticFunctionBlocks)
        return Status::unsupported_format;
    const auto packet = umpFunctionBlockInfo(b);
    if (!packet.valid() || b.number >= s.blocks.size())
        return Status::invalid_argument;
    const auto mask = ((1u << b.groups) - 1) << b.firstGroup;
    for (auto& v : s.blocks)
        if (v.info.number != b.number &&
            (mask & (((1u << v.info.groups) - 1) << v.info.firstGroup)))
            return Status::invalid_argument;
    const auto r = s.add(packet);
    if (r == Status::ok)
        s.blocks[b.number].info = b;
    return r;
}
void UmpEndpointService::reset() noexcept {
    if (state_) {
        state_->head = state_->size = 0;
        state_->negotiator.reset();
    }
}
UmpStreamConfiguration UmpEndpointService::configuration() const noexcept {
    return state_ ? state_->negotiator.localConfiguration() : UmpStreamConfiguration{};
}
std::size_t UmpEndpointService::queuedReplies() const noexcept {
    return state_ ? state_->size : 0;
}
} // namespace cydsp
