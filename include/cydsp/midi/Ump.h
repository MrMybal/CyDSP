#pragma once
#include "cydsp/midi/Midi.h"
#include <array>
namespace cydsp {
// UMP words are host-order integers; byte serialization is explicitly big-endian.
struct UmpPacket {
    std::array<std::uint32_t, 4> words{};
    std::uint8_t count = 0;
    [[nodiscard]] unsigned type() const noexcept {
        return words[0] >> 28;
    }
    [[nodiscard]] unsigned group() const noexcept {
        return (words[0] >> 24) & 15;
    }
    [[nodiscard]] unsigned channel() const noexcept {
        return ((words[0] >> 16) & 15) + 1;
    }
    [[nodiscard]] unsigned status() const noexcept {
        return (words[0] >> 20) & 15;
    }
    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] Status toBytes(std::span<std::uint8_t>, std::size_t&) const noexcept;
};
[[nodiscard]] unsigned umpWordCount(unsigned type) noexcept;
using UmpCallback = void (*)(void*, const UmpPacket&) noexcept;
class UmpStreamParser {
  public:
    Status feed(std::span<const std::uint32_t>, UmpCallback, void*) noexcept;
    Status feedBytes(std::span<const std::uint8_t>, UmpCallback, void*) noexcept;
    void reset() noexcept {
        packet_ = {};
        have_ = byteCount_ = 0;
        byteWord_ = 0;
    }
    [[nodiscard]] bool incomplete() const noexcept {
        return have_ != 0 || byteCount_ != 0;
    }

  private:
    UmpPacket packet_;
    unsigned have_ = 0, byteCount_ = 0;
    std::uint32_t byteWord_ = 0;
};
struct UmpEvent {
    std::uint32_t frame = 0;
    UmpPacket packet;
};
class UmpBuffer {
  public:
    Status prepare(std::size_t maximumEvents) noexcept;
    Status add(std::uint32_t, const UmpPacket&) noexcept;
    void reset() noexcept {
        count_ = 0;
    }
    [[nodiscard]] std::span<const UmpEvent> events() const noexcept {
        return {events_.data(), count_};
    }

  private:
    std::vector<UmpEvent> events_;
    std::size_t count_ = 0;
};
// Factories use groups 0..15, channels 1..16 and exact protocol widths.
[[nodiscard]] UmpPacket umpMidi1(unsigned group, MidiMessageView) noexcept;
[[nodiscard]] UmpPacket umpNote(bool on, unsigned group, unsigned channel, unsigned note,
                                std::uint16_t velocity, std::uint8_t attributeType = 0,
                                std::uint16_t attribute = 0) noexcept;
[[nodiscard]] UmpPacket umpController(unsigned group, unsigned channel, unsigned controller,
                                      std::uint32_t value) noexcept;
[[nodiscard]] UmpPacket umpPitch(unsigned group, unsigned channel, std::uint32_t value,
                                 int note = -1) noexcept;
[[nodiscard]] UmpPacket umpPressure(unsigned group, unsigned channel, std::uint32_t value,
                                    int note = -1) noexcept;
[[nodiscard]] UmpPacket umpPerNoteController(bool registered, unsigned group, unsigned channel,
                                             unsigned note, unsigned index,
                                             std::uint32_t value) noexcept;
[[nodiscard]] UmpPacket umpProgram(unsigned group, unsigned channel, unsigned program,
                                   int bankMsb = -1, int bankLsb = -1) noexcept;
[[nodiscard]] std::uint32_t midiScale7To32(unsigned value) noexcept;
[[nodiscard]] std::uint16_t midiScale7To16(unsigned value) noexcept;
// Precision loss is explicit; unsupported per-note/controllers return unsupported_format.
Status midi1ToMidi2(const UmpPacket&, UmpPacket&) noexcept;
Status midi2ToMidi1(const UmpPacket&, ShortMidiMessage&) noexcept;
// SysEx payload excludes F0/F7. State is 0 complete,1 start,2 continue,3 end.
[[nodiscard]] UmpPacket umpSysEx7(unsigned group, unsigned state,
                                  std::span<const std::uint8_t>) noexcept;
[[nodiscard]] UmpPacket umpSysEx8(unsigned group, unsigned state, std::uint8_t streamId,
                                  std::span<const std::uint8_t>) noexcept;
class UmpSysExAssembler {
  public:
    Status prepare(std::size_t maximumBytes) noexcept;
    void reset() noexcept {
        used_ = 0;
        active_ = complete_ = false;
    }
    // One selected group/stream at a time; unrelated packets return unsupported_format.
    Status append(const UmpPacket&) noexcept;
    [[nodiscard]] std::span<const std::uint8_t> message() const noexcept {
        return complete_ ? std::span<const std::uint8_t>(storage_.data(), used_)
                         : std::span<const std::uint8_t>{};
    }

  private:
    std::vector<std::uint8_t> storage_;
    std::size_t used_ = 0;
    unsigned group_ = 0, type_ = 0, stream_ = 0;
    bool active_ = false, complete_ = false;
};
} // namespace cydsp
