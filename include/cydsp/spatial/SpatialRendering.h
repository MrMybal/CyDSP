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
// SOFA import is available separately through optional cydsp/spatial/Sofa.h.
template <Sample T> struct HrirPoint {
    Vec3 direction{1, 0, 0};
    std::span<const T> left, right;
    double leftDelayFrames = 0, rightDelayFrames = 0;
};
// Approximately equal-area directions; NOT an exact t-design/Lebedev quadrature.
// Fills existing storage without allocation. Optional weights sum to 4*pi.
[[nodiscard]] Status sphericalFibonacci(std::span<Vec3> directions,
                                        std::span<double> solidAngles = {}) noexcept;
struct AnalyticHrirOptions {
    double sampleRate = 48000, headRadiusMetres = .0875, soundSpeed = 343;
    double distanceMetres = 1;
    std::size_t taps = 256, maximumStorageBytes = 512u * 1024u * 1024u;
};
struct AnalyticHrirReport {
    std::size_t points = 0, taps = 0;
    // Largest absolute sample in the next 64 frames after the retained FIR.
    // This diagnostic is not a bound on total omitted energy.
    double maximumTailSample = 0;
};
template <Sample T> class HrirBank;
template <Sample T>
[[nodiscard]] Status analyticHrirs(const AnalyticHrirOptions& options,
                                   std::span<const Vec3> directions, HrirBank<T>& out,
                                   AnalyticHrirReport* report = nullptr) noexcept;
// Owns FIRs AND views. Views remain valid across moves, until owner reprepare/destruction.
template <Sample T> class HrirBank {
  public:
    HrirBank();
    ~HrirBank();
    HrirBank(HrirBank&&) noexcept;
    HrirBank& operator=(HrirBank&&) noexcept;
    [[nodiscard]] std::span<const HrirPoint<T>> points() const noexcept;
    [[nodiscard]] double sampleRate() const noexcept;
    [[nodiscard]] std::size_t taps() const noexcept;

  private:
    template <Sample U>
    friend Status analyticHrirs(const AnalyticHrirOptions&, std::span<const Vec3>, HrirBank<U>&,
                                AnalyticHrirReport*) noexcept;
    struct State;
    std::unique_ptr<State> state_;
};
struct HrtfPreparation {
    double sourceSampleRate = 48000, targetSampleRate = 48000;
    bool detectOnsets = true;
    double onsetThresholdDb = -40, angularWidthRadians = .7;
    std::size_t maximumTaps = 2048;
    std::size_t maximumPoints = 2048, maximumInputTaps = 32768;
    std::size_t maximumStorageBytes = 512u * 1024u * 1024u;
    double maximumDelaySeconds = .1;
    std::size_t resampleHalfLength = 32;
    // Zero retains continuous all-point interpolation. 1..64 uses nearest
    // neighbours selected by an immutable prepared k-d tree.
    std::size_t interpolationPoints = 0;
    // Zero retains linear fractional delay. Odd 3..129 bakes a windowed-sinc
    // delay at direction updates, adding (taps-1)/2 common renderer latency.
    std::size_t fractionalDelayTaps = 0;
    bool rejectTruncation = false;
};
enum class HrtfQuality { economy, realtimeHigh, reference };
// Per-instance presets. No global quality state and no CPU-time guarantee.
[[nodiscard]] Status hrtfPreparation(HrtfQuality quality, double targetSampleRate,
                                     HrtfPreparation& out) noexcept;
struct HrtfStorageReport {
    std::size_t points = 0, taps = 0, coefficientBytes = 0;
    std::size_t interpolationPoints = 0, fractionalDelayTaps = 0;
    bool truncated = false;
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
    [[nodiscard]] HrtfStorageReport storageReport() const noexcept;

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
    // Spherical Shepard interpolation with Gaussian locality; the prepared policy
    // chooses all points or a bounded nearest-neighbour subset.
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
enum class HoaDecoderMethod { leastSquares, magLS };
struct HoaDecoderDesign {
    HoaDecoderMethod method = HoaDecoderMethod::magLS;
    double crossoverHz = 2000, transitionHz = 500;
    // High-band max-rE taper; optional spatial broadening, not magnitude compensation.
    bool maxRe = false;
    double regularization = 1e-6;             // normalized-weight Gram matrix
    std::size_t iterations = 12, fftSize = 0; // 0 chooses >=2*full delayed HRIR length
    std::size_t maximumFftSize = 65536, maximumStorageBytes = 512u * 1024u * 1024u;
    std::span<const double> spatialWeights; // empty = equal; otherwise one per HRTF point
};
struct HoaDecoderReport {
    HoaDecoderMethod method = HoaDecoderMethod::leastSquares;
    std::size_t fftSize = 0, filterTaps = 0, latencyFrames = 0;
    // Relative weighted RMS over supplied design directions (not unseen directions).
    double lowBandComplexError = 0, highBandMagnitudeError = 0;
    double initialHighBandMagnitudeError = 0;
    bool maxRe = false;
    bool spectralMetricsAvailable = false; // legacy time-domain overload does not measure errors
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
    // Explicit spectral design, prepared offline. MagLS preserves complex LF cues
    // and fits magnitudes above the crossover. Adds fftSize/2 causal FIR latency.
    [[nodiscard]] Status prepare(const ProcessSpec& ambisonic, unsigned order,
                                 const HrtfSet<T>& data, const HoaDecoderDesign& design,
                                 CpuBackend backend = CpuBackend::automatic) noexcept;
    [[nodiscard]] HoaDecoderReport designReport() const noexcept;
    [[nodiscard]] Status setRotation(Quaternion) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T> stereo) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t tailFrames() const noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class HrirBank<float>;
extern template class HrirBank<double>;
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
