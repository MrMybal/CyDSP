#include "detail/Midi2Backend.h"
#include <CoreFoundation/CoreFoundation.h>
#include <CoreMIDI/CoreMIDI.h>
#include <array>
#include <atomic>
#include <charconv>
#include <mach/mach_time.h>
#include <thread>
namespace cydsp::detail {
namespace {
std::string name(MIDIObjectRef endpoint) {
    CFStringRef text = nullptr;
    if (MIDIObjectGetStringProperty(endpoint, kMIDIPropertyDisplayName, &text) != noErr || !text)
        return "CoreMIDI UMP";
    struct Guard {
        CFStringRef s;
        ~Guard() {
            CFRelease(s);
        }
    } guard{text};
    const auto size =
        CFStringGetMaximumSizeForEncoding(CFStringGetLength(text), kCFStringEncodingUTF8) + 1;
    std::string s(static_cast<std::size_t>(size), 0);
    if (!CFStringGetCString(text, s.data(), size, kCFStringEncodingUTF8))
        return "CoreMIDI UMP";
    s.resize(std::char_traits<char>::length(s.c_str()));
    return s;
}
struct AppleCallback {
    ReceiveMidi2 receive = nullptr;
    void* context = nullptr;
    std::atomic<bool> active{true};
    std::atomic<unsigned> calls{0};
    mach_timebase_info_data_t scale{};
    std::uint64_t baseTicks = 0, baseNs = 0;
    std::uint64_t nanoseconds(std::uint64_t ticks) const noexcept {
        if (!ticks)
            return midi2TimestampNow();
        if (ticks >= baseTicks) {
            const auto d = ticks - baseTicks;
            return baseNs + d / scale.denom * scale.numer +
                   d % scale.denom * scale.numer / scale.denom;
        }
        const auto d = baseTicks - ticks;
        const auto ns = d / scale.denom * scale.numer + d % scale.denom * scale.numer / scale.denom;
        return ns < baseNs ? baseNs - ns : 0;
    }
    std::uint64_t ticks(std::uint64_t ns) const noexcept {
        if (!ns)
            return 0;
        if (ns >= baseNs) {
            const auto d = ns - baseNs;
            return baseTicks + d / scale.numer * scale.denom +
                   d % scale.numer * scale.denom / scale.numer;
        }
        const auto d = baseNs - ns;
        const auto t = d / scale.numer * scale.denom + d % scale.numer * scale.denom / scale.numer;
        return t < baseTicks ? baseTicks - t : 0;
    }
};
struct AppleMidi final : NativeMidi2 {
    MIDIClientRef client = 0;
    MIDIPortRef input = 0, output = 0;
    MIDIEndpointRef source = 0, destination = 0;
    std::shared_ptr<AppleCallback> callback = std::make_shared<AppleCallback>();
    ~AppleMidi() override {
        callback->active.store(false, std::memory_order_release);
        if (input) {
            if (source)
                MIDIPortDisconnectSource(input, source);
            MIDIPortDispose(input);
        }
        if (output)
            MIDIPortDispose(output);
        if (client)
            MIDIClientDispose(client);
        while (callback->calls.load(std::memory_order_acquire))
            std::this_thread::yield();
    }
    Status send(const Midi2TimedPacket& p) noexcept override {
        if (!output || !destination)
            return Status::unsupported_format;
        MIDIEventList list{};
        auto* packet = MIDIEventListInit(&list, kMIDIProtocol_2_0);
        if (!MIDIEventListAdd(&list, sizeof(list), packet, callback->ticks(p.timestampNs),
                              p.packet.count, p.packet.words.data()))
            return Status::invalid_argument;
        return MIDISendEventList(output, destination, &list) == noErr ? Status::ok
                                                                      : Status::io_error;
    }
    Status poll() noexcept override {
        return Status::ok;
    }
    bool schedules() const noexcept override {
        return true;
    }
};
} // namespace
Status enumerateNativeMidi2(std::vector<Midi2EndpointInfo>& result) noexcept {
    if (__builtin_available(macOS 11.0, iOS 14.0, *)) {
        try {
            std::vector<Midi2EndpointInfo> next;
            for (unsigned direction = 0; direction < 2; ++direction) {
                const auto count =
                    direction ? MIDIGetNumberOfDestinations() : MIDIGetNumberOfSources();
                for (ItemCount i = 0; i < count; ++i) {
                    const auto ep = direction ? MIDIGetDestination(i) : MIDIGetSource(i);
                    SInt32 id = 0;
                    if (MIDIObjectGetIntegerProperty(ep, kMIDIPropertyUniqueID, &id) != noErr)
                        continue;
                    next.push_back({std::string(direction ? "coremidi:out:" : "coremidi:in:") +
                                        std::to_string(id),
                                    name(ep), Midi2Backend::coreMidi, direction == 0,
                                    direction == 1});
                }
            }
            result = std::move(next);
            return Status::ok;
        } catch (const std::bad_alloc&) {
            return Status::allocation_failed;
        } catch (...) {
            return Status::device_unavailable;
        }
    }
    return Status::unsupported_backend;
}
Status createNativeMidi2(const Midi2NativeRequest& r, ReceiveMidi2 receive, void* context,
                         std::unique_ptr<NativeMidi2>& out) noexcept {
    if (r.endpoint.backend != Midi2Backend::coreMidi)
        return Status::unsupported_backend;
    if (__builtin_available(macOS 11.0, iOS 14.0, *)) {
        const bool incoming = r.endpoint.id.starts_with("coremidi:in:"),
                   outgoing = r.endpoint.id.starts_with("coremidi:out:");
        if (!incoming && !outgoing)
            return Status::invalid_argument;
        const auto offset = incoming ? 12u : 13u;
        SInt32 id = 0;
        const auto text = std::string_view(r.endpoint.id).substr(offset);
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), id);
        if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
            return Status::invalid_argument;
        try {
            auto next = std::make_unique<AppleMidi>();
            MIDIObjectRef ep = 0;
            MIDIObjectType type{};
            if (MIDIObjectFindByUniqueID(id, &ep, &type) != noErr ||
                (incoming ? type != kMIDIObjectType_Source : type != kMIDIObjectType_Destination))
                return Status::device_unavailable;
            next->callback->receive = receive;
            next->callback->context = context;
            mach_timebase_info(&next->callback->scale);
            next->callback->baseTicks = mach_absolute_time();
            next->callback->baseNs = midi2TimestampNow();
            if (MIDIClientCreate(CFSTR("CyDSP MIDI 2"), nullptr, nullptr, &next->client) != noErr)
                return Status::device_unavailable;
            if (incoming) {
                next->source = ep;
                const auto callback = next->callback;
                if (MIDIInputPortCreateWithProtocol(
                        next->client, CFSTR("CyDSP UMP input"), kMIDIProtocol_2_0, &next->input,
                        ^(const MIDIEventList* list, void*) {
                          callback->calls.fetch_add(1, std::memory_order_acq_rel);
                          if (callback->active.load(std::memory_order_acquire) && list) {
                              auto* p = &list->packet[0];
                              for (UInt32 i = 0; i < list->numPackets; ++i) {
                                  callback->receive(callback->context, {p->words, p->wordCount},
                                                    callback->nanoseconds(p->timeStamp));
                                  p = MIDIEventPacketNext(p);
                              }
                          }
                          callback->calls.fetch_sub(1, std::memory_order_release);
                        }) != noErr ||
                    MIDIPortConnectSource(next->input, next->source, nullptr) != noErr)
                    return Status::device_unavailable;
            } else {
                next->destination = ep;
                if (MIDIOutputPortCreate(next->client, CFSTR("CyDSP UMP output"), &next->output) !=
                    noErr)
                    return Status::device_unavailable;
            }
            out = std::move(next);
            return Status::ok;
        } catch (const std::bad_alloc&) {
            return Status::allocation_failed;
        } catch (...) {
            return Status::device_unavailable;
        }
    }
    return Status::unsupported_backend;
}
} // namespace cydsp::detail
