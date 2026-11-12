#include "detail/Midi2Backend.h"
#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
namespace cydsp::detail {
namespace {
struct FdMidi final : NativeMidi2 {
    int input = -1, output = -1;
    bool socketOutput = false;
    std::size_t sent = 0, have = 0;
    std::array<std::uint8_t, 4> partial{};
    ReceiveMidi2 receive;
    void* context;
    FdMidi(ReceiveMidi2 f, void* c) : receive(f), context(c) {}
    ~FdMidi() override {
        if (input >= 0)
            ::close(input);
        if (output >= 0)
            ::close(output);
    }
    Status send(const Midi2TimedPacket& p) noexcept override {
        if (output < 0)
            return Status::unsupported_format;
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(p.packet.words.data());
        const auto size = static_cast<std::size_t>(p.packet.count) * 4;
#if defined(MSG_NOSIGNAL)
        const auto n = socketOutput ? ::send(output, bytes + sent, size - sent, MSG_NOSIGNAL)
                                    : ::write(output, bytes + sent, size - sent);
#else
        const auto n = ::write(output, bytes + sent, size - sent);
#endif
        if (n < 0)
            return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ? Status::would_block
                                                                             : Status::io_error;
        sent += static_cast<std::size_t>(n);
        if (sent < size)
            return Status::would_block;
        sent = 0;
        return Status::ok;
    }
    Status poll() noexcept override {
        if (input < 0)
            return Status::ok;
        std::array<std::uint8_t, 4096> bytes{};
        const auto n = ::read(input, bytes.data(), bytes.size());
        if (n < 0)
            return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ? Status::would_block
                                                                             : Status::io_error;
        if (!n)
            return Status::device_unavailable;
        const auto time = midi2TimestampNow();
        for (ssize_t i = 0; i < n; ++i) {
            partial[have++] = bytes[static_cast<std::size_t>(i)];
            if (have == 4) {
                std::uint32_t word;
                std::memcpy(&word, partial.data(), 4);
                receive(context, {&word, 1}, time);
                have = 0;
            }
        }
        return Status::ok;
    }
};
} // namespace
Status createFdMidi2(const Midi2NativeRequest& request, ReceiveMidi2 receive, void* context,
                     std::unique_ptr<NativeMidi2>& result) noexcept {
    try {
        auto next = std::make_unique<FdMidi>(receive, context);
        for (unsigned i = 0; i < 2; ++i) {
            const auto fd = i ? request.outputFd : request.inputFd;
            if (fd < 0)
                continue;
            const auto flags = fcntl(fd, F_GETFL);
            if (flags < 0 || !(flags & O_NONBLOCK) ||
                (i ? (flags & O_ACCMODE) == O_RDONLY : (flags & O_ACCMODE) == O_WRONLY))
                return Status::invalid_argument;
            struct stat info{};
            if (fstat(fd, &info) < 0 || (!S_ISCHR(info.st_mode) && !S_ISSOCK(info.st_mode)))
                return Status::invalid_argument;
            if (i && S_ISSOCK(info.st_mode)) {
                next->socketOutput = true;
#if !defined(MSG_NOSIGNAL) && defined(SO_NOSIGPIPE)
                int noPipe = 0;
                socklen_t length = sizeof(noPipe);
                if (getsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &noPipe, &length) < 0 || !noPipe)
                    return Status::invalid_argument;
#endif
            }
            const auto duplicate = fcntl(fd, F_DUPFD_CLOEXEC, 0);
            if (duplicate < 0)
                return Status::io_error;
            (i ? next->output : next->input) = duplicate;
        }
        result = std::move(next);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
#if !defined(__ANDROID__) && !defined(__APPLE__)
Status enumerateNativeMidi2(std::vector<Midi2EndpointInfo>& result) noexcept {
    try {
        std::vector<Midi2EndpointInfo> next;
        std::error_code error;
        const std::filesystem::path folder("/dev/snd");
        if (!std::filesystem::exists(folder, error)) {
            if (error)
                return Status::io_error;
            result = std::move(next);
            return Status::ok;
        }
        for (const auto& entry : std::filesystem::directory_iterator(folder)) {
            const auto name = entry.path().filename().string();
            if (name.starts_with("umpC") && name.find('D', 4) != std::string::npos)
                next.push_back({entry.path().string(), name, Midi2Backend::linuxUmp, true, true});
        }
        result = std::move(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::io_error;
    }
}
Status createNativeMidi2(const Midi2NativeRequest& r, ReceiveMidi2 f, void* c,
                         std::unique_ptr<NativeMidi2>& out) noexcept {
    if (r.endpoint.backend != Midi2Backend::linuxUmp)
        return Status::unsupported_backend;
    if (!r.endpoint.id.starts_with("/dev/snd/umpC") ||
        r.endpoint.id.find("..") != std::string::npos)
        return Status::invalid_argument;
    const auto access = r.endpoint.receive ? (r.endpoint.send ? O_RDWR : O_RDONLY) : O_WRONLY;
    const int fd = ::open(r.endpoint.id.c_str(), access | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0)
        return Status::device_unavailable;
    Midi2NativeRequest request;
    r.endpoint.receive ? request.inputFd = fd : request.inputFd = -1;
    r.endpoint.send ? request.outputFd = fd : request.outputFd = -1;
    const auto status = createFdMidi2(request, f, c, out);
    ::close(fd);
    return status;
}
#endif
} // namespace cydsp::detail
