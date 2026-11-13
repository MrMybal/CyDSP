#pragma once
#include "cydsp/core/AudioBlock.h"
#include <memory>
namespace cydsp {
struct LoudnessConfig {
    std::size_t maximumGatingBlocks = 288000; // Approximately eight hours, 100 ms hop.
    // Empty is allowed for mono/stereo (unit weights). Other layouts require one
    // explicit energy weight per channel; e.g. LFE=0, surround=1.41.
    std::span<const double> channelWeights;
};
struct LoudnessLevels {
    double momentaryLufs = -std::numeric_limits<double>::infinity();
    double shortTermLufs = -std::numeric_limits<double>::infinity();
    bool momentaryReady = false, shortTermReady = false;
};
// BS.1770 K weighting and two-stage integrated gating; EBU M/S/LRA time scales.
// Exact retained gating energies, bounded at prepare. process returns would_block
// BEFORE mutation if the configured history would overflow. Not certified.
template <Sample T> class LoudnessMeter {
  public:
    LoudnessMeter();
    ~LoudnessMeter();
    LoudnessMeter(LoudnessMeter&&) noexcept;
    LoudnessMeter& operator=(LoudnessMeter&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, const LoudnessConfig& = {}) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>) noexcept;
    void reset() noexcept;
    [[nodiscard]] LoudnessLevels levels() const noexcept;
    // History scans/sort: query outside the critical audio callback, serialized
    // with process. These methods do not allocate or discard recorded energies.
    [[nodiscard]] double integratedLufs() const noexcept;
    [[nodiscard]] double loudnessRangeLu() noexcept;
    [[nodiscard]] std::size_t gatingBlocks() const noexcept;
    [[nodiscard]] std::size_t capacity() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class LoudnessMeter<float>;
extern template class LoudnessMeter<double>;
} // namespace cydsp
