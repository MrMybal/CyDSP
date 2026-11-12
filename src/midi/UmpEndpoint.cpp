#include "cydsp/midi/UmpEndpoint.h"
namespace cydsp {
namespace {
UmpPacket stream(unsigned status, unsigned low = 0, std::uint32_t second = 0) noexcept {
    UmpPacket p;
    p.count = 4;
    p.words = {0xf0000000u | (status << 16) | low, second, 0, 0};
    return p;
}
bool is(const UmpPacket& p, unsigned status) noexcept {
    return p.valid() && p.type() == 15 && ((p.words[0] >> 26) & 3) == 0 &&
           ((p.words[0] >> 16) & 1023) == status;
}
bool supported(UmpEndpointCapabilities c, unsigned protocol) noexcept {
    return protocol == 1 ? c.midi1 : protocol == 2 ? c.midi2 : false;
}
bool equal(UmpStreamConfiguration a, UmpStreamConfiguration b) noexcept {
    return a.protocol == b.protocol && a.receiveJr == b.receiveJr && a.sendJr == b.sendJr;
}
} // namespace
UmpPacket umpEndpointDiscovery(unsigned filter) noexcept {
    return filter < 32 ? stream(0, 0x0101, filter) : UmpPacket{};
}
UmpPacket umpEndpointInfo(UmpEndpointCapabilities c) noexcept {
    if ((!c.midi1 && !c.midi2) || c.functionBlocks > 32)
        return {};
    return stream(1, 0x0101,
                  (c.staticFunctionBlocks ? 0x80000000u : 0u) | (c.functionBlocks << 24) |
                      (c.midi1 ? 0x100u : 0u) | (c.midi2 ? 0x200u : 0u) | (c.receiveJr ? 2u : 0u) |
                      (c.sendJr ? 1u : 0u));
}
UmpPacket umpStreamConfiguration(UmpStreamConfiguration c, bool notification) noexcept {
    if (c.protocol != 1 && c.protocol != 2)
        return {};
    return stream(notification ? 6 : 5,
                  (c.protocol << 8) | (c.receiveJr ? 2 : 0) | (c.sendJr ? 1 : 0));
}
Status parseUmpEndpointInfo(const UmpPacket& p, UmpEndpointCapabilities& out) noexcept {
    if (!is(p, 1) || (p.words[0] & 0xff00) != 0x0100 || ((p.words[1] >> 24) & 127) > 32 ||
        !(p.words[1] & 0x300))
        return Status::invalid_argument;
    out = {(p.words[1] & 0x100) != 0, (p.words[1] & 0x200) != 0, (p.words[1] & 2) != 0,
           (p.words[1] & 1) != 0,     (p.words[1] >> 24) & 127,  (p.words[1] & 0x80000000u) != 0};
    return Status::ok;
}
Status parseUmpStreamConfiguration(const UmpPacket& p, UmpStreamConfiguration& out) noexcept {
    const auto protocol = (p.words[0] >> 8) & 255;
    if ((!is(p, 5) && !is(p, 6)) || (protocol != 1 && protocol != 2) || (p.words[0] & 0xfc) ||
        p.words[1] || p.words[2] || p.words[3])
        return Status::invalid_argument;
    out = {protocol, (p.words[0] & 2) != 0, (p.words[0] & 1) != 0};
    return Status::ok;
}
Status UmpEndpointNegotiator::prepare(UmpEndpointCapabilities c, Sender send,
                                      void* context) noexcept {
    if (!send || (!c.midi1 && !c.midi2) || c.functionBlocks != 0)
        return Status::invalid_argument;
    capabilities_ = c;
    send_ = send;
    context_ = context;
    reset();
    return Status::ok;
}
void UmpEndpointNegotiator::reset() noexcept {
    local_ = {capabilities_.midi1 ? 1u : 2u, false, false};
    peer_ = {};
    pending_ = info_ = current_ = requested_ = false;
}
Status UmpEndpointNegotiator::negotiate(unsigned protocol, std::uint64_t now, bool receive,
                                        bool send) noexcept {
    if (!send_)
        return Status::not_prepared;
    if (!supported(capabilities_, protocol) || (receive && !capabilities_.receiveJr) ||
        (send && !capabilities_.sendJr))
        return Status::invalid_argument;
    if (pending_)
        return Status::transition_in_progress;
    const auto status = send_(context_, umpEndpointDiscovery());
    if (status != Status::ok)
        return status;
    wanted_ = {protocol, send, receive};
    started_ = now;
    info_ = current_ = requested_ = false;
    pending_ = true;
    return Status::ok;
}
Status UmpEndpointNegotiator::advance() noexcept {
    if (!pending_ || !info_ || !current_ || requested_)
        return Status::ok;
    if (!supported(peerCapabilities_, wanted_.protocol) ||
        (wanted_.receiveJr && !peerCapabilities_.receiveJr) ||
        (wanted_.sendJr && !peerCapabilities_.sendJr)) {
        pending_ = false;
        return Status::unsupported_format;
    }
    if (equal(peer_, wanted_)) {
        local_ = {peer_.protocol, peer_.sendJr, peer_.receiveJr};
        pending_ = false;
        return Status::ok;
    }
    const auto status = send_(context_, umpStreamConfiguration(wanted_));
    if (status == Status::ok)
        requested_ = true;
    return status;
}
Status UmpEndpointNegotiator::process(const UmpPacket& p, std::uint64_t now) noexcept {
    if (!send_)
        return Status::not_prepared;
    if (pending_ && now >= started_ && now - started_ >= 3000) {
        pending_ = false;
        return Status::timed_out;
    }
    if (is(p, 0)) {
        if ((p.words[0] & 0xff00) != 0x0100)
            return Status::unsupported_format;
        const auto filter = p.words[1] & 31;
        if (filter & 1) {
            const auto s = send_(context_, umpEndpointInfo(capabilities_));
            if (s != Status::ok)
                return s;
        }
        if (filter & 16)
            return send_(context_, umpStreamConfiguration(local_, true));
        return Status::ok;
    }
    if (is(p, 1)) {
        auto s = parseUmpEndpointInfo(p, peerCapabilities_);
        if (s != Status::ok)
            return s;
        info_ = true;
        return advance();
    }
    if (is(p, 5)) {
        UmpStreamConfiguration requested;
        auto s = parseUmpStreamConfiguration(p, requested);
        if (s != Status::ok)
            return s;
        auto next = local_;
        if (supported(capabilities_, requested.protocol) &&
            (!requested.receiveJr || capabilities_.receiveJr) &&
            (!requested.sendJr || capabilities_.sendJr))
            next = requested;
        s = send_(context_, umpStreamConfiguration(next, true));
        if (s == Status::ok)
            local_ = next;
        return s;
    }
    if (is(p, 6)) {
        UmpStreamConfiguration notified;
        auto s = parseUmpStreamConfiguration(p, notified);
        if (s != Status::ok)
            return s;
        peer_ = notified;
        current_ = true;
        if (pending_ && requested_) {
            pending_ = false;
            local_ = {peer_.protocol, peer_.sendJr, peer_.receiveJr};
            return equal(peer_, wanted_) ? Status::ok : Status::unsupported_format;
        }
        return advance();
    }
    return Status::unsupported_format;
}
Status UmpEndpointNegotiator::poll(std::uint64_t now) noexcept {
    if (!send_)
        return Status::not_prepared;
    if (pending_ && now >= started_ && now - started_ >= 3000) {
        pending_ = false;
        return Status::timed_out;
    }
    return advance();
}
} // namespace cydsp
