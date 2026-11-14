#pragma once
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/core/Cpu.h"
namespace cydsp {
enum class SampleInterpolation { linear, bandLimited };
template <Sample T> class SampleData {
  public:
    Status prepare(AudioBlock<const T>, double sampleRate, unsigned rootNote = 60,
                   CpuBackend = CpuBackend::automatic) noexcept;
    Status setLoop(std::size_t first, std::size_t end,
                   std::size_t crossfadeFrames = 0) noexcept; // [first,end), 0/0 disables
    [[nodiscard]] AudioBlock<const T> samples() const noexcept {
        return audio_.block();
    }
    [[nodiscard]] double sampleRate() const noexcept {
        return rate_;
    }
    [[nodiscard]] unsigned rootNote() const noexcept {
        return root_;
    }
    [[nodiscard]] std::size_t loopStart() const noexcept {
        return first_;
    }
    [[nodiscard]] std::size_t loopEnd() const noexcept {
        return end_;
    }

    [[nodiscard]] std::size_t loopCrossfade() const noexcept {
        return crossfade_;
    }
    [[nodiscard]] std::size_t mipLevels() const noexcept {
        return mips_.size();
    }
    [[nodiscard]] T value(std::size_t channel, double position, double ratio = 1,
                          SampleInterpolation = SampleInterpolation::bandLimited) const noexcept;
    // Sequential block playback. For each output frame: wrap an enabled loop when
    // position >= loopEnd() to loopStart() + loopCrossfade(), write
    // value(min(channel, channels - 1), position, ratio, mode) to every output
    // channel, then advance position by ratio. Frames at or past the end are
    // zero and still advance. The mip level is selected once for the block;
    // every sample equals value(). inside reports the leading frames read
    // before the end. Invalid requests preserve position and output.
    Status render(double& position, double ratio, SampleInterpolation, AudioBlock<T> output,
                  std::size_t& inside) const noexcept;

  private:
    AudioBuffer<T> audio_;
    std::vector<AudioBuffer<T>> mips_;
    double rate_ = 0;
    unsigned root_ = 60;
    std::size_t first_ = 0, end_ = 0, crossfade_ = 0;
};
extern template class SampleData<float>;
extern template class SampleData<double>;
}
