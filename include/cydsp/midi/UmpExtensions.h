#pragma once
#include "cydsp/midi/MidiCi.h"
#include <string_view>
namespace cydsp {
struct UmpFunctionBlockInfo {
    unsigned number = 0, uiHint = 0, midi1 = 0, direction = 3, firstGroup = 0, groups = 1,
             ciVersion = 2, sysex8Streams = 1;
    bool active = true;
};
UmpPacket umpFunctionBlockDiscovery(unsigned number = 255, unsigned filter = 3) noexcept;
UmpPacket umpFunctionBlockInfo(const UmpFunctionBlockInfo&) noexcept;
Status parseUmpFunctionBlockInfo(const UmpPacket&, UmpFunctionBlockInfo&) noexcept;
UmpPacket umpDeviceIdentity(const CiIdentity&) noexcept;
Status parseUmpDeviceIdentity(const UmpPacket&, CiIdentity&) noexcept;
UmpPacket umpClipBoundary(bool start) noexcept;
struct UmpFlexHeader {
    unsigned group = 0, channel = 0, bank = 0, status = 0, form = 0;
}; // Channel0 = group address.
UmpPacket umpFlexData(UmpFlexHeader, std::array<std::uint32_t, 3> payload) noexcept;
Status parseUmpFlexHeader(const UmpPacket&, UmpFlexHeader&) noexcept;
UmpPacket umpTempo(unsigned group, std::uint32_t tenNanosecondsPerQuarter) noexcept;
Status parseUmpTempo(const UmpPacket&, std::uint32_t&) noexcept;
struct UmpTimeSignature {
    unsigned numerator = 4, denominatorPower = 2, notes32PerQuarter = 8;
};
UmpPacket umpTimeSignature(unsigned group, UmpTimeSignature) noexcept;
Status parseUmpTimeSignature(const UmpPacket&, UmpTimeSignature&) noexcept;
struct UmpMetronome {
    unsigned primaryClocks = 24;
    std::array<std::uint8_t, 3> accents{4, 0, 0};
    std::array<std::uint8_t, 2> subdivisions{};
};
UmpPacket umpMetronome(unsigned group, const UmpMetronome&) noexcept;
Status parseUmpMetronome(const UmpPacket&, UmpMetronome&) noexcept;
struct UmpKeySignature {
    int sharpsFlats = 0;
    unsigned tonic = 3;
}; // Tonic1..7 = A..G,0unknown.
UmpPacket umpKeySignature(unsigned group, unsigned channel, UmpKeySignature) noexcept;
Status parseUmpKeySignature(const UmpPacket&, UmpKeySignature&) noexcept;
struct UmpChord {
    int tonicAccidental = 0, bassAccidental = -8;
    unsigned tonic = 0, type = 0, bass = 0, bassType = 0;
    std::array<std::uint8_t, 4> alterations{};
    std::array<std::uint8_t, 2> bassAlterations{};
};
UmpPacket umpChord(unsigned group, unsigned channel, const UmpChord&) noexcept;
Status parseUmpChord(const UmpPacket&, UmpChord&) noexcept;
enum class UmpTextKind { endpointName, productInstance, functionBlockName, metadata, performance };
struct UmpTextHeader {
    UmpTextKind kind = UmpTextKind::endpointName;
    unsigned group = 0, channel = 0, status = 0, functionBlock = 0;
};
Status makeUmpText(UmpTextHeader, std::string_view, std::span<UmpPacket>,
                   std::size_t& written) noexcept;
class UmpTextAssembler {
  public:
    Status prepare(std::size_t maximumBytes = 4096) noexcept;
    Status append(const UmpPacket&) noexcept;
    void reset() noexcept;
    [[nodiscard]] bool complete() const noexcept {
        return complete_;
    }
    [[nodiscard]] std::string_view text() const noexcept {
        return complete_ ? std::string_view(bytes_.data(), used_) : std::string_view{};
    }
    [[nodiscard]] UmpTextHeader header() const noexcept {
        return header_;
    }

  private:
    std::vector<char> bytes_;
    std::size_t used_ = 0;
    UmpTextHeader header_{};
    bool active_ = false, complete_ = false;
};
} // namespace cydsp
