#include <algorithm>
#include <array>
#include <chrono>
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
#if !defined(__ANDROID__)
namespace {
using namespace cydsp;
void require(bool v, const std::source_location where = std::source_location::current()) {
    if (!v)
        throw std::runtime_error("plugin worker line " + std::to_string(where.line()));
}
void ok(Status r, const std::source_location where = std::source_location::current()) {
    require(r == Status::ok, where);
}
template <class T> void run(const std::filesystem::path& directory) {
    const auto executable = std::filesystem::absolute(directory /
#if defined(_WIN32)
                                                      "cydsp_plugin_worker_fixture.exe"
#else
                                                      "cydsp_plugin_worker_fixture"
#endif
    );
    const auto path = std::filesystem::absolute(directory / "cydsp_fixture.clap");
    PluginWorker worker;
    PluginWorkerOptions options;
    options.processing.maximumFrames = 64;
    options.processing.useFloat64 = std::same_as<T, double>;
    ok(worker.launch(executable, PluginFormat::clap, path, "org.cydsp.fixture", options));
    require(worker.info()->inputChannels == std::vector<std::uint32_t>{2, 1});
    require(worker.info()->pipelineLatencyFrames == 64 &&
            worker.info()->pluginTiming.latencyFrames == 17);
    AudioBuffer<T> main, side, out;
    ok(main.resize(2, 64));
    ok(side.resize(1, 64));
    ok(out.resize(2, 64));
    for (std::size_t ch = 0; ch < 2; ++ch)
        std::fill(main.block().channel(ch).begin(), main.block().channel(ch).end(), T{2});
    std::fill(side.block().channel(0).begin(), side.block().channel(0).end(), T{.25});
    std::array<AudioBlock<const T>, 2> inputs{main.block(), side.block()};
    std::array<AudioBlock<T>, 1> outputs{out.block()};
    PluginEventBuffer events, result;
    ok(events.prepare(64, 16000));
    ok(result.prepare(64, 16000));
    std::vector<std::uint8_t> sysex(12000, 0x12);
    sysex.front() = 0xf0;
    sysex.back() = 0xf7;
    PluginEvent event;
    event.kind = PluginEventKind::sysex;
    event.port = 0;
    event.bytes = sysex;
    ok(events.add(event));
    PluginProcess<T> call{inputs, outputs, 64, &events, &result};
    PluginWorkerBlockResult observed;
    ok(worker.process(call, observed));
    require(observed.submitted && !observed.received);
    require(out.block().channel(0)[0] == 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    struct Real {
        PluginWorker* worker;
        PluginProcess<T>* call;
        PluginWorkerBlockResult result{};
        Status status = Status::not_prepared;
    } real{&worker, &call};
    assertNoRealtimeAllocations(
        [](void* p) noexcept {
            auto& r = *static_cast<Real*>(p);
            r.status = r.worker->process(*r.call, r.result);
        },
        &real);
    ok(real.status);
    require(real.result.received && real.result.receivedSequence == 1 &&
            out.block().channel(0)[0] == T{1.25});
    require(result.size() == 1 && result.event(0)->bytes.size() == 12000);
    std::vector<std::uint8_t> state;
    ok(worker.saveState(state));
    require(state.size() == 8);
    ok(worker.loadState(state));
#if defined(_WIN32)
    struct Parent {
        HWND h = nullptr;
        ~Parent() {
            if (h)
                DestroyWindow(h);
        }
    } parent;
    parent.h = CreateWindowExW(0, L"STATIC", L"CyDSP hidden worker parent", WS_OVERLAPPED, 0, 0,
                               640, 480, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    require(parent.h != nullptr);
    PluginEditorSize size;
    ok(worker.openEditor(
        {PluginWindowApi::win32, reinterpret_cast<std::uintptr_t>(parent.h), false, 1}, size));
    require(size.width == 320 && GetWindow(parent.h, GW_CHILD) != nullptr);
    size = {250, 150};
    ok(worker.resizeEditor(size));
    ok(worker.scaleEditor(2));
    ok(worker.showEditor(false));
    ok(worker.showEditor(true));
    ok(worker.closeEditor());
    require(GetWindow(parent.h, GW_CHILD) == nullptr);
#endif
    // Controlled slow foreign plugin proves mailbox saturation never waits.
    events.reset();
    const std::uint8_t delayed[]{0xf0, 0x7d, 0x44, 0xf7};
    event.bytes = delayed;
    ok(events.add(event));
    ok(worker.process(call, observed));
    const auto started = std::chrono::steady_clock::now();
    const auto pending = worker.process(call, observed);
    require(pending == Status::would_block || pending == Status::ok);
    if (pending == Status::would_block)
        require(observed.missedDeadline && !observed.received && out.block().channel(0)[0] == 0);
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                             std::chrono::steady_clock::now() - started)
                             .count();
    std::cout << "Worker nonblocking mailbox call us=" << elapsed
              << " missed=" << observed.missedDeadline << "\n";
    ok(worker.saveState(state)); // Control may wait; audio may not.
    events.reset();
    event = {};
    event.kind = PluginEventKind::noteOn;
    event.port = 0;
    event.channel = 15;
    event.key = 127;
    event.value = 1;
    ok(events.add(event));
    ok(worker.process(call, observed)); // The fixture exits its OWN worker process.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (worker.poll() == Status::ok && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    require(worker.poll() == Status::disconnected);
    require(worker.process(call, observed) == Status::disconnected &&
            out.block().channel(0)[0] == 0);
    ok(worker.launch(executable, PluginFormat::clap, path, "org.cydsp.fixture", options));
    ok(worker.loadState(state));
    events.reset();
    ok(worker.process(call, observed));
    require(observed.submitted && !observed.received);
    worker.close();
}
#if defined(_WIN32)
void vstWorker(const std::filesystem::path& directory) {
    PluginWorker worker;
    PluginWorkerOptions options;
    options.processing.maximumFrames = 64;
    ok(worker.launch(std::filesystem::absolute(directory / "cydsp_plugin_worker_fixture.exe"),
                     PluginFormat::vst3,
                     std::filesystem::absolute(directory / "cydsp_vst_fixture.vst3"),
                     "vst3:43a6be13274d4ef1812b472fd0a0b713", options));
    struct Parent {
        HWND h = nullptr;
        ~Parent() {
            if (h)
                DestroyWindow(h);
        }
    } parent;
    parent.h = CreateWindowExW(0, L"STATIC", L"CyDSP hidden VST3 worker", WS_OVERLAPPED, 0, 0, 640,
                               480, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    require(parent.h != nullptr);
    PluginEditorSize size;
    ok(worker.openEditor(
        {PluginWindowApi::win32, reinterpret_cast<std::uintptr_t>(parent.h), false, 1}, size));
    const auto requested = worker.requestedEditorSize();
    require(requested.width == 400 && requested.height == 200);
    size = {400, 200};
    ok(worker.resizeEditor(size));
    ok(worker.scaleEditor(2));
    ok(worker.focusEditor(true));
    ok(worker.keyEditor({std::uint16_t('a'), PluginEditorKeyCode::none, editorControl, true}));
    ok(worker.wheelEditor(1));
    AudioBuffer<float> main, side, out;
    ok(main.resize(2, 64));
    ok(side.resize(1, 64));
    ok(out.resize(2, 64));
    std::array<AudioBlock<const float>, 2> inputs{main.block(), side.block()};
    std::array<AudioBlock<float>, 1> outputs{out.block()};
    PluginEventBuffer recorded;
    ok(recorded.prepare());
    PluginProcess<float> call{inputs, outputs, 64, nullptr, &recorded};
    PluginWorkerBlockResult observed;
    ok(worker.process(call, observed));
    require(observed.submitted);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    ok(worker.process(call, observed));
    require(observed.received && recorded.size() >= 3);
    require(recorded.event(0)->kind == PluginEventKind::gestureBegin);
    require(recorded.event(1)->kind == PluginEventKind::parameterValue &&
            recorded.event(1)->value == .75);
    require(recorded.event(2)->kind == PluginEventKind::gestureEnd);
    require(GetWindow(parent.h, GW_CHILD) != nullptr);
    ok(worker.closeEditor());
    require(GetWindow(parent.h, GW_CHILD) == nullptr);
    std::vector<std::uint8_t> state;
    ok(worker.saveState(state));
    ok(worker.loadState(state));
    worker.close();
}
#endif
} // namespace
#endif
void pluginWorkerRegression(const std::filesystem::path& directory) {
#if !defined(__ANDROID__)
    run<float>(directory);
    run<double>(directory);
#if defined(_WIN32)
    vstWorker(directory);
#endif
    std::cout
        << "PASS plugin separate process/IPC/state/editor/late blocks/crash/restart/realtime\n";
#else
    (void)directory;
    std::cout << "SKIP plugin worker process: Android application service provider required\n";
#endif
}
