#pragma once
#include "cydsp/io/AudioFile.h"
#include <string_view>
namespace cydsp {
struct AudioMarker {
    std::uint32_t id = 0;
    std::uint64_t frame = 0;
    std::string name, note;
    std::uint32_t position = 0, dataChunkId = 0x61746164, chunkStart = 0, blockStart = 0;
};
struct AudioRegion {
    std::uint32_t markerId = 0;
    std::uint64_t lengthFrames = 0;
    std::uint32_t purpose = 0;
    std::uint16_t country = 0, language = 0, dialect = 0, codePage = 65001;
    std::string text;
};
enum class AudioLoopMode { forward, alternating, backward };
struct AudioSamplerLoop {
    std::uint32_t id = 0;
    std::uint64_t startFrame = 0, endFrame = 0; // Always [start,end), unlike WAV smpl.
    AudioLoopMode mode = AudioLoopMode::forward;
    std::uint32_t fractionalOffset = 0, playCount = 0;
};
struct AudioTag { std::string key, value; };
// Public schema version1. Native metadata is opaque preservation data, not a
// public third-party schema. Keep it when editing typed fields to retain unknowns.
struct AudioMetadata {
    unsigned version = 1;
    std::string container = "WAV";
    std::vector<AudioMarker> markers;
    std::vector<AudioRegion> regions;
    std::vector<AudioSamplerLoop> loops;
    std::vector<AudioTag> tags;
    std::uint32_t unityNote = 60, pitchFraction = 0;
    std::string preservedNative;
};
// Control/worker operations. WAV cue/adtl/smpl/INFO supported; empty metadata succeeds.
// set preserves unrelated chunks and unknown LIST children, replacing managed values.
// All in-memory outputs are unchanged on failure; no sample data is accessed.
Status getAudioMetadata(const AudioFileInfo&, AudioMetadata&) noexcept;
Status setAudioMetadata(AudioFileInfo&, const AudioMetadata&) noexcept;
Status serializeAudioMetadata(const AudioMetadata&, std::string&) noexcept;
Status parseAudioMetadata(std::string_view, AudioMetadata&) noexcept;
} // namespace cydsp
