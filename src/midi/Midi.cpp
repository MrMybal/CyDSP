#include "cydsp/midi/Midi.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace cydsp {
namespace {
unsigned length(std::uint8_t status) {
    if (status < 0x80)
        return 0;
    if (status < 0xf0)
        return (status & 0xe0) == 0xc0 ? 2 : 3;
    switch (status) {
    case 0xf1:
    case 0xf3:
        return 2;
    case 0xf2:
        return 3;
    case 0xf6:
    case 0xf7:
    case 0xf8:
    case 0xfa:
    case 0xfb:
    case 0xfc:
    case 0xfe:
    case 0xff:
        return 1;
    default:
        return 0;
    }
}
ShortMidiMessage shortMessage(unsigned status, unsigned channel, unsigned first, unsigned second,
                              unsigned n = 3) {
    if (channel < 1 || channel > 16 || first > 127 || second > 127)
        return {};
    return {{{static_cast<std::uint8_t>(status + channel - 1), static_cast<std::uint8_t>(first),
              static_cast<std::uint8_t>(second)}},
            n};
}
bool divisionValid(std::uint16_t d) {
    if (!(d & 0x8000))
        return d != 0;
    const auto fps = 256 - (d >> 8);
    return (fps == 24 || fps == 25 || fps == 29 || fps == 30) && (d & 255) != 0;
}
} // namespace
bool MidiMessageView::valid() const noexcept {
    if (data_.empty())
        return false;
    if (data_[0] == 0xf0)
        return data_.size() >= 2 && data_.back() == 0xf7 &&
               std::all_of(data_.begin() + 1, data_.end() - 1, [](auto b) { return b < 0x80; });
    const auto n = length(data_[0]);
    return n && data_.size() == n &&
           std::all_of(data_.begin() + 1, data_.end(), [](auto b) { return b < 0x80; });
}
unsigned MidiMessageView::channel() const noexcept {
    return valid() && data_[0] < 0xf0 ? (data_[0] & 15) + 1 : 0;
}
bool MidiMessageView::noteOn() const noexcept {
    return valid() && (data_[0] & 0xf0) == 0x90 && data_[2] != 0;
}
bool MidiMessageView::noteOff() const noexcept {
    return valid() && ((data_[0] & 0xf0) == 0x80 || ((data_[0] & 0xf0) == 0x90 && data_[2] == 0));
}
unsigned MidiMessageView::note() const noexcept {
    return noteOn() || noteOff() ? data_[1] : 0;
}
ShortMidiMessage midiNoteOn(unsigned ch, unsigned note, unsigned velocity) noexcept {
    return shortMessage(0x90, ch, note, velocity);
}
ShortMidiMessage midiNoteOff(unsigned ch, unsigned note, unsigned velocity) noexcept {
    return shortMessage(0x80, ch, note, velocity);
}
ShortMidiMessage midiController(unsigned ch, unsigned cc, unsigned value) noexcept {
    return shortMessage(0xb0, ch, cc, value);
}
ShortMidiMessage midiPitchBend(unsigned ch, unsigned value) noexcept {
    return value <= 16383 ? shortMessage(0xe0, ch, value & 127, value >> 7) : ShortMidiMessage{};
}
Status MidiBuffer::prepare(std::size_t events, std::size_t bytes) noexcept {
    if (!events || !bytes)
        return Status::invalid_argument;
    try {
        std::vector<Entry> e(events);
        std::vector<std::uint8_t> b(bytes);
        entries_.swap(e);
        bytes_.swap(b);
        reset();
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
}
Status MidiBuffer::add(std::uint32_t frame, std::span<const std::uint8_t> data) noexcept {
    if (entries_.empty())
        return Status::not_prepared;
    if (!MidiMessageView(data).valid())
        return Status::invalid_argument;
    if (count_ == entries_.size() || data.size() > bytes_.size() - used_)
        return Status::update_queue_full;
    // Capture before moving metadata; supplied bytes may borrow another event from this buffer.
    std::copy(data.begin(), data.end(), bytes_.begin() + used_);
    auto i = count_;
    while (i && entries_[i - 1].frame > frame) {
        entries_[i] = entries_[i - 1];
        --i;
    }
    entries_[i] = {frame, used_, data.size()};
    used_ += data.size();
    ++count_;
    return Status::ok;
}
MidiBufferEvent MidiBuffer::event(std::size_t index) const noexcept {
    if (index >= count_)
        return {};
    const auto& e = entries_[index];
    return {e.frame, {bytes_.data() + e.offset, e.size}};
}
Status MidiStreamParser::prepare(std::size_t max) noexcept {
    if (max < 2 || max > 1048576)
        return Status::invalid_argument;
    try {
        std::vector<std::uint8_t> b(max);
        sysex_.swap(b);
        reset();
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    }
}
void MidiStreamParser::reset() noexcept {
    used_ = need_ = have_ = 0;
    running_ = 0;
    inSysEx_ = overflow_ = false;
}
Status MidiStreamParser::feed(std::span<const std::uint8_t> data, double time,
                              MidiStreamCallback callback, void* context) noexcept {
    if (sysex_.empty())
        return Status::not_prepared;
    if (!callback || !isFinite(time) || time < 0)
        return Status::invalid_argument;
    auto result = Status::ok;
    for (auto b : data) {
        if (b >= 0xf8) {
            if (length(b))
                callback(context, {&b, 1}, time);
            else
                result = Status::invalid_argument;
            continue;
        }
        if (inSysEx_) {
            if (b < 0x80 || b == 0xf7) {
                if (used_ < sysex_.size())
                    sysex_[used_++] = b;
                else {
                    overflow_ = true;
                    result = Status::block_too_large;
                }
                if (b == 0xf7) {
                    if (!overflow_)
                        callback(context, {sysex_.data(), used_}, time);
                    inSysEx_ = false;
                    used_ = 0;
                }
                continue;
            }
            inSysEx_ = false;
            used_ = 0;
            result = Status::invalid_argument;
        }
        if (b >= 0x80) {
            have_ = need_ = 0;
            if (b == 0xf0) {
                running_ = 0;
                inSysEx_ = true;
                overflow_ = false;
                sysex_[0] = b;
                used_ = 1;
                continue;
            }
            running_ = b < 0xf0 ? b : 0;
            need_ = length(b);
            short_[0] = b;
            have_ = 1;
            if (!need_) {
                have_ = 0;
                result = Status::invalid_argument;
            } else if (need_ == 1) {
                callback(context, {short_.data(), 1}, time);
                have_ = need_ = 0;
            }
        } else {
            if (!have_ && running_) {
                short_[0] = running_;
                have_ = 1;
                need_ = length(running_);
            }
            if (!have_) {
                result = Status::invalid_argument;
                continue;
            }
            short_[have_++] = b;
            if (have_ == need_) {
                callback(context, {short_.data(), need_}, time);
                have_ = need_ = 0;
            }
        }
    }
    return result;
}
void MidiKeyboardState::reset() noexcept {
    for (auto& ch : down_)
        ch.fill(false);
    for (auto& ch : held_)
        ch.fill(false);
    sustain_.fill(false);
}
bool MidiKeyboardState::active(unsigned ch, unsigned note) const noexcept {
    return ch >= 1 && ch <= 16 && note < 128 && (down_[ch - 1][note] || held_[ch - 1][note]);
}
Status MidiKeyboardState::apply(MidiMessageView m) noexcept {
    if (!m.valid())
        return Status::invalid_argument;
    const auto channel = m.channel();
    if (!channel)
        return Status::ok;
    const auto ch = channel - 1;
    const auto d = m.bytes();
    if (m.noteOn()) {
        down_[ch][m.note()] = true;
        held_[ch][m.note()] = false;
    } else if (m.noteOff()) {
        held_[ch][m.note()] = held_[ch][m.note()] || (down_[ch][m.note()] && sustain_[ch]);
        down_[ch][m.note()] = false;
    } else if ((d[0] & 0xf0) == 0xb0) {
        if (d[1] == 64) {
            sustain_[ch] = d[2] >= 64;
            if (!sustain_[ch])
                held_[ch].fill(false);
        } else if (d[1] == 120) {
            down_[ch].fill(false);
            held_[ch].fill(false);
        } else if (d[1] == 123) {
            for (unsigned n = 0; n < 128; ++n) {
                held_[ch][n] = held_[ch][n] || (down_[ch][n] && sustain_[ch]);
                down_[ch][n] = false;
            }
        } else if (d[1] == 121) {
            sustain_[ch] = false;
            held_[ch].fill(false);
        }
    }
    return Status::ok;
}
namespace {
struct Cursor {
    std::span<const std::uint8_t> data;
    std::size_t at = 0;
    bool byte(std::uint8_t& value) {
        if (at == data.size())
            return false;
        value = data[at++];
        return true;
    }
    bool be(unsigned bytes, std::uint32_t& value) {
        if (bytes > data.size() - at)
            return false;
        value = 0;
        while (bytes--)
            value = (value << 8) | data[at++];
        return true;
    }
    bool vlq(std::uint32_t& value) {
        value = 0;
        for (unsigned n = 0; n < 4; ++n) {
            std::uint8_t b;
            if (!byte(b))
                return false;
            value = (value << 7) | (b & 127);
            if (!(b & 128))
                return true;
        }
        return false;
    }
    bool take(std::size_t n, std::span<const std::uint8_t>& view) {
        if (n > data.size() - at)
            return false;
        view = data.subspan(at, n);
        at += n;
        return true;
    }
};
void be(std::vector<std::uint8_t>& out, std::uint32_t v, unsigned bytes) {
    while (bytes--)
        out.push_back(static_cast<std::uint8_t>(v >> (bytes * 8)));
}
void vlq(std::vector<std::uint8_t>& out, std::uint32_t v) {
    std::array<std::uint8_t, 4> b{};
    unsigned n = 1;
    b[3] = static_cast<std::uint8_t>(v & 127);
    while ((v >>= 7) != 0)
        b[3 - n++] = static_cast<std::uint8_t>((v & 127) | 128);
    out.insert(out.end(), b.end() - n, b.end());
}
} // namespace
Status MidiFile::parse(std::span<const std::uint8_t> bytes, std::size_t max) noexcept {
    if (!max)
        return Status::invalid_argument;
    try {
        Cursor c{bytes};
        std::uint32_t tag, n, f, t, d;
        if (!c.be(4, tag) || tag != 0x4d546864 || !c.be(4, n) || n < 6 || !c.be(2, f) ||
            !c.be(2, t) || !c.be(2, d) || f > 2 || !t || (f == 0 && t != 1) ||
            !divisionValid(static_cast<std::uint16_t>(d)))
            return Status::invalid_argument;
        std::span<const std::uint8_t> skip;
        if (!c.take(n - 6, skip))
            return Status::invalid_argument;
        MidiFile next;
        next.format = static_cast<std::uint16_t>(f);
        next.division = static_cast<std::uint16_t>(d);
        next.tracks.resize(t);
        std::size_t total = 0;
        for (auto& track : next.tracks) {
            if (!c.be(4, tag) || tag != 0x4d54726b || !c.be(4, n) || !c.take(n, skip))
                return Status::invalid_argument;
            Cursor r{skip};
            std::uint64_t tick = 0;
            std::uint8_t running = 0;
            bool end = false;
            while (r.at < r.data.size()) {
                std::uint32_t delta;
                if (!r.vlq(delta) || delta > std::numeric_limits<std::uint64_t>::max() - tick ||
                    ++total > max)
                    return Status::invalid_argument;
                tick += delta;
                std::uint8_t status;
                if (!r.byte(status))
                    return Status::invalid_argument;
                MidiFileEvent e;
                e.tick = tick;
                if (status == 0xff || status == 0xf0 || status == 0xf7) {
                    running = 0;
                    e.kind = status == 0xff ? MidiFileEventKind::meta : MidiFileEventKind::sysex;
                    e.type = status;
                    if (status == 0xff && !r.byte(e.type))
                        return Status::invalid_argument;
                    std::uint32_t size;
                    if (!r.vlq(size) || !r.take(size, skip))
                        return Status::invalid_argument;
                    e.data.assign(skip.begin(), skip.end());
                    if (e.kind == MidiFileEventKind::meta && e.type == 0x2f) {
                        if (size || r.at != r.data.size())
                            return Status::invalid_argument;
                        end = true;
                    }
                } else {
                    if (status < 0x80) {
                        if (!running)
                            return Status::invalid_argument;
                        --r.at;
                        status = running;
                    }
                    if (status >= 0xf0)
                        return Status::invalid_argument;
                    running = status;
                    e.data.push_back(status);
                    const auto size = length(status) - 1;
                    if (!r.take(size, skip))
                        return Status::invalid_argument;
                    e.data.insert(e.data.end(), skip.begin(), skip.end());
                    if (!MidiMessageView(e.data).valid())
                        return Status::invalid_argument;
                }
                track.push_back(std::move(e));
            }
            if (!end)
                return Status::invalid_argument;
        }
        if (c.at != c.data.size())
            return Status::invalid_argument;
        *this = std::move(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
}
Status MidiFile::serialize(std::vector<std::uint8_t>& result) const noexcept {
    if (format > 2 || tracks.empty() || tracks.size() > 65535 ||
        (format == 0 && tracks.size() != 1) || !divisionValid(division))
        return Status::invalid_argument;
    try {
        std::vector<std::uint8_t> out;
        be(out, 0x4d546864, 4);
        be(out, 6, 4);
        be(out, format, 2);
        be(out, static_cast<std::uint32_t>(tracks.size()), 2);
        be(out, division, 2);
        for (const auto& track : tracks) {
            std::vector<std::uint8_t> data;
            std::uint64_t last = 0;
            bool end = false;
            for (const auto& e : track) {
                if (end || e.tick < last || e.tick - last > 0xfffffff || e.data.size() > 0xfffffff)
                    return Status::invalid_argument;
                vlq(data, static_cast<std::uint32_t>(e.tick - last));
                last = e.tick;
                if (e.kind == MidiFileEventKind::message) {
                    if (!MidiMessageView(e.data).valid() || e.data[0] >= 0xf0)
                        return Status::invalid_argument;
                    data.insert(data.end(), e.data.begin(), e.data.end());
                } else {
                    if (e.kind == MidiFileEventKind::meta) {
                        if (e.type > 127)
                            return Status::invalid_argument;
                        data.push_back(0xff);
                        data.push_back(e.type);
                        if (e.type == 0x2f) {
                            if (!e.data.empty())
                                return Status::invalid_argument;
                            end = true;
                        }
                    } else if (e.kind == MidiFileEventKind::sysex &&
                               (e.type == 0xf0 || e.type == 0xf7))
                        data.push_back(e.type);
                    else
                        return Status::invalid_argument;
                    vlq(data, static_cast<std::uint32_t>(e.data.size()));
                    data.insert(data.end(), e.data.begin(), e.data.end());
                }
            }
            if (!end) {
                data.push_back(0);
                data.push_back(0xff);
                data.push_back(0x2f);
                data.push_back(0);
            }
            if (data.size() > 0xffffffffull)
                return Status::invalid_argument;
            be(out, 0x4d54726b, 4);
            be(out, static_cast<std::uint32_t>(data.size()), 4);
            out.insert(out.end(), data.begin(), data.end());
        }
        result.swap(out);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
}
Status MidiFile::tickToSeconds(std::uint64_t tick, double& seconds,
                               std::size_t track) const noexcept {
    if (format > 2 || tracks.empty() || track >= tracks.size() || !divisionValid(division))
        return Status::invalid_argument;
    if (division & 0x8000) {
        const unsigned fps = 256 - (division >> 8);
        seconds =
            static_cast<double>(tick) / ((fps == 29 ? 30000.0 / 1001 : fps) * (division & 255));
        return Status::ok;
    }
    // No temporary tempo vector: walk all tracks at each tempo boundary, including equal-tick
    // order.
    std::uint64_t previous = 0;
    double tempo = 500000, elapsed = 0;
    const auto first = format == 2 ? track : 0, last = format == 2 ? track + 1 : tracks.size();
    std::size_t selectedTrack = 0, selectedEvent = 0;
    bool haveSelected = false;
    for (;;) {
        const MidiFileEvent* next = nullptr;
        std::size_t nt = 0, ne = 0;
        for (std::size_t t = first; t < last; ++t)
            for (std::size_t i = 0; i < tracks[t].size(); ++i) {
                const auto& e = tracks[t][i];
                if (e.kind != MidiFileEventKind::meta || e.type != 0x51 || e.tick > tick ||
                    e.tick < previous)
                    continue;
                if (haveSelected && e.tick == previous &&
                    (t < selectedTrack || (t == selectedTrack && i <= selectedEvent)))
                    continue;
                if (!next || e.tick < next->tick) {
                    next = &e;
                    nt = t;
                    ne = i;
                }
            }
        if (!next)
            break;
        if (next->data.size() != 3)
            return Status::invalid_argument;
        const auto value =
            (unsigned(next->data[0]) << 16) | (unsigned(next->data[1]) << 8) | next->data[2];
        if (!value)
            return Status::invalid_argument;
        elapsed += static_cast<double>(next->tick - previous) * tempo / (1e6 * division);
        previous = next->tick;
        tempo = value;
        selectedTrack = nt;
        selectedEvent = ne;
        haveSelected = true;
    }
    seconds = elapsed + static_cast<double>(tick - previous) * tempo / (1e6 * division);
    return Status::ok;
}
} // namespace cydsp
