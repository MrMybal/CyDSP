#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/midi/MidiEventCollector.h"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
namespace cydsp {
enum class AudioBackend { automatic, wasapi, directSound, winmm, coreAudio, alsa, pulseAudio, jack, aaudio, openSl, webAudio, nullDevice, asio, custom };
enum class AudioAccess { shared, exclusive };
enum class AudioSampleFormat { unknown, unsigned8, signed16, signed24, signed32, float32 };
struct AudioContextOptions { AudioBackend backend = AudioBackend::automatic; };
struct AudioNativeFormat {
    AudioSampleFormat format = AudioSampleFormat::unknown;
    std::uint32_t channels = 0, sampleRate = 0; // Zero means backend reported wildcard/unknown.
    bool exclusive = false;
};
struct AudioDeviceCapabilities {
    std::vector<AudioNativeFormat> nativeFormats;
    std::vector<std::string> channelNames;
    bool nativeFormatsKnown = false, channelNamesKnown = false;
    bool bufferRangeKnown = false, duplexKnown = false, duplex = false;
    std::uint32_t minimumBufferFrames = 0, maximumBufferFrames = 0;
};
enum class AudioMeasurementSource { unknown, callbackSteadyClock, backendDeclared, backendMeasured };
struct AudioTimingValue {
    std::uint64_t nanoseconds = 0;
    AudioMeasurementSource source = AudioMeasurementSource::unknown;
    bool valid = false;
};
struct AudioStreamDirection {
    std::size_t channels = 0;
    std::uint32_t nativeSampleRate = 0, periodFrames = 0, periods = 0;
    std::vector<unsigned> channelMap, activeChannels;
    std::size_t nativeChannels = 0;
    AudioTimingValue latency;
    bool periodKnown = false;
};
struct AudioStreamDescription {
    AudioBackend backend = AudioBackend::automatic;
    AudioAccess access = AudioAccess::shared;
    std::uint32_t sampleRate = 0, generation = 0;
    AudioStreamDirection input, output;
};
struct AudioCallbackContext {
    AudioClockAnchor clock;
    std::uint32_t generation = 0, nativeCallbackFrames = 0;
    AudioTimingValue callbackTime, adcTime, dacTime;
    bool clockValid = false, nativeCallbackSizeKnown = false;
};
struct AudioCallbackDiagnostics {
    std::uint32_t callbacks = 0, callbackErrors = 0, deadlineMisses = 0, deviceXruns = 0;
    std::uint64_t lastDurationNanoseconds = 0, maximumDurationNanoseconds = 0;
    bool deviceXrunsKnown = false, durationClamped = false;
};
struct AudioDeviceInfo {
    std::string id, name;
    bool input = false, isDefault = false;
    std::string persistentId{};
    AudioBackend backend = AudioBackend::automatic;
    bool persistentIdentity = false;
};
struct AudioDeviceSelector {
    std::string persistentId, name;
    AudioBackend backend = AudioBackend::automatic;
    bool input = false, useDefault = false, allowNameFallback = false;
};
// Read-only resolver: never selects the first of ambiguous same-name devices.
Status resolveAudioDevice(std::span<const AudioDeviceInfo>, const AudioDeviceSelector&, std::size_t& index) noexcept;
struct AudioDeviceConfig {
    std::string inputId, outputId; // Empty selects the system default; serialize a selector when persistence matters.
    std::uint32_t sampleRate = 48000, periodFrames = 256;
    std::size_t inputChannels = 0, outputChannels = 2, maxCallbackFrames = 512;
    AudioAccess access = AudioAccess::shared;
    std::vector<unsigned> inputChannelMap{}, outputChannelMap{};
    std::vector<unsigned> activeInputChannels{}, activeOutputChannels{};
    std::uint32_t generation = 1;
    bool diagnostics = false;
};
using AudioDeviceContextCallback = Status (*)(void*, AudioBlock<const float>, AudioBlock<float>,
    const AudioCallbackContext&) noexcept;
using AudioDeviceCallback = Status (*)(void*, AudioBlock<const float>, AudioBlock<float>,
                                       std::uint64_t firstFrame) noexcept;
enum class AudioDeviceNotification : std::uint32_t {
    started = 1,
    stopped = 2,
    rerouted = 4,
    interruptionBegan = 8,
    interruptionEnded = 16,
    unlocked = 32
};
class AudioDevice;
// Context/device lifecycle and enumeration are control-thread operations, externally serialized.
class AudioDeviceContext {
  public:
    AudioDeviceContext();
    ~AudioDeviceContext();
    AudioDeviceContext(AudioDeviceContext&&) noexcept;
    AudioDeviceContext& operator=(AudioDeviceContext&&) noexcept;
    Status prepare(bool virtualDeviceOnly = false) noexcept;
    Status prepare(const AudioContextOptions&) noexcept;
    Status capabilities(std::string_view id, bool input, AudioDeviceCapabilities&) noexcept;
    [[nodiscard]] AudioBackend backend() const noexcept;
    Status enumerate(std::vector<AudioDeviceInfo>&) noexcept;
    [[nodiscard]] std::string backendName() const;

  private:
    struct State;
    std::shared_ptr<State> state_;
    friend class AudioDevice;
};
class AudioDevice {
  public:
    AudioDevice();
    ~AudioDevice();
    AudioDevice(AudioDevice&&) noexcept;
    AudioDevice& operator=(AudioDevice&&) noexcept;
    // Callback/context must remain valid until stop/close returns. Do not call lifecycle from
    // callback.
    Status open(AudioDeviceContext&, const AudioDeviceConfig&, AudioDeviceCallback, void*) noexcept;
    Status openWithContext(AudioDeviceContext&, const AudioDeviceConfig&, AudioDeviceContextCallback, void*) noexcept;
    Status description(AudioStreamDescription&) const noexcept;
    [[nodiscard]] AudioCallbackDiagnostics diagnostics() const noexcept;
    Status start() noexcept;
    Status stop() noexcept;
    void close() noexcept;
    [[nodiscard]] bool running() const noexcept;
    [[nodiscard]] std::uint32_t sampleRate() const noexcept;
    [[nodiscard]] std::uint32_t callbackErrors() const noexcept;
    [[nodiscard]] std::uint32_t takeNotifications() noexcept;

  private:
    Status openImpl(AudioDeviceContext&, const AudioDeviceConfig&, AudioDeviceCallback,
                    AudioDeviceContextCallback, void*) noexcept;
    struct State;
    std::unique_ptr<State> state_;
};
} // namespace cydsp
