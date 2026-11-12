#include "cydsp/io/EditingPcmProxy.h"
#include "cydsp/core/AudioBuffer.h"
#include <algorithm>
#include <bit>
#include <choc/memory/choc_Base64.h>
#include <choc/text/choc_JSON.h>
#include <chrono>
#include <fstream>
#include <random>
#include <stdexcept>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__linux__)
#include <cerrno>
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
namespace cydsp {
namespace {
bool publishCache(const std::filesystem::path& temporary,
                  const std::filesystem::path& cache) noexcept {
#if defined(_WIN32)
    // No REPLACE_EXISTING flag: an intervening destination is never overwritten.
    return MoveFileExW(temporary.c_str(), cache.c_str(), MOVEFILE_WRITE_THROUGH) != 0;
#elif defined(__linux__) && defined(SYS_renameat2)
    if (::syscall(SYS_renameat2, static_cast<long>(AT_FDCWD), temporary.c_str(),
                  static_cast<long>(AT_FDCWD), cache.c_str(), RENAME_NOREPLACE) == 0)
        return true;
    if (errno != ENOSYS && errno != EINVAL && errno != EOPNOTSUPP)
        return false;
#endif
#if !defined(_WIN32)
    std::error_code error;
    std::filesystem::create_hard_link(temporary, cache, error);
    if (error)
        return false;
    std::filesystem::remove(temporary, error);
    return true;
#endif
}
std::uint64_t stamp(const std::filesystem::path& p) {
    return std::uint64_t(std::filesystem::last_write_time(p).time_since_epoch().count());
}
std::string sourceIdentity(const std::filesystem::path& p) {
    auto s = std::filesystem::canonical(p).generic_u8string();
    return {reinterpret_cast<const char*>(s.data()), s.size()};
}
std::uint64_t number(choc::value::ValueView v) {
    if (!v.isInt() || v.get<std::int64_t>() < 0)
        throw std::runtime_error("proxy integer");
    return std::uint64_t(v.get<std::int64_t>());
}
std::string text(choc::value::ValueView v) {
    if (!v.isString() || v.getString().size() > 8388608)
        throw std::runtime_error("proxy text");
    return std::string(v.getString());
}
} // namespace
struct EditingPcmProxy::State {
    AudioFileReader reader;
    AudioFileInfo source;
    PcmProxyStatistics stats;
};
EditingPcmProxy::EditingPcmProxy() = default;
EditingPcmProxy::~EditingPcmProxy() = default;
EditingPcmProxy::EditingPcmProxy(EditingPcmProxy&&) noexcept = default;
EditingPcmProxy& EditingPcmProxy::operator=(EditingPcmProxy&&) noexcept = default;
Status EditingPcmProxy::build(const std::filesystem::path& source,
                              const std::filesystem::path& cache,
                              const PcmProxyOptions& o) noexcept {
    if (!o.blockFrames || o.blockFrames > 1048576 || !o.maximumBytes ||
        (o.encoding != AudioEncoding::float32 && o.encoding != AudioEncoding::float64) ||
        cache.empty())
        return Status::invalid_argument;
    try {
        if (std::filesystem::exists(cache))
            return Status::invalid_argument;
        const auto sourceId = sourceIdentity(source);
        const auto sourceBytes = std::filesystem::file_size(source);
        const auto sourceStamp = stamp(source);
        AudioFileReader input;
        auto r = input.open(source, o.blockFrames);
        if (r != Status::ok)
            return r;
        const auto info = *input.info();
        const auto channelScratchBytes =
            ((o.blockFrames + 7) / 8) * 8 * sizeof(double) + sizeof(double*);
        if (!info.channels ||
            info.frames > std::uint64_t(std::numeric_limits<std::int64_t>::max()) ||
            info.channels > o.maximumWorkingBufferBytes / channelScratchBytes)
            return Status::block_too_large;
        const std::uint64_t bytesPerFrame =
            info.channels * (o.encoding == AudioEncoding::float64 ? 8u : 4u);
        if (info.frames > o.maximumBytes / bytesPerFrame || info.metadataJson.size() > 4194304)
            return Status::block_too_large;
        // Decimal strings preserve unsigned file-clock tokens without a signed JSON conversion.
        auto descriptor = choc::json::create(
            "schema", "CyDSP.PcmProxy", "version", 1, "source", sourceId, "sourceBytes",
            std::to_string(sourceBytes), "sourceStamp", std::to_string(sourceStamp), "frames",
            std::int64_t(info.frames), "rate", info.sampleRate, "channels",
            std::int64_t(info.channels), "format", info.formatName, "encoding",
            std::int64_t(info.encoding), "metadata", info.metadataJson);
        const auto json = choc::json::toString(descriptor);
        if (json.size() > 6291456)
            return Status::block_too_large;
        auto native = choc::value::createEmptyArray();
        native.addArrayElement(
            choc::json::create("type", "CyPx", "content", choc::base64::encodeToString(json)));
        AudioFileInfo pcm = info;
        pcm.formatName = "WAV";
        pcm.encoding = o.encoding;
        pcm.metadataJson = choc::json::toString(native);
        std::random_device random;
        auto temporary = cache;
        temporary += std::filesystem::path(".cydsp-" + std::to_string(random()) + "-" +
                                           std::to_string(random()) + ".tmp");
        if (std::filesystem::exists(temporary))
            return Status::io_error;
        struct Cleanup {
            std::filesystem::path path;
            bool remove = true;
            ~Cleanup() {
                if (remove) {
                    std::error_code ec;
                    std::filesystem::remove(path, ec);
                }
            }
        } cleanup{temporary};
        AudioFileWriter writer;
        r = writer.open(temporary, AudioFileFormat::wav, pcm, {}, o.blockFrames);
        if (r != Status::ok)
            return r;
        AudioBuffer<double> block;
        r = block.resize(info.channels, o.blockFrames);
        if (r != Status::ok)
            return r;
        for (std::uint64_t frame = 0; frame < info.frames;) {
            const auto frames =
                std::size_t(std::min<std::uint64_t>(o.blockFrames, info.frames - frame));
            std::size_t read = 0;
            r = input.read(frame, block.block(frames), read);
            if (r != Status::ok || read != frames)
                return Status::io_error;
            r = writer.write(block.block(frames));
            if (r != Status::ok)
                return r;
            frame += frames;
        }
        r = writer.close();
        if (r != Status::ok)
            return r;
        if (sourceIdentity(source) != sourceId || stamp(source) != sourceStamp ||
            std::filesystem::file_size(source) != sourceBytes)
            return Status::io_error;
        // Atomic no-replace publication: native rename, or hard-link fallback.
        // The complete published file may remain if final reopening fails; never
        // delete a destination that another actor could have replaced meanwhile.
        if (!publishCache(temporary, cache))
            return Status::io_error;
        auto previous = std::move(state_);
        r = open(source, cache, o.blockFrames);
        if (r != Status::ok) {
            state_ = std::move(previous);
            return r;
        }
        state_->stats.framesCopied = info.frames;
        state_->stats.pcmBytes = info.frames * bytesPerFrame;
        state_->stats.workingBufferBytes = info.channels * channelScratchBytes;
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::io_error;
    }
}
Status EditingPcmProxy::open(const std::filesystem::path& source,
                             const std::filesystem::path& cache, std::size_t maximum) noexcept {
    try {
        auto next = std::make_unique<State>();
        auto r = next->reader.open(cache, maximum);
        if (r != Status::ok)
            return r;
        const auto& pcm = *next->reader.info();
        if (pcm.formatName != "WAV" ||
            (pcm.encoding != AudioEncoding::float32 && pcm.encoding != AudioEncoding::float64) ||
            pcm.metadataJson.size() > 16777216)
            return Status::invalid_argument;
        auto metadata = choc::json::parse(pcm.metadataJson);
        choc::value::Value descriptor;
        if (!metadata.isArray() || metadata.size() > 16)
            return Status::invalid_argument;
        for (auto entry : metadata)
            if (entry["type"].isString() && entry["type"].getString() == "CyPx") {
                const auto encoded = text(entry["content"]);
                std::string decoded;
                if (!choc::base64::decode(encoded, [&](std::uint8_t b) { decoded += char(b); }))
                    return Status::invalid_argument;
                if (decoded.size() > 8388608)
                    return Status::block_too_large;
                // Descriptor structure has fixed scalar/string fields; reject nested JSON.
                unsigned depth = 0;
                bool string = false, escape = false;
                for (char c : decoded) {
                    if (string) {
                        if (escape)
                            escape = false;
                        else if (c == '\\')
                            escape = true;
                        else if (c == '"')
                            string = false;
                    } else if (c == '"')
                        string = true;
                    else if (c == '{' || c == '[') {
                        if (++depth > 2)
                            return Status::invalid_argument;
                    } else if (c == '}' || c == ']') {
                        if (!depth)
                            return Status::invalid_argument;
                        --depth;
                    }
                }
                descriptor = choc::json::parse(decoded);
                break;
            }
        if (!descriptor.isObject() || text(descriptor["schema"]) != "CyDSP.PcmProxy" ||
            number(descriptor["version"]) != 1 ||
            text(descriptor["source"]) != sourceIdentity(source) ||
            text(descriptor["sourceBytes"]) != std::to_string(std::filesystem::file_size(source)) ||
            text(descriptor["sourceStamp"]) != std::to_string(stamp(source)))
            return Status::invalid_argument;
        auto& info = next->source;
        info.frames = number(descriptor["frames"]);
        info.channels = std::size_t(number(descriptor["channels"]));
        info.sampleRate = descriptor["rate"].get<double>();
        info.formatName = text(descriptor["format"]);
        auto encoding = number(descriptor["encoding"]);
        if (encoding > unsigned(AudioEncoding::float64))
            return Status::invalid_argument;
        info.encoding = AudioEncoding(encoding);
        info.metadataJson = text(descriptor["metadata"]);
        if (info.frames != pcm.frames || info.channels != pcm.channels ||
            info.sampleRate != pcm.sampleRate)
            return Status::invalid_argument;
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::invalid_argument;
    }
}
Status EditingPcmProxy::read(std::uint64_t frame, AudioBlock<float> out,
                             std::size_t& read) noexcept {
    read = 0;
    if (!state_)
        return Status::not_prepared;
    auto r = state_->reader.read(frame, out, read);
    if (r == Status::ok || r == Status::end_of_stream) {
        ++state_->stats.readCalls;
        state_->stats.framesRead += read;
    }
    return r;
}
Status EditingPcmProxy::read(std::uint64_t frame, AudioBlock<double> out,
                             std::size_t& read) noexcept {
    read = 0;
    if (!state_)
        return Status::not_prepared;
    auto r = state_->reader.read(frame, out, read);
    if (r == Status::ok || r == Status::end_of_stream) {
        ++state_->stats.readCalls;
        state_->stats.framesRead += read;
    }
    return r;
}
const AudioFileInfo* EditingPcmProxy::sourceInfo() const noexcept {
    return state_ ? &state_->source : nullptr;
}
const AudioFileInfo* EditingPcmProxy::pcmInfo() const noexcept {
    return state_ ? state_->reader.info() : nullptr;
}
PcmProxyStatistics EditingPcmProxy::statistics() const noexcept {
    return state_ ? state_->stats : PcmProxyStatistics{};
}
void EditingPcmProxy::close() noexcept {
    state_.reset();
}
} // namespace cydsp
