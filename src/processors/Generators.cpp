#include "cydsp/processors/Generators.h"
#include "detail/OscillatorMath.h"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace cydsp {
Status Adsr::prepare(double rate) noexcept {
    if (!isFinite(rate) || rate <= 0 || rate > 1e9)
        return Status::invalid_spec;
    sampleRate_ = rate;
    reset();
    return Status::ok;
}
Status Adsr::setParameters(const AdsrParameters& p) noexcept {
    if (!isFinite(p.attackSeconds) || !isFinite(p.decaySeconds) || !isFinite(p.sustain) ||
        !isFinite(p.releaseSeconds) || p.attackSeconds < 0 || p.decaySeconds < 0 ||
        p.releaseSeconds < 0 || p.sustain < 0 || p.sustain > 1 || p.attackSeconds > 86400 ||
        p.decaySeconds > 86400 || p.releaseSeconds > 86400)
        return Status::invalid_argument;
    parameters_ = p;
    if (stage_ != Stage::idle)
        enter(stage_);
    return Status::ok;
}
void Adsr::reset() noexcept {
    value_ = step_ = target_ = 0;
    remaining_ = 0;
    stage_ = Stage::idle;
}
void Adsr::enter(Stage stage) noexcept {
    stage_ = stage;
    if (stage == Stage::idle) {
        reset();
        return;
    }
    if (stage == Stage::sustain) {
        value_ = target_ = parameters_.sustain;
        remaining_ = 0;
        return;
    }
    const double seconds = stage == Stage::attack  ? parameters_.attackSeconds
                           : stage == Stage::decay ? parameters_.decaySeconds
                                                   : parameters_.releaseSeconds;
    target_ = stage == Stage::attack ? 1 : stage == Stage::decay ? parameters_.sustain : 0;
    remaining_ = static_cast<std::uint64_t>(std::ceil(seconds * sampleRate_));
    step_ = remaining_ ? (target_ - value_) / static_cast<double>(remaining_) : 0;
    if (!remaining_) {
        value_ = target_;
        enter(stage == Stage::attack  ? Stage::decay
              : stage == Stage::decay ? Stage::sustain
                                      : Stage::idle);
    }
}
void Adsr::noteOn() noexcept {
    if (sampleRate_ > 0)
        enter(Stage::attack);
}
void Adsr::noteOff() noexcept {
    if (stage_ != Stage::idle)
        enter(Stage::release);
}
double Adsr::next() noexcept {
    if (remaining_) {
        value_ += step_;
        if (!--remaining_) {
            value_ = target_;
            const auto following = stage_ == Stage::attack  ? Stage::decay
                                   : stage_ == Stage::decay ? Stage::sustain
                                                            : Stage::idle;
            // Preserve the endpoint of this frame before advancing the state.
            const double result = value_;
            enter(following);
            return result;
        }
    }
    return value_;
}

template <Sample T> Status Oscillator<T>::prepare(const ProcessSpec& spec) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    spec_ = spec;
    prepared_ = true;
    increment_ = std::min(440.0 / spec.sampleRate, 0.125);
    reset();
    return Status::ok;
}
template <Sample T> Status Oscillator<T>::setFrequency(double hz) noexcept {
    if (!prepared_)
        return Status::not_prepared;
    if (!isFinite(hz) || hz < 0 || hz > spec_.sampleRate * 0.5)
        return Status::invalid_argument;
    increment_ = hz / spec_.sampleRate;
    return Status::ok;
}
template <Sample T> void Oscillator<T>::reset(double phase) noexcept {
    phase_ = isFinite(phase) ? phase - std::floor(phase) : 0;
    random_ = 1;
}
template <Sample T> T Oscillator<T>::next() noexcept {
    if (!prepared_)
        return T{};
    double value = 0;
    switch (waveform_) {
    case Waveform::sine:
        value = std::sin(2 * std::numbers::pi * phase_);
        break;
    case Waveform::triangle:
        value = 1 - 4 * std::abs(phase_ - 0.5);
        break;
    case Waveform::saw:
        value = 2 * phase_ - 1 - detail::polyBlep(phase_, increment_);
        break;
    case Waveform::square: {
        double p = phase_ + 0.5;
        if (p >= 1)
            p -= 1;
        value = (phase_ < 0.5 ? 1.0 : -1.0) + detail::polyBlep(phase_, increment_) -
                detail::polyBlep(p, increment_);
        break;
    }
    case Waveform::whiteNoise:
        random_ ^= random_ >> 12;
        random_ ^= random_ << 25;
        random_ ^= random_ >> 27;
        value = static_cast<double>((random_ * 2685821657736338717ULL) >> 11) * 0x1.0p-52 - 1;
        break;
    }
    phase_ += increment_;
    if (phase_ >= 1)
        phase_ -= 1;
    return static_cast<T>(value);
}
template <Sample T> Status Oscillator<T>::process(AudioBlock<T> output, bool add) noexcept {
    if (!prepared_)
        return Status::not_prepared;
    if (!output.isValid())
        return Status::invalid_argument;
    if (output.channels() != spec_.channels)
        return Status::channel_mismatch;
    if (output.frames() > spec_.maxBlockFrames)
        return Status::block_too_large;
    // Validate output aliasing before phase advancement.
    auto status = validateProcessBlocks<T>(spec_, output, output);
    if (status != Status::ok)
        return status;
    for (std::size_t frame = 0; frame < output.frames(); ++frame) {
        const T value = next();
        for (std::size_t ch = 0; ch < output.channels(); ++ch) {
            if (add)
                output.channel(ch)[frame] += value;
            else
                output.channel(ch)[frame] = value;
        }
    }
    return Status::ok;
}
template class Oscillator<float>;
template class Oscillator<double>;
} // namespace cydsp
