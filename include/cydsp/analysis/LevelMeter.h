#pragma once
#include "cydsp/core/AudioBlock.h"
#include <memory>
namespace cydsp {
struct ChannelLevels {
    double peak = 0, rms = 0, mean = 0, heldPeak = 0;
    [[nodiscard]] double peakDbfs() const noexcept;
    [[nodiscard]] double rmsDbfs() const noexcept;
    [[nodiscard]] double crestFactorDb() const noexcept;
};
// Exact sliding window (including peak), startup uses only received samples.
// All analyzer access is owner-serialized; getters are not concurrent UI snapshots.
template <Sample T> class LevelMeter {
  public:
    LevelMeter();
    ~LevelMeter();
    LevelMeter(LevelMeter&&) noexcept;
    LevelMeter& operator=(LevelMeter&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, std::size_t windowFrames = 2048) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>) noexcept;
    void reset() noexcept;
    [[nodiscard]] ChannelLevels levels(std::size_t channel) const noexcept;
    [[nodiscard]] double stereoCorrelation() const noexcept; // First two channels, same window.
    [[nodiscard]] std::size_t windowFrames() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class LevelMeter<float>;
extern template class LevelMeter<double>;
} // namespace cydsp
