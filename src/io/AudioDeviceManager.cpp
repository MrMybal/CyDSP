#include "cydsp/io/AudioDeviceManager.h"
#include <algorithm>
#include <atomic>
#include <chrono>
namespace cydsp {
struct AudioDeviceManager::State {
    struct Native {
        AudioDeviceContext context;
        AudioDevice device;
    } native;
    AudioDeviceDriver driver;
    AudioManagerCallbacks callbacks;
    AudioReconnectOptions options;
    AudioDeviceConfig requested;
    AudioDeviceCallback render = nullptr;
    void* renderContext = nullptr;
    AudioDeviceContextCallback extended=nullptr;
    void* extendedUser=nullptr;
    static Status bridge(void* ptr,AudioBlock<const float> in,AudioBlock<float> out,std::uint64_t frame) noexcept {
        auto& s=*static_cast<State*>(ptr);AudioCallbackContext c;
        const auto ns=std::uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
        c.clock={ns,frame,double(s.rate.load())};c.generation=s.generation.load();
        c.nativeCallbackFrames=0;c.nativeCallbackSizeKnown=false;
        c.clockValid=c.clock.sampleRate>0;c.callbackTime={ns,AudioMeasurementSource::callbackSteadyClock,true};
        return s.extended(s.extendedUser,in,out,c);
    }
    std::vector<AudioDeviceInfo> devices;
    std::string inputKey, outputKey;
    std::uint64_t scannedAt = 0, retriedAt = 0;
    std::atomic<std::uint32_t> generation{0}, rate{0};
    Status error = Status::ok;
    bool wanted = false, opened = false, scanned = false, retried = false, processing = false;
    ~State() {
        if (opened)
            driver.close(driver.context);
    }
    void event(AudioManagerEvent e, Status status = Status::ok) noexcept {
        if (callbacks.changed)
            callbacks.changed(callbacks.context, e, status);
    }
    void failed(Status status) noexcept {
        const bool changed = error != status;
        error = status;
        if (changed)
            event(AudioManagerEvent::failed, status);
    }
    void disconnect() noexcept {
        if (opened) {
            driver.close(driver.context);
            opened = false;
            rate.store(0);
            error = Status::device_unavailable;
            event(AudioManagerEvent::disconnected);
        }
    }
    Status scan() noexcept {
        std::vector<AudioDeviceInfo> next;
        auto r = driver.enumerate(driver.context, next);
        if (r != Status::ok)
            return r;
        const bool same =
            next.size() == devices.size() &&
            std::equal(next.begin(), next.end(), devices.begin(), [](const auto& a, const auto& b) {
                return a.id == b.id && a.name == b.name && a.input == b.input &&
                       a.isDefault == b.isDefault;
            });
        devices.swap(next);
        if (!same)
            event(AudioManagerEvent::deviceListChanged);
        return Status::ok;
    }
    bool resolve(bool input, const std::string& preferred, std::string& key,
                 std::string& selected) {
        if (!preferred.empty()) {
            auto found = std::find_if(devices.begin(), devices.end(), [&](const auto& d) {
                return d.input == input && d.id == preferred;
            });
            if (found != devices.end()) {
                key = found->id;
                selected = preferred;
                return true;
            }
            if (!options.fallbackToDefault)
                return false;
        }
        auto found = std::find_if(devices.begin(), devices.end(),
                                  [&](const auto& d) { return d.input == input && d.isDefault; });
        if (found == devices.end())
            return false;
        key = found->id;
        selected.clear();
        return true;
    }
    Status connect(std::uint64_t now) {
        AudioDeviceConfig config = requested;
        std::string input, output;
        if ((config.inputChannels && !resolve(true, requested.inputId, input, config.inputId)) ||
            (config.outputChannels &&
             !resolve(false, requested.outputId, output, config.outputId))) {
            disconnect();
            failed(Status::device_unavailable);
            return error;
        }
        if (opened && (!driver.running(driver.context) || input != inputKey || output != outputKey))
            disconnect();
        if (opened) {
            error = Status::ok;
            return Status::ok;
        }
        if (retried &&
            (!options.automatic || (now >= retriedAt && now - retriedAt < options.retryIntervalMs)))
            return error;
        retried = true;
        retriedAt = now;
        config.generation=generation.load()+1;
        auto r = driver.open(driver.context, config, render, renderContext);
        if (r != Status::ok) {
            failed(r);
            return r;
        }
        opened = true;
        rate.store(driver.sampleRate(driver.context));
        generation.store(config.generation);
        r = driver.start(driver.context);
        if (r != Status::ok) {
            disconnect();
            failed(r);
            return r;
        }
        inputKey = std::move(input);
        outputKey = std::move(output);
        rate.store(driver.sampleRate(driver.context));
        error = Status::ok;
        event(AudioManagerEvent::connected);
        return Status::ok;
    }
};
namespace {
bool validDriver(const AudioDeviceDriver& d) noexcept {
    return d.enumerate && d.open && d.start && d.close && d.running && d.sampleRate;
}
} // namespace
AudioDeviceManager::AudioDeviceManager() = default;
AudioDeviceManager::~AudioDeviceManager() = default;
AudioDeviceManager::AudioDeviceManager(AudioDeviceManager&&) noexcept = default;
AudioDeviceManager& AudioDeviceManager::operator=(AudioDeviceManager&&) noexcept = default;
Status AudioDeviceManager::prepare(bool onlyNull,AudioManagerCallbacks callbacks) noexcept {
    return prepare(AudioContextOptions{onlyNull?AudioBackend::nullDevice:AudioBackend::automatic},callbacks);
}
Status AudioDeviceManager::prepare(const AudioContextOptions& options,AudioManagerCallbacks callbacks) noexcept {
    if (state_ && state_->processing)
        return Status::transition_in_progress;
    try {
        auto next = std::make_unique<State>();
        const auto r = next->native.context.prepare(options);
        if (r != Status::ok)
            return r;
        auto& d = next->driver;
        d.context = &next->native;
        d.enumerate = [](void* p, std::vector<AudioDeviceInfo>& list) noexcept {
            return static_cast<State::Native*>(p)->context.enumerate(list);
        };
        d.open = [](void* p, const AudioDeviceConfig& c, AudioDeviceCallback cb,
                    void* ctx) noexcept {
            auto& n = *static_cast<State::Native*>(p);
            return n.device.open(n.context, c, cb, ctx);
        };
        d.start = [](void* p) noexcept { return static_cast<State::Native*>(p)->device.start(); };
        d.close = [](void* p) noexcept { static_cast<State::Native*>(p)->device.close(); };
        d.running = [](void* p) noexcept {
            return static_cast<State::Native*>(p)->device.running();
        };
        d.sampleRate = [](void* p) noexcept {
            return static_cast<State::Native*>(p)->device.sampleRate();
        };
        d.notifications = [](void* p) noexcept {
            return static_cast<State::Native*>(p)->device.takeNotifications();
        };
        d.description=[](void* p,AudioStreamDescription& out)noexcept{return static_cast<State::Native*>(p)->device.description(out);};
        d.capabilities=[](void* p,std::string_view id,bool input,AudioDeviceCapabilities& out)noexcept{return static_cast<State::Native*>(p)->context.capabilities(id,input,out);};
        next->callbacks = callbacks;
        state_.swap(next);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status AudioDeviceManager::prepare(AudioDeviceDriver driver,
                                   AudioManagerCallbacks callbacks) noexcept {
    if (state_ && state_->processing)
        return Status::transition_in_progress;
    if (!validDriver(driver))
        return Status::invalid_argument;
    try {
        auto next = std::make_unique<State>();
        next->driver = driver;
        next->callbacks = callbacks;
        state_.swap(next);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status AudioDeviceManager::enumerate(std::vector<AudioDeviceInfo>& list) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (state_->processing)
        return Status::transition_in_progress;
    return state_->driver.enumerate(state_->driver.context, list);
}
Status AudioDeviceManager::open(const AudioDeviceConfig& c,AudioDeviceCallback cb,void* user,const AudioReconnectOptions& o,std::uint64_t now) noexcept {
    return openImpl(c,cb,nullptr,user,o,now);
}
Status AudioDeviceManager::openWithContext(const AudioDeviceConfig& c,AudioDeviceContextCallback cb,void* user,const AudioReconnectOptions& o,std::uint64_t now) noexcept {
    return openImpl(c,nullptr,cb,user,o,now);
}
Status AudioDeviceManager::openImpl(const AudioDeviceConfig& config, AudioDeviceCallback callback,
                                AudioDeviceContextCallback extended,void* context, const AudioReconnectOptions& options,
                                std::uint64_t now) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (s.processing)
        return Status::transition_in_progress;
    if ((!callback&&!extended) || (!config.inputChannels && !config.outputChannels) ||
        config.inputChannels > 256 || config.outputChannels > 256 || !config.sampleRate ||
        config.sampleRate > 768000 || !config.periodFrames || config.periodFrames > 1048576 ||
        !config.maxCallbackFrames || config.maxCallbackFrames > 1048576 ||
        !options.scanIntervalMs || options.scanIntervalMs > 60000 || !options.retryIntervalMs ||
        options.retryIntervalMs > 60000)
        return Status::invalid_argument;
    s.processing = true;
    struct Guard {
        bool& p;
        ~Guard() {
            p = false;
        }
    } guard{s.processing};
    try {
        // Copy potentially allocating preferences before stopping the current stream.
        auto requested = config;
        s.disconnect();
        s.requested = std::move(requested);
        s.extended=extended;s.extendedUser=context;
        s.render=callback?callback:&State::bridge;
        s.renderContext=callback?context:&s;
        s.options = options;
        s.wanted = true;
        s.retried = false;
        s.error = Status::not_prepared;
        auto r = s.scan();
        s.scanned = true;
        s.scannedAt = now;
        if (r != Status::ok) {
            s.failed(r);
            return r;
        }
        return s.connect(now);
    } catch (const std::bad_alloc&) {
        s.failed(Status::allocation_failed);
        return s.error;
    } catch (...) {
        s.failed(Status::io_error);
        return s.error;
    }
}
Status AudioDeviceManager::description(AudioStreamDescription& out)const noexcept {
    if(!state_||!state_->opened)return Status::not_prepared;
    return state_->driver.description?state_->driver.description(state_->driver.context,out):Status::unsupported_backend;
}
Status AudioDeviceManager::capabilities(std::string_view id,bool input,AudioDeviceCapabilities& out) noexcept {
    if(!state_)return Status::not_prepared;
    return state_->driver.capabilities?state_->driver.capabilities(state_->driver.context,id,input,out):Status::unsupported_backend;
}
Status AudioDeviceManager::poll(std::uint64_t now) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (s.processing)
        return Status::transition_in_progress;
    s.processing = true;
    struct Guard {
        bool& p;
        ~Guard() {
            p = false;
        }
    } guard{s.processing};
    if (!s.wanted)
        return Status::ok;
    try {
        const auto flags =
            s.opened && s.driver.notifications ? s.driver.notifications(s.driver.context) : 0;
        if (flags & 4)
            s.event(AudioManagerEvent::rerouted);
        if (flags & 8)
            s.event(AudioManagerEvent::interrupted);
        if (flags & 16)
            s.event(AudioManagerEvent::resumed);
        if (s.scanned && now >= s.scannedAt && now - s.scannedAt < s.options.scanIntervalMs)
            return s.error;
        s.scanned = true;
        s.scannedAt = now;
        const auto r = s.scan();
        if (r != Status::ok) {
            s.failed(r);
            return r;
        }
        return s.connect(now);
    } catch (const std::bad_alloc&) {
        s.failed(Status::allocation_failed);
        return s.error;
    } catch (...) {
        s.failed(Status::io_error);
        return s.error;
    }
}
void AudioDeviceManager::close() noexcept {
    if (!state_ || state_->processing)
        return;
    state_->processing = true;
    state_->wanted = false;
    state_->disconnect();
    state_->error = Status::ok;
    state_->processing = false;
}
bool AudioDeviceManager::running() const noexcept {
    return state_ && state_->opened && state_->driver.running(state_->driver.context);
}
std::uint32_t AudioDeviceManager::generation() const noexcept {
    return state_ ? state_->generation.load() : 0;
}
std::uint32_t AudioDeviceManager::sampleRate() const noexcept {
    return state_ ? state_->rate.load() : 0;
}
Status AudioDeviceManager::lastError() const noexcept {
    return state_ ? state_->error : Status::not_prepared;
}
} // namespace cydsp
