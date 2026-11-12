// Private unmodified RtMidi backend: third_party/licenses/rtmidi.txt
#include "cydsp/io/MidiDevice.h"
#include "cydsp/core/SpscQueue.h"
#include <RtMidi.h>
#include <algorithm>
#include <array>
#include <chrono>
#if defined(_WIN32)
// clang-format off
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
// clang-format on
#endif
namespace cydsp {
namespace {
using namespace rt::midi;
bool validOptions(const MidiInputOptions& o) noexcept {
    return o.queuedMessages && o.queuedMessages <= 65536 && o.maximumMessageBytes &&
           o.maximumMessageBytes <= 1048576 &&
           o.queuedMessages <= 268435456 / o.maximumMessageBytes && o.nativeBufferCount &&
           o.nativeBufferCount <= 64;
}
std::string stableId(unsigned index, bool input, RtMidi::Api backend) {
#if defined(_WIN32)
    if (backend == RtMidi::WINDOWS_MM) {
        ULONG length = 0;
        const auto query = [&](UINT message, DWORD_PTR value, DWORD_PTR bytes = 0) {
            return input ? midiInMessage(reinterpret_cast<HMIDIIN>(std::uintptr_t(index)), message,
                                         value, bytes)
                         : midiOutMessage(reinterpret_cast<HMIDIOUT>(std::uintptr_t(index)),
                                          message, value, bytes);
        };
        if (query(DRV_QUERYDEVICEINTERFACESIZE, reinterpret_cast<DWORD_PTR>(&length)) ==
                MMSYSERR_NOERROR &&
            length >= sizeof(wchar_t) && length <= 65536) {
            std::vector<wchar_t> path(length / sizeof(wchar_t) + 1, 0);
            if (query(DRV_QUERYDEVICEINTERFACE, reinterpret_cast<DWORD_PTR>(path.data()), length) ==
                MMSYSERR_NOERROR) {
                // Lossless UTF-16 code units, independent of locale and enumeration order.
                static constexpr char digits[] = "0123456789abcdef";
                std::string id = "cydsp.midi.winmm/1/";
                for (auto unit : path) {
                    if (!unit)
                        break;
                    for (int shift = 12; shift >= 0; shift -= 4)
                        id += digits[(unsigned(unit) >> shift) & 15];
                }
                if (id.size() > 19)
                    return id;
            }
        }
    }
#else
    (void)index;
    (void)input;
    (void)backend;
#endif
    return {};
}
RtMidi::Api api() {
    std::vector<RtMidi::Api> compiled;
    RtMidi::getCompiledApi(compiled);
    for (auto a : compiled)
        if (a != RtMidi::RTMIDI_DUMMY && RtMidi::checkApiAvailability(a).available)
            return a;
    return RtMidi::RTMIDI_DUMMY;
}
bool virtualSupported(RtMidi::Api a) noexcept {
    return a == RtMidi::MACOSX_CORE || a == RtMidi::LINUX_ALSA || a == RtMidi::UNIX_JACK;
}
bool nameValid(std::string_view name) noexcept {
    return !name.empty() && name.size() <= 255 && name.find(char{}) == std::string_view::npos;
}
} // namespace
Status enumerateMidiPorts(MidiDevicePorts& result) noexcept {
    try {
        const auto a = api();
        if (a == RtMidi::RTMIDI_DUMMY)
            return Status::unsupported_backend;
        RtMidiIn input(a, "CyDSP enumeration");
        RtMidiOut output(a, "CyDSP enumeration");
        MidiDevicePorts next;
        next.backend = RtMidi::getApiName(a);
        next.virtualInput = next.virtualOutput = virtualSupported(a);
        for (unsigned i = 0; i < input.getPortCount(); ++i) {
            auto id = stableId(i, true, a);
            next.inputs.push_back({i, input.getPortName(i), id, !id.empty()});
        }
        for (unsigned i = 0; i < output.getPortCount(); ++i) {
            auto id = stableId(i, false, a);
            next.outputs.push_back({i, output.getPortName(i), id, !id.empty()});
        }
        result = std::move(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::device_unavailable;
    }
}
struct MidiInput::State {
    std::unique_ptr<RtMidiIn> input;
    struct Slot {
        std::vector<std::uint8_t> bytes;
        std::size_t size = 0;
        MidiInputTimestamp stamp;
    };
    std::vector<Slot> slots;
    SpscQueue<std::size_t> free, ready;
    std::atomic<std::uint32_t> dropped{0}, oversized{0};
    double time = 0;
    bool isVirtual = false;
    Status prepare(const MidiInputOptions& o) {
        auto r = free.prepare(o.queuedMessages);
        if (r != Status::ok)
            return r;
        r = ready.prepare(o.queuedMessages);
        if (r != Status::ok)
            return r;
        slots.resize(o.queuedMessages);
        for (std::size_t i = 0; i < slots.size(); ++i) {
            slots[i].bytes.resize(o.maximumMessageBytes);
            (void)free.push(i);
        }
        return Status::ok;
    }
    ~State() {
        if (input) {
            try {
                input->closePort();
                input->cancelCallback();
            } catch (...) {
            }
        }
    }
    static void receive(double delta, std::vector<unsigned char>* bytes, void* context) noexcept {
        auto& s = *static_cast<State*>(context);
        if (isFinite(delta) && delta >= 0)
            s.time += delta;
        if (!bytes || bytes->empty())
            return;
        if (bytes->size() > s.slots[0].bytes.size()) {
            s.oversized.fetch_add(1, std::memory_order_relaxed);
            s.dropped.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        std::size_t id;
        if (!s.free.pop(id)) {
            s.dropped.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        auto& p = s.slots[id];
        p.size = bytes->size();
        p.stamp.relativeSeconds = s.time;
        const auto arrival = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                 std::chrono::steady_clock::now().time_since_epoch())
                                 .count();
        p.stamp.arrivalNanoseconds = arrival > 0 ? static_cast<std::uint64_t>(arrival) : 0;
        std::copy(bytes->begin(), bytes->end(), p.bytes.begin());
        (void)s.ready.push(id);
    }
};
MidiInput::MidiInput() = default;
MidiInput::~MidiInput() = default;
MidiInput::MidiInput(MidiInput&&) noexcept = default;
MidiInput& MidiInput::operator=(MidiInput&&) noexcept = default;
Status MidiInput::open(std::uint32_t port, std::size_t capacity) noexcept {
    return open(port, MidiInputOptions{capacity});
}
Status MidiInput::open(std::uint32_t port, const MidiInputOptions& options) noexcept {
    if (!validOptions(options))
        return Status::invalid_argument;
    try {
        const auto a = api();
        if (a == RtMidi::RTMIDI_DUMMY)
            return Status::unsupported_backend;
        auto next = std::make_unique<State>();
        auto status = next->prepare(options);
        if (status != Status::ok)
            return status;
        next->input = std::make_unique<RtMidiIn>(a, "CyDSP MIDI input");
        if (port >= next->input->getPortCount())
            return Status::invalid_argument;
        next->input->setBufferSize(static_cast<unsigned>(options.maximumMessageBytes),
                                   options.nativeBufferCount);
        next->input->ignoreTypes(false, false, false);
        next->input->setCallback(&State::receive, next.get());
        next->input->openPort(port, "CyDSP input");
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::device_unavailable;
    }
}
Status MidiInput::openVirtual(std::string_view name, std::size_t capacity) noexcept {
    return openVirtual(name, MidiInputOptions{capacity});
}
Status MidiInput::openVirtual(std::string_view name, const MidiInputOptions& options) noexcept {
    if (!nameValid(name) || !validOptions(options))
        return Status::invalid_argument;
    try {
        const auto a = api();
        if (!virtualSupported(a))
            return Status::unsupported_backend;
        auto next = std::make_unique<State>();
        auto r = next->prepare(options);
        if (r != Status::ok)
            return r;
        next->input = std::make_unique<RtMidiIn>(a, "CyDSP virtual input");
        next->input->setBufferSize(static_cast<unsigned>(options.maximumMessageBytes),
                                   options.nativeBufferCount);
        next->input->ignoreTypes(false, false, false);
        next->input->setCallback(&State::receive, next.get());
        next->input->openVirtualPort(std::string(name));
        next->isVirtual = true;
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::device_unavailable;
    }
}
bool MidiInput::isVirtual() const noexcept {
    return state_ && state_->isVirtual;
}
void MidiInput::close() noexcept {
    state_.reset();
}
Status MidiInput::read(std::span<std::uint8_t> destination, std::size_t& size,
                       double& time) noexcept {
    MidiInputTimestamp stamp;
    const auto r = readTimed(destination, size, stamp);
    time = stamp.relativeSeconds;
    return r;
}
Status MidiInput::readTimed(std::span<std::uint8_t> destination, std::size_t& size,
                            MidiInputTimestamp& time) noexcept {
    size = 0;
    time = {};
    if (!state_)
        return Status::not_prepared;
    const auto* id = state_->ready.peek();
    if (!id)
        return Status::end_of_stream;
    const auto index = *id;
    const auto& p = state_->slots[index];
    size = p.size;
    time = p.stamp;
    if (destination.size() < size)
        return Status::block_too_large;
    std::copy_n(p.bytes.begin(), size, destination.begin());
    (void)state_->ready.discard();
    (void)state_->free.push(index);
    return Status::ok;
}
std::uint32_t MidiInput::droppedMessages() const noexcept {
    return state_ ? state_->dropped.load(std::memory_order_relaxed) : 0;
}
std::uint32_t MidiInput::oversizedMessages() const noexcept {
    return state_ ? state_->oversized.load(std::memory_order_relaxed) : 0;
}
struct MidiOutput::State {
    std::unique_ptr<RtMidiOut> output;
    bool isVirtual = false;
};
MidiOutput::MidiOutput() = default;
MidiOutput::~MidiOutput() = default;
MidiOutput::MidiOutput(MidiOutput&&) noexcept = default;
MidiOutput& MidiOutput::operator=(MidiOutput&&) noexcept = default;
Status MidiOutput::open(std::uint32_t port) noexcept {
    try {
        const auto a = api();
        if (a == RtMidi::RTMIDI_DUMMY)
            return Status::unsupported_backend;
        auto next = std::make_unique<State>();
        next->output = std::make_unique<RtMidiOut>(a, "CyDSP MIDI output");
        if (port >= next->output->getPortCount())
            return Status::invalid_argument;
        next->output->openPort(port, "CyDSP output");
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::device_unavailable;
    }
}
Status MidiOutput::openVirtual(std::string_view name) noexcept {
    if (!nameValid(name))
        return Status::invalid_argument;
    try {
        const auto a = api();
        if (!virtualSupported(a))
            return Status::unsupported_backend;
        auto next = std::make_unique<State>();
        next->output = std::make_unique<RtMidiOut>(a, "CyDSP virtual output");
        next->output->openVirtualPort(std::string(name));
        next->isVirtual = true;
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::device_unavailable;
    }
}
bool MidiOutput::isVirtual() const noexcept {
    return state_ && state_->isVirtual;
}
void MidiOutput::close() noexcept {
    state_.reset();
}
Status MidiOutput::send(std::span<const std::uint8_t> bytes) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (bytes.empty() || bytes[0] < 0x80 || bytes.size() > 1048576)
        return Status::invalid_argument;
    try {
        state_->output->sendMessage(bytes.data(), bytes.size());
        return Status::ok;
    } catch (...) {
        return Status::io_error;
    }
}
} // namespace cydsp
