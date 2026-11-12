// Private backend: third_party/licenses/miniaudio.txt
#include "cydsp/io/AudioDevice.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/core/AudioLayout.h"
#include "cydsp/math/VectorOps.h"
#include <algorithm>
#include <atomic>
#include <cstring>
#include <chrono>
#include <limits>
#include <miniaudio.h>
namespace cydsp {

namespace {
ma_backend nativeBackend(AudioBackend b) {
    switch(b) {
    case AudioBackend::wasapi:return ma_backend_wasapi;
    case AudioBackend::directSound:return ma_backend_dsound;
    case AudioBackend::winmm:return ma_backend_winmm;
    case AudioBackend::coreAudio:return ma_backend_coreaudio;
    case AudioBackend::alsa:return ma_backend_alsa;
    case AudioBackend::pulseAudio:return ma_backend_pulseaudio;
    case AudioBackend::jack:return ma_backend_jack;
    case AudioBackend::aaudio:return ma_backend_aaudio;
    case AudioBackend::openSl:return ma_backend_opensl;
    case AudioBackend::webAudio:return ma_backend_webaudio;
    case AudioBackend::nullDevice:return ma_backend_null;
    default:return ma_backend_custom;
    }
}
AudioBackend publicBackend(ma_backend b) {
    for(auto v:{AudioBackend::wasapi,AudioBackend::directSound,AudioBackend::winmm,AudioBackend::coreAudio,
        AudioBackend::alsa,AudioBackend::pulseAudio,AudioBackend::jack,AudioBackend::aaudio,
        AudioBackend::openSl,AudioBackend::webAudio,AudioBackend::nullDevice})
        if(nativeBackend(v)==b)return v;
    return AudioBackend::custom;
}
std::pair<std::string,bool> identity(ma_backend backend,const ma_device_id& id,bool input) {
    std::string value;bool stable=true;
    auto bytes=[&](const auto& data,bool terminated) {
        constexpr char hex[]="0123456789abcdef";
        for(auto unit:data) {
            if(terminated&&unit==0)break;
            for(unsigned i=0;i<sizeof(unit);++i) {
                auto b=unsigned((std::uint64_t(unit)>>(i*8))&255);
                value+=hex[b>>4];value+=hex[b&15];
            }
        }
    };
    switch(backend) {
    case ma_backend_wasapi:bytes(id.wasapi,true);break;
    case ma_backend_dsound:bytes(id.dsound,false);break;
    case ma_backend_coreaudio:bytes(id.coreaudio,true);break;
    case ma_backend_alsa:bytes(id.alsa,true);stable=std::string_view(id.alsa).find("CARD=")!=std::string_view::npos;break;
    case ma_backend_pulseaudio:bytes(id.pulse,true);break;
    case ma_backend_winmm:stable=false;value=std::to_string(id.winmm);break;
    case ma_backend_aaudio:stable=false;value=std::to_string(id.aaudio);break;
    case ma_backend_opensl:stable=false;value=std::to_string(id.opensl);break;
    case ma_backend_jack:stable=false;value=std::to_string(id.jack);break;
    case ma_backend_null:stable=false;value=std::to_string(id.nullbackend);break;
    default:stable=false;break;
    }
    return {"cydsp.audio/1/"+std::to_string(unsigned(publicBackend(backend)))+
        (input?"/input/":"/output/")+value,stable};
}
std::uint64_t nowNs() noexcept {
    return std::uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
}
Status resolveAudioDevice(std::span<const AudioDeviceInfo> list,const AudioDeviceSelector& select,std::size_t& out) noexcept {
    auto find=[&](auto predicate)->Status {
        std::size_t count=0,index=0;
        for(std::size_t i=0;i<list.size();++i) {
            const auto& d=list[i];
            if(d.input==select.input&&(select.backend==AudioBackend::automatic||d.backend==select.backend)&&predicate(d)){++count;index=i;}
        }
        if(count>1)return Status::ambiguous;if(!count)return Status::device_unavailable;
        out=index;return Status::ok;
    };
    if(select.useDefault)return find([](const auto& d){return d.isDefault;});
    if(!select.persistentId.empty()) {
        auto r=find([&](const auto& d){return d.persistentIdentity&&d.persistentId==select.persistentId;});
        if(r!=Status::device_unavailable||!select.allowNameFallback)return r;
    }
    if(select.name.empty())return Status::invalid_argument;
    return find([&](const auto& d){return d.name==select.name;});
}
struct AudioDeviceContext::State {
    ma_context context{};
    bool ready = false;
    struct Record {
        ma_device_id id;
        bool input;
        std::string key, nativeKey;
    };
    std::vector<Record> records;
    ~State() {
        if (ready)
            ma_context_uninit(&context);
    }
};
AudioDeviceContext::AudioDeviceContext() = default;
AudioDeviceContext::~AudioDeviceContext() = default;
AudioDeviceContext::AudioDeviceContext(AudioDeviceContext&&) noexcept = default;
AudioDeviceContext& AudioDeviceContext::operator=(AudioDeviceContext&&) noexcept = default;
Status AudioDeviceContext::prepare(bool onlyNull) noexcept {
    return prepare(AudioContextOptions{onlyNull?AudioBackend::nullDevice:AudioBackend::automatic});
}
Status AudioDeviceContext::prepare(const AudioContextOptions& o) noexcept {
    if(o.backend==AudioBackend::asio||o.backend==AudioBackend::custom||
        unsigned(o.backend)>unsigned(AudioBackend::custom))return Status::unsupported_backend;
    const auto backend=nativeBackend(o.backend);
    if(o.backend!=AudioBackend::automatic&&!ma_is_backend_enabled(backend))return Status::unsupported_backend;
    try {
        auto next=std::make_shared<State>();
        if(ma_context_init(o.backend==AudioBackend::automatic?nullptr:&backend,
            o.backend==AudioBackend::automatic?0:1,nullptr,&next->context)!=MA_SUCCESS)return Status::device_unavailable;
        next->ready=true;state_.swap(next);return Status::ok;
    }catch(const std::bad_alloc&){return Status::allocation_failed;}catch(...){return Status::io_error;}
}
AudioBackend AudioDeviceContext::backend()const noexcept{return state_?publicBackend(state_->context.backend):AudioBackend::automatic;}
Status AudioDeviceContext::capabilities(std::string_view key,bool input,AudioDeviceCapabilities& output) noexcept {
    if(!state_)return Status::not_prepared;
    const ma_device_id* id=nullptr;
    if(!key.empty()) {
        for(auto& r:state_->records)if(r.key==key&&r.input==input){id=&r.id;break;}
        if(!id)return Status::device_unavailable;
    }
    ma_device_info info{};
    if(ma_context_get_device_info(&state_->context,input?ma_device_type_capture:ma_device_type_playback,id,&info)!=MA_SUCCESS)return Status::device_unavailable;
    try {
        AudioDeviceCapabilities next;next.nativeFormatsKnown=info.nativeDataFormatCount!=0;
        for(ma_uint32 i=0;i<info.nativeDataFormatCount&&i<64;++i) {
            const auto& f=info.nativeDataFormats[i];
            auto format=AudioSampleFormat::unknown;
            switch(f.format){case ma_format_u8:format=AudioSampleFormat::unsigned8;break;
                case ma_format_s16:format=AudioSampleFormat::signed16;break;
                case ma_format_s24:format=AudioSampleFormat::signed24;break;
                case ma_format_s32:format=AudioSampleFormat::signed32;break;
                case ma_format_f32:format=AudioSampleFormat::float32;break;default:break;}
            next.nativeFormats.push_back({format,f.channels,f.sampleRate,(f.flags&MA_DATA_FORMAT_FLAG_EXCLUSIVE_MODE)!=0});
        }
        output=std::move(next);return Status::ok;
    }catch(...){return Status::allocation_failed;}
}
Status AudioDeviceContext::enumerate(std::vector<AudioDeviceInfo>& result) noexcept {
    if (!state_)
        return Status::not_prepared;
    try {
        ma_device_info *outputs = nullptr, *inputs = nullptr;
        ma_uint32 no = 0, ni = 0;
        if (ma_context_get_devices(&state_->context, &outputs, &no, &inputs, &ni) != MA_SUCCESS)
            return Status::io_error;
        std::vector<AudioDeviceInfo> next;
        for (bool input : {false, true})
            for (ma_uint32 i = 0, n = input ? ni : no; i < n; ++i) {
                const auto& device = (input ? inputs : outputs)[i];
                auto& records = state_->records;
                const auto token=identity(state_->context.backend,device.id,input);
                auto r = std::find_if(records.begin(), records.end(), [&](const State::Record& v) {
                    return v.input == input && v.nativeKey==token.first;
                });
                if (r == records.end()) {
                    records.push_back({device.id,input,token.second?token.first:"context/"+std::to_string(records.size()),token.first});
                    r = records.end() - 1;
                }
                next.push_back({r->key,device.name,input,device.isDefault!=0,token.second?token.first:std::string{},
                    publicBackend(state_->context.backend),token.second});
            }
        result.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::io_error;
    }
}
std::string AudioDeviceContext::backendName() const {
    return state_ ? ma_get_backend_name(state_->context.backend) : std::string{};
}
struct AudioDevice::State {
    std::shared_ptr<AudioDeviceContext::State> context;
    ma_device device{};
    bool ready = false;
    AudioBuffer<float> input, output;
    VectorOps ops;
    AudioDeviceConfig config;
    AudioDeviceCallback callback = nullptr;
    AudioDeviceContextCallback contextCallback = nullptr;
    std::atomic<std::uint32_t> calls{0},deadlines{0},lastNs{0},maxNs{0},clamped{0};
    void* user = nullptr;
    std::uint64_t position = 0;
    std::atomic<std::uint32_t> errors{0}, notifications{0};
    ~State() {
        if (ready)
            ma_device_uninit(&device);
    }
    static void notify(const ma_device_notification* message) noexcept {
        auto& self = *static_cast<State*>(message->pDevice->pUserData);
        std::uint32_t flag = 0;
        switch (message->type) {
        case ma_device_notification_type_started:
            flag = 1;
            break;
        case ma_device_notification_type_stopped:
            flag = 2;
            break;
        case ma_device_notification_type_rerouted:
            flag = 4;
            break;
        case ma_device_notification_type_interruption_began:
            flag = 8;
            break;
        case ma_device_notification_type_interruption_ended:
            flag = 16;
            break;
        case ma_device_notification_type_unlocked:
            flag = 32;
            break;
        }
        self.notifications.fetch_or(flag, std::memory_order_relaxed);
    }
    static void render(ma_device* device, void* out, const void* in, ma_uint32 frames) noexcept {
        auto& s = *static_cast<State*>(device->pUserData);
        const auto entry=(s.contextCallback||s.config.diagnostics)?nowNs():0;
        auto* output = static_cast<float*>(out);
        const auto* input = static_cast<const float*>(in);
        for (std::size_t first = 0; first < frames;) {
            const auto n = std::min<std::size_t>(s.config.maxCallbackFrames, frames - first);
            auto ib = s.config.inputChannels ? s.input.block(n) : AudioBlock<float>(nullptr, 0, n);
            auto ob =
                s.config.outputChannels ? s.output.block(n) : AudioBlock<float>(nullptr, 0, n);
            if(input&&ib.channels()&&!s.config.activeInputChannels.empty()) {
                for(std::size_t ch=0;ch<ib.channels();++ch)for(std::size_t i=0;i<n;++i)
                    ib.channel(ch)[i]=input[(first+i)*device->capture.channels+s.config.activeInputChannels[ch]];
            } else if (input && ib.channels())
                (void)deinterleave(
                    std::span<const float>(input + first * ib.channels(), n * ib.channels()), ib);
            else
                for (std::size_t ch = 0; ch < ib.channels(); ++ch)
                    s.ops.fill(ib.channel(ch), 0.0f);
            for (std::size_t ch = 0; ch < ob.channels(); ++ch)
                s.ops.fill(ob.channel(ch), 0.0f);
            AudioCallbackContext timing;
            if(s.contextCallback) {
                timing.clock={entry+std::uint64_t(double(first)*1e9/device->sampleRate),s.position,double(device->sampleRate)};
                timing.generation=s.config.generation;timing.nativeCallbackFrames=frames;
                timing.clockValid=true;timing.nativeCallbackSizeKnown=true;timing.callbackTime={timing.clock.monotonicNanoseconds,AudioMeasurementSource::callbackSteadyClock,true};
            }
            const auto result=s.contextCallback?s.contextCallback(s.user,ib,ob,timing):s.callback(s.user,ib,ob,s.position);
            if (result != Status::ok) {
                s.errors.fetch_add(1, std::memory_order_relaxed);
                for (std::size_t ch = 0; ch < ob.channels(); ++ch)
                    std::fill(ob.channel(ch).begin(), ob.channel(ch).end(), 0.0f);
            }
            if(output&&ob.channels()&&!s.config.activeOutputChannels.empty()) {
                std::fill_n(output+first*device->playback.channels,n*device->playback.channels,0.0f);
                for(std::size_t ch=0;ch<ob.channels();++ch)for(std::size_t i=0;i<n;++i)
                    output[(first+i)*device->playback.channels+s.config.activeOutputChannels[ch]]=ob.channel(ch)[i];
            } else if (output && ob.channels())
                (void)interleave(
                    ob, std::span<float>(output + first * ob.channels(), n * ob.channels()));
            s.position += n;
            first += n;
        }
        ++s.calls;
        if(entry) {
            const auto elapsed=nowNs()-entry;
            const auto bounded=std::uint32_t(std::min<std::uint64_t>(elapsed,UINT32_MAX));
            s.lastNs.store(bounded,std::memory_order_relaxed);
            if(bounded>s.maxNs.load(std::memory_order_relaxed))s.maxNs.store(bounded,std::memory_order_relaxed);
            if(elapsed>UINT32_MAX)s.clamped.store(1,std::memory_order_relaxed);
            if(double(elapsed)>double(frames)*1e9/device->sampleRate)++s.deadlines;
        }
    }
};
AudioDevice::AudioDevice() = default;
AudioDevice::~AudioDevice() = default;
AudioDevice::AudioDevice(AudioDevice&&) noexcept = default;
AudioDevice& AudioDevice::operator=(AudioDevice&&) noexcept = default;
Status AudioDevice::open(AudioDeviceContext& context,const AudioDeviceConfig& c,AudioDeviceCallback cb,void* user)noexcept {
    return openImpl(context,c,cb,nullptr,user);
}
Status AudioDevice::openWithContext(AudioDeviceContext& context,const AudioDeviceConfig& c,AudioDeviceContextCallback cb,void* user)noexcept {
    return openImpl(context,c,nullptr,cb,user);
}
Status AudioDevice::openImpl(AudioDeviceContext& context, const AudioDeviceConfig& c,
                         AudioDeviceCallback callback,AudioDeviceContextCallback extended, void* user) noexcept {
    if (!context.state_)
        return Status::not_prepared;
    if ((!callback&&!extended) || (!c.inputChannels && !c.outputChannels) || c.inputChannels > 256 ||
        c.outputChannels > 256 || !c.sampleRate || c.sampleRate > 768000 || !c.periodFrames ||
        c.periodFrames > 1048576 || !c.maxCallbackFrames || c.maxCallbackFrames > 1048576)
        return Status::invalid_argument;
    if(unsigned(c.access)>unsigned(AudioAccess::exclusive))return Status::invalid_argument;
    if(c.access==AudioAccess::exclusive&&context.backend()!=AudioBackend::wasapi&&context.backend()!=AudioBackend::alsa)
        return Status::unsupported_backend;
    if((!c.inputChannelMap.empty()&&c.inputChannelMap.size()!=c.inputChannels)||
       (!c.outputChannelMap.empty()&&c.outputChannelMap.size()!=c.outputChannels))return Status::invalid_argument;
    auto activeValid=[](const auto& channels,std::size_t count,bool output) {
        if(!channels.empty()&&channels.size()!=count)return false;
        for(std::size_t i=0;i<channels.size();++i) {
            if(channels[i]>=256)return false;
            if(output)for(std::size_t j=0;j<i;++j)if(channels[j]==channels[i])return false;
        }
        return true;
    };
    if(!activeValid(c.activeInputChannels,c.inputChannels,false)||!activeValid(c.activeOutputChannels,c.outputChannels,true)||
       (!c.activeInputChannels.empty()&&!c.inputChannelMap.empty())||(!c.activeOutputChannels.empty()&&!c.outputChannelMap.empty()))
        return Status::invalid_argument;
    for(auto v:c.inputChannelMap)if(v>MA_CHANNEL_AUX_31)return Status::invalid_argument;
    for(auto v:c.outputChannelMap)if(v>MA_CHANNEL_AUX_31)return Status::invalid_argument;
    try {
        auto next = std::make_unique<State>();
        const auto kernels = VectorOps::create(CpuBackend::automatic, next->ops);
        if (kernels != Status::ok)
            return kernels;
        next->context = context.state_;
        next->config = c;
        next->callback = callback;
        next->contextCallback=extended;
        next->user = user;
        if (c.inputChannels) {
            const auto status = next->input.resize(c.inputChannels, c.maxCallbackFrames);
            if (status != Status::ok)
                return status;
        }
        if (c.outputChannels) {
            const auto status = next->output.resize(c.outputChannels, c.maxCallbackFrames);
            if (status != Status::ok)
                return status;
        }
        auto config = ma_device_config_init(
            c.inputChannels ? (c.outputChannels ? ma_device_type_duplex : ma_device_type_capture)
                            : ma_device_type_playback);
        config.capture.shareMode=config.playback.shareMode=c.access==AudioAccess::exclusive?ma_share_mode_exclusive:ma_share_mode_shared;
        std::vector<ma_channel> inMap(c.inputChannelMap.begin(),c.inputChannelMap.end()),outMap(c.outputChannelMap.begin(),c.outputChannelMap.end());
        if(!inMap.empty())config.capture.pChannelMap=inMap.data();
        if(!outMap.empty())config.playback.pChannelMap=outMap.data();
        config.sampleRate = c.sampleRate;
        config.periodSizeInFrames = c.periodFrames;
        config.capture.format = ma_format_f32;
        config.playback.format = ma_format_f32;
        config.capture.channels = c.activeInputChannels.empty()?static_cast<ma_uint32>(c.inputChannels):*std::max_element(c.activeInputChannels.begin(),c.activeInputChannels.end())+1;
        config.playback.channels = c.activeOutputChannels.empty()?static_cast<ma_uint32>(c.outputChannels):*std::max_element(c.activeOutputChannels.begin(),c.activeOutputChannels.end())+1;
        auto select = [&](const std::string& key, bool input, const ma_device_id*& id) {
            if (key.empty())
                return true;
            for (const auto& r : context.state_->records)
                if (r.key == key && r.input == input) {
                    id = &r.id;
                    return true;
                }
            return false;
        };
        if (!select(c.inputId, true, config.capture.pDeviceID) ||
            !select(c.outputId, false, config.playback.pDeviceID))
            return Status::invalid_argument;
        config.dataCallback = &State::render;
        config.notificationCallback = &State::notify;
        config.pUserData = next.get();
        if (ma_device_init(&context.state_->context, &config, &next->device) != MA_SUCCESS)
            return Status::device_unavailable;
        next->ready = true;
        if(!c.activeInputChannels.empty()&&next->device.capture.internalChannels!=config.capture.channels)return Status::unsupported_backend;
        if(!c.activeOutputChannels.empty()&&next->device.playback.internalChannels!=config.playback.channels)return Status::unsupported_backend;
        if(c.inputChannels&&next->device.capture.shareMode!=config.capture.shareMode)return Status::unsupported_backend;
        if(c.outputChannels&&next->device.playback.shareMode!=config.playback.shareMode)return Status::unsupported_backend;
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::io_error;
    }
}
Status AudioDevice::description(AudioStreamDescription& output)const noexcept {
    if(!state_)return Status::not_prepared;
    try {
        const auto& s=*state_;AudioStreamDescription next;
        next.backend=publicBackend(s.context->context.backend);next.access=s.config.access;
        next.sampleRate=s.device.sampleRate;next.generation=s.config.generation;
        auto fill=[](auto& target,const auto& native,std::size_t channels) {
            target.channels=channels;target.nativeChannels=native.internalChannels;if(!channels)return;
            target.nativeSampleRate=native.internalSampleRate;
            target.periodFrames=native.internalPeriodSizeInFrames;target.periods=native.internalPeriods;
            target.periodKnown=target.periodFrames!=0;
            for(ma_uint32 i=0;i<native.channels;++i)target.channelMap.push_back(unsigned(native.channelMap[i]));
            // A buffer length is not a measured ADC/DAC latency.
        };
        fill(next.input,s.device.capture,s.config.inputChannels);fill(next.output,s.device.playback,s.config.outputChannels);
        next.input.activeChannels=s.config.activeInputChannels;next.output.activeChannels=s.config.activeOutputChannels;
        output=std::move(next);return Status::ok;
    }catch(...){return Status::allocation_failed;}
}
AudioCallbackDiagnostics AudioDevice::diagnostics()const noexcept {
    if(!state_)return {};
    const auto& s=*state_;
    return {s.calls.load(),s.errors.load(),s.deadlines.load(),0,s.lastNs.load(),s.maxNs.load(),false,s.clamped.load()!=0};
}
Status AudioDevice::start() noexcept {
    return !state_                                          ? Status::not_prepared
           : ma_device_start(&state_->device) == MA_SUCCESS ? Status::ok
                                                            : Status::io_error;
}
Status AudioDevice::stop() noexcept {
    return !state_                                         ? Status::not_prepared
           : ma_device_stop(&state_->device) == MA_SUCCESS ? Status::ok
                                                           : Status::io_error;
}
void AudioDevice::close() noexcept {
    state_.reset();
}
bool AudioDevice::running() const noexcept {
    return state_ && ma_device_is_started(&state_->device);
}
std::uint32_t AudioDevice::sampleRate() const noexcept {
    return state_ ? state_->device.sampleRate : 0;
}
std::uint32_t AudioDevice::callbackErrors() const noexcept {
    return state_ ? state_->errors.load(std::memory_order_relaxed) : 0;
}
std::uint32_t AudioDevice::takeNotifications() noexcept {
    return state_ ? state_->notifications.exchange(0, std::memory_order_relaxed) : 0;
}
} // namespace cydsp
