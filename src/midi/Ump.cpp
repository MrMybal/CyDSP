#include "cydsp/midi/Ump.h"
#include <algorithm>
#include <stdexcept>
namespace cydsp {
unsigned umpWordCount(unsigned t) noexcept {
    constexpr unsigned counts[]{1, 1, 1, 2, 2, 4, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4};
    return t < 16 ? counts[t] : 0;
}
namespace {
std::uint8_t byte(const UmpPacket& p, unsigned n) {
    return static_cast<std::uint8_t>(p.words[n / 4] >> (24 - 8 * (n % 4)));
}
void setByte(UmpPacket& p, unsigned n, std::uint8_t b) {
    p.words[n / 4] |= std::uint32_t(b) << (24 - 8 * (n % 4));
}
bool address(unsigned g, unsigned c) {
    return g < 16 && c >= 1 && c <= 16;
}
UmpPacket voice(unsigned g, unsigned c, unsigned status, unsigned a, unsigned b,
                std::uint32_t value) {
    if (!address(g, c))
        return {};
    return {
        {{0x40000000u | (g << 24) | (status << 20) | ((c - 1) << 16) | (a << 8) | b, value, 0, 0}},
        2};
}
} // namespace
bool UmpPacket::valid() const noexcept {
    if (!count || count != umpWordCount(type()))
        return false;
    if (type() == 2) {
        const auto s = byte(*this, 1);
        if (s < 0x80 || s >= 0xf0 || byte(*this, 2) > 127 || byte(*this, 3) > 127)
            return false;
        if ((s & 0xe0) == 0xc0 && byte(*this, 3))
            return false;
    }
    if (type() == 1) {
        const std::array<std::uint8_t, 3> bytes{byte(*this, 1), byte(*this, 2), byte(*this, 3)};
        const auto status = bytes[0];
        unsigned n = 0;
        if (status == 0xf1 || status == 0xf3)
            n = 2;
        else if (status == 0xf2)
            n = 3;
        else if (status == 0xf6 || status == 0xf8 || status == 0xfa || status == 0xfb ||
                 status == 0xfc || status == 0xfe || status == 0xff)
            n = 1;
        if (!n || !MidiMessageView({bytes.data(), n}).valid())
            return false;
        for (unsigned i = n; i < 3; ++i)
            if (bytes[i])
                return false;
    }
    if (type() == 3) {
        const auto state = (words[0] >> 20) & 15, n = (words[0] >> 16) & 15;
        if (state > 3 || n > 6)
            return false;
        for (unsigned i = 0; i < 6; ++i)
            if (byte(*this, 2 + i) > (i < n ? 127u : 0u))
                return false;
    }
    if (type() == 5 && status() < 4) {
        const auto n = (words[0] >> 16) & 15;
        if (n < 1 || n > 14)
            return false;
        for (unsigned i = n + 2; i < 16; ++i)
            if (byte(*this, i))
                return false;
    }
    if (type() == 4) {
        const unsigned s = status(), a = byte(*this, 2), b = byte(*this, 3);
        if (s == 7)
            return false;
        if (s <= 1) {
            if (a > 127)
                return false;
        } else if (s <= 5) {
            if (a > 127 || b > 127)
                return false;
        } else if (s == 6 || s == 10 || s == 11 || s == 15) {
            if (a > 127)
                return false;
            if (s != 15 && b)
                return false;
            if (s == 15 && (b > 3 || words[1]))
                return false;
        } else if (s == 8 || s == 9) {
            if (a > 127)
                return false;
        } else if (s == 12) {
            if (a || b > 1 || byte(*this, 4) > 127 || byte(*this, 5) || byte(*this, 6) > 127 ||
                byte(*this, 7) > 127)
                return false;
        } else if ((s == 13 || s == 14) && (a || b))
            return false;
    }
    return true;
}
Status UmpPacket::toBytes(std::span<std::uint8_t> out, std::size_t& n) const noexcept {
    n = count * 4;
    if (!valid())
        return Status::invalid_argument;
    if (out.size() < n)
        return Status::block_too_large;
    for (unsigned i = 0; i < n; ++i)
        out[i] = byte(*this, i);
    return Status::ok;
}
Status UmpStreamParser::feed(std::span<const std::uint32_t> words, UmpCallback callback,
                             void* context) noexcept {
    if (!callback || byteCount_)
        return Status::invalid_argument;
    auto result = Status::ok;
    for (auto word : words) {
        if (!have_) {
            packet_ = {};
            packet_.count = static_cast<std::uint8_t>(umpWordCount(word >> 28));
        }
        packet_.words[have_++] = word;
        if (have_ == packet_.count) {
            if (packet_.valid())
                callback(context, packet_);
            else
                result = Status::invalid_argument;
            have_ = 0;
        }
    }
    return result;
}
Status UmpStreamParser::feedBytes(std::span<const std::uint8_t> bytes, UmpCallback callback,
                                  void* context) noexcept {
    if (!callback)
        return Status::invalid_argument;
    auto result = Status::ok;
    for (auto b : bytes) {
        byteWord_ = (byteWord_ << 8) | b;
        if (++byteCount_ == 4) {
            byteCount_ = 0;
            const auto word = byteWord_;
            byteWord_ = 0;
            const auto status = feed({&word, 1}, callback, context);
            if (status != Status::ok)
                result = status;
        }
    }
    return result;
}
Status UmpBuffer::prepare(std::size_t n) noexcept {
    if (!n)
        return Status::invalid_argument;
    try {
        std::vector<UmpEvent> next(n);
        events_.swap(next);
        reset();
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
}
Status UmpBuffer::add(std::uint32_t frame, const UmpPacket& p) noexcept {
    if (events_.empty())
        return Status::not_prepared;
    if (!p.valid())
        return Status::invalid_argument;
    if (count_ == events_.size())
        return Status::update_queue_full;
    auto i = count_;
    while (i && events_[i - 1].frame > frame) {
        events_[i] = events_[i - 1];
        --i;
    }
    events_[i] = {frame, p};
    ++count_;
    return Status::ok;
}
UmpPacket umpMidi1(unsigned group, MidiMessageView message) noexcept {
    if (group > 15 || !message.valid() || message.bytes()[0] == 0xf0)
        return {};
    UmpPacket p;
    p.count = 1;
    const auto data = message.bytes();
    p.words[0] = ((data[0] < 0xf0 ? 2u : 1u) << 28) | (group << 24);
    for (unsigned i = 0; i < data.size(); ++i)
        setByte(p, i + 1, data[i]);
    return p;
}
UmpPacket umpNote(bool on, unsigned g, unsigned c, unsigned note, std::uint16_t velocity,
                  std::uint8_t type, std::uint16_t attribute) noexcept {
    return note < 128
               ? voice(g, c, on ? 9 : 8, note, type, (std::uint32_t(velocity) << 16) | attribute)
               : UmpPacket{};
}
UmpPacket umpController(unsigned g, unsigned c, unsigned cc, std::uint32_t v) noexcept {
    return cc < 128 ? voice(g, c, 11, cc, 0, v) : UmpPacket{};
}
UmpPacket umpPitch(unsigned g, unsigned c, std::uint32_t v, int note) noexcept {
    return note >= -1 && note < 128
               ? voice(g, c, note < 0 ? 14 : 6, note < 0 ? 0u : static_cast<unsigned>(note), 0, v)
               : UmpPacket{};
}
UmpPacket umpPressure(unsigned g, unsigned c, std::uint32_t v, int note) noexcept {
    return note >= -1 && note < 128
               ? voice(g, c, note < 0 ? 13 : 10, note < 0 ? 0u : static_cast<unsigned>(note), 0, v)
               : UmpPacket{};
}
UmpPacket umpPerNoteController(bool reg, unsigned g, unsigned c, unsigned note, unsigned index,
                               std::uint32_t value) noexcept {
    return note < 128 && index < 256 ? voice(g, c, reg ? 0 : 1, note, index, value) : UmpPacket{};
}
UmpPacket umpProgram(unsigned g, unsigned c, unsigned program, int msb, int lsb) noexcept {
    if (program > 127 || msb < -1 || msb > 127 || lsb < -1 || lsb > 127 || (msb < 0) != (lsb < 0))
        return {};
    return voice(
        g, c, 12, 0, msb < 0 ? 0 : 1,
        (program << 24) |
            (msb < 0 ? 0u : (static_cast<unsigned>(msb) << 8) | static_cast<unsigned>(lsb)));
}
std::uint16_t midiScale7To16(unsigned v) noexcept {
    v = std::min(v, 127u);
    const auto base = v << 9, tail = v & 63;
    return static_cast<std::uint16_t>(base | (v > 64 ? (tail << 3) | (tail >> 3) : 0));
}
std::uint32_t midiScale7To32(unsigned v) noexcept {
    v = std::min(v, 127u);
    const auto base = v << 25, tail = v & 63;
    return base |
           (v > 64 ? (tail << 19) | (tail << 13) | (tail << 7) | (tail << 1) | (tail >> 5) : 0);
}
Status midi1ToMidi2(const UmpPacket& p, UmpPacket& out) noexcept {
    if (!p.valid() || p.type() != 2)
        return Status::invalid_argument;
    const auto d1 = byte(p, 2), d2 = byte(p, 3);
    const auto g = p.group(), c = p.channel();
    UmpPacket result;
    switch (p.status()) {
    case 8:
    case 9:
        result = umpNote(p.status() == 9 && d2 != 0, g, c, d1, midiScale7To16(d2));
        break;
    case 10:
        result = umpPressure(g, c, midiScale7To32(d2), d1);
        break;
    case 11:
        result = umpController(g, c, d1, midiScale7To32(d2));
        break;
    case 12:
        result = umpProgram(g, c, d1);
        break;
    case 13:
        result = umpPressure(g, c, midiScale7To32(d1));
        break;
    case 14: {
        const auto v = unsigned(d1) | (unsigned(d2) << 7);
        const auto tail = v & 8191u;
        const auto scaled = (v << 18) | (v > 8192 ? (tail << 5) | (tail >> 8) : 0u);
        result = umpPitch(g, c, scaled);
        break;
    }
    default:
        return Status::unsupported_format;
    }
    out = result;
    return Status::ok;
}
Status midi2ToMidi1(const UmpPacket& p, ShortMidiMessage& out) noexcept {
    if (!p.valid() || p.type() != 4)
        return Status::invalid_argument;
    const unsigned c = p.channel(), note = byte(p, 2);
    ShortMidiMessage result;
    switch (p.status()) {
    case 8:
    case 9:
        result = p.status() == 9 ? midiNoteOn(c, note, std::max(1u, p.words[1] >> 25))
                                 : midiNoteOff(c, note, p.words[1] >> 25);
        break;
    case 10:
        result = {{{static_cast<std::uint8_t>(0xa0 + c - 1), static_cast<std::uint8_t>(note),
                    static_cast<std::uint8_t>(p.words[1] >> 25)}},
                  3};
        break;
    case 11:
        result = midiController(c, note, p.words[1] >> 25);
        break;
    case 12:
        if (byte(p, 3) & 1)
            return Status::unsupported_format;
        result = {{{static_cast<std::uint8_t>(0xc0 + c - 1), byte(p, 4), 0}}, 2};
        break;
    case 13:
        result = {{{static_cast<std::uint8_t>(0xd0 + c - 1),
                    static_cast<std::uint8_t>(p.words[1] >> 25), 0}},
                  2};
        break;
    case 14:
        result = midiPitchBend(c, p.words[1] >> 18);
        break;
    default:
        return Status::unsupported_format;
    }
    out = result;
    return Status::ok;
}
UmpPacket umpSysEx7(unsigned group, unsigned state, std::span<const std::uint8_t> data) noexcept {
    if (group > 15 || state > 3 || data.size() > 6 ||
        std::any_of(data.begin(), data.end(), [](auto b) { return b > 127; }))
        return {};
    UmpPacket p{
        {{0x30000000u | (group << 24) | (state << 20) | (static_cast<unsigned>(data.size()) << 16),
          0, 0, 0}},
        2};
    for (unsigned i = 0; i < data.size(); ++i)
        setByte(p, 2 + i, data[i]);
    return p;
}
UmpPacket umpSysEx8(unsigned group, unsigned state, std::uint8_t id,
                    std::span<const std::uint8_t> data) noexcept {
    if (group > 15 || state > 3 || data.size() > 13)
        return {};
    UmpPacket p{{{0x50000000u | (group << 24) | (state << 20) |
                      (static_cast<unsigned>(data.size() + 1) << 16) | (unsigned(id) << 8),
                  0, 0, 0}},
                4};
    for (unsigned i = 0; i < data.size(); ++i)
        setByte(p, 3 + i, data[i]);
    return p;
}
Status UmpSysExAssembler::prepare(std::size_t n) noexcept {
    if (!n)
        return Status::invalid_argument;
    try {
        std::vector<std::uint8_t> b(n);
        storage_.swap(b);
        reset();
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
}
Status UmpSysExAssembler::append(const UmpPacket& p) noexcept {
    if (storage_.empty())
        return Status::not_prepared;
    if (!p.valid())
        return Status::invalid_argument;
    if (p.type() != 3 && p.type() != 5)
        return Status::unsupported_format;
    const auto state = p.status();
    if (state > 3)
        return Status::unsupported_format;
    const auto stream = p.type() == 5 ? byte(p, 2) : 0u;
    if (state < 2) {
        reset();
        group_ = p.group();
        type_ = p.type();
        stream_ = stream;
        active_ = true;
    } else if (!active_ || group_ != p.group() || type_ != p.type() || stream_ != stream)
        return Status::invalid_argument;
    const auto n = ((p.words[0] >> 16) & 15) - (p.type() == 5 ? 1u : 0u),
               offset = p.type() == 5 ? 3u : 2u;
    if (n > storage_.size() - used_) {
        reset();
        return Status::update_queue_full;
    }
    for (unsigned i = 0; i < n; ++i)
        storage_[used_++] = byte(p, offset + i);
    if (state == 0 || state == 3) {
        active_ = false;
        complete_ = true;
    }
    return Status::ok;
}
} // namespace cydsp
