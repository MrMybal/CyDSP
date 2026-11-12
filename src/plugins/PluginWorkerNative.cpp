#include "PluginWire.h"
#include <chrono>
#include <cstring>
#include <thread>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif !defined(__ANDROID__)
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <spawn.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif
namespace cydsp::detail::wire {
Mapping::~Mapping() {
#if defined(_WIN32)
    if (data)
        UnmapViewOfFile(data);
    if (handle)
        CloseHandle(static_cast<HANDLE>(handle));
#elif !defined(__ANDROID__)
    if (data)
        munmap(data, bytes);
    if (fd >= 0)
        ::close(fd);
    if (owner && !name.empty())
        shm_unlink(name.c_str());
#endif
}
Status Mapping::create(std::string_view key, std::size_t size) noexcept {
    if (data || key.empty() || key.size() > 120 || !size || size > 512 * 1024 * 1024)
        return Status::invalid_argument;
    try {
        name = std::string(key);
        bytes = size;
#if defined(_WIN32)
        std::wstring wide(name.begin(), name.end());
        handle = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, DWORD(size),
                                    wide.c_str());
        if (!handle)
            return Status::io_error;
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            CloseHandle(static_cast<HANDLE>(handle));
            handle = nullptr;
            return Status::ambiguous;
        }
        data = static_cast<std::byte*>(
            MapViewOfFile(static_cast<HANDLE>(handle), FILE_MAP_ALL_ACCESS, 0, 0, size));
#elif !defined(__ANDROID__)
        if (name[0] != '/')
            return Status::invalid_argument;
        fd = shm_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
        if (fd < 0)
            return Status::io_error;
        owner = true;
        if (ftruncate(fd, static_cast<off_t>(size)) != 0)
            return Status::io_error;
        void* mapped = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (mapped != MAP_FAILED)
            data = static_cast<std::byte*>(mapped);
#else
        return Status::unsupported_backend;
#endif
        return data ? Status::ok : Status::io_error;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status Mapping::open(std::string_view key) noexcept {
    if (data || key.empty() || key.size() > 120)
        return Status::invalid_argument;
    try {
        name = std::string(key);
#if defined(_WIN32)
        std::wstring wide(name.begin(), name.end());
        handle = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, wide.c_str());
        if (!handle)
            return Status::io_error;
        data = static_cast<std::byte*>(
            MapViewOfFile(static_cast<HANDLE>(handle), FILE_MAP_ALL_ACCESS, 0, 0, 0));
        if (!data)
            return Status::io_error;
        MEMORY_BASIC_INFORMATION region{};
        if (!VirtualQuery(data, &region, sizeof(region)))
            return Status::io_error;
        bytes = region.RegionSize;
#elif !defined(__ANDROID__)
        if (name[0] != '/')
            return Status::invalid_argument;
        fd = shm_open(name.c_str(), O_RDWR, 0600);
        if (fd < 0)
            return Status::io_error;
        struct stat info{};
        if (fstat(fd, &info) != 0 || info.st_size < off_t(sizeof(Header)) ||
            info.st_size > 512 * 1024 * 1024)
            return Status::invalid_argument;
        bytes = std::size_t(info.st_size);
        void* mapped = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (mapped == MAP_FAILED)
            return Status::io_error;
        data = static_cast<std::byte*>(mapped);
#else
        return Status::unsupported_backend;
#endif
        if (bytes < sizeof(Header))
            return Status::invalid_argument;
        const auto& h = *reinterpret_cast<const Header*>(data);
        return valid(h) && h.mappingBytes <= bytes ? Status::ok : Status::incompatible_version;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Child::~Child() {
    close();
}
Status Child::launch(const std::filesystem::path& exe, std::string_view mapping) noexcept {
    if (!exe.is_absolute() || mapping.empty() || mapping.size() > 120)
        return Status::invalid_argument;
    try {
        std::error_code ec;
        if (!std::filesystem::is_regular_file(exe, ec) || ec)
            return Status::io_error;
#if defined(_WIN32)
        if (handle)
            return Status::transition_in_progress;
        std::wstring command = L"\"" + exe.wstring() + L"\" --cydsp-worker " +
                               std::wstring(mapping.begin(), mapping.end());
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        startup.dwFlags = STARTF_USESHOWWINDOW;
        startup.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION process{};
        if (!CreateProcessW(exe.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                            nullptr, exe.parent_path().c_str(), &startup, &process))
            return Status::io_error;
        CloseHandle(process.hThread);
        handle = process.hProcess;
        return Status::ok;
#elif !defined(__ANDROID__)
        if (pid > 0)
            return Status::transition_in_progress;
        std::string path = exe.string(), key(mapping);
        std::array<char*, 4> arguments{path.data(), const_cast<char*>("--cydsp-worker"), key.data(),
                                       nullptr};
        pid_t child = -1;
        const auto result =
            posix_spawn(&child, path.c_str(), nullptr, nullptr, arguments.data(), environ);
        if (result)
            return Status::io_error;
        pid = int(child);
        return Status::ok;
#else
        return Status::unsupported_backend;
#endif
    } catch (...) {
        return Status::allocation_failed;
    }
}
bool Child::running() noexcept {
#if defined(_WIN32)
    return handle && WaitForSingleObject(static_cast<HANDLE>(handle), 0) == WAIT_TIMEOUT;
#elif !defined(__ANDROID__)
    if (pid <= 0)
        return false;
    int status = 0;
    const auto result = waitpid(pid, &status, WNOHANG);
    if (result == 0)
        return true;
    if (result == pid || (result < 0 && errno == ECHILD))
        pid = -1;
    return false;
#else
    return false;
#endif
}
void Child::close() noexcept {
#if defined(_WIN32)
    if (handle) {
        if (WaitForSingleObject(static_cast<HANDLE>(handle), 1000) == WAIT_TIMEOUT) {
            TerminateProcess(static_cast<HANDLE>(handle), 87);
            (void)WaitForSingleObject(static_cast<HANDLE>(handle), 1000);
        }
        CloseHandle(static_cast<HANDLE>(handle));
        handle = nullptr;
    }
#elif !defined(__ANDROID__)
    if (pid > 0) {
        for (unsigned i = 0; i < 100 && running(); ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        if (pid > 0) {
            ::kill(pid, SIGKILL);
            int status = 0;
            while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
            }
            pid = -1;
        }
    }
#endif
}
std::uint32_t processId() noexcept {
#if defined(_WIN32)
    return GetCurrentProcessId();
#elif !defined(__ANDROID__)
    return std::uint32_t(getpid());
#else
    return 0;
#endif
}
bool parentRunning(std::uint32_t id) noexcept {
#if defined(_WIN32)
    HANDLE parent = OpenProcess(SYNCHRONIZE, FALSE, id);
    if (!parent)
        return false;
    const auto running = WaitForSingleObject(parent, 0) == WAIT_TIMEOUT;
    CloseHandle(parent);
    return running;
#elif !defined(__ANDROID__)
    return std::uint32_t(getppid()) == id;
#else
    (void)id;
    return false;
#endif
}
void pumpNativeMessages() noexcept {
#if defined(_WIN32)
    MSG message{};
    for (unsigned i = 0; i < 256 && PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE); ++i) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
#endif
}
} // namespace cydsp::detail::wire
