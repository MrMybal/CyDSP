// Stream/identity/text packing adapted from the pinned Windows MIDI Services builder.
// Copyright (c) Microsoft Corporation.
// MIT: third_party/licenses/windows-midi.txt. Original CyDSP validation/bounds/parsers
// follow the UMP 1.1.1 specification; no WinRT types or runtime are used.
#include "cydsp/midi/UmpExtensions.h"
#include "detail/Alias.h"
#include <algorithm>
namespace cydsp {
namespace {
UmpPacket stream(unsigned status, unsigned low = 0, std::uint32_t one = 0) noexcept {
    return {{{0xf0000000u | (status << 16) | low, one, 0, 0}}, 4};
}
bool streamIs(const UmpPacket& p, unsigned status) noexcept {
    return p.valid() && p.type() == 15 && ((p.words[0] >> 16) & 0xfff) == status;
}
std::uint8_t byte(const UmpPacket& p, unsigned i) noexcept {
    return std::uint8_t(p.words[i / 4] >> (24 - 8 * (i % 4)));
}
void byte(UmpPacket& p, unsigned i, unsigned value) noexcept {
    p.words[i / 4] |= std::uint32_t(value) << (24 - 8 * (i % 4));
}
bool validBlock(const UmpFunctionBlockInfo& b) noexcept {
    return b.number < 32 && b.uiHint < 4 && b.midi1 < 3 && b.direction >= 1 && b.direction <= 3 &&
           b.firstGroup < 16 && b.groups >= 1 && b.groups <= 16 - b.firstGroup &&
           b.ciVersion < 128 && b.sysex8Streams <= 255 && (!b.midi1 || b.groups == 1);
}
bool utf8(std::string_view text) noexcept {
    for (std::size_t i = 0; i < text.size();) {
        const auto a = static_cast<unsigned char>(text[i++]);
        if (!a)
            return false;
        if (a < 128)
            continue;
        unsigned n = 0, value = 0, minimum = 0;
        if (a >= 0xc2 && a <= 0xdf) {
            n = 1;
            value = a & 31;
            minimum = 128;
        } else if (a >= 0xe0 && a <= 0xef) {
            n = 2;
            value = a & 15;
            minimum = 2048;
        } else if (a >= 0xf0 && a <= 0xf4) {
            n = 3;
            value = a & 7;
            minimum = 65536;
        } else
            return false;
        if (n > text.size() - i)
            return false;
        while (n--) {
            const auto b = static_cast<unsigned char>(text[i++]);
            if ((b & 0xc0) != 0x80)
                return false;
            value = (value << 6) | (b & 63);
        }
        if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
            return false;
    }
    return true;
}
bool same(UmpTextHeader a, UmpTextHeader b) noexcept {
    return a.kind == b.kind && a.group == b.group && a.channel == b.channel &&
           a.status == b.status && a.functionBlock == b.functionBlock;
}
std::size_t maximum(UmpTextKind kind) noexcept {
    return kind == UmpTextKind::endpointName        ? 98
           : kind == UmpTextKind::productInstance   ? 42
           : kind == UmpTextKind::functionBlockName ? 91
                                                    : 65536;
}
bool flexIs(const UmpPacket& p, unsigned status) noexcept {
    UmpFlexHeader h;
    return parseUmpFlexHeader(p, h) == Status::ok && h.bank == 0 && h.status == status &&
           h.form == 0;
}
bool alteration(std::uint8_t v) noexcept {
    return (v >> 4) <= 4 && (!(v >> 4) || ((v & 15) != 0));
}
int signedNibble(unsigned v) noexcept {
    return (v & 8) ? int(v) - 16 : int(v);
}
} // namespace
UmpPacket umpFunctionBlockDiscovery(unsigned number, unsigned filter) noexcept {
    return ((number < 32 || number == 255) && filter < 4) ? stream(16, (number << 8) | filter)
                                                          : UmpPacket{};
}
UmpPacket umpFunctionBlockInfo(const UmpFunctionBlockInfo& b) noexcept {
    if (!validBlock(b))
        return {};
    return stream(17,
                  (b.active ? 0x8000 : 0) | (b.number << 8) | (b.uiHint << 4) | (b.midi1 << 2) |
                      b.direction,
                  (b.firstGroup << 24) | (b.groups << 16) | (b.ciVersion << 8) | b.sysex8Streams);
}
Status parseUmpFunctionBlockInfo(const UmpPacket& p, UmpFunctionBlockInfo& out) noexcept {
    if (!streamIs(p, 17) || (p.words[0] & 0xc0) || p.words[2] || p.words[3])
        return Status::invalid_argument;
    UmpFunctionBlockInfo b;
    b.active = (p.words[0] & 0x8000) != 0;
    b.number = (p.words[0] >> 8) & 127;
    b.uiHint = (p.words[0] >> 4) & 3;
    b.midi1 = (p.words[0] >> 2) & 3;
    b.direction = p.words[0] & 3;
    b.firstGroup = p.words[1] >> 24;
    b.groups = (p.words[1] >> 16) & 255;
    b.ciVersion = (p.words[1] >> 8) & 255;
    b.sysex8Streams = p.words[1] & 255;
    if (!validBlock(b))
        return Status::invalid_argument;
    out = b;
    return Status::ok;
}
UmpPacket umpDeviceIdentity(const CiIdentity& i) noexcept {
    if (i.family > 16383 || i.model > 16383)
        return {};
    for (auto v : i.manufacturer)
        if (v > 127)
            return {};
    for (auto v : i.revision)
        if (v > 127)
            return {};
    auto p = stream(2, 0,
                    (unsigned(i.manufacturer[0]) << 16) | (unsigned(i.manufacturer[1]) << 8) |
                        i.manufacturer[2]);
    p.words[2] = ((i.family & 127) << 24) | ((i.family >> 7) << 16) | ((i.model & 127) << 8) |
                 (i.model >> 7);
    p.words[3] = (unsigned(i.revision[0]) << 24) | (unsigned(i.revision[1]) << 16) |
                 (unsigned(i.revision[2]) << 8) | i.revision[3];
    return p;
}
Status parseUmpDeviceIdentity(const UmpPacket& p, CiIdentity& out) noexcept {
    if (!streamIs(p, 2) || (p.words[0] & 65535) || (p.words[1] & 0xff808080u) ||
        (p.words[2] & 0x80808080u) || (p.words[3] & 0x80808080u))
        return Status::invalid_argument;
    CiIdentity i;
    i.manufacturer = {byte(p, 5), byte(p, 6), byte(p, 7)};
    i.family = std::uint16_t(byte(p, 8) | (unsigned(byte(p, 9)) << 7));
    i.model = std::uint16_t(byte(p, 10) | (unsigned(byte(p, 11)) << 7));
    i.revision = {byte(p, 12), byte(p, 13), byte(p, 14), byte(p, 15)};
    out = i;
    return Status::ok;
}
UmpPacket umpClipBoundary(bool start) noexcept {
    return stream(start ? 32 : 33);
}
UmpPacket umpFlexData(UmpFlexHeader h, std::array<std::uint32_t, 3> body) noexcept {
    if (h.group > 15 || h.channel > 16 || h.bank > 255 || h.status > 255 || h.form > 3)
        return {};
    return {{{0xd0000000u | (h.group << 24) | (h.form << 22) |
                  (h.channel ? ((h.channel - 1) << 16) : 0x100000u) | (h.bank << 8) | h.status,
              body[0], body[1], body[2]}},
            4};
}
Status parseUmpFlexHeader(const UmpPacket& p, UmpFlexHeader& out) noexcept {
    if (!p.valid() || p.type() != 13)
        return Status::invalid_argument;
    const auto address = (p.words[0] >> 20) & 3, channel = (p.words[0] >> 16) & 15;
    if (address > 1 || (address == 1 && channel))
        return Status::invalid_argument;
    out = {p.group(), address == 1 ? 0 : channel + 1, (p.words[0] >> 8) & 255, p.words[0] & 255,
           (p.words[0] >> 22) & 3};
    return Status::ok;
}
UmpPacket umpTempo(unsigned group, std::uint32_t value) noexcept {
    return value ? umpFlexData({group, 0, 0, 0, 0}, {value, 0, 0}) : UmpPacket{};
}
Status parseUmpTempo(const UmpPacket& p, std::uint32_t& value) noexcept {
    UmpFlexHeader h;
    if (parseUmpFlexHeader(p, h) != Status::ok || !flexIs(p, 0) || h.channel || !p.words[1] ||
        p.words[2] || p.words[3])
        return Status::invalid_argument;
    value = p.words[1];
    return Status::ok;
}
UmpPacket umpTimeSignature(unsigned group, UmpTimeSignature s) noexcept {
    if (!s.numerator || s.numerator > 256 || s.denominatorPower > 255 || s.notes32PerQuarter > 255)
        return {};
    return umpFlexData(
        {group, 0, 0, 1, 0},
        {((s.numerator & 255) << 24) | (s.denominatorPower << 16) | (s.notes32PerQuarter << 8), 0,
         0});
}
Status parseUmpTimeSignature(const UmpPacket& p, UmpTimeSignature& out) noexcept {
    UmpFlexHeader h;
    if (parseUmpFlexHeader(p, h) != Status::ok || !flexIs(p, 1) || h.channel ||
        (p.words[1] & 255) || p.words[2] || p.words[3])
        return Status::invalid_argument;
    out = {p.words[1] >> 24 ? p.words[1] >> 24 : 256, (p.words[1] >> 16) & 255,
           (p.words[1] >> 8) & 255};
    return Status::ok;
}
UmpPacket umpMetronome(unsigned group, const UmpMetronome& m) noexcept {
    if (m.primaryClocks > 255)
        return {};
    return umpFlexData({group, 0, 0, 2, 0},
                       {(m.primaryClocks << 24) | (unsigned(m.accents[0]) << 16) |
                            (unsigned(m.accents[1]) << 8) | m.accents[2],
                        (unsigned(m.subdivisions[0]) << 24) | (unsigned(m.subdivisions[1]) << 16),
                        0});
}
Status parseUmpMetronome(const UmpPacket& p, UmpMetronome& out) noexcept {
    UmpFlexHeader h;
    if (parseUmpFlexHeader(p, h) != Status::ok || !flexIs(p, 2) || h.channel ||
        (p.words[2] & 65535) || p.words[3])
        return Status::invalid_argument;
    out = {byte(p, 4), {byte(p, 5), byte(p, 6), byte(p, 7)}, {byte(p, 8), byte(p, 9)}};
    return Status::ok;
}
UmpPacket umpKeySignature(unsigned group, unsigned channel, UmpKeySignature key) noexcept {
    if (key.sharpsFlats < -8 || key.sharpsFlats > 7 || key.tonic > 7)
        return {};
    return umpFlexData({group, channel, 0, 5, 0},
                       {((unsigned(key.sharpsFlats) & 15) << 28) | (key.tonic << 24), 0, 0});
}
Status parseUmpKeySignature(const UmpPacket& p, UmpKeySignature& out) noexcept {
    if (!flexIs(p, 5) || (p.words[1] & 0x00ffffffu) || p.words[2] || p.words[3] ||
        ((p.words[1] >> 24) & 15) > 7)
        return Status::invalid_argument;
    out = {signedNibble(p.words[1] >> 28), (p.words[1] >> 24) & 15};
    return Status::ok;
}
UmpPacket umpChord(unsigned group, unsigned channel, const UmpChord& c) noexcept {
    if (c.tonicAccidental < -2 || c.tonicAccidental > 2 || c.tonic > 7 || c.type > 27 ||
        c.bass > 7 || c.bassType > 27 ||
        (c.bassAccidental != -8 && (c.bassAccidental < -2 || c.bassAccidental > 2)) ||
        ((c.bassAccidental == -8) != (c.bass == 0)))
        return {};
    for (auto a : c.alterations)
        if (!alteration(a))
            return {};
    for (auto a : c.bassAlterations)
        if (!alteration(a))
            return {};
    const auto tonic = ((unsigned(c.tonicAccidental) & 15) << 4) | c.tonic,
               bass = ((unsigned(c.bassAccidental) & 15) << 4) | c.bass;
    return umpFlexData(
        {group, channel, 0, 6, 0},
        {(tonic << 24) | (c.type << 16) | (unsigned(c.alterations[0]) << 8) | c.alterations[1],
         (unsigned(c.alterations[2]) << 24) | (unsigned(c.alterations[3]) << 16) | (bass << 8) |
             c.bassType,
         (unsigned(c.bassAlterations[0]) << 24) | (unsigned(c.bassAlterations[1]) << 16)});
}
Status parseUmpChord(const UmpPacket& p, UmpChord& out) noexcept {
    if (!flexIs(p, 6) || (p.words[3] & 65535))
        return Status::invalid_argument;
    UmpChord c;
    c.tonicAccidental = signedNibble(byte(p, 4) >> 4);
    c.tonic = byte(p, 4) & 15;
    c.type = byte(p, 5);
    c.alterations = {byte(p, 6), byte(p, 7), byte(p, 8), byte(p, 9)};
    c.bassAccidental = signedNibble(byte(p, 10) >> 4);
    c.bass = byte(p, 10) & 15;
    c.bassType = byte(p, 11);
    c.bassAlterations = {byte(p, 12), byte(p, 13)};
    if (!umpChord(p.group(), ((p.words[0] >> 20) & 3) == 1 ? 0 : p.channel(), c).valid())
        return Status::invalid_argument;
    out = c;
    return Status::ok;
}
Status makeUmpText(UmpTextHeader h, std::string_view text, std::span<UmpPacket> out,
                   std::size_t& written) noexcept {
    written = 0;
    if (static_cast<unsigned>(h.kind) > 4 || h.functionBlock > 31 || !utf8(text) ||
        text.size() > maximum(h.kind))
        return Status::invalid_argument;
    const bool flex = h.kind == UmpTextKind::metadata || h.kind == UmpTextKind::performance;
    if ((!flex && (h.group || h.channel || h.status)) ||
        (flex && (h.group > 15 || h.channel > 16 ||
                  h.status > (h.kind == UmpTextKind::metadata ? 12u : 4u))))
        return Status::invalid_argument;
    if (h.kind == UmpTextKind::productInstance &&
        !std::all_of(text.begin(), text.end(), [](unsigned char c) { return c >= 32 && c < 127; }))
        return Status::invalid_argument;
    const auto width = flex ? 12u : h.kind == UmpTextKind::functionBlockName ? 13u : 14u;
    written = std::max<std::size_t>(1, (text.size() + width - 1) / width);
    if (out.size() < written)
        return Status::block_too_large;
    if (detail::overlaps(text.data(), text.size(), out.data(), written * sizeof(UmpPacket)))
        return Status::partial_overlap;
    for (std::size_t i = 0; i < written; ++i) {
        const auto form = written == 1 ? 0u : i == 0 ? 1u : i + 1 == written ? 3u : 2u;
        UmpPacket p;
        if (flex)
            p = umpFlexData(
                {h.group, h.channel, h.kind == UmpTextKind::metadata ? 1u : 2u, h.status, form},
                {0, 0, 0});
        else {
            p = stream(h.kind == UmpTextKind::endpointName      ? 3
                       : h.kind == UmpTextKind::productInstance ? 4
                                                                : 18,
                       h.kind == UmpTextKind::functionBlockName ? h.functionBlock << 8 : 0);
            p.words[0] |= form << 26;
        }
        for (unsigned j = 0; j < width && i * width + j < text.size(); ++j)
            byte(p, 16 - width + j, static_cast<unsigned char>(text[i * width + j]));
        out[i] = p;
    }
    return Status::ok;
}
Status UmpTextAssembler::prepare(std::size_t maximumBytes) noexcept {
    if (!maximumBytes || maximumBytes > 65536)
        return Status::invalid_argument;
    try {
        std::vector<char> next(maximumBytes);
        bytes_.swap(next);
        reset();
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
void UmpTextAssembler::reset() noexcept {
    used_ = 0;
    active_ = complete_ = false;
}
Status UmpTextAssembler::append(const UmpPacket& p) noexcept {
    if (bytes_.empty())
        return Status::not_prepared;
    if (!p.valid())
        return Status::invalid_argument;
    UmpTextHeader h;
    unsigned form = 0, width = 0;
    if (p.type() == 15) {
        const auto status = (p.words[0] >> 16) & 1023;
        form = (p.words[0] >> 26) & 3;
        if (status != 3 && status != 4 && status != 18)
            return Status::unsupported_format;
        h.kind = status == 3   ? UmpTextKind::endpointName
                 : status == 4 ? UmpTextKind::productInstance
                               : UmpTextKind::functionBlockName;
        if (status == 18) {
            h.functionBlock = (p.words[0] >> 8) & 255;
            if (h.functionBlock > 31)
                return Status::invalid_argument;
        }
        width = status == 18 ? 13 : 14;
    } else if (p.type() == 13) {
        UmpFlexHeader f;
        if (parseUmpFlexHeader(p, f) != Status::ok)
            return Status::invalid_argument;
        if (f.bank != 1 && f.bank != 2)
            return Status::unsupported_format;
        h.kind = f.bank == 1 ? UmpTextKind::metadata : UmpTextKind::performance;
        h.group = f.group;
        h.channel = f.channel;
        h.status = f.status;
        if (h.status > (f.bank == 1 ? 12u : 4u))
            return Status::invalid_argument;
        form = f.form;
        width = 12;
    } else
        return Status::unsupported_format;
    if (form <= 1)
        reset();
    else if (!active_ || !same(h, header_)) {
        reset();
        return Status::invalid_argument;
    }
    header_ = h;
    complete_ = false;
    const bool final = form == 0 || form == 3;
    bool padded = false;
    for (unsigned i = 16 - width; i < 16; ++i) {
        const auto c = byte(p, i);
        if (!c) {
            if (!final) {
                reset();
                return Status::invalid_argument;
            }
            padded = true;
            continue;
        }
        if (padded) {
            reset();
            return Status::invalid_argument;
        }
        if (used_ == bytes_.size() || used_ == maximum(h.kind)) {
            reset();
            return Status::block_too_large;
        }
        bytes_[used_++] = char(c);
    }
    active_ = !final;
    complete_ = final;
    if (final) {
        const std::string_view value(bytes_.data(), used_);
        if (!utf8(value) || (h.kind == UmpTextKind::productInstance &&
                             !std::all_of(value.begin(), value.end(),
                                          [](unsigned char c) { return c >= 32 && c < 127; }))) {
            reset();
            return Status::invalid_argument;
        }
    }
    return Status::ok;
}
} // namespace cydsp
