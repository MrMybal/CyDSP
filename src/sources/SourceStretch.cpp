#include "cydsp/sources/SourceStretch.h"
namespace cydsp {
SourceStretchCapabilities sourceStretchCapabilities(SourceStretchMode mode) noexcept {
    SourceStretchCapabilities c;
    if (mode == SourceStretchMode::none) {
        c.available = true;
        c.minimumDurationRatio = c.maximumDurationRatio = 1;
        c.minimumPitch = c.maximumPitch = 0;
        c.minimumFormant = c.maximumFormant = 1;
    } else if (mode == SourceStretchMode::repitch) {
        c.available = true;
        c.timeCurves = true;
        c.minimumPitch = c.maximumPitch = 0;
        c.minimumFormant = c.maximumFormant = 1;
    } else if (mode == SourceStretchMode::automatic || mode == SourceStretchMode::tonal ||
               mode == SourceStretchMode::complex) {
        c.available = CYDSP_WITH_STRETCH != 0;
        c.timeCurves = c.pitchCurves = true;
    }
    return c;
}
} // namespace cydsp
