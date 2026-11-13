#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include "cydsp/spatial/SpatialMath.h"
#include <array>
#include <memory>
#include <span>
namespace cydsp {
// Local listener coordinates (+X forward, +Y left, +Z up), metres.
// Each input channel is an independently placed emitter. Two emitters can model
// a stereo source; a radius blends the directional field toward a direction-neutral field.
struct BinauralEmitter {
    Vec3 position{1, 0, 0};
    double gain = 1, radiusMetres = 0;
};
template <Sample T> class AnalyticBinaural {
  public:
    AnalyticBinaural();
    ~AnalyticBinaural();
    AnalyticBinaural(AnalyticBinaural&&) noexcept;
    AnalyticBinaural& operator=(AnalyticBinaural&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, double headRadiusMetres = .0875,
                                 double soundSpeed = 343) noexcept;
    [[nodiscard]] Status setEmitters(std::span<const BinauralEmitter>,
                                     std::size_t rampFrames = 256) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T> stereo) noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
// The raw-point renderer overload copies delay-removed FIRs at the prepared
// sample rate. Use HrtfSet for onset detection, conversion and shared storage.
// No HRTF database or file loader is included.
template <Sample T> struct HrirPoint {
    Vec3 direction{1, 0, 0};
    std::span<const T> left, right;
    double leftDelayFrames = 0, rightDelayFrames = 0;
};
struct HrtfPreparation {
    double sourceSampleRate = 48000, targetSampleRate = 48000;
    bool detectOnsets = true;
    double onsetThresholdDb = -40, angularWidthRadians = .7;
    std::size_t maximumTaps = 2048;
};
// Immutable, reference-counted prepared IRs. Renderer preparation shares storage;
// processing neither increments nor releases references. Onset removal is integer
// sample threshold detection, before bandlimited sample-rate conversion. Original
// explicit delays and detected onsets are retained in target-rate frame units.
template <Sample T> class HrtfSet {
  public:
    [[nodiscard]] Status prepare(std::span<const HrirPoint<T>>,
                                 const HrtfPreparation& = {}) noexcept;
    [[nodiscard]] std::size_t points() const noexcept;
    [[nodiscard]] std::size_t taps() const noexcept;
    [[nodiscard]] double sampleRate() const noexcept;
    [[nodiscard]] std::size_t
    latencyFrames() const noexcept; // Common SRC FIR delay, zero at equal rates.
    [[nodiscard]] bool truncated() const noexcept;

  private:
    template <Sample U> friend class HrirBinaural;
    template <Sample U> friend class HoaBinaural;
    struct State;
    std::shared_ptr<const State> state_;
};
template <Sample T> class HrirBinaural {
  public:
    HrirBinaural();
    ~HrirBinaural();
    HrirBinaural(HrirBinaural&&) noexcept;
    HrirBinaural& operator=(HrirBinaural&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec& mono, std::span<const HrirPoint<T>>,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec& mono, const HrtfSet<T>&,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    // Continuous all-point spherical Shepard interpolation with Gaussian locality.
    // An exact measured direction reproduces its measured FIR and onset delay.
    // FIR/delay
    // endpoints are crossfaded; an edit during a fade returns transition_in_progress.
    [[nodiscard]] Status setDirection(Vec3, std::size_t transitionFrames = 256) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T> stereo) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t tailFrames() const noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
// Prepared 2D adjacent pairs or 3D convex-hull triplets. Gains have unit energy.
// A direction outside the covered hull either selects the nearest loudspeaker
// explicitly (fallback=true), or returns unsupported_format without changing gains.
class VbapLayout {
  public:
    VbapLayout();
    ~VbapLayout();
    VbapLayout(VbapLayout&&) noexcept;
    VbapLayout& operator=(VbapLayout&&) noexcept;
    [[nodiscard]] Status prepare(std::span<const Vec3> speakers, bool threeDimensional,
                                 bool nearestFallback = false) noexcept;
    [[nodiscard]] Status gains(Vec3 direction, std::span<double>) const noexcept;
    [[nodiscard]] std::size_t speakers() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
// Real spherical harmonics, ACN channel ordering / SN3D normalization, orders 0..7.
[[nodiscard]] Status ambisonicCoefficients(unsigned order, Vec3 direction,
                                           std::span<double>) noexcept;
// Active field rotation: encoding direction d then rotating equals encoding R*d.
// Row-major C x C. No allocation; exact polynomial quadrature through order 7.
[[nodiscard]] Status ambisonicRotation(unsigned order, Quaternion, std::span<double>) noexcept;
template <Sample T> class AmbisonicRenderer {
  public:
    AmbisonicRenderer();
    ~AmbisonicRenderer();
    AmbisonicRenderer(AmbisonicRenderer&&) noexcept;
    AmbisonicRenderer& operator=(AmbisonicRenderer&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec& mono, unsigned order,
                                 std::span<const Vec3> speakers, double regularization = 1e-6,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setDirection(Vec3) noexcept;
    [[nodiscard]] Status setRotation(Quaternion) noexcept;
    [[nodiscard]] Status encode(AudioBlock<const T> mono, AudioBlock<T> ambisonic) noexcept;
    [[nodiscard]] Status rotateField(AudioBlock<const T>, AudioBlock<T>) noexcept;
    [[nodiscard]] Status decode(AudioBlock<const T>, AudioBlock<T> speakers) noexcept;
    void reset() noexcept; // Stateless transform; preserves direction/rotation.
    [[nodiscard]] std::size_t channels() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
// Projects time-aligned HRIRs onto an irregular sampled spherical-harmonic
// field by regularized least squares, then prepares the existing MIMO convolver.
// Input must be ACN/SN3D with (order+1)^2 channels. Rotation is active, in the
// same convention as AmbisonicRenderer. All IR/rotation storage is prepared.
template <Sample T> class HoaBinaural {
  public:
    HoaBinaural();
    ~HoaBinaural();
    HoaBinaural(HoaBinaural&&) noexcept;
    HoaBinaural& operator=(HoaBinaural&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec& ambisonic, unsigned order, const HrtfSet<T>&,
                                 double regularization = 1e-6,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setRotation(Quaternion) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T> stereo) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t tailFrames() const noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class HrtfSet<float>;
extern template class HrtfSet<double>;
extern template class HoaBinaural<float>;
extern template class HoaBinaural<double>;
extern template class AnalyticBinaural<float>;
extern template class AnalyticBinaural<double>;
extern template class HrirBinaural<float>;
extern template class HrirBinaural<double>;
extern template class AmbisonicRenderer<float>;
extern template class AmbisonicRenderer<double>;
} // namespace cydsp
