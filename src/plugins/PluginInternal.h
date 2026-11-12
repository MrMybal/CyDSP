#pragma once
#include "cydsp/plugins/Plugin.h"
#include <thread>
namespace cydsp::detail {
extern thread_local const PluginBackend* currentAudioPlugin;
struct PluginAudioScope {
    const PluginBackend* previous;
    explicit PluginAudioScope(const PluginBackend* p) noexcept : previous(currentAudioPlugin) {
        currentAudioPlugin = p;
    }
    ~PluginAudioScope() {
        currentAudioPlugin = previous;
    }
};
std::unique_ptr<PluginProvider> makeClapProvider();
std::unique_ptr<PluginProvider> makeVst3Provider();
std::unique_ptr<PluginProvider> makeLv2Provider();
std::unique_ptr<PluginProvider> makeJsfxProvider();
class SharedModule {
  public:
    SharedModule() = default;
    ~SharedModule();
    SharedModule(const SharedModule&) = delete;
    SharedModule& operator=(const SharedModule&) = delete;
    Status open(const std::filesystem::path&) noexcept;
    void* symbol(const char*) const noexcept;
    void* nativeHandle() const noexcept {
        return handle_;
    }

  private:
    void* handle_ = nullptr;
#if defined(__APPLE__)
    bool bundle_ = false;
#endif
};
bool validPluginTransport(const PluginTransport&) noexcept;
} // namespace cydsp::detail
