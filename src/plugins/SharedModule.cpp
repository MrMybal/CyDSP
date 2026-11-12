#include "PluginInternal.h"
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#endif
#endif
namespace cydsp::detail {
SharedModule::~SharedModule() {
    if (!handle_)
        return;
#if defined(_WIN32)
    FreeLibrary(static_cast<HMODULE>(handle_));
#elif defined(__APPLE__)
    if (bundle_) {
        CFBundleUnloadExecutable(static_cast<CFBundleRef>(handle_));
        CFRelease(static_cast<CFBundleRef>(handle_));
    } else
        dlclose(handle_);
#else
    dlclose(handle_);
#endif
}
Status SharedModule::open(const std::filesystem::path& path) noexcept {
    if (handle_ || !path.is_absolute())
        return Status::invalid_argument;
#if defined(_WIN32)
    handle_ = LoadLibraryExW(path.c_str(), nullptr,
                             LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
#elif defined(__APPLE__)
    std::error_code error;
    if (std::filesystem::is_directory(path, error) && !error) {
        auto url = CFURLCreateFromFileSystemRepresentation(
            kCFAllocatorDefault, reinterpret_cast<const UInt8*>(path.c_str()),
            CFIndex(path.native().size()), true);
        if (!url)
            return Status::io_error;
        auto bundle = CFBundleCreate(kCFAllocatorDefault, url);
        CFRelease(url);
        if (!bundle)
            return Status::io_error;
        if (!CFBundleLoadExecutable(bundle)) {
            CFRelease(bundle);
            return Status::io_error;
        }
        handle_ = bundle;
        bundle_ = true;
    } else
        handle_ = dlopen(path.c_str(), RTLD_LOCAL | RTLD_NOW);
#else
    handle_ = dlopen(path.c_str(), RTLD_LOCAL | RTLD_NOW);
#endif
    return handle_ ? Status::ok : Status::io_error;
}
void* SharedModule::symbol(const char* name) const noexcept {
    if (!handle_)
        return nullptr;
#if defined(_WIN32)
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle_), name));
#elif defined(__APPLE__)
    if (bundle_) {
        auto key = CFStringCreateWithCString(kCFAllocatorDefault, name, kCFStringEncodingUTF8);
        if (!key)
            return nullptr;
        auto* result = reinterpret_cast<void*>(
            CFBundleGetFunctionPointerForName(static_cast<CFBundleRef>(handle_), key));
        if (!result)
            result = CFBundleGetDataPointerForName(static_cast<CFBundleRef>(handle_), key);
        CFRelease(key);
        return result;
    }
    return dlsym(handle_, name);
#else
    return dlsym(handle_, name);
#endif
}
} // namespace cydsp::detail
