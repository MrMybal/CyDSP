#pragma once
#include "cydsp/core/Cpu.h"
#include "cydsp/processors/Dynamics.h"
#include <memory>
#include <span>
namespace cydsp {
enum class AdaaShape { tanh, atan, cubic, hardClip, wavefolder, hysteresis };
struct SaturationParameters {
    AdaaShape shape = AdaaShape::tanh;
    double drive = 1, bias = 0, outputGain = 1, hysteresisWidth = .1, hysteresisAmount = .5;
    bool antialias = true;
};
// First-order ADAA for memoryless shapes, with the midpoint limit near equal
// samples. Hysteresis uses a bounded play operator followed by tanh; it is a
// phenomenological model, not an ADAA primitive of a stateful magnetization law.
template <Sample T> class AdaaSaturator {
  public:
    AdaaSaturator();
    ~AdaaSaturator();
    AdaaSaturator(AdaaSaturator&&) noexcept;
    AdaaSaturator& operator=(AdaaSaturator&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&) noexcept;
    [[nodiscard]] Status setParameters(const SaturationParameters&) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
struct DynamicEqBand {
    double frequencyHz = 1000, q = 1, thresholdDb = -24, rangeDb = -12, attackMs = 10,
           releaseMs = 100, staticGainDb = 0;
};
// Parallel unity-peak bandpass correction, cascaded per band; detector bands are
// independent and use the original sidechain. rangeDb spans cut or upward EQ.
template <Sample T> class DynamicEqualizer {
  public:
    DynamicEqualizer();
    ~DynamicEqualizer();
    DynamicEqualizer(DynamicEqualizer&&) noexcept;
    DynamicEqualizer& operator=(DynamicEqualizer&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, std::size_t maximumBands = 8) noexcept;
    [[nodiscard]] Status setBands(std::span<const DynamicEqBand>) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<const T> sidechain,
                                 AudioBlock<T>) noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
// Split-band sibilance attenuation; uses the same dynamic-EQ foundation.
template <Sample T> class DeEsser {
  public:
    [[nodiscard]] Status prepare(const ProcessSpec& spec) noexcept {
        return eq_.prepare(spec, 1);
    }
    [[nodiscard]] Status setParameters(double frequencyHz = 6500, double q = .7,
                                       double thresholdDb = -24,
                                       double maximumCutDb = 12) noexcept {
        if (!isFinite(maximumCutDb) || maximumCutDb < 0 || maximumCutDb > 48)
            return Status::invalid_argument;
        const DynamicEqBand b{frequencyHz, q, thresholdDb, -maximumCutDb, 1, 60, 0};
        return eq_.setBands({&b, 1});
    }
    [[nodiscard]] Status process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
        return eq_.process(in, out);
    }
    void reset() noexcept {
        eq_.reset();
    }

  private:
    DynamicEqualizer<T> eq_;
};
struct TransientParameters {
    double attack = 0, sustain = 0, fastMs = 1, slowMs = 30, releaseMs = 150, maximumGainDb = 24;
};
template <Sample T> class TransientShaper {
  public:
    TransientShaper();
    ~TransientShaper();
    TransientShaper(TransientShaper&&) noexcept;
    TransientShaper& operator=(TransientShaper&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&) noexcept;
    [[nodiscard]] Status setParameters(const TransientParameters&) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
enum class MultibandMode { bypass, compressor, saturator };
template <Sample T> class MultibandDynamics {
  public:
    MultibandDynamics();
    ~MultibandDynamics();
    MultibandDynamics(MultibandDynamics&&) noexcept;
    MultibandDynamics& operator=(MultibandDynamics&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, std::span<const double> crossoversHz,
                                 std::size_t taps = 129,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setBand(std::size_t, MultibandMode, const DynamicsParameters& = {},
                                 const SaturationParameters& = {}) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
// Oversampled linked look-ahead limiter, with FIR up/down sampling. Ceiling
// includes a configurable reconstruction margin (default .5 dB). As for a true
// peak meter, intersample safety depends on the reconstruction/filter model;
// this API does not claim broadcast compliance or a universal analogue bound.
template <Sample T> class TruePeakLimiter {
  public:
    TruePeakLimiter();
    ~TruePeakLimiter();
    TruePeakLimiter(TruePeakLimiter&&) noexcept;
    TruePeakLimiter& operator=(TruePeakLimiter&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, unsigned oversampling = 4,
                                 double lookAheadMs = 5,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setParameters(double ceilingDbtp = -1, double releaseMs = 100,
                                       double marginDb = .5) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;
    [[nodiscard]] double latencyFrames() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class AdaaSaturator<float>;
extern template class AdaaSaturator<double>;
extern template class DynamicEqualizer<float>;
extern template class DynamicEqualizer<double>;
extern template class TransientShaper<float>;
extern template class TransientShaper<double>;
extern template class MultibandDynamics<float>;
extern template class MultibandDynamics<double>;
extern template class TruePeakLimiter<float>;
extern template class TruePeakLimiter<double>;
} // namespace cydsp
