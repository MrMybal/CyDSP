#pragma once
#include "cydsp/processors/MultiBusGraph.h"
#include "cydsp/processors/Resampler.h"
#include "cydsp/spatial/FdnReverb.h"
#include "cydsp/spatial/SpatialRendering.h"
#include "cydsp/spectral/HarmonicPercussive.h"
#include "cydsp/synthesis/SampleData.h"
namespace cydsp {
enum class QualityProfile { economy, realtimeHigh, offline };
struct QualitySettings {
    QualityProfile profile = QualityProfile::realtimeHigh;
    StftConfig spectral;
    HpssConfig separation;
    HrtfPreparation hrtf;
    ResampleQuality resampling = ResampleQuality::balanced;
    SampleInterpolation sampleInterpolation = SampleInterpolation::bandLimited;
    FdnConfig reverb;
    unsigned oversampling = 4, truePeakFactor = 4;
    std::size_t suggestedMaximumGrains = 128, transitionFrames = 256;
};
// A coherent preparation policy, not a global mutable quality switch. The host
// builds processors/graphs from these settings off the audio thread. Offline may
// still process in realtime on a sufficiently fast target; no performance promise.
[[nodiscard]] Status qualitySettings(QualityProfile, double sampleRate, QualitySettings&) noexcept;
// Tagged prepared graph transitions, fixed latency budget, no allocation or graph
// destruction during processing. stage may allocate/destroy and is non-realtime.
// Every callback context must remain alive until a later non-realtime stage/clear.
template <Sample T> class QualityGraph {
  public:
    QualityGraph();
    ~QualityGraph();
    QualityGraph(QualityGraph&&) noexcept;
    QualityGraph& operator=(QualityGraph&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, std::span<const DspBus> inputs,
                                 std::span<const DspBus> outputs, std::size_t latencyBudgetFrames,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setInitial(QualityProfile, MultiBusGraph<T>&&) noexcept;
    [[nodiscard]] Status stage(QualityProfile, MultiBusGraph<T>&&) noexcept;
    [[nodiscard]] Status startTransition(std::size_t fadeFrames = 256,
                                         std::size_t extraWarmupFrames = 0) noexcept;
    [[nodiscard]] Status process(std::span<const AudioBlock<const T>>,
                                 std::span<AudioBlock<T>>) noexcept;
    void reset() noexcept; // Cancels a running transition, preserves the active graph/profile.
    [[nodiscard]] QualityProfile activeProfile() const noexcept;
    [[nodiscard]] bool transitioning() const noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class QualityGraph<float>;
extern template class QualityGraph<double>;
} // namespace cydsp
