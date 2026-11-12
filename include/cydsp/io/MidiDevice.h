#pragma once
#include "cydsp/core/Types.h"
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>
namespace cydsp {
struct MidiPortInfo {
    std::uint32_t index = 0;
    std::string name;
    std::string persistentId;
    bool persistentIdentity = false;
};
struct MidiDevicePorts {
    std::string backend;
    std::vector<MidiPortInfo> inputs, outputs;
    bool virtualInput = false, virtualOutput = false;
};
struct MidiPortSelector {
    std::string backend, persistentId, name;
    bool input = true, allowNameFallback = false;
};
// Indices are enumeration snapshots. Duplicate stable IDs/names return ambiguous.
Status resolveMidiPort(const MidiDevicePorts&, const MidiPortSelector&, std::uint32_t&) noexcept;
Status serializeMidiPortSelector(const MidiPortSelector&, std::string&) noexcept;
Status parseMidiPortSelector(std::string_view, MidiPortSelector&) noexcept;
struct MidiInputOptions {
    std::size_t queuedMessages = 128, maximumMessageBytes = 4096;
    unsigned nativeBufferCount = 4;
};
struct MidiInputTimestamp {
    double relativeSeconds = 0;
    std::uint64_t arrivalNanoseconds = 0;
};
// Port indices are snapshots; hotplug changes require a fresh enumeration.
Status enumerateMidiPorts(MidiDevicePorts&) noexcept;
class MidiInput {
  public:
    MidiInput();
    ~MidiInput();
    MidiInput(MidiInput&&) noexcept;
    MidiInput& operator=(MidiInput&&) noexcept;
    // Control thread; bounded preallocated SPSC storage, configurable up to 1 MiB/message.
    Status open(std::uint32_t port, std::size_t queuedMessages = 128) noexcept;
    Status openVirtual(std::string_view name, std::size_t queuedMessages = 128) noexcept;
    Status open(std::uint32_t port, const MidiInputOptions&) noexcept;
    Status openVirtual(std::string_view name, const MidiInputOptions&) noexcept;
    void close() noexcept;
    // Single consumer. Small destination returns block_too_large without consuming the event.
    // Timestamp is monotonic seconds since the first event, accumulated from backend deltas.
    Status read(std::span<std::uint8_t>, std::size_t& size, double& timestampSeconds) noexcept;
    Status readTimed(std::span<std::uint8_t>, std::size_t& size, MidiInputTimestamp&) noexcept;
    [[nodiscard]] bool isVirtual() const noexcept;
    [[nodiscard]] std::uint32_t droppedMessages() const noexcept;
    [[nodiscard]] std::uint32_t oversizedMessages() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
class MidiOutput {
  public:
    MidiOutput();
    ~MidiOutput();
    MidiOutput(MidiOutput&&) noexcept;
    MidiOutput& operator=(MidiOutput&&) noexcept;
    Status open(std::uint32_t port) noexcept;
    Status openVirtual(std::string_view name) noexcept;
    [[nodiscard]] bool isVirtual() const noexcept;
    void close() noexcept;
    // Backend sends can block (especially SysEx). Call from a control/worker thread.
    Status send(std::span<const std::uint8_t>) noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
} // namespace cydsp
