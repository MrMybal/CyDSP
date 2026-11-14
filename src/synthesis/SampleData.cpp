#include "cydsp/synthesis/SampleData.h"
#include "cydsp/math/VectorOps.h"
#include "cydsp/processors/SincInterpolator.h"
#include <algorithm>
#include <cmath>
namespace cydsp {
template <Sample T>
Status SampleData<T>::prepare(AudioBlock<const T> source, double rate, unsigned root,
                              CpuBackend backend) noexcept {
    if (!source.isValid() || !source.channels() || source.channels() > 256 || !source.frames() ||
        !isFinite(rate) || rate <= 0 || rate > 768000 || root > 127)
        return Status::invalid_argument;
    VectorOps ops;
    auto status = VectorOps::create(backend, ops);
    if (status != Status::ok)
        return status;
    AudioBuffer<T> next;
    status = next.resize(source.channels(), source.frames());
    if (status != Status::ok)
        return status;
    for (std::size_t ch = 0; ch < source.channels(); ++ch)
        (void)ops.copy(source.channel(ch), next.block().channel(ch));
    std::vector<AudioBuffer<T>> mips;
    try {
        AudioBlock<const T> current = next.block();
        for (unsigned level = 0; level < 16 && current.frames() >= 16; ++level) {
            const auto frames = (current.frames() + 1) / 2;
            AudioBuffer<T> padded, reduced;
            status = padded.resize(current.channels(), current.frames() + 64);
            if (status != Status::ok)
                return status;
            status = reduced.resize(current.channels(), frames);
            if (status != Status::ok)
                return status;
            for (std::size_t ch = 0; ch < current.channels(); ++ch) {
                ops.fill(padded.block().channel(ch), T{});
                (void)ops.copy(current.channel(ch),
                               padded.block().channel(ch).subspan(31, current.frames()));
            }
            SincInterpolator<T> converter;
            const auto block = std::min<std::size_t>(4096, frames);
            status = converter.prepare({48000, block, current.channels()}, 96000, 64, 256, backend);
            if (status != Status::ok)
                return status;
            for (std::size_t at = 0; at < frames; at += block) {
                status =
                    converter.process(padded.block(), 31 + double(at) * 2,
                                      reduced.block().subBlock(at, std::min(block, frames - at)));
                if (status != Status::ok)
                    return status;
            }
            mips.push_back(std::move(reduced));
            current = mips.back().block();
        }
    } catch (...) {
        return Status::allocation_failed;
    }
    audio_.swap(next);
    mips_.swap(mips);
    rate_ = rate;
    root_ = root;
    first_ = end_ = crossfade_ = 0;
    return Status::ok;
}
template <Sample T>
Status SampleData<T>::setLoop(std::size_t first, std::size_t end, std::size_t crossfade) noexcept {
    if (!first && !end && !crossfade) {
        first_ = end_ = crossfade_ = 0;
        return Status::ok;
    }
    if (first >= end || end > audio_.block().frames() || crossfade > (end - first) / 2)
        return Status::invalid_argument;
    first_ = first;
    end_ = end;
    crossfade_ = crossfade;
    return Status::ok;
}
namespace {
// One read position: both linear taps, shared by every channel of a frame.
struct SampleTap {
    std::size_t a = 0, b = 0;
    double fraction = 0;
    bool valid = false, next = false;
};
// Mip level chosen for one playback ratio. value() and render() both read
// through it, so the block form returns the same samples.
template <class T> struct SampleReader {
    const T* const* channels = nullptr;
    std::size_t offset = 0, frames = 0, loopNext = 0;
    double scale = 1, inverse = 1, loopEnd = 0;
    bool loop = false;
    SampleTap tap(double position) const noexcept {
        // scale is a power of two: this product equals position / scale exactly.
        const double pos = position * inverse;
        SampleTap t;
        t.a = std::size_t(pos);
        if (t.a >= frames)
            return t;
        t.valid = true;
        t.fraction = pos - double(t.a);
        t.b = t.a + 1;
        if (loop && double(t.b) * scale >= loopEnd)
            t.b = loopNext;
        t.next = t.b < frames;
        return t;
    }
    double read(const SampleTap& t, std::size_t channel) const noexcept {
        if (!t.valid)
            return 0.0;
        const T* samples = channels[channel] + offset;
        const double next = t.next ? double(samples[t.b]) : 0;
        return double(samples[t.a]) + (next - samples[t.a]) * t.fraction;
    }
};
template <class T>
SampleReader<T> sampleReader(const AudioBuffer<T>& audio, const std::vector<AudioBuffer<T>>& mips,
                             std::size_t first, std::size_t end, double ratio,
                             SampleInterpolation mode) noexcept {
    std::size_t level = 0;
    double scale = 1;
    if (mode == SampleInterpolation::bandLimited)
        while (ratio / scale > 1 && level < mips.size()) {
            ++level;
            scale *= 2;
        }
    const auto data = level ? mips[level - 1].block() : audio.block();
    SampleReader<T> reader;
    reader.channels = data.channelData();
    reader.offset = data.frameOffset();
    reader.frames = data.frames();
    reader.scale = scale;
    reader.inverse = 1 / scale;
    reader.loop = end != 0;
    reader.loopEnd = double(end);
    reader.loopNext = std::min(std::size_t(double(first) * reader.inverse), data.frames() - 1);
    return reader;
}
} // namespace
template <Sample T>
T SampleData<T>::value(std::size_t channel, double position, double ratio,
                       SampleInterpolation mode) const noexcept {
    if (channel >= audio_.block().channels() || !isFinite(position) || position < 0 ||
        position >= double(audio_.block().frames()) || !isFinite(ratio) || ratio <= 0)
        return T{};
    const auto reader = sampleReader(audio_, mips_, first_, end_, ratio, mode);
    double result = reader.read(reader.tap(position), channel);
    if (crossfade_ && position >= double(end_ - crossfade_) && position < double(end_)) {
        const auto offset = position - double(end_ - crossfade_), mix = offset / double(crossfade_);
        result = result * (1 - mix) +
                 reader.read(reader.tap(double(first_) + offset), channel) * mix;
    }
    return static_cast<T>(result);
}
template <Sample T>
Status SampleData<T>::render(double& position, double ratio, SampleInterpolation mode,
                             AudioBlock<T> output, std::size_t& inside) const noexcept {
    inside = 0;
    const auto source = audio_.block();
    if (!source.frames())
        return Status::not_prepared;
    if (!output.isValid() || !output.channels() || !isFinite(position) || position < 0 ||
        !isFinite(ratio) || ratio <= 0 ||
        static_cast<unsigned>(mode) > static_cast<unsigned>(SampleInterpolation::bandLimited))
        return Status::invalid_argument;
    const auto reader = sampleReader(audio_, mips_, first_, end_, ratio, mode);
    const double frames = double(source.frames()), loopEnd = double(end_),
                 loopFirst = double(first_ + crossfade_),
                 loopLength = double(end_ - first_ - crossfade_),
                 fadeFirst = double(end_ - crossfade_), fadeLength = double(crossfade_),
                 fadeSource = double(first_);
    const auto last = source.channels() - 1, total = output.frames(),
               channels = output.channels();
    T* const* out = output.channelData();
    const auto outOffset = output.frameOffset();
    double pos = position;
    std::size_t frame = 0;
    for (; frame < total; ++frame) {
        if (end_ && pos >= loopEnd)
            pos = loopFirst + std::fmod(pos - loopEnd, loopLength);
        if (!(pos < frames))
            break;
        const auto tap = reader.tap(pos);
        if (crossfade_ && pos >= fadeFirst && pos < loopEnd) {
            const double offset = pos - fadeFirst, mix = offset / fadeLength;
            const auto second = reader.tap(fadeSource + offset);
            for (std::size_t ch = 0; ch < channels; ++ch) {
                const auto from = std::min(ch, last);
                out[ch][outOffset + frame] = static_cast<T>(reader.read(tap, from) * (1 - mix) +
                                                            reader.read(second, from) * mix);
            }
        } else
            for (std::size_t ch = 0; ch < channels; ++ch)
                out[ch][outOffset + frame] = static_cast<T>(reader.read(tap, std::min(ch, last)));
        pos += ratio;
    }
    inside = frame;
    // Past the end: silence, with the same per-frame position sum as the loop above.
    for (std::size_t ch = 0; frame < total && ch < channels; ++ch)
        std::fill(out[ch] + outOffset + frame, out[ch] + outOffset + total, T{});
    for (; frame < total; ++frame)
        pos += ratio;
    position = pos;
    return Status::ok;
}
template class SampleData<float>;
template class SampleData<double>;
}
