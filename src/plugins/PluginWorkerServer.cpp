#include "PluginWire.h"
#include <algorithm>
#include <chrono>
#include <thread>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <objbase.h>
#include <windows.h>
#endif
namespace cydsp {
namespace {
using namespace detail::wire;
bool describe(PluginInstance& instance, Header& h, bool initial) noexcept {
    const auto* d = instance.description();
    if (!d || d->inputs.size() > 64 || d->outputs.size() > 64)
        return false;
    std::uint32_t inputs = 0, outputs = 0;
    for (std::size_t i = 0; i < d->inputs.size(); ++i) {
        if (d->inputs[i].channels > h.channels - inputs)
            return false;
        inputs += d->inputs[i].channels;
        if (!initial && (i >= h.inputCount || h.inputChannels[i] != d->inputs[i].channels))
            return false;
        h.inputChannels[i] = d->inputs[i].channels;
    }
    for (std::size_t i = 0; i < d->outputs.size(); ++i) {
        if (d->outputs[i].channels > h.channels - outputs)
            return false;
        outputs += d->outputs[i].channels;
        if (!initial && (i >= h.outputCount || h.outputChannels[i] != d->outputs[i].channels))
            return false;
        h.outputChannels[i] = d->outputs[i].channels;
    }
    if (!initial && (h.inputCount != d->inputs.size() || h.outputCount != d->outputs.size()))
        return false;
    h.inputCount = std::uint32_t(d->inputs.size());
    h.outputCount = std::uint32_t(d->outputs.size());
    const auto timing = instance.timing();
    h.latency = timing.latencyFrames;
    h.tail = timing.tailFrames;
    h.timingFlags = (timing.latencyKnown ? 1u : 0u) | (timing.tailKnown ? 2u : 0u) |
                    (timing.infiniteTail ? 4u : 0u);
    return true;
}
template <Sample T> struct Server {
    Header& h;
    std::byte* data;
    Layout offsets;
    PluginInstance& plugin;
    PluginEventBuffer incoming, outgoing;
    std::vector<const T*> inputPointers;
    std::vector<T*> outputPointers;
    std::vector<AudioBlock<const T>> inputs;
    std::vector<AudioBlock<T>> outputs;
    Server(Header& header, std::byte* memory, PluginInstance& instance)
        : h(header), data(memory), offsets(layout(h)), plugin(instance) {}
    Status prepare() noexcept {
        try {
            auto r = incoming.prepare(h.events, h.payload);
            if (r != Status::ok)
                return r;
            r = outgoing.prepare(h.events, h.payload);
            if (r != Status::ok)
                return r;
            inputPointers.resize(h.channels);
            outputPointers.resize(h.channels);
            auto* input = reinterpret_cast<const T*>(data + offsets.input);
            auto* output = reinterpret_cast<T*>(data + offsets.output);
            for (std::size_t ch = 0; ch < h.channels; ++ch) {
                inputPointers[ch] = input + ch * h.frames;
                outputPointers[ch] = output + ch * h.frames;
            }
            std::size_t channel = 0;
            for (std::uint32_t i = 0; i < h.inputCount; ++i) {
                inputs.emplace_back(inputPointers.data() + channel, h.inputChannels[i], h.frames);
                channel += h.inputChannels[i];
            }
            channel = 0;
            for (std::uint32_t i = 0; i < h.outputCount; ++i) {
                outputs.emplace_back(outputPointers.data() + channel, h.outputChannels[i],
                                     h.frames);
                channel += h.outputChannels[i];
            }
            return Status::ok;
        } catch (...) {
            return Status::allocation_failed;
        }
    }
    void audio() noexcept {
        Status result =
            readEvents(reinterpret_cast<const Event*>(data + offsets.inputEvents),
                       reinterpret_cast<const std::uint8_t*>(data + offsets.inputPayload),
                       h.inputEvents, h.inputBytes, h.events, h.payload, h.frames, incoming);
        PluginTransport transport;
        if (h.hasTransport > 1 || h.transport.flags > 511 || h.transport.numerator > 65535 ||
            h.transport.denominator > 65535)
            result = Status::invalid_argument;
        transport = decode(h.transport);
        if (result == Status::ok)
            result = plugin.process(
                PluginProcess<T>{inputs, outputs, h.frames, &incoming, &outgoing,
                                 h.hasTransport ? &transport : nullptr, h.steadyFrame});
        h.outputEvents = h.outputBytes = 0;
        if (result == Status::ok)
            result = writeEvents(&outgoing, reinterpret_cast<Event*>(data + offsets.outputEvents),
                                 reinterpret_cast<std::uint8_t*>(data + offsets.outputPayload),
                                 h.events, h.payload, h.outputEvents, h.outputBytes);
        h.audioStatus = std::uint32_t(result);
        h.audioState.store(2, std::memory_order_release);
    }
    Status control() noexcept {
        const auto command = Command(h.command);
        Status result = Status::unsupported_format;
        if (command == Command::save || command == Command::load) {
            auto stopped = plugin.stop();
            if (stopped != Status::ok)
                return stopped;
            if (command == Command::save) {
                std::vector<std::uint8_t> state;
                result = plugin.saveState(state, h.stateBytes);
                if (result == Status::ok) {
                    h.controlBytes = std::uint32_t(state.size());
                    if (!state.empty())
                        std::memcpy(data + offsets.control, state.data(), state.size());
                }
                // Saving must not deactivate/re-prepare and discard DSP histories.
                const auto resumed = plugin.start();
                return resumed == Status::ok ? result : resumed;
            }
            const auto deactivated = plugin.deactivate();
            if (deactivated != Status::ok)
                return deactivated;
            if (h.controlBytes <= h.stateBytes)
                result =
                    plugin.loadState({reinterpret_cast<const std::uint8_t*>(data + offsets.control),
                                      h.controlBytes});
            else
                result = Status::block_too_large;
            const auto refreshed = plugin.refresh();
            if (refreshed != Status::ok)
                return refreshed;
            PluginPrepareOptions options{h.sampleRate, h.minimumFrames,   h.frames,
                                         h.events,     bool(h.flags & 1), bool(h.flags & 2)};
            const auto active = plugin.activate(options);
            if (active != Status::ok)
                return active;
            if (!describe(plugin, h, false))
                return Status::channel_mismatch;
            const auto started = plugin.start();
            if (started != Status::ok)
                return started;
        } else if (command == Command::openEditor) {
#if defined(_WIN32)
            if (h.windowApi != std::uint32_t(PluginWindowApi::win32))
                return Status::unsupported_backend;
#else
            if (!h.floating || h.windowHandle)
                return Status::unsupported_backend;
#endif
            if (h.windowApi > std::uint32_t(PluginWindowApi::wayland) || h.floating > 1)
                return Status::invalid_argument;
            PluginEditorSize size;
            result = plugin.openEditor({PluginWindowApi(h.windowApi),
                                        std::uintptr_t(h.windowHandle), bool(h.floating), h.scale},
                                       size);
            if (result == Status::ok) {
                h.width = size.width;
                h.height = size.height;
            }
        } else if (command == Command::resizeEditor) {
            PluginEditorSize size{h.width, h.height};
            result = plugin.resizeEditor(size);
            if (result == Status::ok) {
                h.width = size.width;
                h.height = size.height;
            }
        } else if (command == Command::scaleEditor)
            result = plugin.scaleEditor(h.scale);
        else if (command == Command::showEditor)
            result = h.boolean <= 1 ? plugin.showEditor(h.boolean != 0) : Status::invalid_argument;
        else if (command == Command::focusEditor)
            result = h.boolean <= 1 ? plugin.focusEditor(h.boolean != 0) : Status::invalid_argument;
        else if (command == Command::keyEditor) {
            if (h.keyCharacter > 65535 || h.keyModifiers > 15 || h.keyDown > 1 ||
                h.keyCode > std::uint32_t(PluginEditorKeyCode::f24))
                return Status::invalid_argument;
            result =
                plugin.keyEditor({std::uint16_t(h.keyCharacter), PluginEditorKeyCode(h.keyCode),
                                  std::uint16_t(h.keyModifiers), h.keyDown != 0});
        } else if (command == Command::wheelEditor)
            result = plugin.wheelEditor(h.wheel);
        else if (command == Command::closeEditor)
            result = plugin.closeEditor();
        return result;
    }
    void run() noexcept {
        auto lastParent = std::chrono::steady_clock::now();
        while (!h.shutdown.load(std::memory_order_acquire)) {
            bool worked = false;
            if (h.audioState.load(std::memory_order_acquire) == 1) {
                audio();
                worked = true;
            }
            if (h.controlState.load(std::memory_order_acquire) == 1) {
                h.controlStatus = std::uint32_t(control());
                h.controlState.store(2, std::memory_order_release);
                worked = true;
            }
            std::uint32_t requests = 0;
            (void)plugin.pumpMainThread(requests);
            if (requests)
                h.requests.fetch_or(requests, std::memory_order_release);
            pumpNativeMessages();
            h.heartbeat.fetch_add(1, std::memory_order_relaxed);
            const auto now = std::chrono::steady_clock::now();
            if (now - lastParent >= std::chrono::milliseconds(250)) {
                if (!parentRunning(h.parentPid))
                    break;
                lastParent = now;
            }
            if (!worked)
                std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
};
} // namespace
int runPluginWorker(std::string_view mappingName) noexcept {
    using namespace detail::wire;
    Mapping memory;
    Header* header = nullptr;
    try {
        auto r = memory.open(mappingName);
        if (r != Status::ok)
            return 80;
        auto& h = *reinterpret_cast<Header*>(memory.data);
        header = &h;
        const auto fail = [&](Status status) {
            h.failure.store(std::uint32_t(status), std::memory_order_relaxed);
            h.boot.store(2, std::memory_order_release);
            return 81;
        };
        if (!valid(h) || h.boot.load() || h.pluginPath.back() != 0 || h.pluginId.back() != 0)
            return fail(Status::incompatible_version);
#if defined(_WIN32)
        struct ComScope {
            HRESULT status = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
            ~ComScope() {
                if (SUCCEEDED(status))
                    CoUninitialize();
            }
        } com;
        if (FAILED(com.status))
            return fail(Status::unsupported_backend);
#endif
        std::unique_ptr<PluginProvider> provider;
        r = createPluginProvider(PluginFormat(h.format), provider);
        if (r != Status::ok)
            return fail(r);
        const auto path = std::filesystem::path(
            std::u8string_view(reinterpret_cast<const char8_t*>(h.pluginPath.data())));
        PluginInstance plugin;
        PluginHostCallbacks callbacks;
        callbacks.context = &h;
        callbacks.resizeEditor = [](void* context, PluginEditorSize size) noexcept {
            auto& header = *static_cast<Header*>(context);
            header.requestedWidth.store(size.width, std::memory_order_relaxed);
            header.requestedHeight.store(size.height, std::memory_order_release);
            header.requests.fetch_or(pluginEditorResize, std::memory_order_release);
            return false; // Parent UI must decide after poll; never acknowledge a resize not
                          // applied.
        };
        r = provider->create(path, h.pluginId.data(), callbacks, plugin);
        if (r != Status::ok)
            return fail(r);
        PluginPrepareOptions options{h.sampleRate, h.minimumFrames,   h.frames,
                                     h.events,     bool(h.flags & 1), bool(h.flags & 2)};
        r = plugin.activate(options);
        if (r != Status::ok)
            return fail(r);
        if (!describe(plugin, h, true))
            return fail(Status::channel_mismatch);
        r = plugin.start();
        if (r != Status::ok)
            return fail(r);
        if (h.flags & 1) {
            Server<double> server(h, memory.data, plugin);
            r = server.prepare();
            if (r != Status::ok)
                return fail(r);
            h.boot.store(1, std::memory_order_release);
            server.run();
        } else {
            Server<float> server(h, memory.data, plugin);
            r = server.prepare();
            if (r != Status::ok)
                return fail(r);
            h.boot.store(1, std::memory_order_release);
            server.run();
        }
        if (plugin.lifecycle() == PluginLifecycle::processing)
            (void)plugin.stop();
        if (plugin.lifecycle() == PluginLifecycle::active)
            (void)plugin.deactivate();
        (void)plugin.close();
        return 0;
    } catch (...) {
        if (header) {
            header->failure.store(std::uint32_t(Status::io_error), std::memory_order_relaxed);
            header->boot.store(2, std::memory_order_release);
        }
        return 82;
    }
}
} // namespace cydsp
