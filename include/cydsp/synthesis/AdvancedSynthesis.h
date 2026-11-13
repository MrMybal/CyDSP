#pragma once
#include "cydsp/processors/Generators.h"
#include "cydsp/synthesis/Wavetable.h"
#include <memory>
#include <utility>
#include <vector>
namespace cydsp {
struct EnvelopeSegment {
    double seconds = .1, target = 1, curve = 0;
};
// Segment curve in [-8,8]; zero is linear. Loops repeat [begin,end), release
// jumps to releaseSegment from the current level, without a discontinuity.
class MultiSegmentEnvelope {
  public:
    MultiSegmentEnvelope() = default;
    MultiSegmentEnvelope(const MultiSegmentEnvelope&) = delete;
    MultiSegmentEnvelope& operator=(const MultiSegmentEnvelope&) = delete;
    MultiSegmentEnvelope(MultiSegmentEnvelope&& other) noexcept {
        swap(other);
    }
    MultiSegmentEnvelope& operator=(MultiSegmentEnvelope&& other) noexcept {
        if (this != &other) {
            MultiSegmentEnvelope next(std::move(other));
            swap(next);
        }
        return *this;
    }
    [[nodiscard]] Status prepare(double sampleRate, std::size_t maximumSegments = 32) noexcept;
    [[nodiscard]] Status setSegments(std::span<const EnvelopeSegment>, std::size_t releaseSegment,
                                     std::size_t loopBegin = 0, std::size_t loopEnd = 0) noexcept;
    void trigger() noexcept;
    void release() noexcept;
    void reset() noexcept;
    [[nodiscard]] double next() noexcept;
    [[nodiscard]] bool active() const noexcept {
        return active_;
    }

  private:
    struct Segment {
        std::size_t frames = 1;
        double target = 0, curve = 0;
    };
    void swap(MultiSegmentEnvelope& o) noexcept {
        segments_.swap(o.segments_);
        std::swap(rate_, o.rate_);
        std::swap(value_, o.value_);
        std::swap(start_, o.start_);
        std::swap(count_, o.count_);
        std::swap(index_, o.index_);
        std::swap(elapsed_, o.elapsed_);
        std::swap(release_, o.release_);
        std::swap(loopBegin_, o.loopBegin_);
        std::swap(loopEnd_, o.loopEnd_);
        std::swap(active_, o.active_);
        std::swap(released_, o.released_);
    }
    std::vector<Segment> segments_;
    double rate_ = 0, value_ = 0, start_ = 0;
    std::size_t count_ = 0, index_ = 0, elapsed_ = 0, release_ = 0, loopBegin_ = 0, loopEnd_ = 0;
    bool active_ = false, released_ = false;
};
enum class LfoShape { sine, triangle, sampleAndHold, smoothRandom };
class ModulationLfo {
  public:
    [[nodiscard]] Status prepare(double sampleRate) noexcept;
    [[nodiscard]] Status setParameters(double frequencyHz, LfoShape) noexcept;
    void reset(std::uint64_t seed = 1) noexcept;
    [[nodiscard]] double next() noexcept;

  private:
    double rate_ = 0, frequency_ = 1, phase_ = 0, held_ = 0, next_ = 0;
    LfoShape shape_ = LfoShape::sine;
    std::uint64_t random_ = 1;
    double randomValue() noexcept;
};
struct ModulatedOscillatorParameters {
    Waveform waveform = Waveform::sine;
    double frequencyHz = 440, fmDepth = 1, pmDepthCycles = 1, syncHz = 0, morph = 0,
           detuneCents = 10, spread = 1, gain = 1;
    std::size_t unison = 1;
};
// FM in Hz and PM in cycles at audio rate. Hard sync runs at 4x and is FIR
// downsampled; saw/square also reuse the core PolyBLEP correction. Mip wavetables
// are borrowed, immutable and host-owned. Output mono/stereo, up to 16 unison voices.
template <Sample T> class ModulatedOscillator {
  public:
    ModulatedOscillator();
    ~ModulatedOscillator();
    ModulatedOscillator(ModulatedOscillator&&) noexcept;
    ModulatedOscillator& operator=(ModulatedOscillator&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&,
                                 std::span<const Wavetable<T>* const> tables = {},
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setParameters(const ModulatedOscillatorParameters&) noexcept;
    [[nodiscard]] Status process(AudioBlock<T>) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T> fm, AudioBlock<const T> pm,
                                 AudioBlock<T>) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
enum class ProceduralKind { wind, engine, impact };
struct ProceduralParameters {
    ProceduralKind kind = ProceduralKind::wind;
    double frequencyHz = 80, intensity = .5, roughness = .5, decaySeconds = .2;
};
// Seeded signal primitives for application-level procedural sound graphs.
template <Sample T> class ProceduralSource {
  public:
    [[nodiscard]] Status prepare(const ProcessSpec&) noexcept;
    [[nodiscard]] Status setParameters(const ProceduralParameters&) noexcept;
    void trigger(double strength = 1) noexcept;
    [[nodiscard]] Status process(AudioBlock<T>) noexcept;
    void reset(std::uint64_t seed = 1) noexcept;

  private:
    ProcessSpec spec_{};
    ProceduralParameters p_;
    double phase_ = 0, low_ = 0, energy_ = 0, pole_ = 0, decay_ = 0;
    std::uint64_t random_ = 1;
    bool prepared_ = false;
};
extern template class ModulatedOscillator<float>;
extern template class ModulatedOscillator<double>;
extern template class ProceduralSource<float>;
extern template class ProceduralSource<double>;
} // namespace cydsp
