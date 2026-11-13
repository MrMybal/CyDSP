#pragma once
#include "cydsp/processors/BandGain.h"
namespace cydsp {
// Thin-screen scalar approximation, not a wedge/UTD or wave-equation solver.
// The host supplies the path geometry. edgeOffsetMetres is signed clearance:
// positive in the geometrical shadow, negative for a visible straight path.
// A finite opening adds a rectangular-aperture directional sinc magnitude.
struct DiffractionPath {
    double sourceToEdgeMetres = 1, edgeToListenerMetres = 1, edgeOffsetMetres = 0;
    double soundSpeed = 343, openingWidthMetres = 0, openingAngleRadians = 0;
};
[[nodiscard]] Status diffractionGain(const DiffractionPath&, double frequencyHz,
                                     double& linearGain) noexcept;
// Low-cost colour approximation at 125/1000/8000 Hz. Propagation delay and
// spreading belong to AcousticPathRenderer; this stage adds neither twice.
template <Sample T> class DiffractionFilter {
  public:
    DiffractionFilter();
    ~DiffractionFilter();
    DiffractionFilter(DiffractionFilter&&) noexcept;
    DiffractionFilter& operator=(DiffractionFilter&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&) noexcept;
    [[nodiscard]] Status setPath(const DiffractionPath&, std::size_t rampFrames = 256) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class DiffractionFilter<float>;
extern template class DiffractionFilter<double>;
} // namespace cydsp
