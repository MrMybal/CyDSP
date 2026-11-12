#pragma once
#include "cydsp/core/Types.h"
#include <array>
#include <cstdint>
#include <span>
#include <vector>
namespace cydsp {
// Wire MIDI 1.0 messages; SMF meta-events and UMP are separate representations.
class MidiMessageView {
  public:
    explicit MidiMessageView(std::span<const std::uint8_t> bytes) : data_(bytes) {}
    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] unsigned channel() const noexcept; // 1..16, or 0 for system messages
    [[nodiscard]] bool noteOn() const noexcept;
    [[nodiscard]] bool noteOff() const noexcept;
    [[nodiscard]] unsigned note() const noexcept;
    [[nodiscard]] std::span<const std::uint8_t> bytes() const noexcept {
        return data_;
    }

  private:
    std::span<const std::uint8_t> data_;
};
struct ShortMidiMessage {
    std::array<std::uint8_t, 3> data{};
    std::size_t size = 0;
    [[nodiscard]] std::span<const std::uint8_t> bytes() const noexcept {
        return {data.data(), size};
    }
};
[[nodiscard]] ShortMidiMessage midiNoteOn(unsigned channel, unsigned note,
                                          unsigned velocity) noexcept;
[[nodiscard]] ShortMidiMessage midiNoteOff(unsigned channel, unsigned note,
                                           unsigned velocity = 0) noexcept;
[[nodiscard]] ShortMidiMessage midiController(unsigned channel, unsigned controller,
                                              unsigned value) noexcept;
[[nodiscard]] ShortMidiMessage midiPitchBend(unsigned channel, unsigned value14) noexcept;
struct MidiBufferEvent {
    std::uint32_t frame = 0;
    std::span<const std::uint8_t> bytes;
};
// Prepared fixed byte/event capacities, stable ordering for equal frame offsets.
class MidiBuffer {
  public:
    Status prepare(std::size_t maximumEvents, std::size_t maximumBytes) noexcept;
    Status add(std::uint32_t frame, std::span<const std::uint8_t>) noexcept;
    void reset() noexcept {
        count_ = used_ = 0;
    }
    [[nodiscard]] std::size_t size() const noexcept {
        return count_;
    }
    [[nodiscard]] MidiBufferEvent event(std::size_t index) const noexcept;

  private:
    struct Entry {
        std::uint32_t frame;
        std::size_t offset, size;
    };
    std::vector<Entry> entries_;
    std::vector<std::uint8_t> bytes_;
    std::size_t count_ = 0, used_ = 0;
};
using MidiStreamCallback = void (*)(void*, std::span<const std::uint8_t>,
                                    double timestampSeconds) noexcept;
class MidiStreamParser {
  public:
    Status prepare(std::size_t maximumSysExBytes = 4096) noexcept;
    void reset() noexcept;
    // Callback borrows bytes only for its duration. Real-time system messages may interrupt SysEx.
    Status feed(std::span<const std::uint8_t>, double timestampSeconds, MidiStreamCallback,
                void*) noexcept;

  private:
    std::vector<std::uint8_t> sysex_;
    std::array<std::uint8_t, 3> short_{};
    std::size_t used_ = 0;
    unsigned need_ = 0, have_ = 0;
    std::uint8_t running_ = 0;
    bool inSysEx_ = false, overflow_ = false;
};
class MidiKeyboardState {
  public:
    void reset() noexcept;
    Status apply(MidiMessageView) noexcept;
    [[nodiscard]] bool active(unsigned channel, unsigned note) const noexcept;

  private:
    std::array<std::array<bool, 128>, 16> down_{}, held_{};
    std::array<bool, 16> sustain_{};
};
// Owns Standard MIDI File data. Parsing/serialization can allocate: worker thread only.
enum class MidiFileEventKind { message, meta, sysex };
struct MidiFileEvent {
    std::uint64_t tick = 0;
    MidiFileEventKind kind = MidiFileEventKind::message;
    std::uint8_t type = 0; // Meta type, or 0xf0/0xf7 for SysEx/escape.
    std::vector<std::uint8_t> data;
};
struct MidiFile {
    std::uint16_t format = 1, division = 960; // PPQN or signed SMPTE high byte + ticks/frame.
    std::vector<std::vector<MidiFileEvent>> tracks;
    Status parse(std::span<const std::uint8_t>, std::size_t maximumEvents = 1000000) noexcept;
    Status serialize(std::vector<std::uint8_t>&) const noexcept;
    // Type 0/1 share a tempo map. Type 2 uses the selected independent track.
    Status tickToSeconds(std::uint64_t tick, double& seconds, std::size_t track = 0) const noexcept;
};
} // namespace cydsp
