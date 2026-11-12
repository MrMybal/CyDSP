#include "cydsp/io/AudioAssets.h"
#include "cydsp/processors/ChannelMatrix.h"
#include "cydsp/processors/SincInterpolator.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <fstream>
#include <stdexcept>
namespace cydsp {
namespace {
struct Stamp {
    std::uint64_t bytes = 0;
    std::int64_t time = 0;
};
Stamp stamp(const std::filesystem::path& path) {
    return {std::filesystem::file_size(path),
            std::bit_cast<std::int64_t>(static_cast<std::uint64_t>(
                std::filesystem::last_write_time(path).time_since_epoch().count()))};
}
std::uint64_t buckets(std::uint64_t frames, std::uint64_t size) {
    return frames / size + (frames % size != 0);
}
void put(std::vector<std::uint8_t>& out, std::uint64_t value, unsigned bytes = 8) {
    for (unsigned i = 0; i < bytes; ++i)
        out.push_back(std::uint8_t(value >> (8 * i)));
}
std::uint64_t hash(std::span<const std::uint8_t> bytes) {
    std::uint64_t result = 14695981039346656037ULL;
    for (auto c : bytes)
        result = (result ^ c) * 1099511628211ULL;
    return result;
}
struct Cursor {
    std::span<const std::uint8_t> bytes;
    std::size_t position = 0;
    std::uint64_t get(unsigned n = 8) {
        if (n > bytes.size() - position)
            throw std::runtime_error("peak cache truncated");
        std::uint64_t result = 0;
        for (unsigned i = 0; i < n; ++i)
            result |= std::uint64_t(bytes[position++]) << (8 * i);
        return result;
    }
};
} // namespace
struct WaveformPeakCache::State {
    struct Level {
        std::uint64_t frames = 0;
        std::size_t count = 0;
        std::vector<AudioPeak> peaks;
    };
    AudioFileInfo info;
    Stamp stamp;
    std::vector<Level> levels;
};
WaveformPeakCache::WaveformPeakCache() = default;
WaveformPeakCache::~WaveformPeakCache() = default;
WaveformPeakCache::WaveformPeakCache(WaveformPeakCache&&) noexcept = default;
WaveformPeakCache& WaveformPeakCache::operator=(WaveformPeakCache&&) noexcept = default;
Status WaveformPeakCache::build(const std::filesystem::path& source, std::size_t size,
                                std::size_t maximum) noexcept {
    if (!size || size > 1048576 || !maximum || maximum > 16777216)
        return Status::invalid_argument;
    try {
        auto next = std::make_unique<State>();
        next->stamp = stamp(source);
        AudioFileReader reader;
        auto r = reader.open(source);
        if (r != Status::ok)
            return r;
        next->info = *reader.info();
        const auto count = buckets(next->info.frames, size);
        if (count > maximum || count > 16777216 / next->info.channels)
            return Status::block_too_large;
        State::Level level;
        level.frames = size;
        level.count = std::size_t(count);
        level.peaks.resize(level.count * next->info.channels);
        AudioBuffer<double> audio;
        r = audio.resize(next->info.channels, 4096);
        if (r != Status::ok)
            return r;
        std::array<double, 256> sums{};
        for (std::uint64_t first = 0; first < next->info.frames;) {
            const auto n = std::size_t(std::min<std::uint64_t>(4096, next->info.frames - first));
            std::size_t read = 0;
            r = reader.read(first, audio.block(n), read);
            if (r != Status::ok)
                return r;
            for (std::size_t i = 0; i < n; ++i) {
                const auto frame = first + i, offset = frame % size;
                const auto bucket = std::size_t(frame / size);
                for (std::size_t ch = 0; ch < next->info.channels; ++ch) {
                    const auto value = audio.block().channel(ch)[i];
                    if (!isFinite(value) || std::abs(value) > std::numeric_limits<float>::max())
                        return Status::invalid_argument;
                    auto& p = level.peaks[ch * level.count + bucket];
                    const auto v = static_cast<float>(value);
                    if (!offset)
                        p.minimum = p.maximum = v;
                    else {
                        p.minimum = std::min(p.minimum, v);
                        p.maximum = std::max(p.maximum, v);
                    }
                    sums[ch] += value * value;
                    if (offset + 1 == size || frame + 1 == next->info.frames) {
                        p.rms = static_cast<float>(std::sqrt(sums[ch] / double(offset + 1)));
                        sums[ch] = 0;
                    }
                }
            }
            first += n;
        }
        next->levels.push_back(std::move(level));
        while (next->levels.back().count > 1) {
            const auto& previous = next->levels.back();
            State::Level reduced;
            reduced.frames = previous.frames * 2;
            reduced.count = (previous.count + 1) / 2;
            reduced.peaks.resize(reduced.count * next->info.channels);
            for (std::size_t ch = 0; ch < next->info.channels; ++ch)
                for (std::size_t i = 0; i < reduced.count; ++i) {
                    auto value = previous.peaks[ch * previous.count + 2 * i];
                    if (2 * i + 1 < previous.count) {
                        const auto& b = previous.peaks[ch * previous.count + 2 * i + 1];
                        const auto na =
                            std::min(previous.frames,
                                     next->info.frames - std::uint64_t(2 * i) * previous.frames);
                        const auto nb =
                            std::min(previous.frames, next->info.frames - std::uint64_t(2 * i + 1) *
                                                                              previous.frames);
                        value.minimum = std::min(value.minimum, b.minimum);
                        value.maximum = std::max(value.maximum, b.maximum);
                        value.rms = static_cast<float>(
                            std::sqrt((double(value.rms) * value.rms * double(na) +
                                       double(b.rms) * b.rms * double(nb)) /
                                      double(na + nb)));
                    }
                    reduced.peaks[ch * reduced.count + i] = value;
                }
            next->levels.push_back(std::move(reduced));
        }
        const auto after = stamp(source);
        if (after.bytes != next->stamp.bytes || after.time != next->stamp.time)
            return Status::transition_in_progress;
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::io_error;
    }
}
Status WaveformPeakCache::save(const std::filesystem::path& path) const noexcept {
    if (!state_)
        return Status::not_prepared;
    try {
        const auto& s = *state_;
        std::vector<std::uint8_t> out{'C', 'y', 'P', 'e', 'a', 'k', 's', '1'};
        put(out, s.info.frames);
        put(out, std::bit_cast<std::uint64_t>(s.info.sampleRate));
        put(out, s.info.channels);
        put(out, s.stamp.bytes);
        put(out, std::bit_cast<std::uint64_t>(s.stamp.time));
        put(out, s.levels.size());
        for (const auto& level : s.levels) {
            put(out, level.frames);
            put(out, level.count);
            for (const auto& p : level.peaks)
                for (float value : {p.minimum, p.maximum, p.rms})
                    put(out, std::bit_cast<std::uint32_t>(value), 4);
        }
        put(out, hash(out));
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        stream.write(reinterpret_cast<const char*>(out.data()), std::streamsize(out.size()));
        stream.flush();
        return stream ? Status::ok : Status::io_error;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::io_error;
    }
}
Status WaveformPeakCache::load(const std::filesystem::path& path,
                               const std::filesystem::path& source) noexcept {
    try {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
            return Status::io_error;
        const auto length = file.tellg();
        if (length < 64 || length > 536870912)
            return Status::invalid_argument;
        std::vector<std::uint8_t> bytes(std::size_t(length), 0);
        file.seekg(0);
        file.read(reinterpret_cast<char*>(bytes.data()), length);
        if (!file)
            return Status::io_error;
        constexpr std::array<std::uint8_t, 8> magic{'C', 'y', 'P', 'e', 'a', 'k', 's', '1'};
        if (!std::equal(magic.begin(), magic.end(), bytes.begin()))
            return Status::unsupported_format;
        Cursor footer{{bytes.data() + bytes.size() - 8, 8}};
        if (footer.get() != hash(std::span(bytes).first(bytes.size() - 8)))
            return Status::invalid_argument;
        Cursor reader{std::span(bytes).first(bytes.size() - 8), 8};
        auto next = std::make_unique<State>();
        next->info.frames = reader.get();
        next->info.sampleRate = std::bit_cast<double>(reader.get());
        const auto channels = reader.get();
        if (!channels || channels > 256 || !isFinite(next->info.sampleRate) ||
            next->info.sampleRate <= 0)
            return Status::invalid_argument;
        next->info.channels = std::size_t(channels);
        next->stamp.bytes = reader.get();
        next->stamp.time = std::bit_cast<std::int64_t>(reader.get());
        const auto expected = stamp(source);
        if (expected.bytes != next->stamp.bytes || expected.time != next->stamp.time)
            return Status::invalid_argument;
        const auto levels = reader.get();
        if (!levels || levels > 64)
            return Status::invalid_argument;
        std::uint64_t previousFrames = 0, previousCount = 0;
        for (std::uint64_t i = 0; i < levels; ++i) {
            State::Level level;
            level.frames = reader.get();
            const auto count = reader.get();
            if (!level.frames || count != buckets(next->info.frames, level.frames) ||
                count > 16777216 / channels || (!i && level.frames > 1048576) ||
                (i && (level.frames != previousFrames * 2 || count != (previousCount + 1) / 2)))
                return Status::invalid_argument;
            if (count * channels > (reader.bytes.size() - reader.position) / 12)
                return Status::invalid_argument;
            level.count = std::size_t(count);
            level.peaks.resize(level.count * std::size_t(channels));
            for (auto& p : level.peaks) {
                p.minimum = std::bit_cast<float>(std::uint32_t(reader.get(4)));
                p.maximum = std::bit_cast<float>(std::uint32_t(reader.get(4)));
                p.rms = std::bit_cast<float>(std::uint32_t(reader.get(4)));
                if (!isFinite(p.minimum) || !isFinite(p.maximum) || !isFinite(p.rms) ||
                    p.minimum > p.maximum || p.rms < 0)
                    return Status::invalid_argument;
            }
            previousFrames = level.frames;
            previousCount = count;
            next->levels.push_back(std::move(level));
        }
        if (previousCount > 1 || reader.position != reader.bytes.size())
            return Status::invalid_argument;
        AudioFileReader original;
        const auto opened = original.open(source);
        if (opened != Status::ok)
            return opened;
        const auto& info = *original.info();
        if (info.frames != next->info.frames || info.channels != next->info.channels ||
            info.sampleRate != next->info.sampleRate)
            return Status::invalid_argument;
        next->info = info;
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::invalid_argument;
    }
}
const AudioFileInfo* WaveformPeakCache::info() const noexcept {
    return state_ ? &state_->info : nullptr;
}
std::size_t WaveformPeakCache::levels() const noexcept {
    return state_ ? state_->levels.size() : 0;
}
std::uint64_t WaveformPeakCache::bucketFrames(std::size_t level) const noexcept {
    return state_ && level < state_->levels.size() ? state_->levels[level].frames : 0;
}
std::span<const AudioPeak> WaveformPeakCache::peaks(std::size_t ch,
                                                    std::size_t level) const noexcept {
    if (!state_ || ch >= state_->info.channels || level >= state_->levels.size())
        return {};
    const auto& data = state_->levels[level];
    return std::span(data.peaks).subspan(ch * data.count, data.count);
}
std::size_t WaveformPeakCache::selectLevel(double frames) const noexcept {
    if (!state_ || !isFinite(frames) || frames < 0)
        return 0;
    std::size_t level = 0;
    while (level + 1 < state_->levels.size() && double(state_->levels[level + 1].frames) <= frames)
        ++level;
    return level;
}
template <Sample T>
Status ImpulseResponse<T>::load(const std::filesystem::path& path,
                                const ImpulseLoadOptions& options) noexcept {
    if (!options.maximumFrames || options.channels > 256 || !isFinite(options.sampleRate) ||
        options.sampleRate < 0 || options.sampleRate > 768000 ||
        !isFinite(options.trimThresholdDb) || options.trimThresholdDb > 0 ||
        options.trimThresholdDb < -300)
        return Status::invalid_argument;
    try {
        AudioFileReader reader;
        auto r = reader.open(path);
        if (r != Status::ok)
            return r;
        const auto file = *reader.info();
        const auto rate = options.sampleRate ? options.sampleRate : file.sampleRate;
        const auto channels = options.channels ? options.channels : file.channels;
        const auto count = std::ceil(double(file.frames) * rate / file.sampleRate);
        if (!file.frames || file.frames > options.maximumFrames || !isFinite(count) || count < 1 ||
            count > double(options.maximumFrames))
            return Status::block_too_large;
        AudioBuffer<T> source, mapped, padded, result;
        r = source.resize(file.channels, std::size_t(file.frames));
        if (r != Status::ok)
            return r;
        for (std::size_t at = 0; at < file.frames; at += 4096) {
            std::size_t n = 0;
            r = reader.read(at,
                            source.block().subBlock(
                                at, std::min<std::size_t>(4096, std::size_t(file.frames) - at)),
                            n);
            if (r != Status::ok)
                return r;
        }
        r = mapped.resize(channels, std::size_t(file.frames));
        if (r != Status::ok)
            return r;
        std::vector<double> coefficients(file.channels * channels);
        r = defaultChannelMatrix(file.channels, channels, coefficients);
        if (r != Status::ok)
            return r;
        ChannelMatrix<T> matrix;
        r = matrix.prepare({file.sampleRate, std::size_t(file.frames), file.channels}, channels,
                           coefficients, options.backend);
        if (r != Status::ok)
            return r;
        r = matrix.process(source.block(), mapped.block());
        if (r != Status::ok)
            return r;
        if (rate == file.sampleRate)
            result.swap(mapped);
        else {
            r = padded.resize(channels, std::size_t(file.frames) + 64);
            if (r != Status::ok)
                return r;
            r = result.resize(channels, std::size_t(count));
            if (r != Status::ok)
                return r;
            VectorOps ops;
            r = VectorOps::create(options.backend, ops);
            if (r != Status::ok)
                return r;
            for (std::size_t ch = 0; ch < channels; ++ch)
                (void)ops.copy(mapped.block().channel(ch),
                               padded.block().channel(ch).subspan(31, std::size_t(file.frames)));
            const auto block = std::min<std::size_t>(4096, std::size_t(count));
            SincInterpolator<T> convert;
            r = convert.prepare({rate, block, channels}, file.sampleRate, 64, 1024,
                                options.backend);
            if (r != Status::ok)
                return r;
            for (std::size_t at = 0; at < std::size_t(count); at += block) {
                r = convert.process(
                    padded.block(), 31 + double(at) * file.sampleRate / rate,
                    result.block().subBlock(at, std::min(block, std::size_t(count) - at)));
                if (r != Status::ok)
                    return r;
            }
        }
        auto frames = result.block().frames();
        double peak = 0;
        for (std::size_t ch = 0; ch < channels; ++ch)
            for (auto v : result.block().channel(ch)) {
                if (!isFinite(v))
                    return Status::invalid_argument;
                peak = std::max(peak, std::abs(double(v)));
            }
        if (options.trimSilence) {
            const auto threshold = std::pow(10.0, options.trimThresholdDb / 20);
            while (frames > 1) {
                bool silent = true;
                for (std::size_t ch = 0; ch < channels; ++ch)
                    if (std::abs(double(result.block().channel(ch)[frames - 1])) > threshold)
                        silent = false;
                if (!silent)
                    break;
                --frames;
            }
        }
        if (options.normalize && peak > 0) {
            VectorOps ops;
            r = VectorOps::create(options.backend, ops);
            if (r != Status::ok)
                return r;
            for (std::size_t ch = 0; ch < channels; ++ch) {
                if (peak >= 1 / double(std::numeric_limits<T>::max()))
                    (void)ops.scale(result.block().channel(ch), result.block().channel(ch),
                                    static_cast<T>(1 / peak));
                else
                    for (auto& v : result.block().channel(ch))
                        v = static_cast<T>(double(v) / peak);
            }
        }
        audio_.swap(result);
        frames_ = frames;
        rate_ = rate;
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::io_error;
    }
}
template class ImpulseResponse<float>;
template class ImpulseResponse<double>;
} // namespace cydsp
