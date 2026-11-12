// Original MIT fixture, official declarations: third_party/licenses/vst3-interfaces.txt
#include "plugins/VstSupport.h"
#include "pluginterfaces/base/keycodes.h"
#include <array>
#include <cstdio>
#include <new>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace {
using namespace cydsp::detail::vst;
const TUID fixtureId = INLINE_UID(0x43a6be13, 0x274d4ef1, 0x812b472f, 0xd0a0b713);
class View final : public IPlugView, public IPlugViewContentScaleSupport {
    std::atomic<uint32> refs_{1};
    IPlugFrame* frame_ = nullptr;
    IComponentHandler* handler_ = nullptr;
    ViewRect rect_{0, 0, 320, 160};
#if defined(_WIN32)
    HWND child_ = nullptr;
#endif
  public:
    explicit View(IComponentHandler* handler) : handler_(handler) {
        if (handler_)
            handler_->addRef();
    }
    ~View() {
        removed();
        setFrame(nullptr);
        if (handler_)
            handler_->release();
    }
    tresult PLUGIN_API queryInterface(const TUID id, void** out) override {
        if (!out)
            return kInvalidArgument;
        *out = nullptr;
        if (equal(id, IPlugView_iid) || equal(id, FUnknown_iid))
            *out = static_cast<IPlugView*>(this);
        else if (equal(id, IPlugViewContentScaleSupport_iid))
            *out = static_cast<IPlugViewContentScaleSupport*>(this);
        else
            return kNoInterface;
        addRef();
        return kResultOk;
    }
    uint32 PLUGIN_API addRef() override {
        return ++refs_;
    }
    uint32 PLUGIN_API release() override {
        const auto n = --refs_;
        if (!n)
            delete this;
        return n;
    }
    tresult PLUGIN_API isPlatformTypeSupported(FIDString type) override {
#if defined(_WIN32)
        return type && std::strcmp(type, kPlatformTypeHWND) == 0 ? kResultOk : kResultFalse;
#else
        (void)type;
        return kResultFalse;
#endif
    }
    tresult PLUGIN_API attached(void* parent, FIDString type) override {
        if (isPlatformTypeSupported(type) != kResultOk)
            return kResultFalse;
        ViewRect wanted(0, 0, 400, 200);
        if (frame_)
            frame_->resizeView(this, &wanted);
#if defined(_WIN32)
        child_ = CreateWindowExW(0, L"STATIC", L"CyDSP VST3 fixture", WS_CHILD, 0, 0,
                                 rect_.getWidth(), rect_.getHeight(), static_cast<HWND>(parent),
                                 nullptr, GetModuleHandleW(nullptr), nullptr);
        return child_ ? kResultOk : kResultFalse;
#else
        (void)parent;
        return kResultFalse;
#endif
    }
    tresult PLUGIN_API removed() override {
#if defined(_WIN32)
        if (child_) {
            DestroyWindow(child_);
            child_ = nullptr;
        }
#endif
        return kResultOk;
    }
    tresult PLUGIN_API onWheel(float distance) override {
        if (distance != 1)
            return kResultFalse;
        if (handler_ &&
            (handler_->beginEdit(7) != kResultOk || handler_->performEdit(7, .75) != kResultOk ||
             handler_->endEdit(7) != kResultOk))
            return kResultFalse;
        return kResultOk;
    }
    tresult PLUGIN_API onKeyDown(char16 key, int16 code, int16 mods) override {
        return key == 'a' && code == 0 && mods == kCommandKey ? kResultOk : kResultFalse;
    }
    tresult PLUGIN_API onKeyUp(char16, int16, int16) override {
        return kResultFalse;
    }
    tresult PLUGIN_API getSize(ViewRect* size) override {
        if (!size)
            return kInvalidArgument;
        *size = rect_;
        return kResultOk;
    }
    tresult PLUGIN_API onSize(ViewRect* size) override {
        if (!size)
            return kInvalidArgument;
        rect_ = *size;
#if defined(_WIN32)
        if (child_)
            SetWindowPos(child_, nullptr, 0, 0, rect_.getWidth(), rect_.getHeight(),
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
        return kResultOk;
    }
    tresult PLUGIN_API onFocus(TBool) override {
        return kResultOk;
    }
    tresult PLUGIN_API setFrame(IPlugFrame* frame) override {
        if (frame)
            frame->addRef();
        if (frame_)
            frame_->release();
        frame_ = frame;
        return kResultOk;
    }
    tresult PLUGIN_API canResize() override {
        return kResultOk;
    }
    tresult PLUGIN_API checkSizeConstraint(ViewRect* size) override {
        if (!size)
            return kInvalidArgument;
        size->right = std::max(size->left + 100, size->right);
        size->bottom = std::max(size->top + 80, size->bottom);
        return kResultOk;
    }
    tresult PLUGIN_API setContentScaleFactor(float factor) override {
        return factor > 0 ? kResultOk : kResultFalse;
    }
};
class Fixture final : public IComponent,
                      public IAudioProcessor,
                      public IEditController,
                      public IMidiMapping {
    std::atomic<uint32> refs_{1};
    IComponentHandler* handler_ = nullptr;
    bool active_ = false, processing_ = false;
    double gain_ = .5, ui_ = .25;
    ProcessSetup setup_{};
    template <class T> tresult audio(ProcessData& p) {
        if (!active_ || !processing_ || p.numInputs != 2 || p.numOutputs != 1 ||
            p.inputs[0].numChannels != 2 || p.inputs[1].numChannels != 1)
            return kResultFalse;
        for (int32 frame = 0; frame < p.numSamples; ++frame) {
            if (p.inputParameterChanges)
                for (int32 i = 0; i < p.inputParameterChanges->getParameterCount(); ++i) {
                    auto* queue = p.inputParameterChanges->getParameterData(i);
                    if (queue->getParameterId() != 7)
                        return kResultFalse;
                    for (int32 j = 0; j < queue->getPointCount(); ++j) {
                        int32 offset = 0;
                        double value = 0;
                        if (queue->getPoint(j, offset, value) != kResultOk)
                            return kResultFalse;
                        if (offset == frame) {
                            gain_ = 2 * value;
                            int32 q = 0, point = 0;
                            auto* output = p.outputParameterChanges->addParameterData(
                                queue->getParameterId(), q);
                            if (!output || output->addPoint(offset, value, point) != kResultOk)
                                return kResultFalse;
                        }
                    }
                }
            for (int32 ch = 0; ch < 2; ++ch) {
                if constexpr (std::is_same_v<T, float>)
                    p.outputs[0].channelBuffers32[ch][frame] =
                        float(p.inputs[0].channelBuffers32[ch][frame] * gain_ +
                              p.inputs[1].channelBuffers32[0][frame]);
                else
                    p.outputs[0].channelBuffers64[ch][frame] =
                        p.inputs[0].channelBuffers64[ch][frame] * gain_ +
                        p.inputs[1].channelBuffers64[0][frame];
            }
        }
        if (p.inputEvents && p.outputEvents)
            for (int32 i = 0; i < p.inputEvents->getEventCount(); ++i) {
                Event e{};
                if (p.inputEvents->getEvent(i, e) != kResultOk ||
                    p.outputEvents->addEvent(e) != kResultOk)
                    return kResultFalse;
            }
        return kResultOk;
    }

  public:
    ~Fixture() {
        if (handler_)
            handler_->release();
    }
    tresult PLUGIN_API queryInterface(const TUID id, void** out) override {
        if (!out)
            return kInvalidArgument;
        *out = nullptr;
        if (equal(id, IComponent_iid) || equal(id, IPluginBase_iid) || equal(id, FUnknown_iid))
            *out = static_cast<IComponent*>(this);
        else if (equal(id, IAudioProcessor_iid))
            *out = static_cast<IAudioProcessor*>(this);
        else if (equal(id, IEditController_iid))
            *out = static_cast<IEditController*>(this);
        else if (equal(id, IMidiMapping_iid))
            *out = static_cast<IMidiMapping*>(this);
        else
            return kNoInterface;
        addRef();
        return kResultOk;
    }
    uint32 PLUGIN_API addRef() override {
        return ++refs_;
    }
    uint32 PLUGIN_API release() override {
        const auto n = --refs_;
        if (!n)
            delete this;
        return n;
    }
    tresult PLUGIN_API initialize(FUnknown* context) override {
        auto* host = query<IHostApplication>(context, IHostApplication_iid);
        if (!host)
            return kResultFalse;
        String128 name{};
        const auto result = host->getName(name);
        TUID id{};
        std::copy_n(IMessage_iid, 16, id);
        IMessage* message = nullptr;
        const auto created = host->createInstance(id, id, reinterpret_cast<void**>(&message));
        host->release();
        if (result != kResultOk || created != kResultOk || !message)
            return kResultFalse;
        auto* attributes = message->getAttributes();
        const auto set = attributes->setInt("fixture", 73);
        int64 value = 0;
        const auto get = attributes->getInt("fixture", value);
        message->release();
        return set == kResultOk && get == kResultOk && value == 73 ? kResultOk : kResultFalse;
    }
    tresult PLUGIN_API terminate() override {
        return kResultOk;
    }
    tresult PLUGIN_API getControllerClassId(TUID) override {
        return kResultFalse;
    }
    tresult PLUGIN_API setIoMode(IoMode) override {
        return kResultOk;
    }
    int32 PLUGIN_API getBusCount(MediaType media, BusDirection dir) override {
        return media == kAudio ? (dir == kInput ? 2 : 1) : 1;
    }
    tresult PLUGIN_API getBusInfo(MediaType media, BusDirection dir, int32 i,
                                  BusInfo& info) override {
        if (i < 0 || i >= getBusCount(media, dir))
            return kInvalidArgument;
        info = {};
        info.mediaType = media;
        info.direction = dir;
        info.channelCount = media == kAudio ? (i ? 1 : 2) : 16;
        info.busType = i ? kAux : kMain;
        info.flags = BusInfo::kDefaultActive;
        const char* name = media == kEvent ? "Events" : i ? "Sidechain" : "Main";
        for (std::size_t n = 0; name[n]; ++n)
            info.name[n] = char16(name[n]);
        return kResultOk;
    }
    tresult PLUGIN_API getRoutingInfo(RoutingInfo&, RoutingInfo&) override {
        return kNotImplemented;
    }
    tresult PLUGIN_API activateBus(MediaType, BusDirection, int32, TBool) override {
        return kResultOk;
    }
    tresult PLUGIN_API setActive(TBool state) override {
        active_ = state != 0;
        return kResultOk;
    }
    tresult PLUGIN_API setState(IBStream* stream) override {
        double value = 0;
        int32 count = 0;
        if (stream->read(&value, sizeof(value), &count) != kResultOk || count != sizeof(value) ||
            value < 0 || value > 2)
            return kResultFalse;
        gain_ = value;
        ui_ = value / 2;
        return kResultOk;
    }
    tresult PLUGIN_API getState(IBStream* stream) override {
        int32 count = 0;
        return stream->write(&gain_, sizeof(gain_), &count);
    }
    tresult PLUGIN_API setBusArrangements(SpeakerArrangement*, int32 ni, SpeakerArrangement*,
                                          int32 no) override {
        return ni == 2 && no == 1 ? kResultOk : kResultFalse;
    }
    tresult PLUGIN_API getBusArrangement(BusDirection dir, int32 i,
                                         SpeakerArrangement& a) override {
        if (i < 0 || i >= (dir == kInput ? 2 : 1))
            return kInvalidArgument;
        a = i ? SpeakerArr::kMono : SpeakerArr::kStereo;
        return kResultOk;
    }
    tresult PLUGIN_API canProcessSampleSize(int32 size) override {
        return size == kSample32 || size == kSample64 ? kResultOk : kResultFalse;
    }
    uint32 PLUGIN_API getLatencySamples() override {
        return 17;
    }
    tresult PLUGIN_API setupProcessing(ProcessSetup& setup) override {
        setup_ = setup;
        return kResultOk;
    }
    tresult PLUGIN_API setProcessing(TBool state) override {
        processing_ = state != 0;
        return kResultOk;
    }
    tresult PLUGIN_API process(ProcessData& p) override {
        return p.symbolicSampleSize == kSample64 ? audio<double>(p) : audio<float>(p);
    }
    uint32 PLUGIN_API getTailSamples() override {
        return 64;
    }
    tresult PLUGIN_API setComponentState(IBStream* state) override {
        return setState(state);
    }
    int32 PLUGIN_API getParameterCount() override {
        return 1;
    }
    tresult PLUGIN_API getParameterInfo(int32 index, ParameterInfo& info) override {
        if (index)
            return kInvalidArgument;
        info = {};
        info.id = 7;
        info.defaultNormalizedValue = .25;
        info.flags = ParameterInfo::kCanAutomate;
        constexpr char name[] = "Gain";
        for (std::size_t i = 0; i < sizeof(name); ++i)
            info.title[i] = char16(name[i]);
        return kResultOk;
    }
    tresult PLUGIN_API getParamStringByValue(ParamID id, ParamValue value,
                                             String128 text) override {
        if (id != 7)
            return kInvalidArgument;
        std::array<char, 128> buffer{};
        std::snprintf(buffer.data(), buffer.size(), "%.3f", value * 2);
        for (std::size_t i = 0; i < buffer.size(); ++i)
            text[i] = char16(buffer[i]);
        return kResultOk;
    }
    tresult PLUGIN_API getParamValueByString(ParamID, TChar*, ParamValue&) override {
        return kNotImplemented;
    }
    ParamValue PLUGIN_API normalizedParamToPlain(ParamID, ParamValue value) override {
        return value * 2;
    }
    ParamValue PLUGIN_API plainParamToNormalized(ParamID, ParamValue value) override {
        return value / 2;
    }
    ParamValue PLUGIN_API getParamNormalized(ParamID) override {
        return ui_;
    }
    tresult PLUGIN_API setParamNormalized(ParamID id, ParamValue value) override {
        if (id != 7)
            return kInvalidArgument;
        ui_ = value;
        return kResultOk;
    }
    tresult PLUGIN_API getMidiControllerAssignment(int32 bus, int16 channel, CtrlNumber cc,
                                                   ParamID& id) override {
        if (bus == 0 && channel == 0 && cc == 7) {
            id = 7;
            return kResultOk;
        }
        return kResultFalse;
    }
    tresult PLUGIN_API setComponentHandler(IComponentHandler* handler) override {
        if (handler)
            handler->addRef();
        if (handler_)
            handler_->release();
        handler_ = handler;
        return kResultOk;
    }
    IPlugView* PLUGIN_API createView(FIDString name) override {
        return name && std::strcmp(name, ViewType::kEditor) == 0 ? new (std::nothrow) View(handler_)
                                                                 : nullptr;
    }
};
class Factory final : public IPluginFactory {
    std::atomic<uint32> refs_{1};

  public:
    tresult PLUGIN_API queryInterface(const TUID id, void** out) override {
        if (!out)
            return kInvalidArgument;
        *out = nullptr;
        if (equal(id, IPluginFactory_iid) || equal(id, FUnknown_iid)) {
            *out = static_cast<IPluginFactory*>(this);
            addRef();
            return kResultOk;
        }
        return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override {
        return ++refs_;
    }
    uint32 PLUGIN_API release() override {
        const auto n = --refs_;
        if (!n)
            delete this;
        return n;
    }
    tresult PLUGIN_API getFactoryInfo(PFactoryInfo* info) override {
        if (!info)
            return kInvalidArgument;
        *info = {};
        std::copy_n("CyDSP", 6, info->vendor);
        return kResultOk;
    }
    int32 PLUGIN_API countClasses() override {
        return 1;
    }
    tresult PLUGIN_API getClassInfo(int32 index, PClassInfo* info) override {
        if (index || !info)
            return kInvalidArgument;
        std::copy_n(fixtureId, 16, info->cid);
        info->cardinality = PClassInfo::kManyInstances;
        std::copy_n(kVstAudioEffectClass, sizeof(kVstAudioEffectClass), info->category);
        std::copy_n("CyDSP VST3 ABI fixture", 23, info->name);
        return kResultOk;
    }
    tresult PLUGIN_API createInstance(FIDString cid, FIDString iid, void** out) override {
        if (!out)
            return kInvalidArgument;
        *out = nullptr;
        if (!equal(cid, fixtureId))
            return kResultFalse;
        auto* fixture = new (std::nothrow) Fixture;
        if (!fixture)
            return kOutOfMemory;
        const auto r = fixture->queryInterface(iid, out);
        fixture->release();
        return r;
    }
};
} // namespace
extern "C" SMTG_EXPORT_SYMBOL Steinberg::IPluginFactory* PLUGIN_API GetPluginFactory() {
    return new (std::nothrow) Factory;
}
#if defined(__APPLE__)
extern "C" SMTG_EXPORT_SYMBOL bool bundleEntry(void*) {
    return true;
}
extern "C" SMTG_EXPORT_SYMBOL bool bundleExit() {
    return true;
}
#elif !defined(_WIN32)
extern "C" SMTG_EXPORT_SYMBOL bool ModuleEntry(void*) {
    return true;
}
extern "C" SMTG_EXPORT_SYMBOL bool ModuleExit() {
    return true;
}
#endif
