#include <cydsp/CyDSP.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
namespace {
using namespace cydsp;
void require(bool v,const char* why){if(!v)throw std::runtime_error(why);}
void ok(Status r){require(r==Status::ok,"device contract status");}
void selectors() {
    std::vector<AudioDeviceInfo> list{{"old0","Same",false,false,"uidA",AudioBackend::wasapi,true},
        {"old1","Same",false,false,"uidB",AudioBackend::wasapi,true}};
    AudioDeviceSelector s;s.backend=AudioBackend::wasapi;s.persistentId="uidB";
    std::size_t index=99;ok(resolveAudioDevice(list,s,index));require(index==1,"persistent device");
    std::reverse(list.begin(),list.end());list[0].id="new1";ok(resolveAudioDevice(list,s,index));require(index==0,"restart/reorder identity");
    s.persistentId="removed";require(resolveAudioDevice(list,s,index)==Status::device_unavailable&&index==0,"removed identity no guess");
    s.persistentId.clear();s.name="Same";
    require(resolveAudioDevice(list,s,index)==Status::ambiguous,"duplicate names explicit ambiguity");
    list[1].isDefault=true;s.useDefault=true;ok(resolveAudioDevice(list,s,index));require(index==1,"explicit default");
}
void negotiated() {
    struct Fake {
        AudioDeviceCallback cb=nullptr;void* user=nullptr;bool running=false;
        AudioDeviceDriver driver() {
            AudioDeviceDriver d;d.context=this;
            d.enumerate=[](void*,std::vector<AudioDeviceInfo>& out)noexcept {
                try{out={{"fake","Fake",false,true}};return Status::ok;}catch(...){return Status::allocation_failed;}
            };
            d.open=[](void* p,const AudioDeviceConfig&,AudioDeviceCallback cb,void* user)noexcept {
                auto& s=*static_cast<Fake*>(p);s.cb=cb;s.user=user;return Status::ok;
            };
            d.start=[](void* p)noexcept {static_cast<Fake*>(p)->running=true;return Status::ok;};
            d.close=[](void* p)noexcept {static_cast<Fake*>(p)->running=false;};
            d.running=[](void* p)noexcept{return static_cast<Fake*>(p)->running;};
            d.sampleRate=[](void*)noexcept{return std::uint32_t(44100);};
            d.description=[](void*,AudioStreamDescription& out)noexcept {
                out={};out.backend=AudioBackend::custom;out.sampleRate=44100;
                out.output.channels=2;out.output.nativeChannels=8;out.output.nativeSampleRate=44100;
                out.output.periodFrames=192;out.output.periodKnown=true;return Status::ok;
            };
            d.capabilities=[](void*,std::string_view,bool,AudioDeviceCapabilities& out)noexcept {
                try{out={};out.nativeFormats={{AudioSampleFormat::float32,8,44100,false}};
                    out.nativeFormatsKnown=true;return Status::ok;}catch(...){return Status::allocation_failed;}
            };return d;
        }
    } fake;
    AudioDeviceManager manager;ok(manager.prepare(fake.driver()));
    AudioDeviceConfig request;request.sampleRate=48000;request.periodFrames=256;
    struct Observed {bool valid=false;} observed;
    ok(manager.openWithContext(request,[](void* p,AudioBlock<const float>,AudioBlock<float>,const AudioCallbackContext& c)noexcept {
        static_cast<Observed*>(p)->valid=c.clockValid&&c.clock.sampleRate==44100&&c.generation==1&&
            c.callbackTime.valid&&!c.adcTime.valid&&!c.dacTime.valid&&!c.nativeCallbackSizeKnown;return Status::ok;
    },&observed));
    AudioStreamDescription actual;ok(manager.description(actual));
    require(actual.sampleRate==44100&&actual.output.periodFrames==192&&actual.output.nativeChannels==8&&
        !actual.output.latency.valid,"negotiated fields distinguish requested/unknown");
    AudioBuffer<float> out;ok(out.resize(2,32));ok(fake.cb(fake.user,{nullptr,0,32},out.block(),0));
    require(observed.valid,"manager clock/generation before start");
    AudioDeviceCapabilities capabilities;ok(manager.capabilities("fake",false,capabilities));
    require(capabilities.nativeFormatsKnown&&capabilities.nativeFormats[0].sampleRate==44100&&
        !capabilities.channelNamesKnown&&!capabilities.bufferRangeKnown,"no fabricated capabilities");
    manager.close();
}
void nullTiming() {
    AudioDeviceContext context;ok(context.prepare(AudioContextOptions{AudioBackend::nullDevice}));
    require(context.backend()==AudioBackend::nullDevice,"explicit backend");
    require(context.prepare(AudioContextOptions{AudioBackend::asio})==Status::unsupported_backend&&
        context.backend()==AudioBackend::nullDevice,"unavailable backend transactional");
    AudioDevice device;AudioDeviceConfig config;config.maxCallbackFrames=37;config.generation=42;config.diagnostics=true;
    std::atomic<unsigned> calls{0},invalid{0};
    struct Context {std::atomic<unsigned>* calls;std::atomic<unsigned>* invalid;} user{&calls,&invalid};
    ok(device.openWithContext(context,config,[](void* p,AudioBlock<const float>,AudioBlock<float> out,const AudioCallbackContext& c)noexcept {
        auto& u=*static_cast<Context*>(p);
        if(!c.clockValid||c.clock.sampleRate!=48000||c.generation!=42||!c.nativeCallbackSizeKnown||
           c.nativeCallbackFrames<out.frames()||c.adcTime.valid||c.dacTime.valid)++*u.invalid;
        ++*u.calls;return Status::ok;
    },&user));
    AudioStreamDescription negotiated;ok(device.description(negotiated));
    require(negotiated.output.channels==2&&!negotiated.output.latency.valid&&negotiated.sampleRate==48000,"null stream");
    ok(device.start());
    for(unsigned i=0;i<100&&calls.load()<20;++i)std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ok(device.stop());auto d=device.diagnostics();
    require(calls.load()>=20&&!invalid.load()&&d.callbacks>0&&d.maximumDurationNanoseconds>0&&
        !d.deviceXrunsKnown,"callback diagnostics");
    config.access=AudioAccess::exclusive;
    require(device.openWithContext(context,config,[](void*,auto,auto,const auto&)noexcept{return Status::ok;},nullptr)==Status::unsupported_backend,"exclusive never silently shared");
}
}
void deviceContractsRegression(){selectors();negotiated();nullTiming();std::cout<<"PASS integration device identity/capabilities/negotiation/timing\n";}
