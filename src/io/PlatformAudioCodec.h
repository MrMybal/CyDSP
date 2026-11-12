#pragma once
#include "cydsp/io/AudioFile.h"
#include <choc/audio/choc_AudioFileFormat.h>
namespace cydsp::detail {
class PlatformAudioWriter : public choc::audio::AudioFileWriter {
  public:
    virtual bool finish() = 0;
};
bool platformEncodingAvailable(AudioFileFormat) noexcept;
bool platformDecodingAvailable(AudioFileFormat) noexcept;
std::unique_ptr<choc::audio::AudioFileWriter>
platformWriter(const std::filesystem::path&, AudioFileFormat, choc::audio::AudioFileProperties);
std::unique_ptr<choc::audio::AudioFileReader> platformReader(const std::filesystem::path&);
} // namespace cydsp::detail
