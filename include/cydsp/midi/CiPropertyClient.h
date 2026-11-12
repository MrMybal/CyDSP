#pragma once
#include "cydsp/midi/CiProperty.h"
namespace cydsp {
enum class CiPropertyEncoding { ascii, mcoded7 };
struct CiPropertyPage {
    std::uint32_t offset = 0, limit = 1;
};
struct CiPropertyQuery {
    std::uint32_t peer = 0;
    std::string_view resource{}, resId{}, mediaType{};
    CiPropertyEncoding encoding = CiPropertyEncoding::ascii;
    std::optional<CiPropertyPage> page{};
};
struct CiPropertyClientOptions {
    std::size_t entries = 32, transactions = 8, subscriptions = 32, acknowledgements = 32;
    std::size_t maximumBytes = 65536, maximumJsonNodes = 4096, maximumPointerBytes = 1024;
};
struct CiPropertyCacheView {
    std::span<const std::uint8_t> data;
    std::string_view mediaType;
    std::optional<std::uint32_t> totalCount;
    std::uint64_t expiresAtMs = 0;
    bool fresh = false;
};
struct CiPropertyClientCallbacks {
    void* context = nullptr;
    void (*updated)(void*, const CiPropertyQuery&, CiPropertyCacheView,
                    bool partial) noexcept = nullptr;
    void (*completed)(void*, const CiPropertyQuery&, std::uint8_t messageType,
                      unsigned status) noexcept = nullptr;
    void (*subscription)(void*, const CiPropertyQuery&, bool active) noexcept = nullptr;
};
// Serialized owner; borrowed MidiCiDevice must keep its address and outlive the client.
class CiPropertyClient {
  public:
    CiPropertyClient();
    ~CiPropertyClient();
    CiPropertyClient(CiPropertyClient&&) noexcept;
    CiPropertyClient& operator=(CiPropertyClient&&) noexcept;
    Status prepare(MidiCiDevice&, const CiPropertyClientOptions& = {},
                   CiPropertyClientCallbacks = {}) noexcept;
    Status fetch(const CiPropertyQuery&, std::uint64_t nowMs, bool force = false) noexcept;
    Status set(const CiPropertyQuery&, std::span<const std::uint8_t> decodedData,
               std::uint64_t nowMs, bool partial = false) noexcept;
    Status subscribe(const CiPropertyQuery&, std::uint64_t nowMs) noexcept;
    Status unsubscribe(const CiPropertyQuery&, std::uint64_t nowMs) noexcept;
    Status process(const CiPropertyMessage&, std::uint64_t nowMs) noexcept;
    void
    poll(std::uint64_t nowMs) noexcept; // Also polls core; drains ACKs and coalesced refreshes.
    [[nodiscard]] std::optional<CiPropertyCacheView>
    cached(const CiPropertyQuery&, std::uint64_t nowMs, bool allowStale = false) const noexcept;
    void invalidate(std::uint32_t peer) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t activeSubscriptions() const noexcept;
    [[nodiscard]] Status lastError() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
} // namespace cydsp
