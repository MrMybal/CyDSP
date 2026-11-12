#include "cydsp/midi/Mpe.h"
#include <algorithm>
namespace cydsp {
void MidiParameterParser::reset() noexcept {
    channels_.fill(State{});
}
bool MidiParameterParser::process(MidiMessageView m, MidiParameter& result) noexcept {
    if (!m.valid() || !m.channel() || (m.bytes()[0] & 0xf0) != 0xb0)
        return false;
    auto& s = channels_[m.channel() - 1];
    const auto cc = m.bytes()[1], value = m.bytes()[2];
    if (cc == 101 || cc == 100 || cc == 99 || cc == 98) {
        const bool reg = cc >= 100;
        if (s.registered != reg) {
            s = State{};
            s.registered = reg;
        }
        if (cc == 101 || cc == 99)
            s.msb = value;
        else
            s.lsb = value;
        s.dataLsb = 0;
        return false;
    }
    if (s.msb == 127 && s.lsb == 127)
        return false;
    if (cc == 6)
        s.dataMsb = value;
    else if (cc == 38)
        s.dataLsb = value;
    else if (cc == 96 || cc == 97) {
        auto v = (s.dataMsb << 7) | s.dataLsb;
        if (cc == 96)
            v = std::min(v + 1, 16383u);
        else if (v)
            --v;
        s.dataMsb = v >> 7;
        s.dataLsb = v & 127;
    } else
        return false;
    result = {m.channel(), (s.msb << 7) | s.lsb, (s.dataMsb << 7) | s.dataLsb, s.registered};
    return true;
}
MpeInstrument::MpeInstrument() noexcept {
    reset();
}
void MpeInstrument::reset() noexcept {
    for (auto& ch : notes_)
        ch.fill(MpeNote{});
    for (auto& ch : sostenuto_)
        ch.fill(false);
    channels_.fill(Channel{});
    for (unsigned ch = 0; ch < 16; ++ch)
        channels_[ch].range = master(ch + 1) < 0 ? 2 : 48;
    parameters_.reset();
    serial_ = 0;
}
Status MpeInstrument::setZones(unsigned lower, unsigned upper) noexcept {
    if (lower > 15 || upper > 15 || (lower && upper && lower + upper > 14))
        return Status::invalid_argument;
    for (const auto& ch : notes_)
        for (const auto& n : ch)
            if (n.active)
                return Status::transition_in_progress;
    lower_ = lower;
    upper_ = upper;
    reset();
    return Status::ok;
}
int MpeInstrument::master(unsigned ch) const noexcept {
    if (lower_ && ch >= 2 && ch <= lower_ + 1)
        return 0;
    if (upper_ && ch >= 16 - upper_ && ch <= 15)
        return 15;
    return -1;
}
const MpeNote* MpeInstrument::note(unsigned ch, unsigned key) const noexcept {
    return ch >= 1 && ch <= 16 && key < 128 && notes_[ch - 1][key].active ? &notes_[ch - 1][key]
                                                                          : nullptr;
}
void MpeInstrument::expression(unsigned channel, std::uint32_t frame, MpeCallback callback,
                               void* context) noexcept {
    for (unsigned ch = 0; ch < 16; ++ch) {
        const auto masterIndex = master(ch + 1);
        if (ch != channel && masterIndex != static_cast<int>(channel))
            continue;
        for (auto& n : notes_[ch])
            if (n.active) {
                n.pitchSemitones = channels_[ch].bend * channels_[ch].range;
                n.pressure = channels_[ch].pressure;
                n.timbre = channels_[ch].timbre;
                n.masterPressure = 0;
                if (masterIndex >= 0) {
                    n.pitchSemitones += channels_[static_cast<unsigned>(masterIndex)].bend *
                                        channels_[static_cast<unsigned>(masterIndex)].range;
                    n.masterPressure = channels_[static_cast<unsigned>(masterIndex)].pressure;
                }
                callback(context, MpeChange::expression, n, frame);
            }
    }
}
void MpeInstrument::release(unsigned channel, bool force, std::uint32_t frame, MpeCallback callback,
                            void* context) noexcept {
    for (unsigned ch = 0; ch < 16; ++ch) {
        const int m = master(ch + 1);
        if (ch != channel && m != static_cast<int>(channel))
            continue;
        const bool pedal =
            channels_[ch].sustain || (m >= 0 && channels_[static_cast<unsigned>(m)].sustain);
        for (unsigned key = 0; key < 128; ++key) {
            auto& n = notes_[ch][key];
            const bool sostenuto = sostenuto_[ch][key] != 0;
            if (n.active && (force || (!n.keyDown && !pedal && !sostenuto))) {
                n.active = false;
                callback(context, force ? MpeChange::allSoundOff : MpeChange::noteOff, n, frame);
            }
        }
    }
}
Status MpeInstrument::process(MidiMessageView message, std::uint32_t frame, MpeCallback callback,
                              void* context) noexcept {
    if (!callback || !message.valid())
        return Status::invalid_argument;
    const auto c = message.channel();
    if (!c)
        return Status::ok;
    const auto ch = c - 1;
    auto& channel = channels_[ch];
    const auto data = message.bytes();
    MidiParameter parameter;
    if (parameters_.process(message, parameter) && parameter.registered) {
        if (parameter.number == 0) {
            channel.range = std::min(96.0, static_cast<double>(parameter.value >> 7) +
                                               static_cast<double>(parameter.value & 127) / 100);
            expression(ch, frame, callback, context);
        } else if (parameter.number == 6 && (c == 1 || c == 16)) {
            const auto members = std::min(15u, parameter.value >> 7);
            for (unsigned i = 0; i < 16; ++i)
                release(i, true, frame, callback, context);
            if (c == 1) {
                lower_ = members;
                if (lower_ && upper_ && lower_ + upper_ > 14)
                    upper_ = lower_ >= 14 ? 0 : 14 - lower_;
            } else {
                upper_ = members;
                if (lower_ && upper_ && lower_ + upper_ > 14)
                    lower_ = upper_ >= 14 ? 0 : 14 - upper_;
            }
        }
    }
    if (parameter.registered && parameter.channel && parameter.number == 6 && (c == 1 || c == 16))
        reset();
    const auto status = data[0] & 0xf0;
    if (message.noteOn()) {
        auto& n = notes_[ch][message.note()];
        if (n.active) {
            n.active = false;
            callback(context, MpeChange::allSoundOff, n, frame);
        }
        sostenuto_[ch][message.note()] = 0;
        n = {++serial_,
             c,
             message.note(),
             static_cast<double>(data[2]) / 127,
             0,
             channel.bend * channel.range,
             channel.pressure,
             channel.timbre,
             0,
             true,
             true};
        const auto m = master(c);
        if (m >= 0) {
            n.pitchSemitones += channels_[static_cast<unsigned>(m)].bend *
                                channels_[static_cast<unsigned>(m)].range;
            n.masterPressure = channels_[static_cast<unsigned>(m)].pressure;
        }
        callback(context, MpeChange::noteOn, n, frame);
    } else if (message.noteOff()) {
        auto& n = notes_[ch][message.note()];
        if (n.active) {
            n.keyDown = false;
            n.releaseVelocity = static_cast<double>(data[2]) / 127;
            release(ch, false, frame, callback, context);
        }
    } else if (status == 0xe0) {
        const auto value = unsigned(data[1]) | (unsigned(data[2]) << 7);
        channel.bend = value < 8192 ? (static_cast<double>(value) - 8192) / 8192
                                    : (static_cast<double>(value) - 8192) / 8191;
        expression(ch, frame, callback, context);
    } else if (status == 0xd0) {
        channel.pressure = static_cast<double>(data[1]) / 127;
        expression(ch, frame, callback, context);
    } else if (status == 0xa0) {
        auto& n = notes_[ch][data[1]];
        if (n.active) {
            n.pressure = static_cast<double>(data[2]) / 127;
            callback(context, MpeChange::expression, n, frame);
        }
    } else if (status == 0xb0) {
        const auto cc = data[1], v = data[2];
        if (cc == 74) {
            channel.timbre = static_cast<double>(v) / 127;
            expression(ch, frame, callback, context);
        } else if (cc == 64) {
            channel.sustain = v >= 64;
            release(ch, false, frame, callback, context);
        } else if (cc == 66) {
            const auto bit = static_cast<std::uint16_t>(1u << ch);
            if (v >= 64 && !channel.sostenuto)
                for (unsigned i = 0; i < 16; ++i)
                    if (i == ch || master(i + 1) == static_cast<int>(ch))
                        for (unsigned key = 0; key < 128; ++key)
                            if (notes_[i][key].keyDown)
                                sostenuto_[i][key] |= bit;
            if (v < 64)
                for (auto& row : sostenuto_)
                    for (auto& held : row)
                        held &= static_cast<std::uint16_t>(~bit);
            channel.sostenuto = v >= 64;
            release(ch, false, frame, callback, context);
        } else if (cc == 120)
            release(ch, true, frame, callback, context);
        else if (cc == 123) {
            for (unsigned i = 0; i < 16; ++i)
                if (i == ch || master(i + 1) == static_cast<int>(ch))
                    for (auto& n : notes_[i])
                        n.keyDown = false;
            release(ch, false, frame, callback, context);
        } else if (cc == 121) {
            channel.bend = channel.pressure = 0;
            channel.timbre = .5;
            channel.sustain = channel.sostenuto = false;
            for (auto& row : sostenuto_)
                for (auto& held : row)
                    held &= static_cast<std::uint16_t>(~(1u << ch));
            release(ch, false, frame, callback, context);
            expression(ch, frame, callback, context);
        }
    }
    return Status::ok;
}
} // namespace cydsp
