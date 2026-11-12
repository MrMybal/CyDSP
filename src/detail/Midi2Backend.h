#pragma once
#include "cydsp/io/Midi2Device.h"
namespace cydsp::detail {
using ReceiveMidi2 = void (*)(void*, std::span<const std::uint32_t>, std::uint64_t) noexcept;
struct Midi2NativeRequest {
    Midi2EndpointInfo endpoint;
    int inputFd = -1, outputFd = -1, receivePort = -1, sendPort = -1;
    std::shared_ptr<void> android;
};
struct NativeMidi2 {
    virtual ~NativeMidi2() = default;
    virtual Status send(const Midi2TimedPacket&) noexcept = 0;
    virtual Status poll() noexcept = 0;
    virtual bool schedules() const noexcept {
        return false;
    }
};
Status enumerateNativeMidi2(std::vector<Midi2EndpointInfo>&) noexcept;
Status createNativeMidi2(const Midi2NativeRequest&, ReceiveMidi2, void*,
                         std::unique_ptr<NativeMidi2>&) noexcept;
#if !defined(_WIN32)
Status createFdMidi2(const Midi2NativeRequest&, ReceiveMidi2, void*,
                     std::unique_ptr<NativeMidi2>&) noexcept;
#endif
#if defined(__ANDROID__)
Status prepareAndroidMidi2(void*, void*, int, int, Midi2NativeRequest&) noexcept;
#endif
} // namespace cydsp::detail
