#pragma once
#include "cydsp/core/AudioBlock.h"
#include <memory>
namespace cydsp {
struct ChorusParameters {
    double rateHz = .5, depth = .5, centreDelayMs = 7, feedback = 0, wet = .5;
};
struct PhaserParameters {
    double rateHz = .5, depth = .5, centreHz = 1000, feedback = 0, wet = .5;
};
struct ReverbParameters {
    double decaySeconds = 1.5, damping = .3, wet = .25, width = 1;
};
// Parameters are set by the processing owner between blocks. Host automation may smooth them.
template <Sample T> class Chorus {
  public:
    Chorus();
    ~Chorus();
    Chorus(Chorus&&) noexcept;
    Chorus& operator=(Chorus&&) noexcept;
    Status prepare(const ProcessSpec&, double maximumDelayMs = 100) noexcept;
    Status setParameters(const ChorusParameters&) noexcept;
    Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
template <Sample T> class Phaser {
  public:
    Phaser();
    ~Phaser();
    Phaser(Phaser&&) noexcept;
    Phaser& operator=(Phaser&&) noexcept;
    Status prepare(const ProcessSpec&) noexcept;
    Status setParameters(const PhaserParameters&) noexcept;
    Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
template <Sample T> class Reverb {
  public:
    Reverb();
    ~Reverb();
    Reverb(Reverb&&) noexcept;
    Reverb& operator=(Reverb&&) noexcept;
    // Original Schroeder topology; decaySeconds is the nominal low-frequency RT60.
    Status prepare(const ProcessSpec&) noexcept;
    Status setParameters(const ReverbParameters&) noexcept;
    Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class Chorus<float>;
extern template class Chorus<double>;
extern template class Phaser<float>;
extern template class Phaser<double>;
extern template class Reverb<float>;
extern template class Reverb<double>;
} // namespace cydsp
