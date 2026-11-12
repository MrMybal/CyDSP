// Original VST3 host; MIT SDK interfaces: third_party/licenses/vst3-interfaces.txt
#include "VstMessages.h"
#include "cydsp/core/SpscQueue.h"
#include "pluginterfaces/base/keycodes.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include <array>
#include <bit>
#include <mutex>
namespace cydsp::detail {
namespace {
using namespace vst;
class HostApplication final : public IHostApplication {
  public:
    CYDSP_VST_BORROWED(IHostApplication)
    tresult PLUGIN_API getName(String128 name) override {
        if (!name)
            return kInvalidArgument;
        std::fill_n(name, 128, char16{});
        constexpr char text[] = "CyDSP";
        for (std::size_t i = 0; i < sizeof(text); ++i)
            name[i] = char16(text[i]);
        return kResultOk;
    }
    tresult PLUGIN_API createInstance(TUID cid, TUID requestedInterface, void** out) override {
        if (!out)
            return kInvalidArgument;
        *out = nullptr;
        try {
            if (equal(cid, IMessage_iid) && (equal(requestedInterface, IMessage_iid) ||
                                             equal(requestedInterface, FUnknown_iid))) {
                *out = static_cast<IMessage*>(new Message);
                return kResultOk;
            }
            if (equal(cid, IAttributeList_iid) && (equal(requestedInterface, IAttributeList_iid) ||
                                                   equal(requestedInterface, FUnknown_iid))) {
                *out = static_cast<IAttributeList*>(new Attributes);
                return kResultOk;
            }
            return kNoInterface;
        } catch (...) {
            return kOutOfMemory;
        }
    }
};
std::recursive_mutex& vstModuleMutex() {
    static std::recursive_mutex mutex;
    return mutex;
}
struct VstModule {
    HostApplication host;
    SharedModule library;
    IPluginFactory* factory = nullptr;
    bool (*exit)() = nullptr;
    bool initialized = false;
    ~VstModule() {
        std::lock_guard lock(vstModuleMutex());
        if (factory)
            factory->release();
        if (initialized && exit)
            (void)exit();
    }
};
std::filesystem::path binaryPath(const std::filesystem::path& path) {
    if (!std::filesystem::is_directory(path))
        return path;
#if defined(_WIN32)
#if defined(_M_ARM64) || defined(__aarch64__)
    constexpr auto architecture = "arm64-win";
#elif defined(_M_IX86) || defined(__i386__)
    constexpr auto architecture = "x86-win";
#else
    constexpr auto architecture = "x86_64-win";
#endif
    return path / "Contents" / architecture / (path.stem().string() + ".vst3");
#elif defined(__APPLE__)
    return path;
#else
#if defined(__aarch64__)
    constexpr auto architecture = "aarch64-linux";
#elif defined(__arm__)
    constexpr auto architecture = "arm-linux";
#elif defined(__i386__)
    constexpr auto architecture = "i386-linux";
#else
    constexpr auto architecture = "x86_64-linux";
#endif
    return path / "Contents" / architecture / (path.stem().string() + ".so");
#endif
}
Status loadVst(const std::filesystem::path& path, std::shared_ptr<VstModule>& out) {
    std::error_code ec;
    const auto file = std::filesystem::canonical(binaryPath(path), ec);
    if (ec || !file.is_absolute())
        return Status::io_error;
    struct Record {
        std::filesystem::path path;
        std::weak_ptr<VstModule> module;
    };
    static std::vector<Record> registry;
    std::unique_lock lock(vstModuleMutex());
    for (auto it = registry.begin(); it != registry.end();) {
        if (auto module = it->module.lock()) {
            if (it->path == file) {
                out = std::move(module);
                return Status::ok;
            }
            ++it;
        } else
            it = registry.erase(it);
    }
    auto module = std::make_shared<VstModule>();
    auto r = module->library.open(file);
    if (r == Status::ok) {
#if defined(_WIN32)
        auto init = reinterpret_cast<bool (*)()>(module->library.symbol("InitDll"));
        module->exit = reinterpret_cast<bool (*)()>(module->library.symbol("ExitDll"));
        module->initialized = !init || init();
#elif defined(__APPLE__)
        auto init = reinterpret_cast<bool (*)(void*)>(module->library.symbol("bundleEntry"));
        module->exit = reinterpret_cast<bool (*)()>(module->library.symbol("bundleExit"));
        module->initialized = init && module->exit && init(module->library.nativeHandle());
#else
        auto init = reinterpret_cast<bool (*)(void*)>(module->library.symbol("ModuleEntry"));
        module->exit = reinterpret_cast<bool (*)()>(module->library.symbol("ModuleExit"));
        module->initialized = init && module->exit && init(module->library.nativeHandle());
#endif
        if (!module->initialized)
            r = Status::io_error;
        else {
            const auto getFactory = reinterpret_cast<IPluginFactory*(PLUGIN_API*)()>(
                module->library.symbol("GetPluginFactory"));
            module->factory = getFactory ? getFactory() : nullptr;
            if (!module->factory)
                r = Status::unsupported_format;
            else if (auto* factory3 =
                         query<IPluginFactory3>(module->factory, IPluginFactory3_iid)) {
                factory3->setHostContext(&module->host);
                factory3->release();
            }
        }
    }
    if (r == Status::ok) {
        registry.push_back({file, module});
        out = module;
    }
    lock.unlock();
    return r;
}
PluginDescriptor classDescriptor(VstModule& module, int32 index) {
    PClassInfo info;
    if (module.factory->getClassInfo(index, &info) != kResultOk ||
        string(info.category) != kVstAudioEffectClass)
        return {};
    PFactoryInfo factoryInfo{};
    module.factory->getFactoryInfo(&factoryInfo);
    PluginDescriptor out{PluginFormat::vst3,
                         identifier(info.cid),
                         string(info.name),
                         string(factoryInfo.vendor),
                         {},
                         {}};
    if (auto* f = query<IPluginFactory2>(module.factory, IPluginFactory2_iid)) {
        PClassInfo2 info2;
        if (f->getClassInfo2(index, &info2) == kResultOk) {
            out.version = string(info2.version);
            const auto vendor = string(info2.vendor);
            if (!vendor.empty())
                out.vendor = vendor;
            out.description = string(info2.subCategories);
        }
        f->release();
    }
    return out;
}
struct UiEdit {
    PluginEvent event;
    bool native = false;
};
class VstBackend final : public PluginBackend, public IComponentHandler, public IPlugFrame {
    std::shared_ptr<VstModule> module_;
    IComponent* component_ = nullptr;
    IAudioProcessor* processor_ = nullptr;
    IEditController* controller_ = nullptr;
    IConnectionPoint* componentConnection_ = nullptr;
    IConnectionPoint* controllerConnection_ = nullptr;
    IPlugView* view_ = nullptr;
    PluginDescription description_;
    PluginHostCallbacks callbacks_;
    std::thread::id main_ = std::this_thread::get_id();
    std::atomic<uint32> references_{1}, requests_{0};
    SpscQueue<UiEdit> uiToAudio_;
    struct ParameterMirror {
        std::atomic<std::uint32_t> sequence{0}, low{0}, high{0}, dirty{0};
    };
    std::unique_ptr<ParameterMirror[]> mirrors_;
    std::array<PluginEvent, 1024> uiBlock_;
    std::size_t uiBlockCount_ = 0;
    void mirror(std::uint32_t id, double value) noexcept {
        const auto* p = parameter(id);
        if (!p)
            return;
        auto& m = mirrors_[std::size_t(p - description_.parameters.data())];
        const auto bits = std::bit_cast<std::uint64_t>(value);
        m.sequence.fetch_add(1, std::memory_order_acq_rel);
        m.low.store(std::uint32_t(bits), std::memory_order_release);
        m.high.store(std::uint32_t(bits >> 32), std::memory_order_release);
        m.sequence.fetch_add(1, std::memory_order_release);
        m.dirty.store(1, std::memory_order_release);
    }
    ParameterChanges incoming_, outgoing_;
    EventList incomingEvents_, outgoingEvents_;
    std::vector<AudioBusBuffers> inputBuses_, outputBuses_;
    std::vector<float*> pointers32_;
    std::vector<double*> pointers64_;
    struct Merged {
        PluginEvent event;
        std::size_t order = 0;
    };
    std::vector<Merged> merged_;
    std::vector<std::uint32_t> midiAssignments_;
    std::size_t mergedCount_ = 0;
    PluginPrepareOptions options_;
    bool initialized_ = false, controllerInitialized_ = false, active_ = false, running_ = false,
         attached_ = false;
    bool onMain() const noexcept {
        return main_ == std::this_thread::get_id() && currentAudioPlugin != this;
    }
    void request(std::uint32_t flags) noexcept {
        requests_.fetch_or(flags, std::memory_order_relaxed);
    }
    const PluginParameter* parameter(std::uint32_t id) const noexcept {
        const auto it =
            std::lower_bound(description_.parameters.begin(), description_.parameters.end(), id,
                             [](const auto& p, auto v) { return p.id < v; });
        return it != description_.parameters.end() && it->id == id ? &*it : nullptr;
    }
    Status uiEvent(PluginEventKind kind, std::uint32_t id, double value = 0,
                   bool native = true) noexcept {
        const auto* p = parameter(id);
        if (!onMain() || !p || p->readOnly || !isFinite(value) || value < 0 || value > 1)
            return Status::invalid_argument;
        PluginEvent e;
        e.kind = kind;
        e.id = id;
        e.value = value;
        e.flags = native ? 1u : 0u;
        if (!uiToAudio_.push({e, native}))
            return Status::update_queue_full;
        if (native && callbacks_.edited)
            callbacks_.edited(callbacks_.context, e);
        request(pluginProcess | pluginStateDirty);
        return Status::ok;
    }
    Status addInput(const PluginEvent& e) noexcept {
        if (e.kind == PluginEventKind::parameterValue) {
            const auto* p = parameter(e.id);
            if (!p || p->readOnly || e.noteId != -1 || e.channel != -1 || e.key != -1 ||
                e.port != -1)
                return Status::unsupported_format;
            const auto r = incoming_.add(e.id, e.frame, e.value);
            if (r != Status::ok)
                return r;
            mirror(e.id, e.value);
            return Status::ok;
        }
        if (e.kind == PluginEventKind::gestureBegin || e.kind == PluginEventKind::gestureEnd)
            return parameter(e.id) ? Status::ok : Status::invalid_argument;
        if (e.kind == PluginEventKind::transport)
            return Status::unsupported_format;
        Event native{};
        native.busIndex = e.port;
        native.sampleOffset = int32(e.frame);
        native.flags = (e.flags & 1) ? Event::kIsLive : 0;
        if (e.port < 0 || std::size_t(e.port) >= description_.noteInputs.size())
            return Status::invalid_argument;
        if (e.kind == PluginEventKind::noteOn) {
            native.type = Event::kNoteOnEvent;
            native.noteOn = {e.channel, e.key, 0, float(e.value), 0, e.noteId};
        } else if (e.kind == PluginEventKind::noteOff) {
            native.type = Event::kNoteOffEvent;
            native.noteOff = {e.channel, e.key, float(e.value), e.noteId, 0};
        } else if (e.kind == PluginEventKind::sysex) {
            native.type = Event::kDataEvent;
            native.data = {uint32(e.bytes.size()), DataEvent::kMidiSysEx, e.bytes.data()};
        } else if (e.kind == PluginEventKind::midi1) {
            MidiMessageView midi(e.bytes);
            if (midi.noteOn()) {
                native.type = Event::kNoteOnEvent;
                native.noteOn = {int16(midi.channel() - 1),
                                 int16(midi.note()),
                                 0,
                                 float(e.bytes[2]) / 127,
                                 0,
                                 -1};
            } else if (midi.noteOff()) {
                native.type = Event::kNoteOffEvent;
                native.noteOff = {int16(midi.channel() - 1), int16(midi.note()),
                                  float(e.bytes[2]) / 127, -1, 0};
            } else {
                const auto status = e.bytes[0] & 0xf0;
                const auto channel = unsigned(e.bytes[0] & 15);
                unsigned control = 0;
                double value = 0;
                if (status == 0xb0) {
                    control = e.bytes[1];
                    value = double(e.bytes[2]) / 127;
                } else if (status == 0xd0) {
                    control = 128;
                    value = double(e.bytes[1]) / 127;
                } else if (status == 0xe0) {
                    control = 129;
                    value = double(e.bytes[1] | (unsigned(e.bytes[2]) << 7)) / 16383;
                } else if (status == 0xa0) {
                    native.type = Event::kPolyPressureEvent;
                    native.polyPressure = {int16(channel), int16(e.bytes[1]),
                                           float(e.bytes[2]) / 127, -1};
                    return incomingEvents_.addEvent(native) == kResultOk
                               ? Status::ok
                               : Status::update_queue_full;
                } else
                    return Status::unsupported_format;
                const auto assignment =
                    midiAssignments_[(std::size_t(e.port) * 16 + channel) * 130 + control];
                return assignment != 0xffffffffu ? incoming_.add(assignment, e.frame, value)
                                                 : Status::unsupported_format;
            }
        } else
            return Status::unsupported_format;
        return incomingEvents_.addEvent(native) == kResultOk ? Status::ok
                                                             : Status::update_queue_full;
    }
    Status collectOutput(PluginEventBuffer* out) noexcept {
        mergedCount_ = 0;
        const auto append = [&](const PluginEvent& e) {
            if (mergedCount_ == merged_.size())
                return false;
            merged_[mergedCount_] = {e, mergedCount_};
            ++mergedCount_;
            return true;
        };
        for (std::size_t i = 0; i < uiBlockCount_; ++i)
            if (!append(uiBlock_[i]))
                return Status::update_queue_full;
        for (std::uint32_t i = 0; i < outgoing_.used; ++i) {
            const auto& point = outgoing_.points[i];
            PluginEvent e;
            e.id = point.id;
            e.frame = point.frame;
            e.value = point.value;
            if (!append(e))
                return Status::update_queue_full;
            mirror(e.id, e.value);
        }
        for (std::uint32_t i = 0; i < outgoingEvents_.count; ++i) {
            const auto& native = outgoingEvents_.entries[i];
            PluginEvent e;
            e.frame = std::uint32_t(native.sampleOffset);
            e.port = std::int16_t(native.busIndex);
            e.flags = (native.flags & Event::kIsLive) ? 1u : 0u;
            if (native.type == Event::kNoteOnEvent) {
                e.kind = PluginEventKind::noteOn;
                e.channel = native.noteOn.channel;
                e.key = native.noteOn.pitch;
                e.noteId = native.noteOn.noteId;
                e.value = native.noteOn.velocity;
            } else if (native.type == Event::kNoteOffEvent) {
                e.kind = PluginEventKind::noteOff;
                e.channel = native.noteOff.channel;
                e.key = native.noteOff.pitch;
                e.noteId = native.noteOff.noteId;
                e.value = native.noteOff.velocity;
            } else if (native.type == Event::kDataEvent) {
                e.kind = PluginEventKind::sysex;
                e.bytes = {native.data.bytes, native.data.size};
            } else
                return Status::unsupported_format;
            if (!append(e))
                return Status::update_queue_full;
        }
        std::sort(merged_.begin(), merged_.begin() + std::ptrdiff_t(mergedCount_),
                  [](const auto& a, const auto& b) {
                      return a.event.frame < b.event.frame ||
                             (a.event.frame == b.event.frame && a.order < b.order);
                  });
        if (out)
            for (std::size_t i = 0; i < mergedCount_; ++i) {
                const auto r = out->add(merged_[i].event);
                if (r != Status::ok)
                    return r;
            }
        return Status::ok;
    }
    template <Sample T> Status processing(const PluginProcess<T>& p) noexcept {
        incoming_.reset(p.frames);
        outgoing_.reset(p.frames);
        incomingEvents_.reset(p.frames);
        outgoingEvents_.reset(p.frames);
        std::size_t uiCount = 0;
        uiBlockCount_ = 0;
        while (const auto* edit = uiToAudio_.peek()) {
            if (uiCount++ >= std::min<std::size_t>(options_.maximumEvents, uiBlock_.size()))
                break;
            const auto r = addInput(edit->event);
            if (r != Status::ok)
                return r;
            if (edit->native)
                uiBlock_[uiBlockCount_++] = edit->event;
            (void)uiToAudio_.discard();
        }
        if (p.inputEvents)
            for (std::size_t i = 0; i < p.inputEvents->size(); ++i) {
                const auto r = addInput(*p.inputEvents->event(i));
                if (r != Status::ok)
                    return r;
            }
        std::size_t offset = 0;
        const auto buffers = [&](const auto& blocks, auto& buses) {
            for (std::size_t i = 0; i < blocks.size(); ++i) {
                auto& b = buses[i];
                b.numChannels = int32(blocks[i].channels());
                b.silenceFlags = 0;
                for (std::size_t ch = 0; ch < blocks[i].channels(); ++ch)
                    if constexpr (std::same_as<T, float>)
                        pointers32_[offset + ch] = const_cast<T*>(blocks[i].channel(ch).data());
                    else
                        pointers64_[offset + ch] = const_cast<T*>(blocks[i].channel(ch).data());
                if constexpr (std::same_as<T, float>)
                    b.channelBuffers32 = pointers32_.data() + offset;
                else
                    b.channelBuffers64 = pointers64_.data() + offset;
                offset += blocks[i].channels();
            }
        };
        buffers(p.inputs, inputBuses_);
        buffers(p.outputs, outputBuses_);
        ProcessContext context{};
        context.sampleRate = options_.sampleRate;
        // VST3 requires a valid project sample position; free-running fallback
        // is the supplied instance steady sample counter.
        context.projectTimeSamples = p.steadyFrame >= 0 ? p.steadyFrame : 0;
        if (p.steadyFrame >= 0) {
            context.continousTimeSamples = p.steadyFrame;
            context.state |= ProcessContext::kContTimeValid;
        }
        if (p.transport) {
            const auto& t = *p.transport;
            if (t.hasSongFrame)
                context.projectTimeSamples = t.songFrame;
            else if (t.hasSeconds)
                context.projectTimeSamples = int64(t.seconds * options_.sampleRate);
            if (t.hasBeats) {
                context.state |=
                    ProcessContext::kProjectTimeMusicValid | ProcessContext::kBarPositionValid;
                context.projectTimeMusic = t.beats;
                context.barPositionMusic = t.barStart;
            }
            if (t.hasTempo) {
                context.state |= ProcessContext::kTempoValid;
                context.tempo = t.tempo;
            }
            if (t.hasSignature) {
                context.state |= ProcessContext::kTimeSigValid;
                context.timeSigNumerator = t.numerator;
                context.timeSigDenominator = t.denominator;
            }
            if (t.playing)
                context.state |= ProcessContext::kPlaying;
            if (t.recording)
                context.state |= ProcessContext::kRecording;
            if (t.looping && t.hasBeats) {
                context.state |= ProcessContext::kCycleActive | ProcessContext::kCycleValid;
                context.cycleStartMusic = t.loopStartBeats;
                context.cycleEndMusic = t.loopEndBeats;
            }
        }
        ProcessData data;
        data.processMode = options_.offline ? kOffline : kRealtime;
        data.symbolicSampleSize = std::same_as<T, double> ? kSample64 : kSample32;
        data.numSamples = int32(p.frames);
        data.numInputs = int32(inputBuses_.size());
        data.numOutputs = int32(outputBuses_.size());
        data.inputs = inputBuses_.data();
        data.outputs = outputBuses_.data();
        data.inputParameterChanges = &incoming_;
        data.outputParameterChanges = &outgoing_;
        data.inputEvents = &incomingEvents_;
        data.outputEvents = &outgoingEvents_;
        data.processContext = &context;
        if (processor_->process(data) != kResultOk)
            return Status::io_error;
        return collectOutput(p.outputEvents);
    }

  public:
    VstBackend(std::shared_ptr<VstModule> module, PluginHostCallbacks cb)
        : module_(std::move(module)), callbacks_(cb) {}
    ~VstBackend() override {
        closeEditor();
        if (running_)
            processor_->setProcessing(false);
        if (active_)
            component_->setActive(false);
        if (controller_)
            controller_->setComponentHandler(nullptr);
        if (componentConnection_ && controllerConnection_) {
            componentConnection_->disconnect(controllerConnection_);
            controllerConnection_->disconnect(componentConnection_);
        }
        vst::release(componentConnection_);
        vst::release(controllerConnection_);
        if (controllerInitialized_ && controller_)
            controller_->terminate();
        vst::release(controller_);
        vst::release(processor_);
        if (initialized_ && component_)
            component_->terminate();
        vst::release(component_);
    }
    tresult PLUGIN_API queryInterface(const TUID id, void** out) override {
        if (!out)
            return kInvalidArgument;
        *out = nullptr;
        if (equal(id, IComponentHandler_iid) || equal(id, FUnknown_iid))
            *out = static_cast<IComponentHandler*>(this);
        else if (equal(id, IPlugFrame_iid))
            *out = static_cast<IPlugFrame*>(this);
        else
            return kNoInterface;
        addRef();
        return kResultOk;
    }
    uint32 PLUGIN_API addRef() override {
        return ++references_;
    }
    uint32 PLUGIN_API release() override {
        return --references_;
    }
    tresult PLUGIN_API beginEdit(ParamID id) override {
        return uiEvent(PluginEventKind::gestureBegin, id) == Status::ok ? kResultOk : kResultFalse;
    }
    tresult PLUGIN_API performEdit(ParamID id, ParamValue value) override {
        return uiEvent(PluginEventKind::parameterValue, id, value) == Status::ok ? kResultOk
                                                                                 : kResultFalse;
    }
    tresult PLUGIN_API endEdit(ParamID id) override {
        return uiEvent(PluginEventKind::gestureEnd, id) == Status::ok ? kResultOk : kResultFalse;
    }
    tresult PLUGIN_API restartComponent(int32 flags) override {
        if (!onMain())
            return kResultFalse;
        std::uint32_t requests = pluginRestart;
        if (flags & kLatencyChanged)
            requests |= pluginLatencyChanged;
        if (flags & kIoChanged)
            requests |= pluginPortsChanged;
        if (flags & (kParamValuesChanged | kParamTitlesChanged))
            requests |= pluginParametersChanged;
        request(requests);
        return kResultOk;
    }
    tresult PLUGIN_API resizeView(IPlugView* view, ViewRect* size) override {
        if (!onMain() || view != view_ || !size || size->getWidth() <= 0 ||
            size->getHeight() <= 0 || size->getWidth() > 32768 || size->getHeight() > 32768 ||
            !callbacks_.resizeEditor)
            return kResultFalse;
        if (!callbacks_.resizeEditor(callbacks_.context,
                                     {uint32(size->getWidth()), uint32(size->getHeight())}))
            return kResultFalse;
        return view_->onSize(size);
    }
    Status initialize(std::string_view id) {
        TUID cid{};
        if (!identifier(id, cid))
            return Status::invalid_argument;
        auto r = uiToAudio_.prepare(1024);
        if (r != Status::ok)
            return r;
        const auto count = module_->factory->countClasses();
        if (count < 0 || count > 4096)
            return Status::unsupported_format;
        for (int32 i = 0; i < count; ++i) {
            auto d = classDescriptor(*module_, i);
            if (d.id == id) {
                description_.plugin = std::move(d);
                break;
            }
        }
        if (description_.plugin.id.empty())
            return Status::unsupported_format;
        if (module_->factory->createInstance(cid, IComponent_iid,
                                             reinterpret_cast<void**>(&component_)) != kResultOk ||
            !component_)
            return Status::unsupported_format;
        TUID controllerId{};
        const auto hasController = component_->getControllerClassId(controllerId) == kResultOk;
        (void)component_->setIoMode(kAdvanced);
        if (component_->initialize(&module_->host) != kResultOk)
            return Status::io_error;
        initialized_ = true;
        processor_ = query<IAudioProcessor>(component_, IAudioProcessor_iid);
        if (!processor_)
            return Status::unsupported_format;
        controller_ = query<IEditController>(component_, IEditController_iid);
        if (!controller_ && hasController) {
            if (module_->factory->createInstance(controllerId, IEditController_iid,
                                                 reinterpret_cast<void**>(&controller_)) !=
                    kResultOk ||
                !controller_)
                return Status::unsupported_format;
            if (controller_->initialize(&module_->host) != kResultOk)
                return Status::io_error;
            controllerInitialized_ = true;
        }
        if (controller_) {
            if (controller_->setComponentHandler(this) != kResultOk)
                return Status::io_error;
            componentConnection_ = query<IConnectionPoint>(component_, IConnectionPoint_iid);
            controllerConnection_ = query<IConnectionPoint>(controller_, IConnectionPoint_iid);
            if (componentConnection_ && controllerConnection_) {
                if (componentConnection_->connect(controllerConnection_) != kResultOk ||
                    controllerConnection_->connect(componentConnection_) != kResultOk)
                    return Status::io_error;
            }
            MemoryStream state;
            if (component_->getState(&state) == kResultOk && !state.failed) {
                state.position = 0;
                (void)controller_->setComponentState(&state);
            }
        }
        return refresh();
    }
    const PluginDescription& description() const noexcept override {
        return description_;
    }
    Status refresh() noexcept override {
        if (active_)
            return Status::transition_in_progress;
        try {
            PluginDescription next;
            next.plugin = description_.plugin;
            next.state = true;
            next.editor = controller_ != nullptr;
            next.float32 = processor_->canProcessSampleSize(kSample32) == kResultOk;
            next.float64 = processor_->canProcessSampleSize(kSample64) == kResultOk;
            std::size_t channels = 0;
            for (auto direction : {kInput, kOutput})
                for (auto media : {kAudio, kEvent}) {
                    const auto count = component_->getBusCount(media, direction);
                    if (count < 0 || count > 64)
                        return Status::unsupported_format;
                    for (int32 i = 0; i < count; ++i) {
                        BusInfo info{};
                        if (component_->getBusInfo(media, direction, i, info) != kResultOk ||
                            info.channelCount < 0 || info.channelCount > 256)
                            return Status::unsupported_format;
                        if (media == kAudio) {
                            channels += std::size_t(info.channelCount);
                            auto& list = direction == kInput ? next.inputs : next.outputs;
                            list.push_back({uint32(i),
                                            uint32(info.channelCount),
                                            0xffffffffu,
                                            utf8(info.name),
                                            {},
                                            info.busType == kMain,
                                            next.float64});
                        } else {
                            auto& list = direction == kInput ? next.noteInputs : next.noteOutputs;
                            list.push_back({uint32(i), 3, 1, utf8(info.name)});
                        }
                    }
                }
            for (std::size_t i = 0; i < std::min(next.inputs.size(), next.outputs.size()); ++i)
                if (next.inputs[i].channels == next.outputs[i].channels) {
                    next.inputs[i].inPlacePair = next.outputs[i].id;
                    next.outputs[i].inPlacePair = next.inputs[i].id;
                }
            if (controller_) {
                const auto count = controller_->getParameterCount();
                if (count < 0 || count > 65536)
                    return Status::unsupported_format;
                for (int32 i = 0; i < count; ++i) {
                    ParameterInfo info{};
                    if (controller_->getParameterInfo(i, info) != kResultOk ||
                        !isFinite(info.defaultNormalizedValue) || info.defaultNormalizedValue < 0 ||
                        info.defaultNormalizedValue > 1)
                        return Status::unsupported_format;
                    next.parameters.push_back({info.id,
                                               utf8(info.title),
                                               {},
                                               0,
                                               1,
                                               info.defaultNormalizedValue,
                                               info.stepCount > 0,
                                               (info.flags & ParameterInfo::kIsReadOnly) != 0,
                                               (info.flags & ParameterInfo::kCanAutomate) != 0,
                                               false,
                                               (info.flags & ParameterInfo::kIsHidden) != 0,
                                               true});
                    next.parameters.back().units = utf8(info.units);
                }
                std::sort(next.parameters.begin(), next.parameters.end(),
                          [](const auto& a, const auto& b) { return a.id < b.id; });
                for (std::size_t i = 1; i < next.parameters.size(); ++i)
                    if (next.parameters[i - 1].id == next.parameters[i].id)
                        return Status::unsupported_format;
            }
            std::vector<AudioBusBuffers> ins(next.inputs.size()), outs(next.outputs.size());
            std::vector<float*> p32(channels);
            std::vector<double*> p64(channels);
            incoming_.prepare(next.parameters, options_.maximumEvents);
            outgoing_.prepare(next.parameters, options_.maximumEvents);
            incomingEvents_.prepare(options_.maximumEvents);
            outgoingEvents_.prepare(options_.maximumEvents);
            outgoingEvents_.output = true;
            merged_.resize(std::size_t(options_.maximumEvents) * 2 + 1024);
            std::vector<std::uint32_t> mapping(next.noteInputs.size() * 16 * 130, 0xffffffffu);
            if (auto* midi = query<IMidiMapping>(controller_, IMidiMapping_iid)) {
                for (std::size_t bus = 0; bus < next.noteInputs.size(); ++bus)
                    for (unsigned channel = 0; channel < 16; ++channel)
                        for (unsigned cc = 0; cc < 130; ++cc) {
                            ParamID id = 0xffffffffu;
                            if (midi->getMidiControllerAssignment(int32(bus), int16(channel),
                                                                  CtrlNumber(cc), id) == kResultOk)
                                mapping[(bus * 16 + channel) * 130 + cc] = id;
                        }
                midi->release();
            }
            midiAssignments_.swap(mapping);
            auto mirrors = std::make_unique<ParameterMirror[]>(next.parameters.size());
            description_ = std::move(next);
            mirrors_ = std::move(mirrors);
            inputBuses_.swap(ins);
            outputBuses_.swap(outs);
            pointers32_.swap(p32);
            pointers64_.swap(p64);
            return Status::ok;
        } catch (...) {
            return Status::allocation_failed;
        }
    }
    Status activate(const PluginPrepareOptions& o) noexcept override {
        options_ = o;
        auto r = refresh();
        if (r != Status::ok)
            return r;
        ProcessSetup setup{o.offline ? kOffline : kRealtime, o.useFloat64 ? kSample64 : kSample32,
                           int32(o.maximumFrames), o.sampleRate};
        if (processor_->setupProcessing(setup) != kResultOk)
            return Status::invalid_spec;
        for (auto direction : {kInput, kOutput})
            for (auto media : {kAudio, kEvent}) {
                const auto count = component_->getBusCount(media, direction);
                for (int32 i = 0; i < count; ++i)
                    if (component_->activateBus(media, direction, i, true) != kResultOk)
                        return Status::unsupported_format;
            }
        active_ = component_->setActive(true) == kResultOk;
        return active_ ? Status::ok : Status::invalid_spec;
    }
    Status deactivate() noexcept override {
        if (active_ && component_->setActive(false) != kResultOk)
            return Status::io_error;
        active_ = false;
        return Status::ok;
    }
    Status start() noexcept override {
        running_ = processor_->setProcessing(true) == kResultOk;
        return running_ ? Status::ok : Status::io_error;
    }
    void stop() noexcept override {
        (void)processor_->setProcessing(false);
        running_ = false;
    }
    Status reset() noexcept override {
        return Status::unsupported_format;
    } // No universal realtime reset in VST3.
    Status process(const PluginProcess<float>& p) noexcept override {
        return processing(p);
    }
    Status process(const PluginProcess<double>& p) noexcept override {
        return processing(p);
    }
    PluginTiming timing() const noexcept override {
        if (!active_)
            return {};
        const auto tail = processor_->getTailSamples();
        return {processor_->getLatencySamples(), tail, true, true, tail == kInfiniteTail};
    }
    Status parameterValue(std::uint32_t id, double& value) noexcept override {
        if (!controller_ || !parameter(id))
            return Status::invalid_argument;
        value = controller_->getParamNormalized(id);
        return isFinite(value) ? Status::ok : Status::io_error;
    }
    Status parameterText(std::uint32_t id, double value, std::string& out) noexcept override {
        if (!controller_ || !parameter(id) || value < 0 || value > 1)
            return Status::invalid_argument;
        String128 text{};
        if (controller_->getParamStringByValue(id, value, text) != kResultOk)
            return Status::io_error;
        try {
            out = utf8(text);
            return Status::ok;
        } catch (...) {
            return Status::allocation_failed;
        }
    }
    Status flush(const PluginEventBuffer& input, PluginEventBuffer&) noexcept override {
        for (std::size_t i = 0; i < input.size(); ++i) {
            const auto& e = *input.event(i);
            if (e.kind != PluginEventKind::parameterValue &&
                e.kind != PluginEventKind::gestureBegin && e.kind != PluginEventKind::gestureEnd)
                return Status::unsupported_format;
            const auto r = uiEvent(e.kind, e.id, e.value, false);
            if (r != Status::ok)
                return r;
            if (e.kind == PluginEventKind::parameterValue)
                (void)controller_->setParamNormalized(e.id, e.value);
        }
        return Status::ok; // Processor changes arrive at the next audio block.
    }
    Status saveState(std::vector<std::uint8_t>& result, std::size_t limit) noexcept override {
        if (uiToAudio_.peek())
            return Status::would_block;
        if (limit < 24)
            return Status::block_too_large;
        try {
            MemoryStream component, controller;
            component.limit = controller.limit = limit;
            if (component_->getState(&component) != kResultOk || component.failed)
                return Status::io_error;
            if (controller_ && controller_->getState(&controller) != kResultOk)
                return Status::io_error;
            if (controller.failed || component.bytes.size() > limit - 24 ||
                controller.bytes.size() > limit - 24 - component.bytes.size())
                return Status::block_too_large;
            std::vector<std::uint8_t> state(24);
            const char signature[] = "CyDSP.VST3State1";
            std::copy_n(signature, 16, state.begin());
            const auto write = [&](std::size_t at, std::uint32_t n) {
                for (unsigned i = 0; i < 4; ++i)
                    state[at + i] = std::uint8_t(n >> (8 * i));
            };
            write(16, uint32(component.bytes.size()));
            write(20, uint32(controller.bytes.size()));
            state.insert(state.end(), component.bytes.begin(), component.bytes.end());
            state.insert(state.end(), controller.bytes.begin(), controller.bytes.end());
            result.swap(state);
            return Status::ok;
        } catch (...) {
            return Status::allocation_failed;
        }
    }
    Status loadState(std::span<const std::uint8_t> state) noexcept override {
        const char signature[] = "CyDSP.VST3State1";
        if (state.size() < 24 || !std::equal(state.begin(), state.begin() + 16,
                                             reinterpret_cast<const std::uint8_t*>(signature)))
            return Status::unsupported_format;
        const auto word = [&](std::size_t at) {
            std::uint32_t n = 0;
            for (unsigned i = 0; i < 4; ++i)
                n |= std::uint32_t(state[at + i]) << (8 * i);
            return n;
        };
        const auto a = std::size_t(word(16)), b = std::size_t(word(20));
        if (a > state.size() - 24 || b != state.size() - 24 - a)
            return Status::invalid_argument;
        MemoryStream component(state.subspan(24, a));
        if (component_->setState(&component) != kResultOk)
            return Status::io_error;
        if (controller_) {
            component.position = 0;
            if (controller_->setComponentState(&component) != kResultOk)
                return Status::io_error;
            MemoryStream controller(state.subspan(24 + a, b));
            if (b && controller_->setState(&controller) != kResultOk)
                return Status::io_error;
        }
        UiEdit stale;
        while (uiToAudio_.pop(stale)) {
        }
        for (std::size_t i = 0; i < description_.parameters.size(); ++i)
            mirrors_[i].dirty.store(0, std::memory_order_relaxed);
        return Status::ok;
    }
    Status openEditor(const PluginWindow& window, PluginEditorSize& size) noexcept override {
        if (!controller_ || window.floating)
            return Status::unsupported_format;
        const char* api = window.api == PluginWindowApi::win32   ? kPlatformTypeHWND
                          : window.api == PluginWindowApi::cocoa ? kPlatformTypeNSView
                          : window.api == PluginWindowApi::x11   ? kPlatformTypeX11EmbedWindowID
                                                                 : nullptr;
        if (!api)
            return Status::unsupported_backend;
        view_ = controller_->createView(ViewType::kEditor);
        if (!view_)
            return Status::unsupported_format;
        if (view_->isPlatformTypeSupported(api) != kResultOk) {
            closeEditor();
            return Status::unsupported_backend;
        }
        if (view_->setFrame(this) != kResultOk) {
            closeEditor();
            return Status::io_error;
        }
        (void)scaleEditor(window.scale);
        if (view_->attached(reinterpret_cast<void*>(window.handle), api) != kResultOk) {
            closeEditor();
            return Status::io_error;
        }
        attached_ = true;
        ViewRect rect;
        if (view_->getSize(&rect) != kResultOk || rect.getWidth() <= 0 || rect.getHeight() <= 0 ||
            rect.getWidth() > 32768 || rect.getHeight() > 32768) {
            closeEditor();
            return Status::io_error;
        }
        size = {uint32(rect.getWidth()), uint32(rect.getHeight())};
        return Status::ok;
    }
    Status resizeEditor(PluginEditorSize& size) noexcept override {
        if (!view_ || view_->canResize() != kResultOk)
            return Status::unsupported_format;
        ViewRect rect(0, 0, int32(size.width), int32(size.height));
        if (view_->checkSizeConstraint(&rect) != kResultOk || rect.getWidth() <= 0 ||
            rect.getHeight() <= 0 || rect.getWidth() > 32768 || rect.getHeight() > 32768 ||
            view_->onSize(&rect) != kResultOk)
            return Status::io_error;
        size = {uint32(rect.getWidth()), uint32(rect.getHeight())};
        return Status::ok;
    }
    Status scaleEditor(double scale) noexcept override {
        auto* support =
            view_ ? query<IPlugViewContentScaleSupport>(view_, IPlugViewContentScaleSupport_iid)
                  : nullptr;
        if (!support)
            return Status::unsupported_format;
        const auto r = support->setContentScaleFactor(float(scale));
        support->release();
        return r == kResultOk ? Status::ok : Status::unsupported_format;
    }
    Status showEditor(bool) noexcept override {
        return Status::unsupported_format;
    } // Host owns visibility of the VST3 parent.
    Status focusEditor(bool focus) noexcept override {
        return view_ && view_->onFocus(focus) == kResultOk ? Status::ok
                                                           : Status::unsupported_format;
    }
    Status keyEditor(const PluginEditorKey& e) noexcept override {
        if (!view_)
            return Status::not_prepared;
        constexpr std::array<int16, 40> keys{
            0,          KEY_BACK,   KEY_TAB,  KEY_RETURN, KEY_ESCAPE,   KEY_SPACE, KEY_LEFT,
            KEY_UP,     KEY_RIGHT,  KEY_DOWN, KEY_PAGEUP, KEY_PAGEDOWN, KEY_HOME,  KEY_END,
            KEY_INSERT, KEY_DELETE, KEY_F1,   KEY_F2,     KEY_F3,       KEY_F4,    KEY_F5,
            KEY_F6,     KEY_F7,     KEY_F8,   KEY_F9,     KEY_F10,      KEY_F11,   KEY_F12,
            KEY_F13,    KEY_F14,    KEY_F15,  KEY_F16,    KEY_F17,      KEY_F18,   KEY_F19,
            KEY_F20,    KEY_F21,    KEY_F22,  KEY_F23,    KEY_F24};
        const auto index = std::size_t(e.key);
        if (index >= keys.size())
            return Status::invalid_argument;
        int16 modifiers = 0;
        if (e.modifiers & editorShift)
            modifiers |= kShiftKey;
        if (e.modifiers & editorAlt)
            modifiers |= kAlternateKey;
#if defined(__APPLE__)
        if (e.modifiers & editorControl)
            modifiers |= kControlKey;
        if (e.modifiers & editorSuper)
            modifiers |= kCommandKey;
#else
        if (e.modifiers & editorControl)
            modifiers |= kCommandKey;
        if (e.modifiers & editorSuper)
            modifiers |= kControlKey;
#endif
        const auto r = e.down ? view_->onKeyDown(char16(e.character), keys[index], modifiers)
                              : view_->onKeyUp(char16(e.character), keys[index], modifiers);
        return r == kResultOk ? Status::ok : Status::unsupported_format;
    }
    Status wheelEditor(float distance) noexcept override {
        return view_ && view_->onWheel(distance) == kResultOk ? Status::ok
                                                              : Status::unsupported_format;
    }
    void closeEditor() noexcept override {
        if (view_) {
            if (attached_)
                view_->removed();
            attached_ = false;
            view_->setFrame(nullptr);
            vst::release(view_);
        }
    }
    std::uint32_t pumpMainThread() noexcept override {
        // Last-value mailboxes preserve final automation across slow UI pumps.
        // All synchronisation uses lock-free32-bit atomics, including on ARM32.
        if (controller_)
            for (std::size_t i = 0; i < description_.parameters.size(); ++i) {
                auto& m = mirrors_[i];
                if (!m.dirty.exchange(0, std::memory_order_acq_rel))
                    continue;
                const auto before = m.sequence.load(std::memory_order_acquire);
                const auto low = m.low.load(std::memory_order_acquire),
                           high = m.high.load(std::memory_order_acquire);
                const auto after = m.sequence.load(std::memory_order_acquire);
                if ((before & 1u) || before != after) {
                    m.dirty.store(1, std::memory_order_release);
                    continue;
                }
                const auto value =
                    std::bit_cast<double>(std::uint64_t(low) | (std::uint64_t(high) << 32));
                controller_->setParamNormalized(description_.parameters[i].id, value);
            }
        const auto flags = requests_.exchange(0, std::memory_order_acq_rel);
        if (callbacks_.requests && flags)
            callbacks_.requests(callbacks_.context, flags);
        return flags;
    }
};
class VstProvider final : public PluginProvider {
  public:
    Status scan(const std::filesystem::path& path,
                std::vector<PluginDescriptor>& result) noexcept override {
        try {
            std::shared_ptr<VstModule> module;
            const auto r = loadVst(path, module);
            if (r != Status::ok)
                return r;
            const auto count = module->factory->countClasses();
            if (count < 0 || count > 4096)
                return Status::unsupported_format;
            std::vector<PluginDescriptor> next;
            for (int32 i = 0; i < count; ++i) {
                auto d = classDescriptor(*module, i);
                if (!d.id.empty())
                    next.push_back(std::move(d));
            }
            result.swap(next);
            return Status::ok;
        } catch (const std::bad_alloc&) {
            return Status::allocation_failed;
        } catch (...) {
            return Status::unsupported_format;
        }
    }
    Status create(const std::filesystem::path& path, std::string_view id,
                  const PluginHostCallbacks& cb, PluginInstance& result) noexcept override {
        try {
            std::shared_ptr<VstModule> module;
            auto r = loadVst(path, module);
            if (r != Status::ok)
                return r;
            auto backend = std::make_unique<VstBackend>(std::move(module), cb);
            r = backend->initialize(id);
            return r == Status::ok ? PluginInstance::adopt(std::move(backend), result) : r;
        } catch (const std::bad_alloc&) {
            return Status::allocation_failed;
        } catch (...) {
            return Status::unsupported_format;
        }
    }
};
} // namespace
std::unique_ptr<PluginProvider> makeVst3Provider() {
    return std::make_unique<VstProvider>();
}
} // namespace cydsp::detail
