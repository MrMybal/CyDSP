#include "cydsp/sources/SourceTimeMap.h"
#include <algorithm>
namespace cydsp {
Status SourceTimeMap::prepare(std::span<const SourceTimePoint> points,
                              const SourceTimeMapLimits& limits) noexcept {
    if (points.size() < 2 || points.size() > 65536 || points[0].outputFrame ||
        !isFinite(limits.minimumSpeed) || !isFinite(limits.maximumSpeed) ||
        limits.minimumSpeed <= 0 || limits.maximumSpeed < limits.minimumSpeed ||
        !isFinite(limits.minimumPitch) || !isFinite(limits.maximumPitch) ||
        limits.minimumPitch > limits.maximumPitch)
        return Status::invalid_argument;
    double minimum = limits.maximumSpeed, maximum = limits.minimumSpeed;
    bool pitch = false;
    for (std::size_t i = 0; i < points.size(); ++i) {
        const auto& p = points[i];
        if (p.outputFrame > 4503599627370495ull || !isFinite(p.sourceFrame) || p.sourceFrame < 0 ||
            p.sourceFrame > 4503599627370495.0 || !isFinite(p.pitchSemitones) ||
            p.pitchSemitones < limits.minimumPitch || p.pitchSemitones > limits.maximumPitch)
            return Status::invalid_argument;
        pitch = pitch || p.pitchSemitones != points[0].pitchSemitones;
        if (i) {
            const auto& previous = points[i - 1];
            if (p.outputFrame <= previous.outputFrame || p.sourceFrame <= previous.sourceFrame)
                return Status::invalid_argument;
            const auto speed = (p.sourceFrame - previous.sourceFrame) /
                               double(p.outputFrame - previous.outputFrame);
            if (!isFinite(speed) || speed < limits.minimumSpeed * (1 - 1e-12) ||
                speed > limits.maximumSpeed * (1 + 1e-12))
                return Status::invalid_argument;
            minimum = std::min(minimum, speed);
            maximum = std::max(maximum, speed);
        }
    }
    try {
        std::vector<SourceTimePoint> next(points.begin(), points.end());
        points_.swap(next);
        minimumSpeed_ = minimum;
        maximumSpeed_ = maximum;
        variablePitch_ = pitch;
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status SourceTimeMap::evaluate(double frame, SourceTimePosition& out) const noexcept {
    if (points_.empty())
        return Status::not_prepared;
    if (!isFinite(frame) || frame < -4503599627370495.0 || frame > 4503599627370495.0)
        return Status::invalid_argument;
    auto it = std::upper_bound(
        points_.begin(), points_.end(), frame,
        [](double value, const auto& point) { return value < double(point.outputFrame); });
    const auto index = it == points_.begin() ? 0
                       : it == points_.end() ? points_.size() - 2
                                             : std::size_t(it - points_.begin() - 1);
    const auto& a = points_[index];
    const auto& b = points_[index + 1];
    const auto speed = (b.sourceFrame - a.sourceFrame) / double(b.outputFrame - a.outputFrame);
    const auto delta = frame - double(a.outputFrame);
    const auto fraction = delta / double(b.outputFrame - a.outputFrame);
    const auto pitch = frame < double(points_.front().outputFrame) ? points_.front().pitchSemitones
                       : frame > double(points_.back().outputFrame)
                           ? points_.back().pitchSemitones
                           : a.pitchSemitones + (b.pitchSemitones - a.pitchSemitones) * fraction;
    out = {a.sourceFrame + delta * speed, speed, pitch};
    return Status::ok;
}
} // namespace cydsp
