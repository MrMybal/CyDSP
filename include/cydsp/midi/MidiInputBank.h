#pragma once
#include "cydsp/midi/MidiEventCollector.h"
#include <memory>
namespace cydsp {
struct MidiInputBankOptions {
    std::size_t ports = 8, messagesPerPort = 128, maximumMessageBytes = 4096;
    LateMidiPolicy late = LateMidiPolicy::clampToFirstFrame;
};
struct MidiInputPortBlock {
    const MidiBuffer* midi = nullptr;
    const UmpBuffer* ump = nullptr;
    std::uint32_t generation = 0;
    bool releaseVoices = false;
};
// Each port has exactly one producer; one audio consumer collects all ports.
// Different device producers never share an SPSC queue. prepare/destruction need all idle.
// Per-port views retain source identity, valid until the next collect/prepare.
class MidiInputBank {
  public:
    MidiInputBank();
    ~MidiInputBank();
    MidiInputBank(MidiInputBank&&) noexcept;
    MidiInputBank& operator=(MidiInputBank&&) noexcept;
    Status prepare(const MidiInputBankOptions& = {}) noexcept;
    Status ingest(std::size_t port, std::uint64_t arrivalNanoseconds,
                  std::span<const std::uint8_t> fragments) noexcept;
    Status ingestUmp(std::size_t port, std::uint64_t, const UmpPacket&) noexcept;
    // Producer only. Discards partial SysEx/old events. Audio receives voice-release
    // notification and MIDI1 sustain/all-notes/all-sound-off on all 16 channels.
    // UMP consumers must apply releaseVoices to ALL groups (not just MIDI group 0).
    Status invalidate(std::size_t port) noexcept;
    Status collect(const AudioClockAnchor&, std::uint32_t frames) noexcept;
    [[nodiscard]] MidiInputPortBlock block(std::size_t port) const noexcept;
    [[nodiscard]] std::size_t ports() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
} // namespace cydsp
