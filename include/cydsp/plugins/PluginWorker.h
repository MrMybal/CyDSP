#pragma once
#include "cydsp/plugins/Plugin.h"
namespace cydsp {
struct PluginWorkerOptions {
    PluginPrepareOptions processing;
    std::uint32_t maximumChannelsPerDirection = 64, maximumPayloadBytes = 65536,
                  maximumStateBytes = 16 * 1024 * 1024;
    unsigned startupTimeoutMs = 5000, controlTimeoutMs = 2000, heartbeatTimeoutMs = 3000;
};
struct PluginWorkerInfo {
    std::vector<std::uint32_t> inputChannels, outputChannels;
    PluginTiming pluginTiming;
    std::uint32_t pipelineLatencyFrames = 0, protocolVersion = 1;
};
struct PluginWorkerBlockResult {
    std::uint64_t submittedSequence = 0, receivedSequence = 0;
    bool submitted = false, received = false, missedDeadline = false;
};
// Separate OS process, bounded shared-memory mailbox, one fixed-size block of
// pipeline latency. Audio methods never wait/query OS state/allocate. Late blocks
// are discarded; the output is silenced. Call poll() on control to detect crashes.
// launch/control/close require audio idle. The supplied worker executable calls
// runPluginWorker() below; production presets never create/run an executable.
class PluginWorker {
  public:
    PluginWorker();
    ~PluginWorker();
    PluginWorker(PluginWorker&&) noexcept;
    PluginWorker& operator=(PluginWorker&&) noexcept;
    Status launch(const std::filesystem::path& workerExecutable, PluginFormat,
                  const std::filesystem::path& plugin, std::string_view id,
                  const PluginWorkerOptions& = {}) noexcept;
    const PluginWorkerInfo* info() const noexcept;
    Status process(const PluginProcess<float>&, PluginWorkerBlockResult&) noexcept;
    Status process(const PluginProcess<double>&, PluginWorkerBlockResult&) noexcept;
    Status poll() noexcept;
    Status poll(std::uint32_t& requests) noexcept;
    Status saveState(std::vector<std::uint8_t>&) noexcept;
    Status loadState(std::span<const std::uint8_t>) noexcept;
    Status openEditor(const PluginWindow&, PluginEditorSize&) noexcept;
    Status resizeEditor(PluginEditorSize&) noexcept;
    Status scaleEditor(double) noexcept;
    Status showEditor(bool) noexcept;
    Status focusEditor(bool) noexcept;
    Status keyEditor(const PluginEditorKey&) noexcept;
    Status wheelEditor(float) noexcept;
    PluginEditorSize requestedEditorSize() const noexcept;
    Status closeEditor() noexcept;
    void close() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
    template <Sample T>
    Status processImpl(const PluginProcess<T>&, PluginWorkerBlockResult&) noexcept;
};
// Call only for explicitly supplied --cydsp-worker <mapping> command-line mode.
// Returns an exit code; serialized plugin main/audio operations and native Windows
// message pumping live in this process. No platform license/SDK is accepted here.
int runPluginWorker(std::string_view mappingName) noexcept;
} // namespace cydsp
