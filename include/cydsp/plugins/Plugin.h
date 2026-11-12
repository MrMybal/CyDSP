#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/midi/Ump.h"
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
namespace cydsp {
enum class PluginFormat { clap, vst3, audioUnit, aax, lv2, jsfx, custom };
enum class PluginWindowApi { win32, cocoa, x11, wayland };
struct PluginWindow {
    PluginWindowApi api = PluginWindowApi::win32;
    std::uintptr_t handle = 0;
    bool floating = false;
    double scale = 1;
};
enum class PluginEditorKeyCode : std::uint16_t {
    none,
    backspace,
    tab,
    enter,
    escape,
    space,
    left,
    up,
    right,
    down,
    pageUp,
    pageDown,
    home,
    end,
    insert,
    deleteKey,
    f1,
    f2,
    f3,
    f4,
    f5,
    f6,
    f7,
    f8,
    f9,
    f10,
    f11,
    f12,
    f13,
    f14,
    f15,
    f16,
    f17,
    f18,
    f19,
    f20,
    f21,
    f22,
    f23,
    f24
};
enum PluginEditorModifier : std::uint16_t {
    editorShift = 1,
    editorAlt = 2,
    editorControl = 4,
    editorSuper = 8
};
struct PluginEditorKey {
    std::uint16_t character = 0; // UTF-16 code unit; surrogate pairs arrive as separate events.
    PluginEditorKeyCode key = PluginEditorKeyCode::none;
    std::uint16_t modifiers = 0;
    bool down = true;
};
struct PluginEditorSize {
    std::uint32_t width = 0, height = 0;
};
struct PluginDescriptor {
    PluginFormat format = PluginFormat::custom;
    std::string id, name, vendor, version, description;
};
struct PluginAudioPort {
    std::uint32_t id = 0, channels = 0, inPlacePair = 0xffffffffu;
    std::string name, type;
    bool main = false, supports64 = false;
};
struct PluginNotePort {
    std::uint32_t id = 0, dialects = 0, preferredDialect = 0;
    std::string name;
};
enum PluginParameterTarget : std::uint32_t {
    parameterNoteId = 1,
    parameterPort = 2,
    parameterChannel = 4,
    parameterKey = 8
};
struct PluginParameter {
    std::uint32_t id = 0;
    std::string name, module;
    double minimum = 0, maximum = 1, defaultValue = 0;
    bool stepped = false, readOnly = false, automatable = false, modulatable = false,
         hidden = false;
    bool normalized = false; // VST3 values use [0,1]; CLAP keeps plain native values.
    std::uint32_t automationTargets = 0,
                  modulationTargets = 0; // Per-note/port/channel/key support.
    std::string units{};
};
struct PluginDescription {
    PluginDescriptor plugin;
    std::vector<PluginAudioPort> inputs, outputs;
    std::vector<PluginNotePort> noteInputs, noteOutputs;
    std::vector<PluginParameter> parameters;
    bool state = false, editor = false, float32 = true, float64 = false;
};
struct PluginTiming {
    std::uint32_t latencyFrames = 0, tailFrames = 0;
    bool latencyKnown = false, tailKnown = false, infiniteTail = false;
};
struct PluginTransport {
    std::int64_t songFrame = 0;
    double beats = 0, seconds = 0, tempo = 120, tempoIncrement = 0;
    double loopStartBeats = 0, loopEndBeats = 0, loopStartSeconds = 0, loopEndSeconds = 0;
    double barStart = 0;
    std::int32_t barNumber = 0;
    std::uint16_t numerator = 4, denominator = 4;
    bool hasSongFrame = false, hasBeats = false, hasSeconds = false, hasTempo = false,
         hasSignature = false, playing = false, recording = false, looping = false, preRoll = false;
};
enum class PluginEventKind {
    noteOn,
    noteOff,
    noteChoke,
    noteEnd,
    noteExpression,
    parameterValue,
    parameterModulation,
    gestureBegin,
    gestureEnd,
    transport,
    midi1,
    sysex,
    ump
};
struct PluginEvent {
    PluginEventKind kind = PluginEventKind::parameterValue;
    std::uint32_t frame = 0, id = 0, flags = 0;
    std::int32_t noteId = -1;
    std::int16_t port = -1, channel = -1, key = -1; // Channel 0..15; -1 wildcard.
    double value = 0;
    std::span<const std::uint8_t> bytes; // MIDI/SysEx, copied by add().
    UmpPacket packet;
    PluginTransport time;
};
// Stable ordered events and copied variable payload, prepared on control thread.
// add/reset/event are allocation-free; never retains foreign plugin payload pointers.
class PluginEventBuffer {
  public:
    Status prepare(std::size_t maximumEvents = 1024, std::size_t maximumBytes = 65536) noexcept;
    Status add(const PluginEvent&) noexcept;
    void reset() noexcept {
        count_ = used_ = 0;
    }
    [[nodiscard]] std::size_t size() const noexcept {
        return count_;
    }
    [[nodiscard]] const PluginEvent* event(std::size_t index) const noexcept;

  private:
    std::vector<PluginEvent> events_;
    std::vector<std::uint8_t> bytes_;
    std::size_t count_ = 0, used_ = 0;
};
struct PluginPrepareOptions {
    double sampleRate = 48000;
    std::uint32_t minimumFrames = 1, maximumFrames = 512, maximumEvents = 1024;
    bool useFloat64 = false, offline = false;
};
template <Sample T> struct PluginProcess {
    std::span<const AudioBlock<const T>> inputs;
    std::span<const AudioBlock<T>> outputs;
    std::uint32_t frames = 0;
    const PluginEventBuffer* inputEvents = nullptr;
    PluginEventBuffer* outputEvents = nullptr; // Reset by instance before process.
    const PluginTransport* transport = nullptr;
    std::int64_t steadyFrame = -1;
};
enum PluginRequest : std::uint32_t {
    pluginRestart = 1,
    pluginProcess = 2,
    pluginMainThread = 4,
    pluginParametersChanged = 8,
    pluginPortsChanged = 16,
    pluginLatencyChanged = 32,
    pluginTailChanged = 64,
    pluginStateDirty = 128,
    pluginEditorClosed = 256,
    pluginEditorShow = 512,
    pluginEditorHide = 1024,
    pluginEditorResize = 2048
};
struct PluginHostCallbacks {
    void* context = nullptr;
    // Called only from pumpMainThread()/main editor methods. Resize callback must
    // synchronously resize the parent's client area before accepting.
    bool (*resizeEditor)(void*, PluginEditorSize) noexcept = nullptr;
    void (*requests)(void*, std::uint32_t flags) noexcept = nullptr;
    // Optional native controller edits on UI. Audio/worker recording receives
    // the same live gestures/values through PluginProcess::outputEvents.
    void (*edited)(void*, const PluginEvent&) noexcept = nullptr;
};
// Original provider boundary for separately supplied licensed/native formats.
// Control/UI functions run on the creating UI thread; start/stop/reset/process
// share a single serialized symbolic audio thread. The application owns handoff.
// A backend is in-process; crash isolation requires the separate worker service.
class PluginBackend {
  public:
    virtual ~PluginBackend() = default;
    [[nodiscard]] virtual const PluginDescription& description() const noexcept = 0;
    virtual Status activate(const PluginPrepareOptions&) noexcept = 0;
    virtual Status deactivate() noexcept = 0;
    virtual Status start() noexcept = 0;
    virtual void stop() noexcept = 0;
    virtual Status reset() noexcept = 0;
    virtual Status process(const PluginProcess<float>&) noexcept = 0;
    virtual Status process(const PluginProcess<double>&) noexcept = 0;
    virtual Status refresh() noexcept {
        return Status::ok;
    }
    virtual PluginTiming timing() const noexcept {
        return {};
    }
    virtual Status parameterValue(std::uint32_t, double&) noexcept {
        return Status::unsupported_format;
    }
    virtual Status parameterText(std::uint32_t, double, std::string&) noexcept {
        return Status::unsupported_format;
    }
    virtual Status flush(const PluginEventBuffer&, PluginEventBuffer&) noexcept {
        return Status::unsupported_format;
    }
    virtual Status saveState(std::vector<std::uint8_t>&, std::size_t) noexcept {
        return Status::unsupported_format;
    }
    virtual Status loadState(std::span<const std::uint8_t>) noexcept {
        return Status::unsupported_format;
    }
    virtual Status openEditor(const PluginWindow&, PluginEditorSize&) noexcept {
        return Status::unsupported_format;
    }
    virtual Status resizeEditor(PluginEditorSize&) noexcept {
        return Status::unsupported_format;
    }
    virtual Status scaleEditor(double) noexcept {
        return Status::unsupported_format;
    }
    virtual Status showEditor(bool) noexcept {
        return Status::unsupported_format;
    }
    virtual Status focusEditor(bool) noexcept {
        return Status::unsupported_format;
    }
    virtual Status keyEditor(const PluginEditorKey&) noexcept {
        return Status::unsupported_format;
    }
    virtual Status wheelEditor(float) noexcept {
        return Status::unsupported_format;
    }
    virtual void closeEditor() noexcept {}
    virtual std::uint32_t pumpMainThread() noexcept {
        return 0;
    }
};
enum class PluginLifecycle { empty, initialized, active, processing };
// Enforces lifecycle, bus sizes, prepared precision, event offsets and UI affinity.
// destroy/move/control require audio owner idle; stop() must precede deactivate().
// The foreign plugin's own realtime behavior is outside CyDSP's control.
class PluginInstance {
  public:
    PluginInstance();
    ~PluginInstance();
    PluginInstance(PluginInstance&&) noexcept;
    PluginInstance& operator=(PluginInstance&&) noexcept;
    static Status adopt(std::unique_ptr<PluginBackend>, PluginInstance&) noexcept;
    const PluginDescription* description() const noexcept;
    PluginLifecycle lifecycle() const noexcept;
    Status activate(const PluginPrepareOptions& = {}) noexcept;
    Status start() noexcept;
    Status process(const PluginProcess<float>&) noexcept;
    Status process(const PluginProcess<double>&) noexcept;
    Status stop() noexcept;
    Status reset() noexcept;
    Status deactivate() noexcept;
    Status refresh() noexcept; // Inactive; refreshes ports/parameters after restart/rescan.
    Status parameterValue(std::uint32_t, double&) noexcept; // Main, not concurrently processing.
    Status parameterText(std::uint32_t, double, std::string&) noexcept;
    Status flush(const PluginEventBuffer&, PluginEventBuffer&) noexcept; // Inactive main only.
    Status saveState(std::vector<std::uint8_t>&,
                     std::size_t maximumBytes = 64 * 1024 * 1024) noexcept;
    Status loadState(std::span<const std::uint8_t>) noexcept; // Inactive, bounded by 64 MiB.
    PluginTiming timing() const noexcept;                     // Cached during activate/pump.
    Status openEditor(const PluginWindow&, PluginEditorSize&) noexcept;
    Status resizeEditor(PluginEditorSize&) noexcept;
    Status scaleEditor(double) noexcept;
    Status showEditor(bool) noexcept;
    Status focusEditor(bool) noexcept;
    Status keyEditor(const PluginEditorKey&) noexcept;
    Status wheelEditor(float distance) noexcept;
    Status closeEditor() noexcept;
    Status pumpMainThread(std::uint32_t& requests) noexcept;
    Status close() noexcept; // Main + audio idle, processing must already be stopped.
  private:
    struct State;
    std::unique_ptr<State> state_;
    template <Sample T> Status processImpl(const PluginProcess<T>&) noexcept;
};
class PluginProvider {
  public:
    virtual ~PluginProvider() = default;
    virtual Status scan(const std::filesystem::path&, std::vector<PluginDescriptor>&) noexcept = 0;
    virtual Status create(const std::filesystem::path&, std::string_view id,
                          const PluginHostCallbacks&, PluginInstance&) noexcept = 0;
};
// Creates an actual built-in adapter when compiled/supported. Never substitutes formats.
Status createPluginProvider(PluginFormat, std::unique_ptr<PluginProvider>&) noexcept;
} // namespace cydsp
