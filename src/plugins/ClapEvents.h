#pragma once
// Uses pinned MIT CLAP interfaces privately: third_party/licenses/clap.txt
#include "PluginInternal.h"
#include <clap/clap.h>
#include <cmath>
#include <cstring>
namespace cydsp::detail {
union ClapEventStorage {
    clap_event_note_t note;
    clap_event_note_expression_t expression;
    clap_event_param_value_t value;
    clap_event_param_mod_t modulation;
    clap_event_param_gesture_t gesture;
    clap_event_midi_t midi;
    clap_event_midi_sysex_t sysex;
    clap_event_midi2_t ump;
    clap_event_transport_t transport;
    ClapEventStorage() : transport{} {}
};
inline clap_event_transport_t clapTransport(const PluginTransport& t,
                                            std::uint32_t frame = 0) noexcept {
    clap_event_transport_t c{};
    c.header = {sizeof(c), frame, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_TRANSPORT, 0};
    if (t.hasTempo)
        c.flags |= CLAP_TRANSPORT_HAS_TEMPO;
    if (t.hasBeats)
        c.flags |= CLAP_TRANSPORT_HAS_BEATS_TIMELINE;
    if (t.hasSeconds)
        c.flags |= CLAP_TRANSPORT_HAS_SECONDS_TIMELINE;
    if (t.hasSignature)
        c.flags |= CLAP_TRANSPORT_HAS_TIME_SIGNATURE;
    if (t.playing)
        c.flags |= CLAP_TRANSPORT_IS_PLAYING;
    if (t.recording)
        c.flags |= CLAP_TRANSPORT_IS_RECORDING;
    if (t.looping)
        c.flags |= CLAP_TRANSPORT_IS_LOOP_ACTIVE;
    if (t.preRoll)
        c.flags |= CLAP_TRANSPORT_IS_WITHIN_PRE_ROLL;
    const auto fixed = [](double x) {
        return std::int64_t(std::llround(x * double(CLAP_BEATTIME_FACTOR)));
    };
    if (t.hasBeats) {
        c.song_pos_beats = fixed(t.beats);
        c.loop_start_beats = fixed(t.loopStartBeats);
        c.loop_end_beats = fixed(t.loopEndBeats);
        c.bar_start = fixed(t.barStart);
    }
    if (t.hasSeconds) {
        c.song_pos_seconds = fixed(t.seconds);
        c.loop_start_seconds = fixed(t.loopStartSeconds);
        c.loop_end_seconds = fixed(t.loopEndSeconds);
    }
    c.tempo = t.tempo;
    c.tempo_inc = t.tempoIncrement;
    c.bar_number = t.barNumber;
    c.tsig_num = t.numerator;
    c.tsig_denom = t.denominator;
    return c;
}
inline PluginTransport pluginTransport(const clap_event_transport_t& c) noexcept {
    PluginTransport t;
    t.hasTempo = (c.flags & CLAP_TRANSPORT_HAS_TEMPO) != 0;
    t.hasBeats = (c.flags & CLAP_TRANSPORT_HAS_BEATS_TIMELINE) != 0;
    t.hasSeconds = (c.flags & CLAP_TRANSPORT_HAS_SECONDS_TIMELINE) != 0;
    t.hasSignature = (c.flags & CLAP_TRANSPORT_HAS_TIME_SIGNATURE) != 0;
    t.playing = (c.flags & CLAP_TRANSPORT_IS_PLAYING) != 0;
    t.recording = (c.flags & CLAP_TRANSPORT_IS_RECORDING) != 0;
    t.looping = (c.flags & CLAP_TRANSPORT_IS_LOOP_ACTIVE) != 0;
    t.preRoll = (c.flags & CLAP_TRANSPORT_IS_WITHIN_PRE_ROLL) != 0;
    t.beats = double(c.song_pos_beats) / double(CLAP_BEATTIME_FACTOR);
    t.seconds = double(c.song_pos_seconds) / double(CLAP_SECTIME_FACTOR);
    t.loopStartBeats = double(c.loop_start_beats) / double(CLAP_BEATTIME_FACTOR);
    t.loopEndBeats = double(c.loop_end_beats) / double(CLAP_BEATTIME_FACTOR);
    t.loopStartSeconds = double(c.loop_start_seconds) / double(CLAP_SECTIME_FACTOR);
    t.loopEndSeconds = double(c.loop_end_seconds) / double(CLAP_SECTIME_FACTOR);
    t.barStart = double(c.bar_start) / double(CLAP_BEATTIME_FACTOR);
    t.barNumber = c.bar_number;
    t.tempo = c.tempo;
    t.tempoIncrement = c.tempo_inc;
    t.numerator = c.tsig_num;
    t.denominator = c.tsig_denom;
    return t;
}
inline void encodeClapEvent(const PluginEvent& e, ClapEventStorage& c,
                            void* cookie = nullptr) noexcept {
    const auto header = [&](std::uint32_t size, std::uint16_t type) {
        return clap_event_header_t{size, e.frame, CLAP_CORE_EVENT_SPACE_ID, type, e.flags};
    };
    switch (e.kind) {
    case PluginEventKind::noteOn:
    case PluginEventKind::noteOff:
    case PluginEventKind::noteChoke:
    case PluginEventKind::noteEnd:
        c.note = {header(sizeof(c.note), std::uint16_t(e.kind)),
                  e.noteId,
                  e.port,
                  e.channel,
                  e.key,
                  e.value};
        break;
    case PluginEventKind::noteExpression:
        c.expression = {header(sizeof(c.expression), CLAP_EVENT_NOTE_EXPRESSION),
                        std::int32_t(e.id),
                        e.noteId,
                        e.port,
                        e.channel,
                        e.key,
                        e.value};
        break;
    case PluginEventKind::parameterValue:
        c.value = {header(sizeof(c.value), CLAP_EVENT_PARAM_VALUE),
                   e.id,
                   cookie,
                   e.noteId,
                   e.port,
                   e.channel,
                   e.key,
                   e.value};
        break;
    case PluginEventKind::parameterModulation:
        c.modulation = {header(sizeof(c.modulation), CLAP_EVENT_PARAM_MOD),
                        e.id,
                        cookie,
                        e.noteId,
                        e.port,
                        e.channel,
                        e.key,
                        e.value};
        break;
    case PluginEventKind::gestureBegin:
    case PluginEventKind::gestureEnd:
        c.gesture = {header(sizeof(c.gesture), std::uint16_t(e.kind == PluginEventKind::gestureBegin
                                                                 ? CLAP_EVENT_PARAM_GESTURE_BEGIN
                                                                 : CLAP_EVENT_PARAM_GESTURE_END)),
                     e.id};
        break;
    case PluginEventKind::transport:
        c.transport = clapTransport(e.time, e.frame);
        break;
    case PluginEventKind::midi1:
        c.midi = {header(sizeof(c.midi), CLAP_EVENT_MIDI), std::uint16_t(e.port), {}};
        std::copy(e.bytes.begin(), e.bytes.end(), c.midi.data);
        break;
    case PluginEventKind::sysex:
        c.sysex = {header(sizeof(c.sysex), CLAP_EVENT_MIDI_SYSEX), std::uint16_t(e.port),
                   e.bytes.data(), std::uint32_t(e.bytes.size())};
        break;
    case PluginEventKind::ump:
        c.ump = {header(sizeof(c.ump), CLAP_EVENT_MIDI2), std::uint16_t(e.port), {}};
        std::copy(e.packet.words.begin(), e.packet.words.end(), c.ump.data);
        break;
    }
}
inline Status decodeClapEvent(const clap_event_header_t* h, PluginEventBuffer& out,
                              std::uint32_t frames) noexcept {
    if (!h || h->space_id != CLAP_CORE_EVENT_SPACE_ID || h->time >= frames)
        return Status::invalid_argument;
    PluginEvent e;
    e.frame = h->time;
    e.flags = h->flags;
    if (h->type <= CLAP_EVENT_NOTE_END) {
        if (h->size < sizeof(clap_event_note_t))
            return Status::invalid_argument;
        const auto& c = *reinterpret_cast<const clap_event_note_t*>(h);
        e.kind = PluginEventKind(h->type);
        e.noteId = c.note_id;
        e.port = c.port_index;
        e.channel = c.channel;
        e.key = c.key;
        e.value = c.velocity;
    } else if (h->type == CLAP_EVENT_NOTE_EXPRESSION) {
        if (h->size < sizeof(clap_event_note_expression_t))
            return Status::invalid_argument;
        const auto& c = *reinterpret_cast<const clap_event_note_expression_t*>(h);
        e.kind = PluginEventKind::noteExpression;
        e.id = std::uint32_t(c.expression_id);
        e.noteId = c.note_id;
        e.port = c.port_index;
        e.channel = c.channel;
        e.key = c.key;
        e.value = c.value;
    } else if (h->type == CLAP_EVENT_PARAM_VALUE || h->type == CLAP_EVENT_PARAM_MOD) {
        if (h->type == CLAP_EVENT_PARAM_VALUE) {
            if (h->size < sizeof(clap_event_param_value_t))
                return Status::invalid_argument;
            const auto& c = *reinterpret_cast<const clap_event_param_value_t*>(h);
            e.kind = PluginEventKind::parameterValue;
            e.id = c.param_id;
            e.noteId = c.note_id;
            e.port = c.port_index;
            e.channel = c.channel;
            e.key = c.key;
            e.value = c.value;
        } else {
            if (h->size < sizeof(clap_event_param_mod_t))
                return Status::invalid_argument;
            const auto& c = *reinterpret_cast<const clap_event_param_mod_t*>(h);
            e.kind = PluginEventKind::parameterModulation;
            e.id = c.param_id;
            e.noteId = c.note_id;
            e.port = c.port_index;
            e.channel = c.channel;
            e.key = c.key;
            e.value = c.amount;
        }
    } else if (h->type == CLAP_EVENT_PARAM_GESTURE_BEGIN ||
               h->type == CLAP_EVENT_PARAM_GESTURE_END) {
        if (h->size < sizeof(clap_event_param_gesture_t))
            return Status::invalid_argument;
        e.kind = h->type == CLAP_EVENT_PARAM_GESTURE_BEGIN ? PluginEventKind::gestureBegin
                                                           : PluginEventKind::gestureEnd;
        e.id = reinterpret_cast<const clap_event_param_gesture_t*>(h)->param_id;
    } else if (h->type == CLAP_EVENT_MIDI) {
        if (h->size < sizeof(clap_event_midi_t))
            return Status::invalid_argument;
        const auto& c = *reinterpret_cast<const clap_event_midi_t*>(h);
        const auto status = c.data[0];
        const std::size_t count = status < 0xc0                      ? 3
                                  : status < 0xe0                    ? 2
                                  : status < 0xf0                    ? 3
                                  : status == 0xf1 || status == 0xf3 ? 2
                                  : status == 0xf2                   ? 3
                                                                     : 1;
        e.kind = PluginEventKind::midi1;
        e.port = std::int16_t(c.port_index);
        e.bytes = {c.data, count};
    } else if (h->type == CLAP_EVENT_MIDI_SYSEX) {
        if (h->size < sizeof(clap_event_midi_sysex_t))
            return Status::invalid_argument;
        const auto& c = *reinterpret_cast<const clap_event_midi_sysex_t*>(h);
        if (!c.buffer || c.size > 16 * 1024 * 1024)
            return Status::invalid_argument;
        e.kind = PluginEventKind::sysex;
        e.port = std::int16_t(c.port_index);
        e.bytes = {c.buffer, c.size};
    } else if (h->type == CLAP_EVENT_MIDI2) {
        if (h->size < sizeof(clap_event_midi2_t))
            return Status::invalid_argument;
        const auto& c = *reinterpret_cast<const clap_event_midi2_t*>(h);
        e.kind = PluginEventKind::ump;
        e.port = std::int16_t(c.port_index);
        std::copy(std::begin(c.data), std::end(c.data), e.packet.words.begin());
        e.packet.count = std::uint8_t(umpWordCount(e.packet.type()));
    } else if (h->type == CLAP_EVENT_TRANSPORT) {
        if (h->size < sizeof(clap_event_transport_t))
            return Status::invalid_argument;
        e.kind = PluginEventKind::transport;
        e.time = pluginTransport(*reinterpret_cast<const clap_event_transport_t*>(h));
    } else
        return Status::unsupported_format;
    return out.add(e);
}
} // namespace cydsp::detail
