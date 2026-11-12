#pragma once
#include "cydsp/core/Types.h"
#include <cstdint>
#include <span>
#include <vector>
namespace cydsp {
struct SourceTimePoint {
    std::uint64_t outputFrame = 0;
    double sourceFrame = 0, pitchSemitones = 0;
};
struct SourceTimePosition {
    double sourceFrame = 0, sourceFramesPerOutputFrame = 1, pitchSemitones = 0;
};
struct SourceTimeMapLimits {
    double minimumSpeed = 1.0 / 64, maximumSpeed = 64, minimumPitch = -48, maximumPitch = 48;
};
// Immutable after prepare. Frames use the resampled source/output sample rate.
// Linear source/pitch segments, absolute positions (no accumulated phase drift).
// Outside the nodes: endpoint speed is extrapolated, endpoint pitch is held for
// seek priming/tails. No public control value is silently clamped.
class SourceTimeMap {
  public:
    Status prepare(std::span<const SourceTimePoint>, const SourceTimeMapLimits& = {}) noexcept;
    Status evaluate(double outputFrame, SourceTimePosition&) const noexcept;
    std::span<const SourceTimePoint> points() const noexcept {
        return points_;
    }
    double minimumSpeed() const noexcept {
        return minimumSpeed_;
    }
    double maximumSpeed() const noexcept {
        return maximumSpeed_;
    }
    bool variablePitch() const noexcept {
        return variablePitch_;
    }
    std::uint64_t outputFrames() const noexcept {
        return points_.empty() ? 0 : points_.back().outputFrame;
    }

  private:
    std::vector<SourceTimePoint> points_;
    double minimumSpeed_ = 1, maximumSpeed_ = 1;
    bool variablePitch_ = false;
};
} // namespace cydsp
