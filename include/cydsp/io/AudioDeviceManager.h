#pragma once
#include "cydsp/io/AudioDevice.h"
namespace cydsp {
enum class AudioManagerEvent {
    deviceListChanged,
    connected,
    disconnected,
    rerouted,
    interrupted,
    resumed,
    failed
};
struct AudioManagerCallbacks {
    void* context = nullptr;
    void (*changed)(void*, AudioManagerEvent, Status) noexcept = nullptr;
};
struct AudioReconnectOptions {
    bool automatic = true, fallbackToDefault = false;
    std::uint32_t scanIntervalMs = 250, retryIntervalMs = 1000;
};
// Optional host-owned driver. All calls are serialized control/worker operations.
// Native callbacks must be stopped before close returns. Context outlives manager.
struct AudioDeviceDriver {
    void* context = nullptr;
    Status (*enumerate)(void*, std::vector<AudioDeviceInfo>&) noexcept = nullptr;
    Status (*open)(void*, const AudioDeviceConfig&, AudioDeviceCallback, void*) noexcept = nullptr;
    Status (*start)(void*) noexcept = nullptr;
    void (*close)(void*) noexcept = nullptr;
    bool (*running)(void*) noexcept = nullptr;
    std::uint32_t (*sampleRate)(void*) noexcept = nullptr;
    std::uint32_t (*notifications)(void*) noexcept = nullptr;
    Status (*description)(void*,AudioStreamDescription&) noexcept = nullptr;
    Status (*capabilities)(void*,std::string_view,bool,AudioDeviceCapabilities&) noexcept = nullptr;
};
// Owns the native context by default. Poll on the control owner; no hidden thread.
// Callbacks execute from this owner and must not reenter lifecycle/poll operations.
class AudioDeviceManager {
  public:
    AudioDeviceManager();
    ~AudioDeviceManager();
    AudioDeviceManager(AudioDeviceManager&&) noexcept;
    AudioDeviceManager& operator=(AudioDeviceManager&&) noexcept;
    Status prepare(bool virtualDeviceOnly = false, AudioManagerCallbacks = {}) noexcept;
    Status prepare(const AudioContextOptions&,AudioManagerCallbacks = {}) noexcept;
    Status description(AudioStreamDescription&) const noexcept;
    Status capabilities(std::string_view id,bool input,AudioDeviceCapabilities&) noexcept;
    Status prepare(AudioDeviceDriver, AudioManagerCallbacks = {}) noexcept;
    Status enumerate(std::vector<AudioDeviceInfo>&) noexcept;
    // Selects configuration and starts it. Failed initial connection remains eligible
    // for automatic retry. Stream frame numbering restarts on each generation.
    Status open(const AudioDeviceConfig&, AudioDeviceCallback, void*,
                const AudioReconnectOptions& = {}, std::uint64_t nowMs = 0) noexcept;
    Status openWithContext(const AudioDeviceConfig&,AudioDeviceContextCallback,void*,
        const AudioReconnectOptions& = {},std::uint64_t nowMs=0) noexcept;
    Status poll(std::uint64_t nowMs) noexcept;
    void close() noexcept;
    [[nodiscard]] bool running() const noexcept;
    [[nodiscard]] std::uint32_t generation() const noexcept;
    [[nodiscard]] std::uint32_t sampleRate() const noexcept;
    [[nodiscard]] Status lastError() const noexcept;

  private:
    Status openImpl(const AudioDeviceConfig&,AudioDeviceCallback,AudioDeviceContextCallback,void*,
                    const AudioReconnectOptions&,std::uint64_t) noexcept;
    struct State;
    std::unique_ptr<State> state_;
};
} // namespace cydsp
