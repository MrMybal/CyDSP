#pragma once
#include "cydsp/midi/MidiCi.h"
#include <optional>
#include <string_view>
namespace cydsp {
struct CiPropertyValue {
    std::span<const std::uint8_t> json; // Borrowed complete primitive JSON token.
    Status string(std::span<char>, std::size_t& utf8Bytes) const noexcept;
    Status unsignedInteger(std::uint32_t&) const noexcept;
    Status boolean(bool&) const noexcept;
};
class CiPropertyHeader {
  public:
    // Strict PE flat header: no exterior whitespace, primitive values, <=64 unique keys.
    // Borrows input until the header and all retrieved values are no longer used.
    Status parse(std::span<const std::uint8_t>) noexcept;
    [[nodiscard]] std::optional<CiPropertyValue> find(std::string_view key) const noexcept;
    [[nodiscard]] std::string_view firstKey() const noexcept;

  private:
    struct Field {
        std::array<char, 20> key{};
        std::size_t size = 0;
        CiPropertyValue value;
    };
    std::array<Field, 64> fields_{};
    std::size_t count_ = 0;
};
// Wire JSON must be ASCII (Unicode uses valid UTF-16 escapes), depth <=32.
Status validateCiPropertyJson(std::span<const std::uint8_t>) noexcept;
// UTF-8 to one quoted ASCII JSON string, without allocation. Transactional output.
Status ciPropertyJsonString(std::string_view utf8, std::span<std::uint8_t>, std::size_t&) noexcept;
class CiPropertyPatch {
  public:
    CiPropertyPatch();
    ~CiPropertyPatch();
    CiPropertyPatch(CiPropertyPatch&&) noexcept;
    CiPropertyPatch& operator=(CiPropertyPatch&&) noexcept;
    Status prepare(std::size_t maximumNodes = 4096,
                   std::size_t maximumPointerBytes = 1024) noexcept;
    // MIDI-CI partial SET: existing RFC6901 paths -> string/number/bool replacements.
    // Rejects missing/overlapping paths, null/containers and aliased output before writes.
    Status apply(std::span<const std::uint8_t> document, std::span<const std::uint8_t> changes,
                 std::span<std::uint8_t> output, std::size_t& written) noexcept;
    Status list(std::span<const std::uint8_t>, std::uint32_t offset, std::uint32_t limit,
                std::span<std::uint8_t> output, std::size_t& written,
                std::uint32_t& totalCount) noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
struct CiChannelDescription {
    std::string title, programTitle;
    unsigned channel = 1; // Function-block-relative 1..256.
    std::array<std::uint8_t, 3> bankPC{};
    bool hasBankPC = false;
    unsigned clusterChannelStart = 0, clusterLength = 0, clusterMidiMode = 3;
    std::string clusterType = "other"; // other/profile/mpe1
};
struct CiPropertyInstance {
    std::string id;
    std::vector<std::uint8_t> data;
};
struct CiDeviceInformation {
    std::string manufacturer, family, model, version, serialNumber;
};
enum class CiSubscriptionDelivery { notify, full, partial };
struct CiPropertyDefinition {
    std::string resource;           // Custom names start X-, <=36 ASCII characters.
    std::vector<std::uint8_t> data; // ASCII wire JSON, or decoded binary for other media.
    std::string mediaType = "application/json";
    std::string schema; // Optional application JSON schema, included in ResourceList.
    std::size_t maximumBytes = 65536;
    bool writable = false, subscribable = false;
    bool partial = false, paginated = false;
    std::uint32_t cacheTimeSeconds = 0;
    CiSubscriptionDelivery delivery = CiSubscriptionDelivery::notify;
    std::vector<CiPropertyInstance> instances; // Nonempty -> requireResId; data must then be empty.
};
struct CiPropertyStoreOptions {
    std::size_t subscriptions = 32, replySlots = 8;
    std::span<const CiChannelDescription> channels{}; // Copied/generated only during prepare.
    std::size_t maximumJsonNodes = 4096, maximumPointerBytes = 1024;
};
struct CiPropertyStoreCallbacks {
    void* context = nullptr;
    bool (*authorize)(void*, std::uint32_t peer, std::string_view resource,
                      bool writing) noexcept = nullptr;
    bool (*validate)(void*, std::string_view resource,
                     std::span<const std::uint8_t>) noexcept = nullptr;
    void (*changed)(void*, std::string_view resource,
                    std::span<const std::uint8_t>) noexcept = nullptr;
};
// One serialized control owner. Prepared GET/SET/subscription processing retains memory.
// Partial SET and list operations use retained bounded workspaces; schema evaluation is a callback.
class CiPropertyStore {
  public:
    CiPropertyStore();
    ~CiPropertyStore();
    CiPropertyStore(CiPropertyStore&&) noexcept;
    CiPropertyStore& operator=(CiPropertyStore&&) noexcept;
    Status prepare(MidiCiDevice&, const CiDeviceInformation&,
                   std::span<const CiPropertyDefinition> = {}, const CiPropertyStoreOptions& = {},
                   CiPropertyStoreCallbacks = {}) noexcept;
    Status process(const CiPropertyMessage&) noexcept; // Use from CiCallbacks::property.
    Status publish(std::string_view resource, std::span<const std::uint8_t>,
                   std::string_view resId = {}) noexcept;
    Status publishChannels(std::span<const CiChannelDescription>) noexcept;
    [[nodiscard]] std::span<const std::uint8_t> data(std::string_view resource,
                                                     std::string_view resId = {}) const noexcept;
    void
    poll(std::uint64_t nowMs) noexcept; // Also calls MidiCiDevice::poll; flushes replies/updates.
    void invalidate(std::uint32_t peer) noexcept; // Use on peer removal / MUID invalidation.
    void reset() noexcept; // Clear jobs/subscriptions, retain current resources/storage.
    [[nodiscard]] std::size_t activeSubscriptions() const noexcept;
    [[nodiscard]] Status lastError() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
} // namespace cydsp
