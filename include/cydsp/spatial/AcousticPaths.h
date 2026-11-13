#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/spatial/SpatialMath.h"
#include <cstdint>
#include <memory>
namespace cydsp {
struct AcousticPath {
    std::uint64_t id = 1;
    Vec3 arrivalDirection{1, 0, 0};
    double delaySeconds = 0;
    std::array<double, 3> bandGains{1, 1, 1};
};
// Shared mono history and stable slots for up to 64 arrivals. Stereo/multichannel
// input is averaged into a point source. Output has maximumPaths channels, one per
// slot, for subsequent binaural/VBAP/Ambisonic rendering. Input/output disjoint.
// Removing paths fades them; a full pool never steals an audible fading path.
template <Sample T> class AcousticPathRenderer {
  public:
    AcousticPathRenderer();
    ~AcousticPathRenderer();
    AcousticPathRenderer(AcousticPathRenderer&&) noexcept;
    AcousticPathRenderer& operator=(AcousticPathRenderer&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, std::size_t maximumPaths = 16,
                                 double maximumDelaySeconds = 2) noexcept;
    [[nodiscard]] Status setPaths(std::span<const AcousticPath>,
                                  std::size_t transitionFrames = 128) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T> arrivals) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t maximumPaths() const noexcept;
    [[nodiscard]] std::size_t activePaths() const noexcept;
    [[nodiscard]] AcousticPath path(std::size_t slot) const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class AcousticPathRenderer<float>;
extern template class AcousticPathRenderer<double>;
} // namespace cydsp
