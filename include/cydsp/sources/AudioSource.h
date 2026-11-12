#pragma once
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/math/VectorOps.h"
namespace cydsp {
struct AudioSourceInfo {
    double sampleRate = 0;
    std::uint64_t frames = 0;
    std::size_t channels = 0;
};
template <Sample T> class AudioSource {
  public:
    virtual ~AudioSource() = default;
    [[nodiscard]] virtual AudioSourceInfo info() const noexcept = 0;
    // No I/O/allocation/locks on this call. Short reads zero-pad and report exact frames.
    virtual Status readAt(std::uint64_t, AudioBlock<T>, std::size_t& framesRead) noexcept = 0;
};
template <Sample T> class MemoryAudioSource : public AudioSource<T> {
  public:
    Status prepare(AudioBlock<const T> data, double rate,
                   CpuBackend backend = CpuBackend::automatic) noexcept {
        if (!data.isValid() || !data.channels() || !isFinite(rate) || rate <= 0)
            return Status::invalid_argument;
        VectorOps next;
        const auto status = VectorOps::create(backend, next);
        if (status != Status::ok)
            return status;
        data_ = data;
        rate_ = rate;
        ops_ = next;
        return Status::ok;
    }
    AudioSourceInfo info() const noexcept override {
        return {rate_, data_.frames(), data_.channels()};
    }
    Status readAt(std::uint64_t first, AudioBlock<T> out, std::size_t& count) noexcept override {
        count = 0;
        if (!rate_)
            return Status::not_prepared;
        const auto status = validateProcessBlocks<T>(
            {rate_, std::numeric_limits<std::size_t>::max(), data_.channels()}, out, out);
        if (status != Status::ok)
            return status;
        const auto disjoint = validateDisjointBlocks<T>(data_, out);
        if (disjoint != Status::ok)
            return disjoint;
        // Source storage must not alias destination. It is borrowed and remains immutable.
        count = first < data_.frames()
                    ? std::min(out.frames(), data_.frames() - static_cast<std::size_t>(first))
                    : 0;
        if (count) {
            auto input = data_.subBlock(static_cast<std::size_t>(first), count);
            for (std::size_t ch = 0; ch < out.channels(); ++ch) {
                const auto copied = ops_.copy(input.channel(ch), out.channel(ch).first(count));
                if (copied != Status::ok) {
                    count = 0;
                    return copied;
                }
            }
        }
        for (std::size_t ch = 0; ch < out.channels(); ++ch)
            ops_.fill(out.channel(ch).subspan(count), T{});
        return count < out.frames() ? Status::end_of_stream : Status::ok;
    }

  private:
    AudioBlock<const T> data_;
    double rate_ = 0;
    VectorOps ops_;
};
} // namespace cydsp
