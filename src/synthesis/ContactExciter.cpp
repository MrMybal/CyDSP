#include "cydsp/synthesis/ContactExciter.h"
#include "cydsp/core/AudioBuffer.h"
#include "detail/Analysis.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
namespace cydsp {
template <Sample T> struct ContactExciter<T>::State {
    struct Pulse {
        double amplitude = 0;
        std::size_t at = 0, frames = 0;
    };
    ProcessSpec spec;
    ContactParameters p;
    AudioBuffer<T> zeros;
    std::array<Pulse, 8> pulses{};
    std::uint64_t rng = 0x4359445350ULL;
    double bristle = 0, texture = 0, distance = 0, previous = 0, highpass = 0, pole = 0,
           texturePole = 0;
    double random() noexcept {
        rng ^= rng >> 12;
        rng ^= rng << 25;
        rng ^= rng >> 27;
        return double((rng * 2685821657736338717ULL) >> 11) * (2. / 9007199254740992.) - 1;
    }
    Status pulse(double impulse) noexcept {
        for (auto& pulse : pulses)
            if (!pulse.frames) {
                const double seconds =
                    std::clamp(std::numbers::pi * std::sqrt(p.massKg / p.stiffnessNewtonsPerMetre),
                               2 / spec.sampleRate, .1);
                pulse.frames =
                    std::max<std::size_t>(2, std::size_t(std::round(seconds * spec.sampleRate)));
                pulse.at = 0;
                // Discrete half-sine sum is cot(pi/(2*N)); preserve impulse exactly.
                pulse.amplitude = impulse * spec.sampleRate *
                                  std::tan(std::numbers::pi / (2 * double(pulse.frames)));
                return Status::ok;
            }
        return Status::update_queue_full;
    }
};
template <Sample T> ContactExciter<T>::ContactExciter() = default;
template <Sample T> ContactExciter<T>::~ContactExciter() = default;
template <Sample T> ContactExciter<T>::ContactExciter(ContactExciter&&) noexcept = default;
template <Sample T>
ContactExciter<T>& ContactExciter<T>::operator=(ContactExciter&&) noexcept = default;
template <Sample T> Status ContactExciter<T>::prepare(const ProcessSpec& spec) noexcept {
    if (!detail::validAnalysisSpec(spec) || spec.channels != 1)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->pole = std::exp(-2 * std::numbers::pi * 20 / spec.sampleRate);
        s->texturePole = std::exp(-2 * std::numbers::pi * std::min(12000., spec.sampleRate * .4) /
                                  spec.sampleRate);
        auto st = s->zeros.resize(1, spec.maxBlockFrames);
        if (st != Status::ok)
            return st;
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status ContactExciter<T>::setParameters(const ContactParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (unsigned(p.kind) > unsigned(ContactKind::bowed) || !isFinite(p.normalForceNewtons) ||
        p.normalForceNewtons < 0 || p.normalForceNewtons > 10000 ||
        !isFinite(p.velocityMetresPerSecond) || std::abs(p.velocityMetresPerSecond) > 100 ||
        !isFinite(p.friction) || p.friction < 0 || p.friction > 2 || !isFinite(p.roughness) ||
        p.roughness < 0 || p.roughness > 1 || !isFinite(p.roughnessSpacingMetres) ||
        p.roughnessSpacingMetres < 1e-5 || p.roughnessSpacingMetres > 1 || !isFinite(p.massKg) ||
        p.massKg < 1e-6 || p.massKg > 100 || !isFinite(p.stiffnessNewtonsPerMetre) ||
        p.stiffnessNewtonsPerMetre < 1 || p.stiffnessNewtonsPerMetre > 1e9 || !isFinite(p.gain) ||
        std::abs(p.gain) > 100)
        return Status::invalid_argument;
    state_->p = p;
    return Status::ok;
}
template <Sample T> Status ContactExciter<T>::strike(double impulse) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!isFinite(impulse) || std::abs(impulse) > 100)
        return Status::invalid_argument;
    return impulse == 0 ? Status::ok : state_->pulse(impulse);
}
template <Sample T>
Status ContactExciter<T>::process(AudioBlock<const T> velocity, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto st = validateProcessBlocks(s.spec, velocity, out);
    if (st != Status::ok)
        return st;
    st = detail::validateAnalysis(s.spec, velocity);
    if (st != Status::ok)
        return st;
    const double dt = 1 / s.spec.sampleRate, maximumForce = s.p.friction * s.p.normalForceNewtons;
    for (std::size_t i = 0; i < out.frames(); ++i) {
        const double v =
            std::clamp(s.p.velocityMetresPerSecond - double(velocity.channel(0)[i]), -1000., 1000.);
        s.texture = s.texturePole * s.texture + (1 - s.texturePole) * s.random();
        double force = 0;
        if (s.p.kind == ContactKind::sliding || s.p.kind == ContactKind::bowed) {
            const double stribeck =
                s.p.kind == ContactKind::bowed ? .35 + .65 * std::exp(-std::abs(v) / .04) : 1;
            const double limit = maximumForce * stribeck;
            const double compliance = std::max(1e-12, limit / s.p.stiffnessNewtonsPerMetre);
            s.bristle = (s.bristle + dt * v) / (1 + dt * std::abs(v) / compliance);
            force = std::clamp(s.p.stiffnessNewtonsPerMetre * s.bristle, -limit, limit);
            force *= (1 + s.p.roughness * s.texture);
        } else if (s.p.kind == ContactKind::rolling) {
            s.distance += std::abs(v) * dt / s.p.roughnessSpacingMetres;
            if (s.distance >= 1) {
                s.distance -= std::floor(s.distance);
                (void)s.pulse(
                    s.p.normalForceNewtons * s.p.roughness *
                    std::min(.01, s.p.roughnessSpacingMetres / std::max(.001, std::abs(v))) *
                    (.5 + .5 * s.random()));
            }
        }
        for (auto& pulse : s.pulses)
            if (pulse.frames) {
                force += pulse.amplitude *
                         std::sin(std::numbers::pi * double(pulse.at) / double(pulse.frames));
                if (++pulse.at == pulse.frames)
                    pulse.frames = 0;
            }
        s.highpass = s.pole * s.highpass + force - s.previous;
        s.previous = force;
        out.channel(0)[i] = T(s.p.gain * s.highpass);
    }
    return Status::ok;
}
template <Sample T> Status ContactExciter<T>::render(AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (out.frames() > state_->spec.maxBlockFrames)
        return Status::block_too_large;
    return process(state_->zeros.block(out.frames()), out);
}
template <Sample T> void ContactExciter<T>::reset(std::uint64_t seed) noexcept {
    if (state_) {
        auto& s = *state_;
        s.pulses = {};
        s.rng = seed ? seed : 1;
        s.bristle = s.texture = s.distance = s.previous = s.highpass = 0;
    }
}
template class ContactExciter<float>;
template class ContactExciter<double>;
} // namespace cydsp
