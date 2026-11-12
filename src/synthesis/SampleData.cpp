#include "cydsp/synthesis/SampleData.h"
#include "cydsp/math/VectorOps.h"
#include "cydsp/processors/SincInterpolator.h"
#include <algorithm>
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
template <Sample T>
T SampleData<T>::value(std::size_t channel, double position, double ratio,
                       SampleInterpolation mode) const noexcept {
    if (channel >= audio_.block().channels() || !isFinite(position) || position < 0 ||
        position >= double(audio_.block().frames()) || !isFinite(ratio) || ratio <= 0)
        return T{};
    std::size_t level = 0;
    double scale = 1;
    if (mode == SampleInterpolation::bandLimited)
        while (ratio / scale > 1 && level < mips_.size()) {
            ++level;
            scale *= 2;
        }
    auto data = level ? mips_[level - 1].block() : audio_.block();
    auto read = [&](double pos) {
        pos /= scale;
        const auto a = std::size_t(pos);
        if (a >= data.frames())
            return 0.0;
        const auto fraction = pos - double(a);
        auto b = a + 1;
        if (end_ && double(b) * scale >= double(end_))
            b = std::min(std::size_t(double(first_) / scale), data.frames() - 1);
        const auto next = b < data.frames() ? double(data.channel(channel)[b]) : 0;
        return double(data.channel(channel)[a]) + (next - data.channel(channel)[a]) * fraction;
    };
    double result = read(position);
    if (crossfade_ && position >= double(end_ - crossfade_) && position < double(end_)) {
        const auto offset = position - double(end_ - crossfade_), mix = offset / double(crossfade_);
        result = result * (1 - mix) + read(double(first_) + offset) * mix;
    }
    return static_cast<T>(result);
}
template class SampleData<float>;
template class SampleData<double>;
}
