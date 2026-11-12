#pragma once
#include "cydsp/core/AudioBlock.h"
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>
namespace cydsp {
enum class AudioFileFormat { wav, flac, oggVorbis, aiff, mp3, aifc, caf, m4a, aac, wma };
enum class AudioEncoding { pcm8, pcm16, pcm24, pcm32, float32, float64 };
struct AudioFileInfo {
    std::string formatName;
    double sampleRate = 0;
    std::uint64_t frames = 0;
    std::size_t channels = 0;
    AudioEncoding encoding = AudioEncoding::pcm16;
    std::string metadataJson;
};
struct AudioFormatCapabilities {
    AudioFileFormat format;
    bool read = false, write = false;
    std::size_t maximumChannels = 0;
    std::vector<AudioEncoding> writableEncodings;
    std::vector<std::string> qualityLevels;
};
// Integer PCM is normalized by 2^(bits-1); float data keeps its stored scale.
// Integer export clips to [-1,1], with at most one LSB loss at the negative endpoint.
// All file/stream methods are control/worker-thread operations (I/O and allocation).
[[nodiscard]] std::vector<AudioFormatCapabilities> audioFileFormats();
class AudioFileReader {
  public:
    AudioFileReader();
    ~AudioFileReader();
    AudioFileReader(AudioFileReader&&) noexcept;
    AudioFileReader& operator=(AudioFileReader&&) noexcept;
    // Failed open preserves an existing reader. Streams must be binary and seekable.
    Status open(const std::filesystem::path&, std::size_t maximumBlockFrames = 4096) noexcept;
    Status open(std::shared_ptr<std::istream>, std::size_t maximumBlockFrames = 4096) noexcept;
    void close() noexcept;
    [[nodiscard]] const AudioFileInfo* info() const noexcept;
    // Successful short reads zero-pad the tail and return end_of_stream. FramesRead is exact.
    Status read(std::uint64_t firstFrame, AudioBlock<float>, std::size_t& framesRead) noexcept;
    Status read(std::uint64_t firstFrame, AudioBlock<double>, std::size_t& framesRead) noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
    template <Sample T> Status readImpl(std::uint64_t, AudioBlock<T>, std::size_t&) noexcept;
};
class AudioFileWriter {
  public:
    AudioFileWriter();
    ~AudioFileWriter();
    AudioFileWriter(AudioFileWriter&&) noexcept;
    AudioFileWriter& operator=(AudioFileWriter&&) noexcept;
    // Path overload creates/truncates the destination. Close/flush finalises headers.
    Status open(const std::filesystem::path&, AudioFileFormat, const AudioFileInfo&,
                std::string quality = {}, std::size_t maximumBlockFrames = 4096) noexcept;
    Status open(std::shared_ptr<std::ostream>, AudioFileFormat, const AudioFileInfo&,
                std::string quality = {}, std::size_t maximumBlockFrames = 4096) noexcept;
    Status write(AudioBlock<const float>) noexcept;
    Status write(AudioBlock<const double>) noexcept;
    Status flush() noexcept;
    Status close() noexcept;
    [[nodiscard]] std::uint64_t framesWritten() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
    template <Sample T> Status writeImpl(AudioBlock<const T>) noexcept;
};
} // namespace cydsp
