#pragma once
#include "cydsp/core/AudioBlock.h"
#include <cstdint>
#include <memory>
#include <span>
namespace cydsp {
struct ResonantMode {
    double frequencyHz = 440, decaySeconds = 2, gain = 1, pan = 0;
};
enum class ModalSurface { rectangularMembrane, rectangularPlate };
// Stable two-pole modal bank. A rectangular membrane uses sqrt(m²/a²+n²/b²),
// a simply supported thin plate uses (m²/a²+n²/b²). No geometry/FEM dependency.
template <Sample T> class ModalResonator {
  public:
    ModalResonator();
    ~ModalResonator();
    ModalResonator(ModalResonator&&) noexcept;
    ModalResonator& operator=(ModalResonator&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec& mono, std::size_t outputChannels = 2,
                                 std::size_t maximumModes = 128) noexcept;
    [[nodiscard]] Status setModes(std::span<const ResonantMode>) noexcept;
    [[nodiscard]] Status setSurface(ModalSurface, double fundamentalHz, double aspect = 1.3,
                                    double decaySeconds = 2, double strikeX = .37,
                                    double strikeY = .43) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T> excitation, AudioBlock<T>) noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
enum class WaveguideModel { karplusStrong, string, tube };
struct WaveguideParameters {
    WaveguideModel model = WaveguideModel::karplusStrong;
    double frequencyHz = 220, decaySeconds = 2, brightness = .5, dispersion = .2;
    bool closedTube = false;
};
// Fractional round-trip waveguide with passive linear interpolation and bounded
// allpass dispersion. Karplus-Strong has a two-point averaging loss filter.
// Closed tubes invert a round trip and tune its delay for odd resonances.
template <Sample T> class Waveguide {
  public:
    Waveguide();
    ~Waveguide();
    Waveguide(Waveguide&&) noexcept;
    Waveguide& operator=(Waveguide&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, double minimumFrequencyHz = 20) noexcept;
    [[nodiscard]] Status setParameters(const WaveguideParameters&) noexcept;
    void pluck(double gain = 1, std::uint64_t seed = 1) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T> excitation, AudioBlock<T>) noexcept;
    [[nodiscard]] Status render(AudioBlock<T>) noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class ModalResonator<float>;
extern template class ModalResonator<double>;
extern template class Waveguide<float>;
extern template class Waveguide<double>;
} // namespace cydsp
