#include "cydsp/sources/ReadAheadSource.h"
#include "cydsp/core/SpscQueue.h"
#include "cydsp/processors/ChannelMatrix.h"
#include "cydsp/processors/SincInterpolator.h"
#include "cydsp/processors/TimeStretch.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <thread>
namespace cydsp {
namespace {
struct RecorderJob {
    std::size_t index = 0, frames = 0;
};
struct CacheRequest {
    std::uint64_t frame = 0;
    std::uint32_t epoch = 0;
};
} // namespace
template <Sample T> struct ReadAheadSource<T>::State {
    struct Page {
        AudioBuffer<T> audio;
        std::uint64_t first = 0;
        std::size_t count = 0;
        std::uint32_t epoch = 0;
        Status status = Status::ok;
    };
    AudioFileReader reader;
    AudioSourceInfo info;
    std::uint32_t inputRate = 0, outputRate = 0;
    std::size_t pageFrames = 0, pageCount = 0;
    AudioBuffer<T> input, decoded, stretchInput;
    std::size_t fileChannels = 0, decodeLimit = 0;
    std::uint64_t resampledFrames = 0;
    ChannelMatrix<T> matrix;
    double durationRatio = 1, formant = 1, tonalityLimit = 0;
    SourceTimeMap timeMap;
    SourceStretchInfo policy;
    std::size_t controlQuantum = 64;
    std::unique_ptr<SourceStretchProcessor<T>> provider;
    AudioBuffer<T> repitchInput, repitchOutput;
    SincInterpolator<T> repitchConverter;
    std::vector<double> positions;
    std::size_t repitchLimit = 0;
    bool repitch = false;
    SourceTimePosition point(std::uint64_t frame) const noexcept {
        SourceTimePosition p;
        (void)timeMap.evaluate(double(frame), p);
        return p;
    }
    std::size_t inputLatency() const noexcept {
        if (provider)
            return provider->inputLatency();
#if CYDSP_SOURCES_STRETCH
        return stretch.inputLatency();
#else
        return 0;
#endif
    }
    std::size_t outputLatency() const noexcept {
        if (provider)
            return provider->outputLatency();
#if CYDSP_SOURCES_STRETCH
        return stretch.outputLatency();
#else
        return 0;
#endif
    }
    bool stretched = false;
#if CYDSP_SOURCES_STRETCH
    TimeStretch<T> stretch;
#endif
    std::uint64_t stretchNext = std::numeric_limits<std::uint64_t>::max(), inputCursor = 0;
    SincInterpolator<T> converter;
    VectorOps ops;
    std::vector<Page> pages;
    SpscQueue<std::size_t> free, ready;
    SpscQueue<CacheRequest> requests;
    std::thread worker;
    std::atomic<bool> stopped{false};
    std::atomic<std::uint32_t> queued{0}, ack{0}, failure{0}, prefetched{0};
    std::uint64_t expected = 0;
    std::uint32_t epoch = 0;
    std::atomic<std::uint32_t> misses{0};
    std::size_t held = std::numeric_limits<std::size_t>::max();
    bool requested = false;
    ~State() {
        stopped.store(true, std::memory_order_release);
        if (worker.joinable())
            worker.join();
    }
    void releaseHeld() noexcept {
        if (held != std::numeric_limits<std::size_t>::max()) {
            queued.fetch_sub(static_cast<std::uint32_t>(pages[held].count),
                             std::memory_order_relaxed);
            (void)free.push(held);
            held = std::numeric_limits<std::size_t>::max();
        }
    }
    bool request(std::uint64_t frame) noexcept {
        releaseHeld();
        std::size_t id;
        for (std::size_t i = 0; i < pageCount && ready.pop(id); ++i) {
            queued.fetch_sub(static_cast<std::uint32_t>(pages[id].count),
                             std::memory_order_relaxed);
            (void)free.push(id);
        }
        const auto next = epoch + 1;
        if (!requests.push({frame, next})) {
            requested = false;
            return false;
        }
        epoch = next;
        expected = frame;
        requested = true;
        return true;
    }
    Status decode(std::uint64_t first, AudioBlock<T> out) noexcept {
        if (inputRate == outputRate) {
            std::size_t n = 0;
            const auto result = reader.read(first, out, n);
            return result == Status::end_of_stream ? Status::ok : result;
        }
        const auto quotient = first / outputRate, remainder = first % outputRate;
        if (quotient > std::numeric_limits<std::uint64_t>::max() / inputRate)
            return Status::invalid_argument;
        const auto product = remainder * inputRate;
        const auto base = quotient * inputRate, extra = product / outputRate;
        if (extra > std::numeric_limits<std::uint64_t>::max() - base)
            return Status::invalid_argument;
        const auto whole = base + extra;
        const auto fraction = static_cast<double>(product % outputRate) / outputRate;
        constexpr std::size_t left = 31;
        const auto prefix = whole < left ? left - static_cast<std::size_t>(whole) : 0;
        const auto inputFirst = whole < left ? 0 : whole - left;
        auto source = input.block();
        for (std::size_t ch = 0; ch < source.channels(); ++ch)
            ops.fill(source.channel(ch), T{});
        std::size_t read = 0;
        const auto result =
            reader.read(inputFirst, source.subBlock(prefix, source.frames() - prefix), read);
        if (result != Status::ok && result != Status::end_of_stream)
            return result;
        return converter.process(source, static_cast<double>(left) + fraction, out);
    }
    Status mapped(std::uint64_t first, AudioBlock<T> out) noexcept {
        if (fileChannels == info.channels)
            return decode(first, out);
        auto raw = decoded.block(out.frames());
        const auto r = decode(first, raw);
        if (r != Status::ok)
            return r;
        return matrix.process(raw, out);
    }
    Status fillRepitch(std::uint64_t first, AudioBlock<T> out) noexcept {
        const auto ratio = double(inputRate) / outputRate;
        const auto start = point(first).sourceFrame * ratio;
        const auto end = point(first + out.frames() - 1).sourceFrame * ratio;
        if (start < 0 || !isFinite(end))
            return Status::invalid_argument;
        constexpr std::size_t left = 255, taps = 512;
        const auto whole = std::uint64_t(std::floor(start));
        const auto prefix = whole < left ? left - std::size_t(whole) : 0;
        const auto inputFirst = whole < left ? 0 : whole - left;
        const auto length = std::size_t(std::ceil(end - double(whole))) + taps + 2;
        if (length > repitchLimit)
            return Status::block_too_large;
        auto raw = repitchInput.block(length);
        for (std::size_t ch = 0; ch < fileChannels; ++ch)
            ops.fill(raw.channel(ch), T{});
        std::size_t read = 0;
        auto r = reader.read(inputFirst, raw.subBlock(prefix, length - prefix), read);
        if (r != Status::ok && r != Status::end_of_stream)
            return r;
        for (std::size_t i = 0; i < out.frames(); ++i)
            positions[i] = double(left) + point(first + i).sourceFrame * ratio - double(whole);
        auto interpolated = repitchOutput.block(out.frames());
        r = repitchConverter.processPositions(raw, {positions.data(), out.frames()}, interpolated);
        if (r != Status::ok)
            return r;
        if (fileChannels != info.channels)
            return matrix.process(interpolated, out);
        for (std::size_t ch = 0; ch < info.channels; ++ch)
            (void)ops.copy(interpolated.channel(ch), out.channel(ch));
        return Status::ok;
    }
    Status fill(Page& page, std::uint64_t first) noexcept {
        page.first = first;
        page.count = std::size_t(std::min<std::uint64_t>(pageFrames, info.frames - first));
        auto out = page.audio.block(page.count);
        if (repitch)
            return fillRepitch(first, out);
        if (!stretched)
            return mapped(first, out);
        const auto targetInput = [&](std::uint64_t frame) {
            return std::uint64_t(std::floor(point(frame).sourceFrame));
        };
        const auto latency = outputLatency();
        if (stretchNext != first) {
            const auto target = targetInput(first), future = targetInput(first + latency) - target;
            const auto past = inputLatency(), length = past + std::size_t(future);
            const auto prefix = target < past ? past - std::size_t(target) : 0;
            auto history = stretchInput.block(length);
            for (std::size_t ch = 0; ch < info.channels; ++ch)
                ops.fill(history.channel(ch), T{});
            auto r = Status::ok;
            if (policy.seek == SourceSeekPolicy::primeHistory)
                r = mapped(target < past ? 0 : target - past,
                           history.subBlock(prefix, length - prefix));
            else if (future)
                r = mapped(target, history.subBlock(past, std::size_t(future)));
            if (r != Status::ok)
                return r;
            const SourceStretchControl control{
                &timeMap, first, target < past ? 0 : target - past, formant, tonalityLimit, prefix};
            if (provider)
                r = provider->prime(history, control);
#if CYDSP_SOURCES_STRETCH
            else {
                r = stretch.setPitchSemitones(point(first).pitchSemitones, tonalityLimit);
                if (r == Status::ok)
                    r = stretch.prime(history);
            }
#else
            else
                r = Status::unsupported_backend;
#endif
            if (r != Status::ok)
                return r;
            inputCursor = target + future;
            stretchNext = first;
        }
        for (std::size_t offset = 0; offset < out.frames();) {
            const auto frames = std::min(controlQuantum, out.frames() - offset);
            const auto outputFirst = first + offset,
                       end = targetInput(outputFirst + latency + frames);
            if (end < inputCursor || end - inputCursor > decodeLimit)
                return Status::block_too_large;
            auto in = stretchInput.block(std::size_t(end - inputCursor));
            auto r = mapped(inputCursor, in);
            if (r != Status::ok)
                return r;
            const SourceStretchControl control{&timeMap, outputFirst, inputCursor, formant,
                                               tonalityLimit};
            if (provider)
                r = provider->process(in, out.subBlock(offset, frames), control);
#if CYDSP_SOURCES_STRETCH
            else {
                r = stretch.setPitchSemitones(point(outputFirst).pitchSemitones, tonalityLimit);
                if (r == Status::ok)
                    r = stretch.process(in, out.subBlock(offset, frames));
            }
#else
            else
                r = Status::unsupported_backend;
#endif
            if (r != Status::ok)
                return r;
            inputCursor = end;
            offset += frames;
            stretchNext = outputFirst + frames;
        }
        return Status::ok;
    }
    void run() noexcept {
        CacheRequest request;
        std::uint64_t next = 0;
        std::uint32_t generation = 0;
        bool active = false;
        while (!stopped.load(std::memory_order_acquire)) {
            bool changed = false;
            for (std::size_t i = 0; i < requests.capacity() && requests.pop(request); ++i) {
                next = request.frame;
                generation = request.epoch;
                active = true;
                changed = true;
            }
            if (changed) {
#if CYDSP_SOURCES_STRETCH
                stretchNext = std::numeric_limits<std::uint64_t>::max();
#endif
                failure.store(0, std::memory_order_relaxed);
                prefetched.store(0, std::memory_order_relaxed);
                ack.store(generation, std::memory_order_release);
            }
            std::size_t index;
            if (!active || next >= info.frames || !free.pop(index)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            auto& page = pages[index];
            page.epoch = generation;
            page.status = fill(page, next);
            if (page.status != Status::ok) {
                page.count = 0;
                failure.store(static_cast<std::uint32_t>(page.status), std::memory_order_release);
                active = false;
            }
            next += page.count;
            queued.fetch_add(static_cast<std::uint32_t>(page.count), std::memory_order_relaxed);
            if (!ready.push(index)) {
                failure.store(static_cast<std::uint32_t>(Status::update_queue_full),
                              std::memory_order_release);
                active = false;
            } else
                prefetched.fetch_add(static_cast<std::uint32_t>(page.count),
                                     std::memory_order_release);
        }
    }
};
template <Sample T> ReadAheadSource<T>::ReadAheadSource() = default;
template <Sample T> ReadAheadSource<T>::~ReadAheadSource() = default;
template <Sample T> ReadAheadSource<T>::ReadAheadSource(ReadAheadSource&&) noexcept = default;
template <Sample T>
ReadAheadSource<T>& ReadAheadSource<T>::operator=(ReadAheadSource&&) noexcept = default;
template <Sample T>
Status ReadAheadSource<T>::prepare(const std::filesystem::path& path, std::uint32_t rate,
                                   std::size_t frames, std::size_t count,
                                   CpuBackend backend) noexcept {
    ReadAheadOptions options;
    options.outputRate = rate;
    options.pageFrames = frames;
    options.pages = count;
    options.backend = backend;
    return prepare(path, options);
}
template <Sample T>
Status ReadAheadSource<T>::prepare(const std::filesystem::path& path,
                                   const ReadAheadOptions& options) noexcept {
    const auto frames = options.pageFrames, count = options.pages;
    const auto rate = options.outputRate;
    const auto backend = options.backend;
    if (!frames || frames > 65536 || count < 2 || count > 128 || options.outputChannels > 256 ||
        !isFinite(options.durationRatio) || options.durationRatio <= 0 ||
        !isFinite(options.pitchSemitones) || !isFinite(options.formant) || options.formant <= 0 ||
        !options.controlQuantumFrames || options.controlQuantumFrames > 65536 ||
        !isFinite(options.tonalityLimitHz) || options.tonalityLimitHz < 0 ||
        static_cast<unsigned>(options.mode) > static_cast<unsigned>(SourceStretchMode::external) ||
        static_cast<unsigned>(options.seek) >
            static_cast<unsigned>(SourceSeekPolicy::resetHistory) ||
        static_cast<unsigned>(options.tails) >
            static_cast<unsigned>(SourceTailPolicy::includeAlgorithmTail))
        return Status::invalid_argument;
    try {
        auto next = std::make_unique<State>();
        auto status = next->reader.open(path, 1048576);
        if (status != Status::ok)
            return status;
        const auto& file = *next->reader.info();
        if (file.sampleRate < 1 || file.sampleRate > 768000 ||
            std::floor(file.sampleRate) != file.sampleRate)
            return Status::unsupported_format;
        next->inputRate = static_cast<std::uint32_t>(file.sampleRate);
        next->outputRate = rate ? rate : next->inputRate;
        if (next->outputRate > 768000)
            return Status::invalid_argument;
        const auto whole = file.frames / next->inputRate, remainder = file.frames % next->inputRate;
        if (whole > std::numeric_limits<std::uint64_t>::max() / next->outputRate)
            return Status::invalid_argument;
        const auto base = whole * next->outputRate;
        const auto extra = (remainder * next->outputRate + next->inputRate - 1) / next->inputRate;
        if (extra > std::numeric_limits<std::uint64_t>::max() - base)
            return Status::invalid_argument;
        next->resampledFrames = base + extra;
        next->fileChannels = file.channels;
        next->decodeLimit = frames;
        next->durationRatio = options.durationRatio;
        next->formant = options.formant;
        next->controlQuantum = std::min(options.controlQuantumFrames, frames);
        const auto channels = options.outputChannels ? options.outputChannels : file.channels;
        next->info = {double(next->outputRate), next->resampledFrames, channels};
        auto mode = options.mode;
        if (mode == SourceStretchMode::automatic)
            mode = (options.durationRatio != 1 || options.pitchSemitones != 0 ||
                    options.formant != 1 || !options.timeMap.empty())
                       ? SourceStretchMode::complex
                       : SourceStretchMode::none;
        auto caps = sourceStretchCapabilities(mode);
        if (options.provider) {
            status = options.provider->create(mode, next->provider);
            if (status != Status::ok)
                return status;
            if (!next->provider)
                return Status::invalid_spec;
            caps = next->provider->capabilities();
        }
        if (!caps.available)
            return Status::unsupported_format;
        if (!isFinite(caps.minimumDurationRatio) || caps.minimumDurationRatio <= 0 ||
            !isFinite(caps.maximumDurationRatio) ||
            caps.maximumDurationRatio < caps.minimumDurationRatio || !isFinite(caps.minimumPitch) ||
            !isFinite(caps.maximumPitch) || caps.minimumPitch > caps.maximumPitch ||
            !isFinite(caps.minimumFormant) || caps.minimumFormant <= 0 ||
            !isFinite(caps.maximumFormant) || caps.maximumFormant < caps.minimumFormant)
            return Status::invalid_spec;
        if (options.formant < caps.minimumFormant || options.formant > caps.maximumFormant)
            return Status::invalid_argument;
        next->policy.mode = mode;
        next->policy.seek = options.seek;
        next->policy.tails = options.tails;
        next->policy.capabilities = caps;
        next->policy.controlQuantumFrames = next->controlQuantum;
        next->policy.externalProvider = bool(next->provider);
        next->tonalityLimit = mode == SourceStretchMode::tonal ? options.tonalityLimitHz : 0;
        if (next->tonalityLimit > double(next->outputRate) / 2)
            return Status::invalid_argument;
        if (!options.timeMap.empty() && (options.durationRatio != 1 || options.pitchSemitones != 0))
            return Status::invalid_argument;
        if (!next->resampledFrames) {
            if (mode != SourceStretchMode::none || !options.timeMap.empty())
                return Status::invalid_argument;
        } else {
            std::vector<SourceTimePoint> points = options.timeMap;
            if (points.empty()) {
                if (options.durationRatio < caps.minimumDurationRatio ||
                    options.durationRatio > caps.maximumDurationRatio ||
                    options.pitchSemitones < caps.minimumPitch ||
                    options.pitchSemitones > caps.maximumPitch)
                    return Status::invalid_argument;
                const auto total = std::ceil(double(next->resampledFrames) * options.durationRatio);
                if (!isFinite(total) || total < 1 || total > 4503599627370495.0)
                    return Status::invalid_argument;
                points = {
                    {0, 0, options.pitchSemitones},
                    {std::uint64_t(total), total / options.durationRatio, options.pitchSemitones}};
            } else if (points.back().sourceFrame > double(next->resampledFrames))
                return Status::invalid_argument;
            status = next->timeMap.prepare(points, {1 / caps.maximumDurationRatio,
                                                    1 / caps.minimumDurationRatio,
                                                    caps.minimumPitch, caps.maximumPitch});
            if (status != Status::ok)
                return status;
            if ((!caps.timeCurves &&
                 next->timeMap.minimumSpeed() != next->timeMap.maximumSpeed()) ||
                (!caps.pitchCurves && next->timeMap.variablePitch()))
                return Status::unsupported_format;
            next->policy.timelineFrames = next->timeMap.outputFrames();
            next->info.frames = next->policy.timelineFrames;
        }
        next->repitch = mode == SourceStretchMode::repitch && !next->provider;
        next->stretched = mode != SourceStretchMode::none && !next->repitch;
        if (mode == SourceStretchMode::none &&
            (!options.timeMap.empty() || options.durationRatio != 1 ||
             options.pitchSemitones != 0 || options.formant != 1 || next->provider))
            return Status::invalid_argument;
        if (next->stretched) {
            if (next->provider) {
                status = next->provider->prepare({double(next->outputRate), 1048576, channels},
                                                 frames, backend);
                if (status != Status::ok)
                    return status;
            } else {
#if CYDSP_SOURCES_STRETCH
                status = next->stretch.prepare({double(next->outputRate), 1048576, channels},
                                               frames, options.cheaperStretch, true, backend);
                if (status != Status::ok)
                    return status;
                status = next->stretch.setFormant(options.formant);
                if (status != Status::ok)
                    return status;
#else
                return Status::unsupported_backend;
#endif
            }
            const auto speed = next->timeMap.maximumSpeed();
            const auto needed = std::max(std::ceil(double(frames) * speed) + 2,
                                         double(next->inputLatency()) +
                                             std::ceil(double(next->outputLatency()) * speed) + 2);
            if (!isFinite(needed) || needed > 1048576)
                return Status::block_too_large;
            next->decodeLimit = std::size_t(needed);
            status = next->stretchInput.resize(channels, next->decodeLimit);
            if (status != Status::ok)
                return status;
            next->policy.primePastFrames = next->inputLatency();
            next->policy.primeFutureFrames =
                std::size_t(std::ceil(double(next->outputLatency()) * speed));
            if (options.tails == SourceTailPolicy::includeAlgorithmTail) {
                const auto tail =
                    std::ceil(double(next->inputLatency()) / next->timeMap.minimumSpeed()) +
                    double(next->outputLatency());
                if (!isFinite(tail) || tail > 1048576)
                    return Status::block_too_large;
                next->policy.tailFrames = std::uint64_t(tail);
                next->info.frames += next->policy.tailFrames;
            }
            if (next->info.frames > 4503599627370495ull - next->outputLatency())
                return Status::invalid_argument;
        }
        if (next->repitch) {
            const auto step =
                next->timeMap.maximumSpeed() * double(next->inputRate) / next->outputRate;
            const auto needed = std::ceil(double(frames) * step) + 514;
            if (!isFinite(needed) || needed > 1048576 || step > 768000)
                return Status::block_too_large;
            next->repitchLimit = std::size_t(needed);
            next->positions.resize(frames);
            status = next->repitchInput.resize(file.channels, next->repitchLimit);
            if (status != Status::ok)
                return status;
            status = next->repitchOutput.resize(file.channels, frames);
            if (status != Status::ok)
                return status;
            status = next->repitchConverter.prepare({1, frames, file.channels}, step, 512, 1024,
                                                    backend);
            if (status != Status::ok)
                return status;
            next->policy.repitchAntiAliasMaximumStep = step;
        }
        if (channels != file.channels) {
            status = next->decoded.resize(file.channels, next->decodeLimit);
            if (status != Status::ok)
                return status;
            std::vector<double> coefficients(file.channels * channels);
            status = defaultChannelMatrix(file.channels, channels, coefficients);
            if (status != Status::ok)
                return status;
            status =
                next->matrix.prepare({double(next->outputRate), next->decodeLimit, file.channels},
                                     channels, coefficients, backend);
            if (status != Status::ok)
                return status;
        }
        next->pageFrames = frames;
        next->pageCount = count;
        status = VectorOps::create(backend, next->ops);
        if (status != Status::ok)
            return status;
        status = next->free.prepare(count);
        if (status != Status::ok)
            return status;
        status = next->ready.prepare(count);
        if (status != Status::ok)
            return status;
        status = next->requests.prepare(8);
        if (status != Status::ok)
            return status;
        next->pages.resize(count);
        for (std::size_t i = 0; i < count; ++i) {
            status = next->pages[i].audio.resize(channels, frames);
            if (status != Status::ok)
                return status;
            (void)next->free.push(i);
        }
        if (next->inputRate != next->outputRate) {
            const auto length =
                static_cast<std::size_t>(std::ceil(static_cast<double>(next->decodeLimit) *
                                                   next->inputRate / next->outputRate)) +
                66;
            if (length > 1048576)
                return Status::invalid_argument;
            status = next->input.resize(file.channels, length);
            if (status != Status::ok)
                return status;
            status = next->converter.prepare(
                {static_cast<double>(next->outputRate), next->decodeLimit, file.channels},
                file.sampleRate, 64, 1024, backend);
            if (status != Status::ok)
                return status;
        }
        next->worker = std::thread([state = next.get()] { state->run(); });
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    } catch (...) {
        return Status::io_error;
    }
}
template <Sample T> AudioSourceInfo ReadAheadSource<T>::info() const noexcept {
    return state_ ? state_->info : AudioSourceInfo{};
}
template <Sample T> SourceStretchInfo ReadAheadSource<T>::stretchInfo() const noexcept {
    return state_ ? state_->policy : SourceStretchInfo{};
}

template <Sample T>
Status ReadAheadSource<T>::readAt(std::uint64_t first, AudioBlock<T> out,
                                  std::size_t& count) noexcept {
    count = 0;
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    const auto status =
        validateProcessBlocks<T>({s.info.sampleRate, 1048576, s.info.channels}, out, out);
    if (status != Status::ok)
        return status;
    for (std::size_t ch = 0; ch < out.channels(); ++ch)
        s.ops.fill(out.channel(ch), T{});
    if (!out.frames())
        return Status::ok;
    if (first >= s.info.frames)
        return Status::end_of_stream;
    if (!s.requested || first != s.expected)
        if (!s.request(first)) {
            ++s.misses;
            return Status::would_block;
        }
    std::size_t visited = 0;
    while (count < out.frames() && first + count < s.info.frames && visited <= s.pageCount) {
        if (s.held == std::numeric_limits<std::size_t>::max()) {
            if (!s.ready.pop(s.held))
                break;
            ++visited;
        }
        auto& page = s.pages[s.held];
        const auto position = first + count;
        if (page.epoch != s.epoch || position >= page.first + page.count) {
            s.releaseHeld();
            continue;
        }
        if (page.status != Status::ok)
            return page.status;
        if (position < page.first)
            break;
        const auto offset = static_cast<std::size_t>(position - page.first),
                   n = std::min(out.frames() - count, page.count - offset);
        for (std::size_t ch = 0; ch < out.channels(); ++ch)
            (void)s.ops.copy(page.audio.block().channel(ch).subspan(offset, n),
                             out.channel(ch).subspan(count, n));
        count += n;
        if (offset + n == page.count)
            s.releaseHeld();
    }
    s.expected = first + count;
    if (count == out.frames())
        return Status::ok;
    if (s.expected >= s.info.frames)
        return Status::end_of_stream;
    const auto error = s.failure.load(std::memory_order_acquire);
    if (error && s.ack.load(std::memory_order_acquire) == s.epoch)
        return static_cast<Status>(error);
    ++s.misses;
    return Status::would_block;
}
template <Sample T>
Status ReadAheadSource<T>::prefill(std::uint64_t first, std::size_t minimum,
                                   unsigned timeout) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (first > s.info.frames || minimum > s.pageFrames * s.pageCount || timeout > 60000)
        return Status::invalid_argument;
    if (first == s.info.frames)
        return Status::end_of_stream;
    if (!s.request(first))
        return Status::update_queue_full;
    const auto wanted = std::min<std::uint64_t>(minimum, s.info.frames - first);
    const auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout);
    do {
        // An old in-flight read can publish after request() drained ready.
        // Recycle stale pages while waiting, otherwise a full-cache prefill
        // could wait forever for the slot occupied by that obsolete page.
        if (s.held == std::numeric_limits<std::size_t>::max()) {
            for (std::size_t i = 0; i < s.pageCount && s.ready.pop(s.held); ++i) {
                if (s.pages[s.held].epoch == s.epoch)
                    break; // Retain the first current page for the audio consumer.
                s.releaseHeld();
            }
        }
        if (s.ack.load(std::memory_order_acquire) == s.epoch) {
            const auto failure = s.failure.load(std::memory_order_acquire);
            if (failure)
                return static_cast<Status>(failure);
            if (s.prefetched.load(std::memory_order_acquire) >= wanted)
                return Status::ok;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (std::chrono::steady_clock::now() < until);
    return Status::would_block;
}
template <Sample T> std::uint32_t ReadAheadSource<T>::underruns() const noexcept {
    return state_ ? state_->misses.load(std::memory_order_relaxed) : 0;
}
template <Sample T> void ReadAheadSource<T>::close() noexcept {
    state_.reset();
}
template <Sample T> struct BackgroundRecorder<T>::State {
    AudioFileWriter writer;
    ProcessSpec spec;
    VectorOps ops;
    std::vector<AudioBuffer<T>> blocks;
    SpscQueue<std::size_t> free;
    SpscQueue<RecorderJob> ready;
    std::thread worker;
    std::atomic<bool> stop{false};
    std::atomic<std::uint32_t> error{0}, dropped{0};
    ~State() {
        stop.store(true, std::memory_order_release);
        if (worker.joinable())
            worker.join();
    }
    void run() noexcept {
        for (;;) {
            RecorderJob job;
            bool available = ready.pop(job);
            if (!available && stop.load(std::memory_order_acquire)) {
                // Recheck after acquiring stop: the final producer publication may
                // have followed the first empty observation. finish serializes it.
                available = ready.pop(job);
                if (!available)
                    break;
            }
            if (available) {
                const auto status =
                    writer.write(AudioBlock<const T>(blocks[job.index].block(job.frames)));
                if (status != Status::ok)
                    error.store(static_cast<std::uint32_t>(status), std::memory_order_release);
                (void)free.push(job.index);
            } else
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        const auto status = writer.close();
        if (status != Status::ok)
            error.store(static_cast<std::uint32_t>(status), std::memory_order_release);
    }
};
template <Sample T> BackgroundRecorder<T>::BackgroundRecorder() = default;
template <Sample T> BackgroundRecorder<T>::~BackgroundRecorder() = default;
template <Sample T>
BackgroundRecorder<T>::BackgroundRecorder(BackgroundRecorder&&) noexcept = default;
template <Sample T>
BackgroundRecorder<T>& BackgroundRecorder<T>::operator=(BackgroundRecorder&&) noexcept = default;
template <Sample T>
Status BackgroundRecorder<T>::prepare(const std::filesystem::path& path, AudioFileFormat format,
                                      const AudioFileInfo& info, std::size_t frames,
                                      std::size_t count, CpuBackend backend) noexcept {
    if (state_ && !state_->stop.load(std::memory_order_acquire))
        return Status::transition_in_progress;
    if (!frames || frames > 65536 || count < 2 || count > 128)
        return Status::invalid_argument;
    try {
        auto next = std::make_unique<State>();
        next->spec = {info.sampleRate, frames, info.channels};
        auto status = VectorOps::create(backend, next->ops);
        if (status != Status::ok)
            return status;
        status = next->free.prepare(count);
        if (status != Status::ok)
            return status;
        status = next->ready.prepare(count);
        if (status != Status::ok)
            return status;
        next->blocks.resize(count);
        for (std::size_t i = 0; i < count; ++i) {
            status = next->blocks[i].resize(info.channels, frames);
            if (status != Status::ok)
                return status;
            (void)next->free.push(i);
        }
        status = next->writer.open(path, format, info, {}, frames);
        if (status != Status::ok)
            return status;
        next->worker = std::thread([s = next.get()] { s->run(); });
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::io_error;
    }
}
template <Sample T> Status BackgroundRecorder<T>::push(AudioBlock<const T> in) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (!in.isValid() || in.channels() != s.spec.channels)
        return Status::invalid_argument;
    if (in.frames() > s.spec.maxBlockFrames)
        return Status::block_too_large;
    const auto error = s.error.load(std::memory_order_acquire);
    if (error)
        return static_cast<Status>(error);
    if (s.stop.load(std::memory_order_acquire))
        return Status::end_of_stream;
    if (!in.frames())
        return Status::ok;
    std::size_t i;
    if (!s.free.pop(i)) {
        s.dropped.fetch_add(1, std::memory_order_relaxed);
        return Status::update_queue_full;
    }
    for (std::size_t ch = 0; ch < in.channels(); ++ch)
        (void)s.ops.copy(in.channel(ch), s.blocks[i].block(in.frames()).channel(ch));
    return s.ready.push({i, in.frames()}) ? Status::ok : Status::update_queue_full;
}
template <Sample T> Status BackgroundRecorder<T>::finish() noexcept {
    if (!state_)
        return Status::not_prepared;
    state_->stop.store(true, std::memory_order_release);
    if (state_->worker.joinable())
        state_->worker.join();
    return static_cast<Status>(state_->error.load(std::memory_order_acquire));
}
template <Sample T> std::uint32_t BackgroundRecorder<T>::droppedBlocks() const noexcept {
    return state_ ? state_->dropped.load(std::memory_order_relaxed) : 0;
}
template class ReadAheadSource<float>;
template class ReadAheadSource<double>;
template class BackgroundRecorder<float>;
template class BackgroundRecorder<double>;
} // namespace cydsp
