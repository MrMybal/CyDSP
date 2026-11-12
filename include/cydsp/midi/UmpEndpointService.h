#pragma once
#include "cydsp/midi/UmpEndpoint.h"
#include "cydsp/midi/UmpExtensions.h"
#include <memory>
#include <string>
namespace cydsp {
struct UmpFunctionBlock {
    UmpFunctionBlockInfo info;
    std::string name;
};
struct UmpEndpointDescription {
    UmpEndpointCapabilities capabilities;
    CiIdentity identity;
    std::string name = "CyDSP", productInstanceId;
    std::vector<UmpFunctionBlock> blocks;
};
// Serialized owner, prepared bounded replies. poll retains replies when the
// transport is full. No transport I/O is performed except through Sender.
class UmpEndpointService {
  public:
    using Sender = UmpEndpointNegotiator::Sender;
    UmpEndpointService();
    ~UmpEndpointService();
    UmpEndpointService(UmpEndpointService&&) noexcept;
    UmpEndpointService& operator=(UmpEndpointService&&) noexcept;
    Status prepare(const UmpEndpointDescription&, Sender, void*,
                   std::size_t replies = 512) noexcept;
    Status process(const UmpPacket&, std::uint64_t nowMs) noexcept;
    Status poll(std::uint64_t nowMs) noexcept;
    Status negotiate(unsigned protocol, std::uint64_t nowMs) noexcept;
    Status updateFunctionBlock(const UmpFunctionBlockInfo&) noexcept;
    void reset() noexcept;
    [[nodiscard]] UmpStreamConfiguration configuration() const noexcept;
    [[nodiscard]] std::size_t queuedReplies() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
} // namespace cydsp
