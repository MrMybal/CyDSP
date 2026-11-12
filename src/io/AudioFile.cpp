// Private CHOC codecs with owned WAV metadata adaptation; notices: third_party/licenses/choc.txt
#include "cydsp/io/AudioFile.h"
#include "AiffCodec.h"
#include "PlatformAudioCodec.h"
#include "cydsp/core/AudioBuffer.h"
#include <algorithm>
#include <choc/audio/choc_AudioFileFormat_FLAC.h>
#include <choc/audio/choc_AudioFileFormat_MP3.h>
#include <choc/audio/choc_AudioFileFormat_Ogg.h>
#include "WavCodec.h"
#include <cmath>
#include <limits>
namespace cydsp {
namespace {
using Depth = choc::audio::BitDepth;
Depth depth(AudioEncoding e) {
    return static_cast<Depth>(static_cast<unsigned>(e) + 1);
}
AudioEncoding encoding(Depth d) {
    return static_cast<AudioEncoding>(static_cast<unsigned>(d) - 1);
}
std::unique_ptr<choc::audio::AudioFileFormat> format(AudioFileFormat f) {
    switch (f) {
    case AudioFileFormat::wav:
        return std::make_unique<choc::audio::CyDSPWavFormat<true>>();
    case AudioFileFormat::flac:
        return std::make_unique<choc::audio::FLACAudioFileFormat<true>>();
    case AudioFileFormat::oggVorbis:
        return std::make_unique<choc::audio::OggAudioFileFormat<true>>();
    case AudioFileFormat::aiff:
        return std::make_unique<detail::AiffCodec>();
    case AudioFileFormat::aifc:
        return std::make_unique<detail::AiffCodec>(detail::aiff::Container::aifc);
    case AudioFileFormat::caf:
        return std::make_unique<detail::AiffCodec>(detail::aiff::Container::caf);
    case AudioFileFormat::mp3:
        return std::make_unique<choc::audio::MP3AudioFileFormat>();
    case AudioFileFormat::m4a:
    case AudioFileFormat::aac:
    case AudioFileFormat::wma:
        return {};
    }
    return {};
}
choc::audio::AudioFileFormatList formats() {
    choc::audio::AudioFileFormatList list;
    for (auto f :
         {AudioFileFormat::wav, AudioFileFormat::flac, AudioFileFormat::oggVorbis,
          AudioFileFormat::aiff, AudioFileFormat::aifc, AudioFileFormat::caf, AudioFileFormat::mp3})
        list.addFormat(format(f));
    return list;
}
double pcmScale(AudioEncoding encoding) {
    const auto n = static_cast<unsigned>(encoding);
    if (n > static_cast<unsigned>(AudioEncoding::pcm32))
        return 1;
    constexpr unsigned bits[]{8, 16, 24, 32};
    const double denominator = std::ldexp(1.0, static_cast<int>(bits[n] - 1));
    return (denominator - 1) / denominator;
}
bool valid(const AudioFileInfo& info, std::size_t max) {
    return isFinite(info.sampleRate) && info.sampleRate > 0 && info.sampleRate <= 1e9 &&
           info.channels > 0 && info.channels <= 256 && max > 0 && max <= 1048576 &&
           static_cast<unsigned>(info.encoding) <= static_cast<unsigned>(AudioEncoding::float64);
}
} // namespace
std::vector<AudioFormatCapabilities> audioFileFormats() {
    std::vector<AudioFormatCapabilities> result;
    for (auto f : {AudioFileFormat::wav, AudioFileFormat::flac, AudioFileFormat::oggVorbis,
                   AudioFileFormat::aiff, AudioFileFormat::aifc, AudioFileFormat::caf,
                   AudioFileFormat::mp3}) {
        auto codec = format(f);
        AudioFormatCapabilities c{f,
                                  true,
                                  codec->supportsWriting(),
                                  std::min<std::size_t>(codec->getMaximumNumChannels(),
                                                        f == AudioFileFormat::flac ? 8 : 256),
                                  {},
                                  codec->getQualityLevels()};
        if (c.write)
            for (auto d : codec->getSupportedBitDepths())
                if (d != Depth::unknown)
                    c.writableEncodings.push_back(encoding(d));
        if (f == AudioFileFormat::oggVorbis)
            c.writableEncodings = {AudioEncoding::float32};
        result.push_back(std::move(c));
    }
    for (auto f : {AudioFileFormat::m4a, AudioFileFormat::aac, AudioFileFormat::wma})
        result.push_back({f,
                          detail::platformDecodingAvailable(f),
                          detail::platformEncodingAvailable(f),
                          2,
                          {AudioEncoding::pcm16},
                          {"128", "192", "256", "320"}});
    for (auto& c : result)
        if (c.format == AudioFileFormat::mp3 && detail::platformEncodingAvailable(c.format)) {
            c.write = true;
            c.writableEncodings = {AudioEncoding::pcm16};
            c.qualityLevels = {"128", "192", "256", "320"};
        }
    return result;
}
struct AudioFileReader::State {
    std::unique_ptr<choc::audio::AudioFileReader> reader;
    AudioFileInfo info;
    std::size_t max = 0;
    double scale = 1;
    std::vector<float*> f;
    std::vector<double*> d;
    Status initialise(std::size_t blockFrames) {
        const auto& p = reader->getProperties();
        info = {p.formatName,
                p.sampleRate,
                p.numFrames,
                p.numChannels,
                p.bitDepth == Depth::unknown ? AudioEncoding::float32 : encoding(p.bitDepth),
                choc::json::toString(p.metadata)};
        if (p.formatName == "Ogg-Vorbis" || p.formatName == "MP3")
            info.encoding = AudioEncoding::float32;
        if (!valid(info, blockFrames))
            return Status::invalid_argument;
        if (p.formatName == "WAV" || p.formatName == "FLAC")
            scale = pcmScale(info.encoding);
        f.resize(p.numChannels);
        d.resize(p.numChannels);
        max = blockFrames;
        return Status::ok;
    }
};
AudioFileReader::AudioFileReader() = default;
AudioFileReader::~AudioFileReader() = default;
AudioFileReader::AudioFileReader(AudioFileReader&&) noexcept = default;
AudioFileReader& AudioFileReader::operator=(AudioFileReader&&) noexcept = default;
Status AudioFileReader::open(const std::filesystem::path& path, std::size_t max) noexcept {
    try {
        const auto portable = open(std::make_shared<std::ifstream>(path, std::ios::binary), max);
        if (portable != Status::unsupported_format)
            return portable;
        if (!max || max > 1048576)
            return Status::invalid_argument;
        auto next = std::make_unique<State>();
        next->reader = detail::platformReader(path);
        if (!next->reader)
            return Status::unsupported_format;
        const auto ready = next->initialise(max);
        if (ready != Status::ok)
            return ready;
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::io_error;
    }
}
Status AudioFileReader::open(std::shared_ptr<std::istream> stream, std::size_t max) noexcept {
    if (!stream || !*stream || !max || max > 1048576)
        return Status::invalid_argument;
    try {
        auto next = std::make_unique<State>();
        next->reader = formats().createReader(std::move(stream));
        if (!next->reader)
            return Status::unsupported_format;
        const auto ready = next->initialise(max);
        if (ready != Status::ok)
            return ready;
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::io_error;
    }
}
void AudioFileReader::close() noexcept {
    state_.reset();
}
const AudioFileInfo* AudioFileReader::info() const noexcept {
    return state_ ? &state_->info : nullptr;
}
template <Sample T>
Status AudioFileReader::readImpl(std::uint64_t first, AudioBlock<T> out,
                                 std::size_t& count) noexcept {
    count = 0;
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    const auto status =
        validateProcessBlocks<T>({s.info.sampleRate, s.max, s.info.channels}, out, out);
    if (status != Status::ok)
        return status;
    if (!out.frames())
        return Status::ok;
    const auto available = first < s.info.frames ? s.info.frames - first : 0;
    const auto n = static_cast<std::size_t>(std::min<std::uint64_t>(available, out.frames()));
    try {
        if (n) {
            auto& pointers = [&]() -> auto& {
                if constexpr (std::same_as<T, float>)
                    return s.f;
                else
                    return s.d;
            }();
            for (std::size_t ch = 0; ch < out.channels(); ++ch)
                pointers[ch] = out.channel(ch).data();
            if (!s.reader->readFrames(first, choc::buffer::createChannelArrayView(
                                                 pointers.data(),
                                                 static_cast<std::uint32_t>(out.channels()),
                                                 static_cast<std::uint32_t>(n))))
                return Status::io_error;
        }
        for (std::size_t ch = 0; ch < out.channels(); ++ch) {
            if (s.scale != 1)
                for (std::size_t i = 0; i < n; ++i)
                    out.channel(ch)[i] = static_cast<T>(out.channel(ch)[i] * s.scale);
            std::fill(out.channel(ch).begin() + n, out.channel(ch).end(), T{});
        }
        count = n;
        return n < out.frames() ? Status::end_of_stream : Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::io_error;
    }
}
Status AudioFileReader::read(std::uint64_t first, AudioBlock<float> out, std::size_t& n) noexcept {
    return readImpl(first, out, n);
}
Status AudioFileReader::read(std::uint64_t first, AudioBlock<double> out, std::size_t& n) noexcept {
    return readImpl(first, out, n);
}
struct AudioFileWriter::State {
    std::unique_ptr<choc::audio::AudioFileWriter> writer;
    ProcessSpec spec;
    std::uint64_t frames = 0;
    double scale = 1;
    AudioBuffer<double> scratch;
    std::vector<const float*> f;
    std::vector<const double*> d;
};
AudioFileWriter::AudioFileWriter() = default;
AudioFileWriter::~AudioFileWriter() = default;
AudioFileWriter::AudioFileWriter(AudioFileWriter&&) noexcept = default;
AudioFileWriter& AudioFileWriter::operator=(AudioFileWriter&&) noexcept = default;
Status AudioFileWriter::open(const std::filesystem::path& path, AudioFileFormat f,
                             const AudioFileInfo& info, std::string quality,
                             std::size_t max) noexcept {
    if (!valid(info, max))
        return Status::invalid_argument;
    try {
        if (f == AudioFileFormat::mp3 || f == AudioFileFormat::m4a || f == AudioFileFormat::aac ||
            f == AudioFileFormat::wma) {
            if (info.channels > 2 || info.encoding != AudioEncoding::pcm16 ||
                info.sampleRate != std::floor(info.sampleRate) || info.sampleRate > 384000 ||
                !info.metadataJson.empty() ||
                (!quality.empty() && quality != "128" && quality != "192" && quality != "256" &&
                 quality != "320"))
                return Status::unsupported_format;
            if (!detail::platformEncodingAvailable(f))
                return Status::unsupported_backend;
            choc::audio::AudioFileProperties props;
            props.sampleRate = info.sampleRate;
            props.numChannels = static_cast<std::uint32_t>(info.channels);
            props.bitDepth = Depth::int16;
            props.quality = std::move(quality);
            auto next = std::make_unique<State>();
            next->writer = detail::platformWriter(path, f, std::move(props));
            if (!next->writer)
                return Status::unsupported_format;
            next->spec = {info.sampleRate, max, info.channels};
            next->f.resize(info.channels);
            next->d.resize(info.channels);
            state_.swap(next);
            return Status::ok;
        }
        auto codec = format(f);
        if (!codec || !codec->supportsWriting())
            return Status::unsupported_format;
        const auto depths = codec->getSupportedBitDepths();
        if ((f == AudioFileFormat::oggVorbis
                 ? info.encoding != AudioEncoding::float32
                 : std::find(depths.begin(), depths.end(), depth(info.encoding)) == depths.end()) ||
            info.channels > codec->getMaximumNumChannels())
            return Status::unsupported_format;
        return open(std::make_shared<std::ofstream>(path, std::ios::binary | std::ios::trunc), f,
                    info, std::move(quality), max);
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::io_error;
    }
}
Status AudioFileWriter::open(std::shared_ptr<std::ostream> stream, AudioFileFormat f,
                             const AudioFileInfo& info, std::string quality,
                             std::size_t max) noexcept {
    if (!valid(info, max) || !stream || !*stream)
        return Status::invalid_argument;
    try {
        auto codec = format(f);
        if (!codec || !codec->supportsWriting())
            return Status::unsupported_format;
        const auto depths = codec->getSupportedBitDepths();
        if ((f == AudioFileFormat::oggVorbis
                 ? info.encoding != AudioEncoding::float32
                 : std::find(depths.begin(), depths.end(), depth(info.encoding)) == depths.end()) ||
            info.channels > codec->getMaximumNumChannels())
            return Status::unsupported_format;
        auto next = std::make_unique<State>();
        if (f == AudioFileFormat::wav || f == AudioFileFormat::flac)
            next->scale = pcmScale(info.encoding);
        if (next->scale != 1) {
            const auto status = next->scratch.resize(info.channels, max);
            if (status != Status::ok)
                return status;
        }
        choc::audio::AudioFileProperties p;
        p.sampleRate = info.sampleRate;
        p.numChannels = static_cast<std::uint32_t>(info.channels);
        p.bitDepth = f == AudioFileFormat::oggVorbis ? Depth::int16 : depth(info.encoding);
        p.quality = std::move(quality);
        if (!info.metadataJson.empty())
            p.metadata = choc::json::parse(info.metadataJson);
        next->spec = {info.sampleRate, max, info.channels};
        next->f.resize(info.channels);
        next->d.resize(info.channels);
        next->writer = codec->createWriter(std::move(stream), std::move(p));
        if (!next->writer)
            return Status::unsupported_format;
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::io_error;
    }
}
template <Sample T> Status AudioFileWriter::writeImpl(AudioBlock<const T> in) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (!in.isValid())
        return Status::invalid_argument;
    if (in.channels() != s.spec.channels)
        return Status::channel_mismatch;
    if (in.frames() > s.spec.maxBlockFrames ||
        in.frames() > std::numeric_limits<std::uint64_t>::max() - s.frames)
        return Status::block_too_large;
    if (!in.frames())
        return Status::ok;
    for (std::size_t ch = 0; ch < in.channels(); ++ch)
        for (auto sample : in.channel(ch))
            if (!isFinite(sample))
                return Status::invalid_argument;
    try {
        bool written = false;
        if (s.scale != 1) {
            auto scratch = s.scratch.block(in.frames());
            for (std::size_t ch = 0; ch < in.channels(); ++ch) {
                for (std::size_t i = 0; i < in.frames(); ++i)
                    scratch.channel(ch)[i] =
                        std::clamp(static_cast<double>(in.channel(ch)[i]), -1.0, 1.0) / s.scale;
                s.d[ch] = scratch.channel(ch).data();
            }
            written = s.writer->appendFrames(choc::buffer::createChannelArrayView(
                s.d.data(), static_cast<std::uint32_t>(in.channels()),
                static_cast<std::uint32_t>(in.frames())));
        } else {
            auto& pointers = [&]() -> auto& {
                if constexpr (std::same_as<T, float>)
                    return s.f;
                else
                    return s.d;
            }();
            for (std::size_t ch = 0; ch < in.channels(); ++ch)
                pointers[ch] = in.channel(ch).data();
            written = s.writer->appendFrames(choc::buffer::createChannelArrayView(
                pointers.data(), static_cast<std::uint32_t>(in.channels()),
                static_cast<std::uint32_t>(in.frames())));
        }
        if (!written)
            return Status::io_error;
        s.frames += in.frames();
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::io_error;
    }
}
Status AudioFileWriter::write(AudioBlock<const float> in) noexcept {
    return writeImpl(in);
}
Status AudioFileWriter::write(AudioBlock<const double> in) noexcept {
    return writeImpl(in);
}
Status AudioFileWriter::flush() noexcept {
    if (!state_)
        return Status::not_prepared;
    try {
        return state_->writer->flush() ? Status::ok : Status::io_error;
    } catch (...) {
        return Status::io_error;
    }
}
Status AudioFileWriter::close() noexcept {
    if (!state_)
        return Status::ok;
    auto status = flush();
    if (auto* native = dynamic_cast<detail::PlatformAudioWriter*>(state_->writer.get()))
        if (!native->finish())
            status = Status::io_error;
    state_.reset();
    return status;
}
std::uint64_t AudioFileWriter::framesWritten() const noexcept {
    return state_ ? state_->frames : 0;
}
} // namespace cydsp
