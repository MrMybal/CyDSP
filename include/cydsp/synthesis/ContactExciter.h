#pragma once
#include "cydsp/core/AudioBlock.h"
#include <cstdint>
#include <memory>
namespace cydsp {
enum class ContactKind { impact, sliding, rolling, bowed };
struct ContactParameters {
    ContactKind kind = ContactKind::sliding;
    double normalForceNewtons = 1, velocityMetresPerSecond = .1, friction = .5;
    double roughness = .2, roughnessSpacingMetres = .001;
    double massKg = .01, stiffnessNewtonsPerMetre = 10000, gain = 1;
};
// Mono force excitation for ModalResonator/Waveguide, not a complete rigid-body
// solver. Sliding uses a bounded implicit bristle model, rolling adds asperity
// contacts, bowing uses relative surface velocity and a Stribeck friction curve.
// Strike pulses have the requested impulse in N.s before the 20 Hz DC blocker.
// Physical units precede output gain. The host closes any instrument feedback.
template <Sample T> class ContactExciter {
  public:
    ContactExciter();
    ~ContactExciter();
    ContactExciter(ContactExciter&&) noexcept;
    ContactExciter& operator=(ContactExciter&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec& mono) noexcept;
    [[nodiscard]] Status setParameters(const ContactParameters&) noexcept;
    [[nodiscard]] Status strike(double impulseNewtonSeconds) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T> surfaceVelocity, AudioBlock<T>) noexcept;
    [[nodiscard]] Status render(AudioBlock<T>) noexcept;
    void reset(std::uint64_t seed = 0x4359445350ULL) noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class ContactExciter<float>;
extern template class ContactExciter<double>;
} // namespace cydsp
