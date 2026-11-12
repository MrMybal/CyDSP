#pragma once
#include "cydsp/io/AudioFile.h"
namespace cydsp {
struct PcmProxyOptions {
    AudioEncoding encoding = AudioEncoding::float64;
    std::size_t blockFrames = 4096;
    std::uint64_t maximumBytes = UINT64_C(68719476736);
    std::uint64_t maximumWorkingBufferBytes = UINT64_C(67108864);
};
struct PcmProxyStatistics {
    std::uint64_t framesCopied = 0, pcmBytes = 0, workingBufferBytes = 0;
    std::uint64_t readCalls = 0, framesRead = 0;
};
// Control/worker-owned disk editing proxy. Build uses a new destination and never
// overwrites an existing cache. Native source decoders may need an initial indexing
// pass; subsequent cache reads seek directly to exact decoded PCM frames.
// A complete published cache can remain if reopening fails. Block scratch is
// bounded; OS/codec workspace is separate.
class EditingPcmProxy {
  public:
    EditingPcmProxy();
    ~EditingPcmProxy();
    EditingPcmProxy(EditingPcmProxy&&) noexcept;
    EditingPcmProxy& operator=(EditingPcmProxy&&) noexcept;
    Status build(const std::filesystem::path& source, const std::filesystem::path& newCache,
                 const PcmProxyOptions& = {}) noexcept;
    Status open(const std::filesystem::path& source, const std::filesystem::path& cache,
                std::size_t maximumBlockFrames = 4096) noexcept;
    Status read(std::uint64_t, AudioBlock<float>, std::size_t&) noexcept;
    Status read(std::uint64_t, AudioBlock<double>, std::size_t&) noexcept;
    [[nodiscard]] const AudioFileInfo* sourceInfo() const noexcept;
    [[nodiscard]] const AudioFileInfo* pcmInfo() const noexcept;
    [[nodiscard]] PcmProxyStatistics statistics() const noexcept;
    void close() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
} // namespace cydsp
