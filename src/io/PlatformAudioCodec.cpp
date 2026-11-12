#include "PlatformAudioCodec.h"
#include <algorithm>
#include <cmath>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <propvarutil.h>
#include <windows.h>
#include <wrl/client.h>
namespace cydsp::detail {
namespace {
using Microsoft::WRL::ComPtr;
struct ThreadCom {
    HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    ~ThreadCom() {
        if (SUCCEEDED(result))
            CoUninitialize();
    }
};
struct Session {
    HRESULT mf = MFStartup(MF_VERSION);
    ~Session() {
        if (SUCCEEDED(mf))
            MFShutdown();
    }
};
const GUID* subtype(AudioFileFormat f) {
    switch (f) {
    case AudioFileFormat::mp3:
        return &MFAudioFormat_MP3;
    case AudioFileFormat::m4a:
    case AudioFileFormat::aac:
        return &MFAudioFormat_AAC;
    case AudioFileFormat::wma:
        return &MFAudioFormat_WMAudioV9;
    default:
        return nullptr;
    }
}
const GUID* container(AudioFileFormat f) {
    switch (f) {
    case AudioFileFormat::mp3:
        return &MFTranscodeContainerType_MP3;
    case AudioFileFormat::m4a:
        return &MFTranscodeContainerType_MPEG4;
    case AudioFileFormat::aac:
        return &MFTranscodeContainerType_ADTS;
    case AudioFileFormat::wma:
        return &MFTranscodeContainerType_ASF;
    default:
        return nullptr;
    }
}
bool available(AudioFileFormat f, bool encode) {
    const auto sub = subtype(f);
    if (!sub)
        return false;
    ThreadCom com;
    Session session;
    if (FAILED(session.mf))
        return false;
    MFT_REGISTER_TYPE_INFO type{MFMediaType_Audio, *sub};
    IMFActivate** list = nullptr;
    UINT32 count = 0;
    const auto hr =
        MFTEnumEx(encode ? MFT_CATEGORY_AUDIO_ENCODER : MFT_CATEGORY_AUDIO_DECODER,
                  MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_LOCALMFT | MFT_ENUM_FLAG_SORTANDFILTER,
                  encode ? nullptr : &type, encode ? &type : nullptr, &list, &count);
    for (UINT32 i = 0; i < count; ++i)
        list[i]->Release();
    CoTaskMemFree(list);
    return SUCCEEDED(hr) && count != 0;
}
ComPtr<IMFMediaType> outputType(AudioFileFormat f, const choc::audio::AudioFileProperties& p) {
    ComPtr<IMFCollection> list;
    ComPtr<IMFMediaType> result;
    const auto sub = subtype(f);
    if (!sub)
        return {};
    if (FAILED(MFTranscodeGetAudioOutputAvailableTypes(
            *sub, MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_LOCALMFT | MFT_ENUM_FLAG_SORTANDFILTER,
            nullptr, &list)))
        return {};
    DWORD count = 0;
    if (FAILED(list->GetElementCount(&count)))
        return {};
    const auto target = p.quality == "128"   ? 128000
                        : p.quality == "256" ? 256000
                        : p.quality == "320" ? 320000
                                             : 192000;
    auto distance = std::numeric_limits<std::int64_t>::max();
    for (DWORD i = 0; i < count; ++i) {
        ComPtr<IUnknown> element;
        ComPtr<IMFMediaType> type;
        if (FAILED(list->GetElement(i, &element)) || FAILED(element.As(&type)))
            continue;
        UINT32 channels = 0, rate = 0, bytes = 0;
        if (FAILED(type->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels)) ||
            FAILED(type->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate)) ||
            FAILED(type->GetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &bytes)))
            continue;
        if (channels != p.numChannels || double(rate) != p.sampleRate)
            continue;
        const auto d = std::abs(std::int64_t(bytes) * 8 - target);
        if (d < distance) {
            distance = d;
            result = type;
        }
    }
    return result;
}
struct Writer : PlatformAudioWriter {
    Session session;
    ComPtr<IMFSinkWriter> writer;
    choc::audio::AudioFileProperties properties;
    DWORD index = 0;
    bool finished = false;
    const choc::audio::AudioFileProperties& getProperties() override {
        return properties;
    }
    ~Writer() override {
        ThreadCom com;
        if (writer && !finished)
            (void)writer->Finalize();
        writer.Reset();
    }
    bool initialise(const std::filesystem::path& path, AudioFileFormat f) {
        ThreadCom com;
        if (FAILED(session.mf) || !container(f))
            return false;
        auto output = outputType(f, properties);
        if (!output)
            return false;
        ComPtr<IMFAttributes> attributes;
        if (FAILED(MFCreateAttributes(&attributes, 2)) ||
            FAILED(attributes->SetGUID(MF_TRANSCODE_CONTAINERTYPE, *container(f))) ||
            FAILED(MFCreateSinkWriterFromURL(path.c_str(), nullptr, attributes.Get(), &writer)) ||
            FAILED(writer->AddStream(output.Get(), &index)))
            return false;
        ComPtr<IMFMediaType> input;
        if (FAILED(MFCreateMediaType(&input)))
            return false;
        const auto channels = properties.numChannels, rate = UINT32(properties.sampleRate);
        if (FAILED(input->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio)) ||
            FAILED(input->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM)) ||
            FAILED(input->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, channels)) ||
            FAILED(input->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, rate)) ||
            FAILED(input->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16)) ||
            FAILED(input->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, channels * 2)) ||
            FAILED(input->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, rate * channels * 2)) ||
            FAILED(input->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE)) ||
            FAILED(writer->SetInputMediaType(index, input.Get(), nullptr)) ||
            FAILED(writer->BeginWriting()))
            return false;
        properties.numFrames = 0;
        properties.formatName = f == AudioFileFormat::mp3   ? "MP3"
                                : f == AudioFileFormat::wma ? "WMA"
                                : f == AudioFileFormat::m4a ? "M4A"
                                                            : "AAC";
        return true;
    }
    bool flush() override {
        return bool(writer);
    } // WriteSample is synchronous; close finalizes.
    bool finish() override {
        ThreadCom com;
        if (finished)
            return true;
        if (!writer)
            return false;
        if (FAILED(writer->Finalize()))
            return false;
        finished = true;
        return true;
    }
    template <class T> bool frames(choc::buffer::ChannelArrayView<const T> in) {
        ThreadCom com;
        if (finished || in.getNumChannels() != properties.numChannels)
            return false;
        const auto frames = std::uint64_t(in.getNumFrames());
        if (frames > UINT32_MAX / (properties.numChannels * 2) ||
            properties.numFrames >
                std::uint64_t(INT64_MAX / 10000000) * std::uint64_t(properties.sampleRate) - frames)
            return false;
        const DWORD bytes = DWORD(frames * properties.numChannels * 2);
        ComPtr<IMFMediaBuffer> buffer;
        ComPtr<IMFSample> sample;
        if (FAILED(MFCreateMemoryBuffer(bytes, &buffer)))
            return false;
        BYTE* ptr = nullptr;
        if (FAILED(buffer->Lock(&ptr, nullptr, nullptr)))
            return false;
        auto* dest = reinterpret_cast<std::int16_t*>(ptr);
        for (std::uint32_t i = 0; i < in.getNumFrames(); ++i)
            for (std::uint32_t c = 0; c < properties.numChannels; ++c)
                *dest++ = std::int16_t(std::clamp(
                    std::round(std::clamp(double(in.getSample(c, i)), -1.0, 1.0) * 32768), -32768.0,
                    32767.0));
        buffer->Unlock();
        const auto start =
            LONGLONG(std::round(double(properties.numFrames) * 10000000 / properties.sampleRate));
        const auto end = LONGLONG(
            std::round(double(properties.numFrames + frames) * 10000000 / properties.sampleRate));
        if (FAILED(buffer->SetCurrentLength(bytes)) || FAILED(MFCreateSample(&sample)) ||
            FAILED(sample->AddBuffer(buffer.Get())) || FAILED(sample->SetSampleTime(start)) ||
            FAILED(sample->SetSampleDuration(end - start)) ||
            FAILED(writer->WriteSample(index, sample.Get())))
            return false;
        properties.numFrames += frames;
        return true;
    }
    bool appendFrames(choc::buffer::ChannelArrayView<const float> b) override {
        return frames(b);
    }
    bool appendFrames(choc::buffer::ChannelArrayView<const double> b) override {
        return frames(b);
    }
};
struct Reader : choc::audio::AudioFileReader {
    Session session;
    ComPtr<IMFSourceReader> reader;
    choc::audio::AudioFileProperties properties;
    std::vector<float> cached;
    std::uint64_t position = 0;
    std::size_t offset = 0;
    bool eof = false;
    const choc::audio::AudioFileProperties& getProperties() override {
        return properties;
    }
    ~Reader() override {
        ThreadCom com;
        reader.Reset();
    }
    bool seek(std::uint64_t frame) {
        ThreadCom com;
        // Exact decoded-frame indexing: reset at the beginning and discard frames.
        // This avoids container timestamp rounding and compressed packet preroll ambiguity.
        PROPVARIANT v;
        PropVariantInit(&v);
        v.vt = VT_I8;
        v.hVal.QuadPart = 0;
        if (FAILED(reader->SetCurrentPosition(GUID_NULL, v)))
            return false;
        cached.clear();
        offset = 0;
        position = 0;
        eof = false;
        while (position < frame) {
            if (offset == cached.size() && !next())
                return false;
            const auto n = std::min<std::uint64_t>(
                (cached.size() - offset) / properties.numChannels, frame - position);
            offset += std::size_t(n) * properties.numChannels;
            position += n;
        }
        return true;
    }
    bool next() {
        ThreadCom com;
        cached.clear();
        offset = 0;
        for (unsigned attempt = 0; attempt < 65536 && !eof; ++attempt) {
            DWORD flags = 0;
            LONGLONG time = 0;
            ComPtr<IMFSample> sample;
            if (FAILED(reader->ReadSample(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM), 0, nullptr,
                                          &flags, &time, &sample)))
                return false;
            if (flags & (MF_SOURCE_READERF_ERROR | MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED))
                return false;
            eof = (flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0;
            if (sample) {
                ComPtr<IMFMediaBuffer> buffer;
                BYTE* p = nullptr;
                DWORD bytes = 0;
                if (FAILED(sample->ConvertToContiguousBuffer(&buffer)) ||
                    FAILED(buffer->Lock(&p, nullptr, &bytes)))
                    return false;
                if (bytes % (properties.numChannels * sizeof(float)) || bytes > 67108864) {
                    buffer->Unlock();
                    return false;
                }
                try {
                    cached.resize(bytes / sizeof(float));
                } catch (...) {
                    buffer->Unlock();
                    throw;
                }
                std::copy_n(reinterpret_cast<const float*>(p), cached.size(), cached.data());
                buffer->Unlock();
                if (!cached.empty())
                    return true;
            }
        }
        return false;
    }
    bool initialise(const std::filesystem::path& path) {
        ThreadCom com;
        if (FAILED(session.mf) ||
            FAILED(MFCreateSourceReaderFromURL(path.c_str(), nullptr, &reader)))
            return false;
        if (FAILED(reader->SetStreamSelection(DWORD(MF_SOURCE_READER_ALL_STREAMS), FALSE)) ||
            FAILED(reader->SetStreamSelection(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM), TRUE)))
            return false;
        ComPtr<IMFMediaType> type;
        if (FAILED(MFCreateMediaType(&type)) ||
            FAILED(type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio)) ||
            FAILED(type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float)) ||
            FAILED(reader->SetCurrentMediaType(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM), nullptr,
                                               type.Get())))
            return false;
        type.Reset();
        UINT32 rate = 0, channels = 0, bits = 0;
        if (FAILED(
                reader->GetCurrentMediaType(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM), &type)) ||
            FAILED(type->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate)) ||
            FAILED(type->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels)) ||
            FAILED(type->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bits)) || bits != 32 || !rate ||
            !channels || channels > 256)
            return false;
        properties.sampleRate = rate;
        properties.numChannels = channels;
        properties.bitDepth = choc::audio::BitDepth::float32;
        properties.formatName = "Windows platform";
        // One bounded-memory indexing pass establishes exact decoded length (no duration estimate).
        while (next()) {
            const auto n = cached.size() / channels;
            if (properties.numFrames > 9007199254740991ULL - n)
                return false;
            properties.numFrames += n;
        }
        if (!eof)
            return false;
        return seek(0);
    }
    template <class T> bool frames(std::uint64_t first, choc::buffer::ChannelArrayView<T> out) {
        if (out.getNumChannels() != properties.numChannels)
            return false;
        if (first != position && !seek(std::min(first, properties.numFrames)))
            return false;
        std::uint32_t written = 0;
        while (written < out.getNumFrames() && position < properties.numFrames) {
            if (offset == cached.size() && !next())
                return false;
            auto n = std::min<std::size_t>((cached.size() - offset) / properties.numChannels,
                                           out.getNumFrames() - written);
            for (std::size_t i = 0; i < n; ++i)
                for (std::uint32_t c = 0; c < properties.numChannels; ++c)
                    out.getSample(c, written + std::uint32_t(i)) =
                        static_cast<T>(cached[offset + i * properties.numChannels + c]);
            offset += n * properties.numChannels;
            position += n;
            written += std::uint32_t(n);
        }
        for (auto i = written; i < out.getNumFrames(); ++i)
            for (std::uint32_t c = 0; c < properties.numChannels; ++c)
                out.getSample(c, i) = 0;
        return true;
    }
    bool readFrames(std::uint64_t i, choc::buffer::ChannelArrayView<float> b) override {
        return frames(i, b);
    }
    bool readFrames(std::uint64_t i, choc::buffer::ChannelArrayView<double> b) override {
        return frames(i, b);
    }
};
} // namespace
bool platformEncodingAvailable(AudioFileFormat f) noexcept {
    try {
        return available(f, true);
    } catch (...) {
        return false;
    }
}
bool platformDecodingAvailable(AudioFileFormat f) noexcept {
    try {
        return available(f, false);
    } catch (...) {
        return false;
    }
}
std::unique_ptr<choc::audio::AudioFileWriter> platformWriter(const std::filesystem::path& path,
                                                             AudioFileFormat f,
                                                             choc::audio::AudioFileProperties p) {
    ThreadCom com;
    auto w = std::make_unique<Writer>();
    w->properties = std::move(p);
    return w->initialise(path, f) ? std::move(w) : nullptr;
}
std::unique_ptr<choc::audio::AudioFileReader> platformReader(const std::filesystem::path& path) {
    ThreadCom com;
    auto r = std::make_unique<Reader>();
    return r->initialise(path) ? std::move(r) : nullptr;
}
} // namespace cydsp::detail

#elif defined(__APPLE__)
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>
namespace cydsp::detail {
namespace {
AudioFormatID codec(AudioFileFormat f) {
    return f == AudioFileFormat::mp3                                  ? kAudioFormatMPEGLayer3
           : (f == AudioFileFormat::m4a || f == AudioFileFormat::aac) ? kAudioFormatMPEG4AAC
                                                                      : 0;
}
bool available(AudioFileFormat f, bool encode) {
    const auto id = codec(f);
    UInt32 size = 0;
    return id &&
           AudioFormatGetPropertyInfo(encode ? kAudioFormatProperty_Encoders
                                             : kAudioFormatProperty_Decoders,
                                      sizeof(id), &id, &size) == noErr &&
           size >= sizeof(AudioClassDescription);
}
struct Url {
    CFURLRef value = nullptr;
    explicit Url(const std::filesystem::path& p) {
        const auto path = p.u8string();
        value = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
                                                        reinterpret_cast<const UInt8*>(path.data()),
                                                        CFIndex(path.size()), false);
    }
    ~Url() {
        if (value)
            CFRelease(value);
    }
};
AudioStreamBasicDescription client(double rate, unsigned channels) {
    AudioStreamBasicDescription d{};
    d.mSampleRate = rate;
    d.mFormatID = kAudioFormatLinearPCM;
    d.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
    d.mBitsPerChannel = 32;
    d.mChannelsPerFrame = channels;
    d.mFramesPerPacket = 1;
    d.mBytesPerFrame = d.mBytesPerPacket = channels * 4;
    return d;
}
struct Reader : choc::audio::AudioFileReader {
    ExtAudioFileRef file = nullptr;
    choc::audio::AudioFileProperties properties;
    std::vector<float> scratch;
    std::uint64_t position = 0;
    ~Reader() override {
        if (file)
            ExtAudioFileDispose(file);
    }
    const choc::audio::AudioFileProperties& getProperties() override {
        return properties;
    }
    bool initialise(const std::filesystem::path& path) {
        Url url(path);
        if (!url.value || ExtAudioFileOpenURL(url.value, &file) != noErr)
            return false;
        AudioStreamBasicDescription data{};
        UInt32 size = sizeof(data);
        SInt64 frames = 0;
        if (ExtAudioFileGetProperty(file, kExtAudioFileProperty_FileDataFormat, &size, &data) !=
            noErr)
            return false;
        size = sizeof(frames);
        if (ExtAudioFileGetProperty(file, kExtAudioFileProperty_FileLengthFrames, &size, &frames) !=
                noErr ||
            frames < 0 || !data.mChannelsPerFrame || data.mChannelsPerFrame > 256 ||
            data.mSampleRate <= 0 || !std::isfinite(data.mSampleRate))
            return false;
        auto pcm = client(data.mSampleRate, data.mChannelsPerFrame);
        if (ExtAudioFileSetProperty(file, kExtAudioFileProperty_ClientDataFormat, sizeof(pcm),
                                    &pcm) != noErr)
            return false;
        properties.formatName = "Apple platform";
        properties.sampleRate = data.mSampleRate;
        properties.numChannels = data.mChannelsPerFrame;
        properties.numFrames = std::uint64_t(frames);
        properties.bitDepth = choc::audio::BitDepth::float32;
        return true;
    }
    template <class T> bool frames(std::uint64_t first, choc::buffer::ChannelArrayView<T> out) {
        if (out.getNumChannels() != properties.numChannels || first > std::uint64_t(INT64_MAX) ||
            (first != position && ExtAudioFileSeek(file, SInt64(first)) != noErr))
            return false;
        auto n =
            UInt32(first < properties.numFrames
                       ? std::min<std::uint64_t>(out.getNumFrames(), properties.numFrames - first)
                       : 0);
        if (n > UINT32_MAX / (properties.numChannels * 4))
            return false;
        scratch.resize(std::size_t(n) * properties.numChannels);
        AudioBufferList list{};
        list.mNumberBuffers = 1;
        list.mBuffers[0].mNumberChannels = properties.numChannels;
        list.mBuffers[0].mDataByteSize = n * properties.numChannels * 4;
        list.mBuffers[0].mData = scratch.data();
        auto got = n;
        if (n && ExtAudioFileRead(file, &got, &list) != noErr)
            return false;
        if (got != n)
            return false;
        for (UInt32 i = 0; i < out.getNumFrames(); ++i)
            for (UInt32 c = 0; c < properties.numChannels; ++c)
                out.getSample(c, i) =
                    i < n ? static_cast<T>(scratch[std::size_t(i) * properties.numChannels + c])
                          : T{};
        position = first + n;
        return true;
    }
    bool readFrames(std::uint64_t i, choc::buffer::ChannelArrayView<float> b) override {
        return frames(i, b);
    }
    bool readFrames(std::uint64_t i, choc::buffer::ChannelArrayView<double> b) override {
        return frames(i, b);
    }
};
struct Writer : PlatformAudioWriter {
    ExtAudioFileRef file = nullptr;
    choc::audio::AudioFileProperties properties;
    std::vector<float> scratch;
    ~Writer() override {
        if (file)
            ExtAudioFileDispose(file);
    }
    const choc::audio::AudioFileProperties& getProperties() override {
        return properties;
    }
    bool initialise(const std::filesystem::path& path, AudioFileFormat f) {
        if (!available(f, true))
            return false;
        AudioStreamBasicDescription encoded{};
        encoded.mFormatID = codec(f);
        encoded.mSampleRate = properties.sampleRate;
        encoded.mChannelsPerFrame = properties.numChannels;
        UInt32 size = sizeof(encoded);
        if (AudioFormatGetProperty(kAudioFormatProperty_FormatInfo, 0, nullptr, &size, &encoded) !=
            noErr)
            return false;
        Url url(path);
        const auto type = f == AudioFileFormat::mp3   ? kAudioFileMP3Type
                          : f == AudioFileFormat::m4a ? kAudioFileM4AType
                                                      : kAudioFileAAC_ADTSType;
        if (!url.value || ExtAudioFileCreateWithURL(url.value, type, &encoded, nullptr,
                                                    kAudioFileFlags_EraseFile, &file) != noErr)
            return false;
        auto pcm = client(properties.sampleRate, properties.numChannels);
        if (ExtAudioFileSetProperty(file, kExtAudioFileProperty_ClientDataFormat, sizeof(pcm),
                                    &pcm) != noErr)
            return false;
        AudioConverterRef converter = nullptr;
        size = sizeof(converter);
        if (ExtAudioFileGetProperty(file, kExtAudioFileProperty_AudioConverter, &size,
                                    &converter) != noErr)
            return false;
        const UInt32 bitrate = properties.quality == "128"   ? 128000
                               : properties.quality == "256" ? 256000
                               : properties.quality == "320" ? 320000
                                                             : 192000;
        if (AudioConverterSetProperty(converter, kAudioConverterEncodeBitRate, sizeof(bitrate),
                                      &bitrate) != noErr)
            return false;
        properties.numFrames = 0;
        return true;
    }
    bool flush() override {
        return file != nullptr;
    }
    bool finish() override {
        if (!file)
            return true;
        auto old = std::exchange(file, nullptr);
        return ExtAudioFileDispose(old) == noErr;
    }
    template <class T> bool frames(choc::buffer::ChannelArrayView<const T> in) {
        if (!file || in.getNumChannels() != properties.numChannels ||
            in.getNumFrames() > UINT32_MAX / (properties.numChannels * 4))
            return false;
        const auto samples = std::size_t(in.getNumFrames()) * properties.numChannels;
        scratch.resize(samples);
        for (UInt32 i = 0; i < in.getNumFrames(); ++i)
            for (UInt32 c = 0; c < properties.numChannels; ++c)
                scratch[std::size_t(i) * properties.numChannels + c] =
                    float(std::clamp(double(in.getSample(c, i)), -1.0, 1.0));
        AudioBufferList list{};
        list.mNumberBuffers = 1;
        list.mBuffers[0].mNumberChannels = properties.numChannels;
        list.mBuffers[0].mDataByteSize = UInt32(samples * 4);
        list.mBuffers[0].mData = scratch.data();
        if (ExtAudioFileWrite(file, in.getNumFrames(), &list) != noErr)
            return false;
        properties.numFrames += in.getNumFrames();
        return true;
    }
    bool appendFrames(choc::buffer::ChannelArrayView<const float> b) override {
        return frames(b);
    }
    bool appendFrames(choc::buffer::ChannelArrayView<const double> b) override {
        return frames(b);
    }
};
} // namespace
bool platformEncodingAvailable(AudioFileFormat f) noexcept {
    return available(f, true);
}
bool platformDecodingAvailable(AudioFileFormat f) noexcept {
    return available(f, false);
}
std::unique_ptr<choc::audio::AudioFileWriter> platformWriter(const std::filesystem::path& path,
                                                             AudioFileFormat f,
                                                             choc::audio::AudioFileProperties p) {
    auto w = std::make_unique<Writer>();
    w->properties = std::move(p);
    return w->initialise(path, f) ? std::move(w) : nullptr;
}
std::unique_ptr<choc::audio::AudioFileReader> platformReader(const std::filesystem::path& path) {
    auto r = std::make_unique<Reader>();
    return r->initialise(path) ? std::move(r) : nullptr;
}
} // namespace cydsp::detail
#else
namespace cydsp::detail {
bool platformEncodingAvailable(AudioFileFormat) noexcept {
    return false;
}
bool platformDecodingAvailable(AudioFileFormat) noexcept {
    return false;
}
std::unique_ptr<choc::audio::AudioFileWriter>
platformWriter(const std::filesystem::path&, AudioFileFormat, choc::audio::AudioFileProperties) {
    return {};
}
std::unique_ptr<choc::audio::AudioFileReader> platformReader(const std::filesystem::path&) {
    return {};
}
} // namespace cydsp::detail
#endif
