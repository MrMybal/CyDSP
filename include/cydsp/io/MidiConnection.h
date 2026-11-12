#pragma once
#include "cydsp/io/Midi2Device.h"
#include "cydsp/midi/MidiEventCollector.h"
namespace cydsp {
struct MidiConnectionPort {
    std::string id, name;
    bool receive = true, send = true;
};
struct MidiConnectionSelection {
    std::string inputId, outputId;
};
struct MidiConnectionMessage {
    std::array<std::uint8_t, 4096> bytes{};
    std::size_t size = 0;
    UmpPacket packet;
    std::uint64_t timestampNs = 0;
    bool ump = false;
};
struct MidiConnectionDriver {
    void* context = nullptr;
    Status (*enumerate)(void*, std::vector<MidiConnectionPort>&) noexcept = nullptr;
    Status (*open)(void*, const MidiConnectionSelection&) noexcept = nullptr;
    void (*close)(void*) noexcept = nullptr;
    Status (*read)(void*, MidiConnectionMessage&) noexcept = nullptr;
    Status (*send)(void*, const MidiConnectionMessage&) noexcept = nullptr;
    Status (*health)(void*) noexcept = nullptr;
};
struct MidiReconnectOptions {
    std::uint64_t scanIntervalMs = 250, retryIntervalMs = 1000;
    std::size_t queuedMessages = 1024, maximumReadPerPoll = 256;
};
enum class MidiConnectionEvent { connected, disconnected, failed };
struct MidiConnectionCallbacks {
    void* context = nullptr;
    void (*changed)(void*, MidiConnectionEvent, Status) noexcept = nullptr;
};
// poll/enumerate/open/close are serialized worker operations. The stable collector
// and outgoing SPSC allow collect/send on one audio thread during reconnection.
// On disconnect, old-epoch work is discarded; host silences any held voices when
// it observes a changed generation/connected flag. No hidden polling thread.
class MidiConnection {
  public:
    MidiConnection();
    ~MidiConnection();
    MidiConnection(MidiConnection&&) noexcept;
    MidiConnection& operator=(MidiConnection&&) noexcept;
    Status prepare(bool midi2 = false, MidiReconnectOptions = {},
                   MidiConnectionCallbacks = {}) noexcept;
    Status prepare(MidiConnectionDriver, MidiReconnectOptions = {},
                   MidiConnectionCallbacks = {}) noexcept;
    Status enumerate(std::vector<MidiConnectionPort>&) noexcept;
    Status open(const MidiConnectionSelection&, std::uint64_t nowMs = 0) noexcept;
    Status poll(std::uint64_t nowMs) noexcept;
    void close() noexcept;
    Status send(const MidiConnectionMessage&) noexcept;
    Status collect(const AudioClockAnchor&, std::uint32_t, MidiBuffer&, UmpBuffer&) noexcept;
    [[nodiscard]] bool connected() const noexcept;
    [[nodiscard]] std::uint32_t generation() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
} // namespace cydsp
