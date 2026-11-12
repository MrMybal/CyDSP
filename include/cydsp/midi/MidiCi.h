#pragma once
#include "cydsp/midi/Ump.h"
#include <memory>
#include <string>
namespace cydsp {
inline constexpr std::uint32_t ciBroadcastMuid = 0x0fffffff;
[[nodiscard]] bool validCiMuid(std::uint32_t) noexcept;
struct CiIdentity {
    std::array<std::uint8_t, 3> manufacturer{
        0x7d, 0, 0}; // Development/non-commercial ID; set yours for shipping.
    std::uint16_t family = 0, model = 0;
    std::array<std::uint8_t, 4> revision{};
};
struct CiMessageView {
    std::uint8_t address = 0x7f, type = 0, version = 2;
    std::uint32_t source = 0, destination = ciBroadcastMuid;
    std::span<const std::uint8_t> body;
};
// Payload excludes F0/F7, as transported by UMP SysEx7. Full wire SysEx is accepted on parse.
Status parseCiMessage(std::span<const std::uint8_t>, CiMessageView&) noexcept;
Status makeCiMessage(const CiMessageView&, std::span<std::uint8_t>, std::size_t&) noexcept;
Status ciEncodeMcoded7(std::span<const std::uint8_t>, std::span<std::uint8_t>,
                       std::size_t&) noexcept;
Status ciDecodeMcoded7(std::span<const std::uint8_t>, std::span<std::uint8_t>,
                       std::size_t&) noexcept;
struct CiDiscovery {
    CiIdentity identity;
    std::uint8_t categories = 0, outputPath = 0, functionBlock = 0x7f;
    std::uint32_t maximumSysExBytes = 4096; // Includes F0/F7.
};
Status parseCiDiscovery(const CiMessageView&, CiDiscovery&) noexcept;
Status makeCiDiscoveryBody(const CiDiscovery&, bool reply, unsigned version,
                           std::span<std::uint8_t>, std::size_t&) noexcept;
struct CiProfile {
    std::array<std::uint8_t, 5> id{};
    std::uint8_t address = 0x7f;
    bool enabled = false;
    std::uint16_t channels = 0;
};
struct CiPropertyChunk {
    std::uint8_t request = 0;
    std::uint16_t total = 1, number = 1;
    std::span<const std::uint8_t> header, data;
};
Status parseCiProperty(const CiMessageView&, CiPropertyChunk&) noexcept;
Status makeCiPropertyBody(const CiPropertyChunk&, std::span<std::uint8_t>, std::size_t&) noexcept;
struct CiPropertyMessage {
    std::uint32_t source = 0, destination = 0;
    std::uint8_t type = 0, request = 0;
    std::span<const std::uint8_t> header, data; // Borrowed, encoded 7-bit PE content.
};
class CiPropertyAssembler {
  public:
    Status prepare(std::size_t concurrent = 8, std::size_t maximumHeader = 4096,
                   std::size_t maximumData = 65536) noexcept;
    // nowMs is a caller-owned monotonic clock. Expiration is explicit, no OS clock in process.
    Status append(const CiMessageView&, std::uint64_t nowMs, CiPropertyMessage&,
                  bool& complete) noexcept;
    void expire(std::uint64_t nowMs, std::uint64_t timeoutMs = 3000) noexcept;
    void invalidate(std::uint32_t muid) noexcept;
    void keepAlive(std::uint32_t source, std::uint32_t destination, std::uint8_t request,
                   std::uint64_t nowMs) noexcept;
    void cancel(std::uint32_t source, std::uint32_t destination, std::uint8_t request) noexcept;
    void reset() noexcept;

  private:
    struct Slot {
        std::uint32_t source = 0, destination = 0;
        std::uint8_t type = 0, request = 0;
        std::uint16_t next = 1, total = 0;
        std::uint64_t last = 0;
        std::vector<std::uint8_t> header, data;
        std::size_t headerSize = 0, dataSize = 0;
        bool active = false;
    };
    std::vector<Slot> slots_;
};
struct CiPeer {
    std::uint32_t muid = 0;
    CiDiscovery discovery;
    std::uint8_t version = 0;
};
struct CiConfiguration {
    CiDiscovery discovery;
    unsigned group = 0;
    // Broadcast sentinel selects a fresh random session MUID during prepare.
    std::uint32_t muid = ciBroadcastMuid;
    std::size_t peers = 32, maximumProfiles = 32, propertyTransactions = 8;
    std::size_t maximumPropertyBytes = 65536;
    std::string productInstanceId;
    bool propertyExchange = false;
};
struct CiCallbacks {
    void* context = nullptr;
    Status (*send)(void*, unsigned group, std::span<const std::uint8_t> payload) noexcept = nullptr;
    void (*peer)(void*, const CiPeer&, bool removed) noexcept = nullptr;
    void (*profile)(void*, std::uint32_t source, const CiProfile&) noexcept = nullptr;
    Status (*property)(void*, const CiPropertyMessage&) noexcept = nullptr;
    void (*message)(void*, const CiMessageView&) noexcept = nullptr;
    void (*timeout)(void*, std::uint32_t destination, std::uint8_t request) noexcept = nullptr;
    void (*invalidated)(void*, std::uint32_t muid) noexcept = nullptr;
};
// One serialized owner. Prepared protocol processing retains memory; callbacks borrow their views.
// Resource schemas/authorization and property encodings are application policy, via property().
class MidiCiDevice {
  public:
    MidiCiDevice();
    ~MidiCiDevice();
    MidiCiDevice(MidiCiDevice&&) noexcept;
    MidiCiDevice& operator=(MidiCiDevice&&) noexcept;
    Status prepare(const CiConfiguration&, CiCallbacks, std::span<const CiProfile> = {}) noexcept;
    Status discover() noexcept;
    Status process(std::span<const std::uint8_t> sysex, std::uint64_t nowMs) noexcept;
    Status processUmp(const UmpPacket&, std::uint64_t nowMs) noexcept;
    Status send(std::uint32_t destination, std::uint8_t type,
                std::span<const std::uint8_t> body = {}, std::uint8_t address = 0x7f) noexcept;
    Status requestProfiles(std::uint32_t destination, std::uint8_t address = 0x7f) noexcept;
    Status setRemoteProfile(std::uint32_t destination, const CiProfile&, bool enabled) noexcept;
    Status requestPropertyCapabilities(std::uint32_t destination) noexcept;
    // Only inquiries 0x34/0x36/0x38 allocate a request ID. Replies use sendProperty with the
    // received ID.
    Status requestProperty(std::uint32_t destination, std::uint8_t type,
                           std::span<const std::uint8_t> header, std::span<const std::uint8_t> data,
                           std::uint64_t nowMs, std::uint8_t& requestId) noexcept;
    Status sendProperty(std::uint32_t destination, std::uint8_t type, std::uint8_t requestId,
                        std::span<const std::uint8_t> header,
                        std::span<const std::uint8_t> data) noexcept;
    // Notify statuses: 100 keep-alive, 144 cancel, 408 timeout. No Notify acknowledgement.
    Status notifyProperty(std::uint32_t destination, std::uint8_t requestId,
                          unsigned status) noexcept;
    // Local abandonment only; already accepted transport bytes cannot be recalled.
    void cancelPropertyRequest(std::uint32_t destination, std::uint8_t requestId) noexcept;
    [[nodiscard]] bool propertyRequestActive(std::uint32_t destination,
                                             std::uint8_t requestId) const noexcept;
    [[nodiscard]] CiDiscovery localDiscovery() const noexcept;
    [[nodiscard]] std::size_t maximumPropertyBytes() const noexcept;
    void poll(std::uint64_t nowMs) noexcept;
    void reset() noexcept;
    [[nodiscard]] Status lastSendError() const noexcept;
    [[nodiscard]] std::uint32_t muid() const noexcept;
    [[nodiscard]] std::span<const CiPeer> peers() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
} // namespace cydsp
