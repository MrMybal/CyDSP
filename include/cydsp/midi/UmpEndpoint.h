#pragma once
#include "cydsp/midi/Ump.h"
namespace cydsp {
struct UmpEndpointCapabilities {
    bool midi1 = true, midi2 = true, receiveJr = false, sendJr = false;
    unsigned functionBlocks = 0;
    bool staticFunctionBlocks = true;
};
struct UmpStreamConfiguration {
    unsigned protocol = 1;
    bool receiveJr = false, sendJr = false;
};
[[nodiscard]] UmpPacket umpEndpointDiscovery(unsigned filter = 0x11) noexcept;
[[nodiscard]] UmpPacket umpEndpointInfo(UmpEndpointCapabilities) noexcept;
[[nodiscard]] UmpPacket umpStreamConfiguration(UmpStreamConfiguration,
                                               bool notification = false) noexcept;
Status parseUmpEndpointInfo(const UmpPacket&, UmpEndpointCapabilities&) noexcept;
Status parseUmpStreamConfiguration(const UmpPacket&, UmpStreamConfiguration&) noexcept;
// Explicit raw-endpoint negotiation. OS-managed transports normally negotiate themselves.
// This endpoint declares no Function Blocks; unrelated Stream messages remain host policy.
class UmpEndpointNegotiator {
  public:
    using Sender = Status (*)(void*, const UmpPacket&) noexcept;
    Status prepare(UmpEndpointCapabilities, Sender, void*) noexcept;
    Status negotiate(unsigned preferredProtocol, std::uint64_t nowMs, bool receiveJr = false,
                     bool sendJr = false) noexcept;
    Status process(const UmpPacket&, std::uint64_t nowMs) noexcept;
    Status poll(std::uint64_t nowMs) noexcept;
    void reset() noexcept;
    [[nodiscard]] UmpStreamConfiguration localConfiguration() const noexcept {
        return local_;
    }
    [[nodiscard]] UmpStreamConfiguration peerConfiguration() const noexcept {
        return peer_;
    }
    [[nodiscard]] bool pending() const noexcept {
        return pending_;
    }

  private:
    UmpEndpointCapabilities capabilities_{}, peerCapabilities_{};
    UmpStreamConfiguration local_{}, peer_{}, wanted_{};
    Sender send_ = nullptr;
    void* context_ = nullptr;
    std::uint64_t started_ = 0;
    bool pending_ = false, info_ = false, current_ = false, requested_ = false;
    Status advance() noexcept;
};
} // namespace cydsp
