#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include <array>
#include <cstdint>
#include <memory>
namespace cydsp {
struct LiveGrainParameters {
    double ageSeconds = .1, durationMs = 80, pitchRatio = 1, gain = 1;
    std::size_t inputChannel = 0;
    std::array<double, 8> channelGains{1, 0, 0, 0, 0, 0, 0, 0};
};
struct LiveCloudParameters {
    LiveGrainParameters grain;
    double grainsPerSecond = 0, ageSpreadSeconds = 0, pitchSpreadSemitones = 0;
};
// Bounded live capture, signed independent grain rates and up to 8 output channels.
// A 32-tap/64-phase low-pass bank is prepared for pitch ratios through 4x.
// ageSeconds excludes the 16-frame causal interpolation guard. A grain ends if
// it reaches uncaptured/overwritten history. Freeze stops capture, not grains.
template <Sample T> class LiveGranulator {
  public:
    LiveGranulator();
    ~LiveGranulator();
    LiveGranulator(LiveGranulator&&) noexcept;
    LiveGranulator& operator=(LiveGranulator&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec& input, std::size_t outputChannels = 2,
                                 double historySeconds = 4, std::size_t maximumGrains = 128,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status trigger(const LiveGrainParameters&, std::size_t frameOffset = 0) noexcept;
    [[nodiscard]] Status setCloud(const LiveCloudParameters&) noexcept;
    void setFrozen(bool) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T> capture, AudioBlock<T> output) noexcept;
    void reset(std::uint64_t seed = 1) noexcept;
    [[nodiscard]] std::size_t activeGrains() const noexcept;
    [[nodiscard]] std::uint64_t droppedGrains() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class LiveGranulator<float>;
extern template class LiveGranulator<double>;
} // namespace cydsp
