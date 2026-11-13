#pragma once
#include "cydsp/synthesis/SampleData.h"
#include <cstdint>
#include <memory>
namespace cydsp {
enum class GrainEnvelope { hann, triangular, rectangular };
struct GrainParameters {
    double positionSeconds = 0, durationMs = 80, pitchRatio = 1, gain = 1, pan = 0;
    bool wrap = true;
    GrainEnvelope envelope = GrainEnvelope::hann;
    SampleInterpolation interpolation = SampleInterpolation::bandLimited;
};
struct GrainCloudParameters {
    GrainParameters grain;
    double grainsPerSecond = 0;
    double positionSpreadSeconds = 0, pitchSpreadSemitones = 0, panSpread = 0;
};
// Borrows SampleData: keep it alive and unchanged until reset/reprepare, serialized
// with processing. Mono/stereo sources and outputs; bounded voices, no stealing.
template <Sample T> class Granulator {
  public:
    Granulator();
    ~Granulator();
    Granulator(Granulator&&) noexcept;
    Granulator& operator=(Granulator&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, const SampleData<T>&,
                                 std::size_t maximumGrains = 128,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    // Queued offset counts output frames; smaller process blocks carry it forward.
    [[nodiscard]] Status trigger(const GrainParameters&, std::size_t frameOffset = 0) noexcept;
    [[nodiscard]] Status setCloud(const GrainCloudParameters&) noexcept;
    void reset(std::uint64_t seed = 1) noexcept;
    [[nodiscard]] Status process(AudioBlock<T>, bool add = false) noexcept;
    [[nodiscard]] std::size_t activeGrains() const noexcept; // Includes pending voices.
    [[nodiscard]] std::uint64_t droppedGrains() const noexcept;
    [[nodiscard]] CpuBackend backend() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class Granulator<float>;
extern template class Granulator<double>;
} // namespace cydsp
