#pragma once
#include "PluginInternal.h"
#include "cydsp/plugins/PluginWorker.h"
#include <array>
#include <atomic>
#include <cstring>
namespace cydsp::detail::wire {
inline constexpr std::uint64_t magic = 0x3150434950534443ull;
inline constexpr std::uint32_t version = 1;
enum class Command : std::uint32_t {
    save = 1,
    load,
    openEditor,
    resizeEditor,
    closeEditor,
    scaleEditor,
    showEditor,
    focusEditor,
    keyEditor,
    wheelEditor
};
struct Transport {
    std::int64_t songFrame = 0;
    double beats = 0, seconds = 0, tempo = 120, tempoIncrement = 0;
    double loopStartBeats = 0, loopEndBeats = 0, loopStartSeconds = 0, loopEndSeconds = 0,
           barStart = 0;
    std::int32_t barNumber = 0;
    std::uint32_t numerator = 4, denominator = 4, flags = 0;
};
inline Transport encode(const PluginTransport& t) noexcept {
    return {t.songFrame,
            t.beats,
            t.seconds,
            t.tempo,
            t.tempoIncrement,
            t.loopStartBeats,
            t.loopEndBeats,
            t.loopStartSeconds,
            t.loopEndSeconds,
            t.barStart,
            t.barNumber,
            t.numerator,
            t.denominator,
            (t.hasSongFrame ? 1u : 0u) | (t.hasBeats ? 2u : 0u) | (t.hasSeconds ? 4u : 0u) |
                (t.hasTempo ? 8u : 0u) | (t.hasSignature ? 16u : 0u) | (t.playing ? 32u : 0u) |
                (t.recording ? 64u : 0u) | (t.looping ? 128u : 0u) | (t.preRoll ? 256u : 0u)};
}
inline PluginTransport decode(const Transport& t) noexcept {
    PluginTransport out;
    out.songFrame = t.songFrame;
    out.beats = t.beats;
    out.seconds = t.seconds;
    out.tempo = t.tempo;
    out.tempoIncrement = t.tempoIncrement;
    out.loopStartBeats = t.loopStartBeats;
    out.loopEndBeats = t.loopEndBeats;
    out.loopStartSeconds = t.loopStartSeconds;
    out.loopEndSeconds = t.loopEndSeconds;
    out.barStart = t.barStart;
    out.barNumber = t.barNumber;
    out.numerator = std::uint16_t(t.numerator);
    out.denominator = std::uint16_t(t.denominator);
    out.hasSongFrame = t.flags & 1;
    out.hasBeats = t.flags & 2;
    out.hasSeconds = t.flags & 4;
    out.hasTempo = t.flags & 8;
    out.hasSignature = t.flags & 16;
    out.playing = t.flags & 32;
    out.recording = t.flags & 64;
    out.looping = t.flags & 128;
    out.preRoll = t.flags & 256;
    return out;
}
struct Event {
    std::uint32_t kind = 0, frame = 0, id = 0, flags = 0;
    std::int32_t noteId = -1, port = -1, channel = -1, key = -1;
    double value = 0;
    std::array<std::uint32_t, 4> words{};
    std::uint32_t wordCount = 0, offset = 0, size = 0;
    Transport transport;
};
struct Header {
    std::uint64_t magicValue = magic, mappingBytes = 0;
    std::uint32_t protocol = version, wordBits = sizeof(void*) * 8, headerBytes = sizeof(Header),
                  eventBytes = sizeof(Event);
    std::uint32_t format = 0, minimumFrames = 1, frames = 512, events = 1024, channels = 64,
                  payload = 65536, stateBytes = 16 * 1024 * 1024, flags = 0, parentPid = 0;
    double sampleRate = 48000;
    std::array<char, 32768> pluginPath{};
    std::array<char, 4096> pluginId{};
    std::array<std::byte, 128> bootSeparation{};
    std::atomic<std::uint32_t> boot{0}, shutdown{0}, failure{0}, requests{0}, heartbeat{0};
    std::uint32_t inputCount = 0, outputCount = 0, latency = 0, tail = 0, timingFlags = 0;
    std::array<std::uint32_t, 64> inputChannels{}, outputChannels{};
    std::array<std::byte, 128> audioSeparation{};
    std::atomic<std::uint32_t> audioState{0};
    std::uint64_t sequence = 0;
    std::int64_t steadyFrame = -1;
    std::uint32_t inputEvents = 0, inputBytes = 0, outputEvents = 0, outputBytes = 0,
                  audioStatus = 0, hasTransport = 0;
    Transport transport;
    std::array<std::byte, 128> controlSeparation{};
    std::atomic<std::uint32_t> controlState{0};
    std::uint32_t command = 0, controlStatus = 0, controlBytes = 0, windowApi = 0, floating = 0,
                  width = 0, height = 0;
    std::uint64_t windowHandle = 0;
    double scale = 1;
    std::uint32_t keyCharacter = 0, keyCode = 0, keyModifiers = 0, keyDown = 1, boolean = 0;
    float wheel = 0;
    std::atomic<std::uint32_t> requestedWidth{0}, requestedHeight{0};
};
static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
struct Layout {
    std::size_t input = 0, output = 0, inputEvents = 0, outputEvents = 0, inputPayload = 0,
                outputPayload = 0, control = 0, total = 0;
};
inline Layout layout(const Header& h) noexcept {
    Layout l;
    std::size_t at = sizeof(Header);
    const auto reserve = [&](std::size_t bytes) {
        at = (at + 63) & ~std::size_t(63);
        const auto result = at;
        at += bytes;
        return result;
    };
    const auto audio =
        std::size_t(h.channels) * h.frames * ((h.flags & 1) ? sizeof(double) : sizeof(float));
    l.input = reserve(audio);
    l.output = reserve(audio);
    l.inputEvents = reserve(std::size_t(h.events) * sizeof(Event));
    l.outputEvents = reserve(std::size_t(h.events) * sizeof(Event));
    l.inputPayload = reserve(h.payload);
    l.outputPayload = reserve(h.payload);
    l.control = reserve(h.stateBytes);
    l.total = at;
    return l;
}
inline bool valid(const Header& h) noexcept {
    if (h.magicValue != magic || h.protocol != version || h.wordBits != sizeof(void*) * 8 ||
        h.headerBytes != sizeof(Header) || h.eventBytes != sizeof(Event) ||
        h.format > std::uint32_t(PluginFormat::custom) || !isFinite(h.sampleRate) ||
        h.sampleRate <= 0 || h.sampleRate > 1000000 || !h.minimumFrames ||
        h.minimumFrames > h.frames || !h.frames || h.frames > 65536 || !h.events ||
        h.events > 65536 || !h.channels || h.channels > 256 || h.payload > 16 * 1024 * 1024 ||
        !h.stateBytes || h.stateBytes > 64 * 1024 * 1024 || h.flags > 3)
        return false;
    return layout(h).total == h.mappingBytes && h.mappingBytes <= 512 * 1024 * 1024;
}
inline Status writeEvents(const PluginEventBuffer* in, Event* records, std::uint8_t* payload,
                          std::uint32_t maximumEvents, std::uint32_t maximumBytes,
                          std::uint32_t& count, std::uint32_t& used) noexcept {
    count = used = 0;
    if (!in)
        return Status::ok;
    if (in->size() > maximumEvents)
        return Status::block_too_large;
    for (std::size_t i = 0; i < in->size(); ++i) {
        const auto& e = *in->event(i);
        if (e.bytes.size() > maximumBytes - used)
            return Status::block_too_large;
        records[count++] = {std::uint32_t(e.kind),
                            e.frame,
                            e.id,
                            e.flags,
                            e.noteId,
                            e.port,
                            e.channel,
                            e.key,
                            e.value,
                            e.packet.words,
                            e.packet.count,
                            used,
                            std::uint32_t(e.bytes.size()),
                            encode(e.time)};
        if (!e.bytes.empty())
            std::memcpy(payload + used, e.bytes.data(), e.bytes.size());
        used += std::uint32_t(e.bytes.size());
    }
    return Status::ok;
}
inline Status readEvents(const Event* records, const std::uint8_t* payload, std::uint32_t count,
                         std::uint32_t used, std::uint32_t maximumEvents,
                         std::uint32_t maximumBytes, std::uint32_t frames,
                         PluginEventBuffer& out) noexcept {
    out.reset();
    if (count > maximumEvents || used > maximumBytes)
        return Status::invalid_argument;
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto& e = records[i];
        if (e.kind > std::uint32_t(PluginEventKind::ump) || e.frame >= frames || e.offset > used ||
            e.size > used - e.offset || e.wordCount > 4 || e.port < -1 || e.port > 32767 ||
            e.channel < -1 || e.channel > 15 || e.key < -1 || e.key > 127 ||
            e.transport.flags > 511 || e.transport.numerator > 65535 ||
            e.transport.denominator > 65535)
            return Status::invalid_argument;
        PluginEvent p;
        p.kind = PluginEventKind(e.kind);
        p.frame = e.frame;
        p.id = e.id;
        p.flags = e.flags;
        p.noteId = e.noteId;
        p.port = std::int16_t(e.port);
        p.channel = std::int16_t(e.channel);
        p.key = std::int16_t(e.key);
        p.value = e.value;
        p.bytes = {payload + e.offset, e.size};
        p.packet.words = e.words;
        p.packet.count = std::uint8_t(e.wordCount);
        p.time = decode(e.transport);
        const auto r = out.add(p);
        if (r != Status::ok)
            return r;
    }
    return Status::ok;
}
class Mapping {
  public:
    Mapping() = default;
    ~Mapping();
    Mapping(const Mapping&) = delete;
    Mapping& operator=(const Mapping&) = delete;
    Status create(std::string_view, std::size_t) noexcept;
    Status open(std::string_view) noexcept;
    std::byte* data = nullptr;
    std::size_t bytes = 0;

  private:
#if defined(_WIN32)
    void* handle = nullptr;
#elif !defined(__ANDROID__)
    int fd = -1;
    bool owner = false;
#endif
    std::string name;
};
class Child {
  public:
    Child() = default;
    ~Child();
    Status launch(const std::filesystem::path&, std::string_view) noexcept;
    bool running() noexcept;
    void close() noexcept;

  private:
#if defined(_WIN32)
    void* handle = nullptr;
#elif !defined(__ANDROID__)
    int pid = -1;
#endif
};
std::uint32_t processId() noexcept;
bool parentRunning(std::uint32_t) noexcept;
void pumpNativeMessages() noexcept;
} // namespace cydsp::detail::wire
