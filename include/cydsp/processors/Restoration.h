#pragma once
#include "cydsp/spectral/Stft.h"
#include <memory>
namespace cydsp {
struct DenoiseParameters {
    double reductionDb = 18, strength = 1, smoothingMs = 30, resonanceThresholdDb = 18,
           resonanceReductionDb = 0, persistenceMs = 500;
    bool learnNoise = false;
};
// Spectral subtraction/Wiener amplitude gain with a prepared noise-power profile
// and gain smoothing. Optional persistent narrow-bin suppression compares a
// time-smoothed bin against its neighbours. Learning leaves the signal audible.
template <Sample T> class SpectralDenoiser {
  public:
    SpectralDenoiser();
    ~SpectralDenoiser();
    SpectralDenoiser(SpectralDenoiser&&) noexcept;
    SpectralDenoiser& operator=(SpectralDenoiser&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, const StftConfig& = {},
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setParameters(const DenoiseParameters&) noexcept;
    [[nodiscard]] Status setNoiseProfile(std::span<const double> power) noexcept;
    [[nodiscard]] std::span<const double> noiseProfile() const noexcept; // channel-major bins
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept; // Clears stream state, retains the learned profile.
    [[nodiscard]] std::size_t latencyFrames() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
struct DehumParameters {
    double fundamentalHz = 50, q = 30, reductionDb = 60;
    std::size_t harmonics = 8;
};
template <Sample T> class Dehum {
  public:
    Dehum();
    ~Dehum();
    Dehum(Dehum&&) noexcept;
    Dehum& operator=(Dehum&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, std::size_t maximumHarmonics = 32) noexcept;
    [[nodiscard]] Status setParameters(const DehumParameters&) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
struct DeclickParameters {
    double absoluteThreshold = .02, deviationRatio = 6;
};
// Median/MAD outlier detection with future context; replaces clicks by bounded
// interpolation between non-outlier neighbours. Latency = contextFrames.
template <Sample T> class Declicker {
  public:
    Declicker();
    ~Declicker();
    Declicker(Declicker&&) noexcept;
    Declicker& operator=(Declicker&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, std::size_t contextFrames = 8) noexcept;
    [[nodiscard]] Status setParameters(const DeclickParameters&) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept;
    [[nodiscard]] std::uint64_t repairedSamples() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class SpectralDenoiser<float>;
extern template class SpectralDenoiser<double>;
extern template class Dehum<float>;
extern template class Dehum<double>;
extern template class Declicker<float>;
extern template class Declicker<double>;
} // namespace cydsp
