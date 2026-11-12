#pragma once
#include "cydsp/midi/Ump.h"
#include <memory>
#include <string>
namespace cydsp {
enum class Midi2Backend {
    automatic,
    windowsServices,
    coreMidi,
    linuxUmp,
    androidAmidi,
    fileDescriptors,
    loopback
};
struct Midi2EndpointInfo {
    std::string id, name;
    Midi2Backend backend = Midi2Backend::automatic;
    bool receive = true, send = true;
};
struct Midi2DeviceOptions {
    std::size_t incomingPackets = 1024, outgoingPackets = 1024, queuedSysEx = 8;
    std::size_t maximumSysExBytes = 4096;
};
struct Midi2TimedPacket {
    UmpPacket packet;
    std::uint64_t timestampNs = 0;
};
// Native discovery only. Windows needs current Windows MIDI Services, Apple CoreMIDI 11+,
// Linux /dev/snd/umpC*D*. Android discovery/USB permissions belong to the host MidiManager.
Status enumerateMidi2Endpoints(std::vector<Midi2EndpointInfo>&) noexcept;
[[nodiscard]] std::uint64_t
midi2TimestampNow() noexcept; // Same steady-clock domain as timestampNs.
class Midi2Device {
  public:
    Midi2Device();
    ~Midi2Device();
    Midi2Device(Midi2Device&&) noexcept;
    Midi2Device& operator=(Midi2Device&&) noexcept;
    Status open(const Midi2EndpointInfo&, const Midi2DeviceOptions& = {}) noexcept;
    Status openLoopback(const Midi2DeviceOptions& = {}) noexcept;
    // POSIX UMP character devices or sockets (SIGPIPE-protected): duplicate caller-owned, already
    // nonblocking descriptors. -1 disables a direction.
    Status openFileDescriptors(int inputFd, int outputFd, const Midi2DeviceOptions& = {}) noexcept;
    // Android host opens a MIDI 2/UMP Java MidiDevice first. Pointers are JNIEnv* / jobject.
    // Ports are device output (receive) / device input (send); -1 disables a direction.
    Status openAndroid(void* jniEnvironment, void* javaMidiDevice, int receivePort, int sendPort,
                       const Midi2DeviceOptions& = {}) noexcept;
    void close() noexcept; // Control thread, after producers/consumer idle. Joins I/O worker.
    // Single audio producer, no I/O/locks/allocations. Success means queued, not delivered.
    Status send(const UmpPacket&, std::uint64_t timestampNs = 0) noexcept;
    // Separate single control producer for CI/SysEx. Atomically queues a whole payload (no F0/F7).
    Status sendSysEx7(unsigned group, std::span<const std::uint8_t>,
                      std::uint64_t timestampNs = 0) noexcept;
    // One consumer; no I/O/locks/allocations. Empty active stream returns would_block.
    Status read(Midi2TimedPacket&) noexcept;
    [[nodiscard]] Status lastError() const noexcept;
    [[nodiscard]] std::uint32_t droppedPackets() const noexcept;
    [[nodiscard]] Midi2Backend backend() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
} // namespace cydsp
