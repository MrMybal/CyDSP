// Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include "cydsp/processors/Biquad.h"
#include "cydsp/spatial/SpatialMath.h"
#include <array>
#include <memory>
namespace cydsp {
inline constexpr std::size_t neuralHrtfMaxSections = 32;
struct NeuralHrtfSection {
    double frequencyHz = 1000, q = 1, gainDb = 0;
};
// Desired total ITD is right minus left; ear levels include the training offset.
struct NeuralHrtfParameters {
    std::array<std::array<NeuralHrtfSection, neuralHrtfMaxSections>, 2> sections{};
    std::array<double, 2> levelDb{};
    std::size_t sectionCount = 0;
    double sampleRate = 0, itdMicroseconds = 0;
};
struct NeuralHrtfInfo {
    std::size_t width = 0, featureBands = 0, sectionsPerEar = 0, parameters = 0;
    double sampleRate = 0, magnitudeOffsetDb = 0;
    CpuBackend backend = CpuBackend::scalar;
};
// Immutable shared weights. load allocates and validates CYHRTF02 little-endian
// float32 assets; prediction uses bounded stack scratch and no external runtime.
// Independent const predictions may run concurrently. Load/assignment must not.
class NeuralHrtfModel {
  public:
    [[nodiscard]] Status load(std::span<const std::byte>,
                              CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] NeuralHrtfInfo info() const noexcept;
    [[nodiscard]] Status predict(Vec3 listenerDirection, NeuralHrtfParameters&) const noexcept;
    // Sequential batch. Whole-batch preflight, disjoint input/output spans,
    // no allocations; numerically identical to individual calls.
    [[nodiscard]] Status predict(std::span<const Vec3>,
                                 std::span<NeuralHrtfParameters>) const noexcept;

  private:
    struct State;
    std::shared_ptr<const State> state_;
};
// Prepared stable cascades plus causal residual delays. The cascade's intrinsic
// low-band ITD is removed before assigning external delays; never add raw ITD.
struct NeuralHrtfDesign {
    std::array<std::array<BiquadCoefficients, neuralHrtfMaxSections>, 2> sections{};
    std::array<double, 2> gain{1, 1}, delayFrames{};
    std::size_t sectionCount = 0;
    double sampleRate = 0, desiredItdMicroseconds = 0, intrinsicItdMicroseconds = 0;
};
// Bounded coefficient/phase design, more expensive than prediction. May run on a
// control/worker thread; publish the completed value between process calls.
[[nodiscard]] Status designNeuralHrtf(const NeuralHrtfParameters&, NeuralHrtfDesign&) noexcept;
struct NeuralBinauralOptions {
    double maximumDelaySeconds = .005;
    // Bounded raw-input replay at a fading design update (0 = legacy cold start).
    // Replay costs CPU in setDesign/setDesigns; max 65536, no allocation after prepare.
    std::size_t transitionHistoryFrames = 512;
};
// Experimental magnitude + low-band ITD renderer, not full measured HRIR phase.
// Reuses CyDSP double biquad kernels and a unity-magnitude Thiran fractional delay. No common
// latency; fractional delay phase is frequency dependent. Model rate must match.
// Prepare allocates; process/reset/setDesign/setDirection do not. One owner must
// serialize updates and processing. Input/output must be disjoint, mono->stereo.
template <Sample T> class NeuralBinaural {
  public:
    NeuralBinaural();
    ~NeuralBinaural();
    NeuralBinaural(NeuralBinaural&&) noexcept;
    NeuralBinaural& operator=(NeuralBinaural&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec& monoSpec, const NeuralHrtfModel&,
                                 const NeuralBinauralOptions& = {}) noexcept;
    // Full old/new output crossfade after bounded history replay under the new design.
    // Replay approximates the infinite IIR past; busy until the fade completes.
    // First design or transitionFrames=0 clears filter/delay states immediately.
    // Raw replay input is retained until reset, independently of endpoint states.
    [[nodiscard]] Status setDesign(const NeuralHrtfDesign&,
                                   std::size_t transitionFrames = 256) noexcept;
    [[nodiscard]] Status setDirection(Vec3, std::size_t transitionFrames = 256) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T> mono, AudioBlock<T> stereo) noexcept;
    // Group up to four voices per SIMD kernel. Same rate/backend, disjoint audio.
    // Sources has N mono channels; outputs has 2*N channels: L0,R0,L1,R1,...
    // Whole-call preflight precedes any state/output mutation. No implicit mixing.
    [[nodiscard]] static Status processBatch(std::span<NeuralBinaural> voices,
                                             AudioBlock<const T> sources,
                                             AudioBlock<T> stereoPairs) noexcept;
    // Prepared worker designs, one per voice; batch preflight and grouped replay.
    [[nodiscard]] static Status setDesigns(std::span<NeuralBinaural> voices,
                                           std::span<const NeuralHrtfDesign> designs,
                                           std::size_t transitionFrames = 256) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t transitionRemainingFrames() const noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept {
        return 0;
    }

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class NeuralBinaural<float>;
extern template class NeuralBinaural<double>;
} // namespace cydsp
