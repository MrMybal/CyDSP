#pragma once
#include "cydsp/core/AudioBlock.h"
#include <cstdint>
namespace cydsp {
struct AdsrParameters {
    double attackSeconds = 0.01, decaySeconds = 0.1, sustain = 0.7, releaseSeconds = 0.2;
};
class Adsr {
  public:
    [[nodiscard]] Status prepare(double sampleRate) noexcept;
    [[nodiscard]] Status setParameters(const AdsrParameters&) noexcept;
    void reset() noexcept;
    void noteOn() noexcept;
    void noteOff() noexcept;
    [[nodiscard]] double next() noexcept;
    [[nodiscard]] bool active() const noexcept {
        return stage_ != Stage::idle;
    }
    [[nodiscard]] double value() const noexcept {
        return value_;
    }

  private:
    enum class Stage { idle, attack, decay, sustain, release };
    void enter(Stage) noexcept;
    AdsrParameters parameters_{};
    double sampleRate_ = 0, value_ = 0, step_ = 0, target_ = 0;
    std::uint64_t remaining_ = 0;
    Stage stage_ = Stage::idle;
};
enum class Waveform { sine, triangle, saw, square, whiteNoise };
template <Sample T> class Oscillator {
  public:
    [[nodiscard]] Status prepare(const ProcessSpec&) noexcept;
    [[nodiscard]] Status setFrequency(double hz) noexcept;
    void setWaveform(Waveform value) noexcept {
        waveform_ = value;
    }
    void reset(double phase = 0) noexcept;
    [[nodiscard]] T next() noexcept;
    [[nodiscard]] Status process(AudioBlock<T> output, bool add = false) noexcept;

  private:
    ProcessSpec spec_{};
    Waveform waveform_ = Waveform::sine;
    double phase_ = 0, increment_ = 0;
    std::uint64_t random_ = 1;
    bool prepared_ = false;
};
extern template class Oscillator<float>;
extern template class Oscillator<double>;
} // namespace cydsp
