#pragma once
#include "cydsp/io/MidiDevice.h"
#include "cydsp/midi/MidiInputBank.h"
namespace cydsp {
struct MidiInputGroupOptions {
    MidiInputOptions input;
    std::uint64_t scanIntervalMs = 250;
    std::size_t maximumMessagesPerPoll = 256;
};
// prepare/poll/close run on one control worker; collect/block on one audio consumer.
// Native callbacks feed separate device queues. The control worker alone produces
// into the bank, preserving SPSC ownership even with many simultaneous inputs.
class MidiInputGroup {
  public:
    MidiInputGroup();
    ~MidiInputGroup();
    MidiInputGroup(MidiInputGroup&&) noexcept;
    MidiInputGroup& operator=(MidiInputGroup&&) noexcept;
    Status prepare(std::span<const MidiPortSelector>, const MidiInputGroupOptions& = {}) noexcept;
    Status poll(std::uint64_t nowMs) noexcept;
    // Explicit backend failure/hotplug notification from the application, on producer.
    Status disconnect(std::size_t port) noexcept;
    Status collect(const AudioClockAnchor&, std::uint32_t frames) noexcept;
    [[nodiscard]] MidiInputPortBlock block(std::size_t port) const noexcept;
    [[nodiscard]] bool connected(std::size_t port) const noexcept;
    [[nodiscard]] Status portStatus(std::size_t port) const noexcept;
    void close() noexcept; // Both owners idle.
  private:
    struct State;
    std::unique_ptr<State> state_;
};
} // namespace cydsp
