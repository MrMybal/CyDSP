#include <algorithm>
#include <array>
#include <cydsp/CyDSP.h>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <thread>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
void assertNoRealtimeAllocations(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool v, const std::source_location at = std::source_location::current()) {
    if (!v)
        throw std::runtime_error("plugin contract line " + std::to_string(at.line()));
}
void ok(Status r, const std::source_location at = std::source_location::current()) {
    require(r == Status::ok, at);
}
template <class T> void audio(PluginInstance& instance) {
    PluginPrepareOptions o;
    o.maximumFrames = 64;
    o.useFloat64 = std::same_as<T, double>;
    ok(instance.activate(o));
    const auto timing = instance.timing();
    require(timing.latencyKnown && timing.latencyFrames == 17 && timing.tailKnown &&
            timing.tailFrames == 64);
    AudioBuffer<T> input, side, output;
    ok(input.resize(2, 96));
    ok(side.resize(1, 96));
    ok(output.resize(2, 96));
    for (std::size_t ch = 0; ch < 2; ++ch)
        std::fill(input.block().channel(ch).begin(), input.block().channel(ch).end(), T{2});
    std::fill(side.block().channel(0).begin(), side.block().channel(0).end(), T{.25});
    for (std::size_t ch = 0; ch < 2; ++ch)
        std::fill(output.block().channel(ch).begin(), output.block().channel(ch).end(), T{-77});
    const std::array<AudioBlock<const T>, 2> ins{input.block().subBlock(11, 64),
                                                 side.block().subBlock(11, 64)};
    const std::array<AudioBlock<T>, 1> outs{output.block().subBlock(11, 64)};
    PluginEventBuffer events, result;
    ok(events.prepare(32, 20000));
    ok(result.prepare(32, 20000));
    PluginEvent e;
    e.kind = PluginEventKind::gestureBegin;
    e.id = 7;
    ok(events.add(e));
    e.kind = PluginEventKind::parameterValue;
    e.value = .5;
    ok(events.add(e));
    std::vector<std::uint8_t> sysex(12000, 0x12);
    sysex.front() = 0xf0;
    sysex.back() = 0xf7;
    e = {};
    e.kind = PluginEventKind::sysex;
    e.frame = 3;
    e.port = 0;
    e.bytes = sysex;
    ok(events.add(e));
    e = {};
    e.kind = PluginEventKind::noteOn;
    e.frame = 7;
    e.port = 0;
    e.channel = 0;
    e.key = 60;
    e.value = .8;
    ok(events.add(e));
    e = {};
    e.kind = PluginEventKind::parameterValue;
    e.frame = 19;
    e.id = 7;
    e.value = 1;
    const auto cc = midiController(1, 7, 127);
    if (instance.description()->parameters[0].normalized) {
        e.kind = PluginEventKind::midi1;
        e.port = 0;
        e.bytes = cc.bytes();
    }
    ok(events.add(e));
    e = {};
    e.kind = PluginEventKind::gestureEnd;
    e.frame = 31;
    e.id = 7;
    ok(events.add(e));
    PluginTransport transport;
    transport.hasTempo = transport.hasBeats = transport.hasSeconds = true;
    transport.playing = true;
    transport.tempo = 123;
    transport.beats = 4;
    transport.seconds = 1.5;
    PluginProcess<T> call{ins, outs, 64, &events, &result, &transport, 0};
    ok(instance.start());
    struct Realtime {
        PluginInstance* instance;
        PluginProcess<T>* call;
        Status status = Status::not_prepared;
    } realtime{&instance, &call};
    assertNoRealtimeAllocations(
        [](void* p) noexcept {
            auto& r = *static_cast<Realtime*>(p);
            r.status = r.instance->process(*r.call);
        },
        &realtime);
    ok(realtime.status);
    for (std::size_t ch = 0; ch < 2; ++ch) {
        for (std::size_t i = 0; i < 64; ++i)
            require(output.block().channel(ch)[11 + i] ==
                    T(instance.description()->parameters[0].normalized ? (i < 19 ? 2.25 : 4.25)
                                                                       : (i < 19 ? 1.25 : 2.25)));
        require(output.block().channel(ch)[10] == T{-77} &&
                output.block().channel(ch)[75] == T{-77});
    }
    require(result.size() == 4 && result.event(1)->bytes.size() == 12000 &&
            result.event(1)->bytes[11999] == 0xf7);
    std::fill(sysex.begin(), sysex.end(), std::uint8_t{0});
    require(result.event(1)->bytes[1] == 0x12); // Owned output copy.
    require(instance.process(call) ==
            Status::invalid_argument); // Repeated known steady time is invalid.
    const std::array<AudioBlock<T>, 1> aliased{input.block().subBlock(11, 64)};
    auto alias = call;
    alias.outputs = aliased;
    alias.steadyFrame = 64;
    const auto aliasStatus = instance.process(alias);
    require(
        aliasStatus ==
        (instance.description()->parameters[0].normalized ? Status::ok : Status::partial_overlap));
    require(instance.deactivate() == Status::transition_in_progress);
    std::vector<std::uint8_t> forbidden;
    require(instance.saveState(forbidden) == Status::transition_in_progress);
    ok(instance.stop());
    ok(instance.deactivate());
}
} // namespace
void pluginRegression(const std::filesystem::path& directory) {
    using namespace cydsp;
    std::unique_ptr<PluginProvider> provider;
    ok(createPluginProvider(PluginFormat::clap, provider));
    std::vector<PluginDescriptor> descriptors;
    const auto path = std::filesystem::absolute(directory / "cydsp_fixture.clap");
    ok(provider->scan(path, descriptors));
    require(descriptors.size() == 1 && descriptors[0].id == "org.cydsp.fixture");
    struct Host {
        PluginEditorSize size;
        std::uint32_t requests = 0;
    } host;
    PluginHostCallbacks cb{
        &host,
        [](void* p, PluginEditorSize size) noexcept {
            static_cast<Host*>(p)->size = size;
            return true;
        },
        [](void* p, std::uint32_t flags) noexcept { static_cast<Host*>(p)->requests |= flags; }};
    PluginInstance instance;
    ok(provider->create(path, descriptors[0].id, cb, instance));
    // A second scan while an instance lives must share entry init/deinit ownership.
    ok(provider->scan(path, descriptors));
    provider.reset();
    require(instance.description()->inputs.size() == 2 &&
            instance.description()->inputs[1].channels == 1 &&
            instance.description()->outputs.size() == 1);
    require(instance.description()->parameters[0].id == 7 && instance.description()->float64);
    double value = 0;
    ok(instance.parameterValue(7, value));
    require(value == .5);
    Status wrong = Status::ok;
    std::thread thread([&] { wrong = instance.activate(); });
    thread.join();
    require(wrong == Status::invalid_argument);
    std::vector<std::uint8_t> state;
    ok(instance.saveState(state));
    require(state.size() == 8);
    require(instance.saveState(state, 1) == Status::io_error && state.size() == 8);
    audio<float>(instance);
    ok(instance.loadState(state));
    audio<double>(instance);
    ok(instance.loadState(state));
    ok(instance.parameterValue(7, value));
    require(value == .5);
    std::string display;
    ok(instance.parameterText(7, .25, display));
    require(display == "0.250");
#if defined(_WIN32)
    struct Parent {
        HWND handle = nullptr;
        ~Parent() {
            if (handle)
                DestroyWindow(handle);
        }
    } parent;
    parent.handle =
        CreateWindowExW(0, L"STATIC", L"CyDSP hidden fixture parent", WS_OVERLAPPED, 0, 0, 640, 480,
                        nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    require(parent.handle != nullptr);
    PluginEditorSize size;
    ok(instance.openEditor(
        {PluginWindowApi::win32, reinterpret_cast<std::uintptr_t>(parent.handle), false, 1.5},
        size));
    require(size.width == 320 && size.height == 160 &&
            GetWindow(parent.handle, GW_CHILD) != nullptr);
    size = {32, 20};
    ok(instance.resizeEditor(size));
    require(size.width == 100 && size.height == 80);
    ok(instance.scaleEditor(2));
    ok(instance.showEditor(false));
    ok(instance.showEditor(true));
    ok(instance.activate());
    std::uint32_t requests = 0;
    ok(instance.pumpMainThread(requests));
    require(host.size.width == 400 && host.size.height == 200);
    ok(instance.pumpMainThread(requests));
    require((host.requests & pluginStateDirty) != 0);
    ok(instance.deactivate());
    ok(instance.closeEditor());
    require(GetWindow(parent.handle, GW_CHILD) == nullptr);
#endif
    ok(instance.close());
    require(instance.lifecycle() == PluginLifecycle::empty);
    std::cout << "PASS plugin CLAP ABI/multibus/automation/state/editor/lifecycle/realtime\n";
}

void vstPluginRegression(const std::filesystem::path& directory) {
    using namespace cydsp;
    std::unique_ptr<PluginProvider> provider;
    ok(createPluginProvider(PluginFormat::vst3, provider));
    const auto path = std::filesystem::absolute(directory / "cydsp_vst_fixture.vst3");
    std::vector<PluginDescriptor> list;
    ok(provider->scan(path, list));
    require(list.size() == 1);
    require(list[0].id == "vst3:43a6be13274d4ef1812b472fd0a0b713");
    PluginInstance instance;
    struct Ui {
        PluginEditorSize accepted;
        std::size_t edits = 0;
    } ui;
    PluginHostCallbacks cb{
        &ui,
        [](void* p, PluginEditorSize size) noexcept {
            static_cast<Ui*>(p)->accepted = size;
            return true;
        },
        nullptr, [](void* p, const PluginEvent&) noexcept { ++static_cast<Ui*>(p)->edits; }};
    ok(provider->create(path, list[0].id, cb, instance));
    provider.reset();
    double value = 0;
    ok(instance.parameterValue(7, value));
    require(value == .25);
    require(instance.description()->parameters[0].normalized);
    std::vector<std::uint8_t> state;
    ok(instance.saveState(state));
    require(state.size() == 40);
    audio<float>(instance);
    ok(instance.loadState(state));
    audio<double>(instance);
    ok(instance.loadState(state));
    std::string text;
    ok(instance.parameterText(7, .125, text));
    require(text == "0.250");
#if defined(_WIN32)
    struct Parent {
        HWND handle = nullptr;
        ~Parent() {
            if (handle)
                DestroyWindow(handle);
        }
    } parent;
    parent.handle = CreateWindowExW(0, L"STATIC", L"CyDSP hidden VST3 parent", WS_OVERLAPPED, 0, 0,
                                    640, 480, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    require(parent.handle != nullptr);
    PluginEditorSize size;
    ok(instance.openEditor(
        {PluginWindowApi::win32, reinterpret_cast<std::uintptr_t>(parent.handle), false, 1.5},
        size));
    require(size.width == 400 && size.height == 200 && ui.accepted.width == 400 &&
            GetWindow(parent.handle, GW_CHILD) != nullptr);
    size = {32, 20};
    ok(instance.resizeEditor(size));
    require(size.width == 100 && size.height == 80);
    ok(instance.scaleEditor(2));
    ok(instance.focusEditor(true));
    ok(instance.keyEditor({std::uint16_t('a'), PluginEditorKeyCode::none, editorControl, true}));
    ok(instance.wheelEditor(1));
    require(ui.edits == 3);
    ok(instance.closeEditor());
    require(GetWindow(parent.handle, GW_CHILD) == nullptr);
#endif
    // A native UI gesture survives the audio handoff, and slow UI pumping retains
    // the final controller value beyond the former 1024-point queue capacity.
    {
        PluginPrepareOptions stress;
        stress.maximumFrames = 64;
        stress.maximumEvents = 8192;
        ok(instance.activate(stress));
        ok(instance.start());
        AudioBuffer<float> main, side, out;
        ok(main.resize(2, 64));
        ok(side.resize(1, 64));
        ok(out.resize(2, 64));
        std::array<AudioBlock<const float>, 2> inputs{main.block(), side.block()};
        std::array<AudioBlock<float>, 1> outputs{out.block()};
        PluginEventBuffer automation, recorded;
        ok(automation.prepare(4096, 0));
        ok(recorded.prepare(8192, 0));
        for (std::size_t i = 0; i < 4096; ++i) {
            PluginEvent e;
            e.kind = PluginEventKind::parameterValue;
            e.id = 7;
            e.value = double(i) / 4095.;
            ok(automation.add(e));
        }
        ok(instance.process(PluginProcess<float>{inputs, outputs, 64, &automation, &recorded}));
#if defined(_WIN32)
        require(recorded.size() >= 3);
        require(recorded.event(0)->kind == PluginEventKind::gestureBegin);
        require(recorded.event(1)->kind == PluginEventKind::parameterValue &&
                recorded.event(1)->value == .75);
        require(recorded.event(2)->kind == PluginEventKind::gestureEnd);
#endif
        ok(instance.stop());
        std::uint32_t requests = 0;
        ok(instance.pumpMainThread(requests));
        ok(instance.parameterValue(7, value));
        require(value == 1);
        ok(instance.deactivate());
        ok(instance.loadState(state));
    }
    // Explicit rejection rather than dropping a CLAP-only event in VST3.
    PluginPrepareOptions options;
    options.maximumFrames = 64;
    ok(instance.activate(options));
    ok(instance.start());
    AudioBuffer<float> main, side, out;
    ok(main.resize(2, 64));
    ok(side.resize(1, 64));
    ok(out.resize(2, 64));
    std::array<AudioBlock<const float>, 2> inputs{main.block(), side.block()};
    std::array<AudioBlock<float>, 1> outputs{out.block()};
    PluginEventBuffer events;
    ok(events.prepare());
    PluginEvent event;
    event.kind = PluginEventKind::noteChoke;
    event.port = 0;
    ok(events.add(event));
    require(instance.process(PluginProcess<float>{inputs, outputs, 64, &events}) ==
            Status::unsupported_format);
    ok(instance.stop());
    ok(instance.deactivate());
    ok(instance.close());
    std::cout << "PASS plugin VST3 ABI/controller/messages/multibus/state/editor/realtime\n";
}
