#include "cydsp/processors/Mixing.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <vector>
namespace cydsp {
namespace {
std::array<double, 2> mixGains(double wet, MixLaw law) noexcept {
    const double angle = wet * std::numbers::pi / 2;
    switch (law) {
    case MixLaw::linear:
        return {1 - wet, wet};
    case MixLaw::sine3dB:
        return {std::cos(angle), std::sin(angle)};
    case MixLaw::sine4_5dB:
        return {std::pow(std::cos(angle), 1.5), std::pow(std::sin(angle), 1.5)};
    case MixLaw::sine6dB:
        return {std::pow(std::cos(angle), 2), std::pow(std::sin(angle), 2)};
    case MixLaw::squareRoot3dB:
        return {std::sqrt(1 - wet), std::sqrt(wet)};
    case MixLaw::squareRoot4_5dB:
        return {std::pow(1 - wet, 0.75), std::pow(wet, 0.75)};
    }
    return {0, 0};
}
} // namespace
template <Sample T>
Status Panner<T>::prepare(const ProcessSpec& spec, CpuBackend backend) noexcept {
    if (spec.channels != 1 && spec.channels != 2)
        return Status::channel_mismatch;
    const double gain = std::sqrt(0.5);
    const std::array<double, 4> coefficients = spec.channels == 1
                                                   ? std::array<double, 4>{gain, gain, 0, 0}
                                                   : std::array<double, 4>{gain, 0, 0, gain};
    return matrix_.prepare(spec, 2, std::span<const double>(coefficients.data(), spec.channels * 2),
                           backend);
}
template <Sample T> Status Panner<T>::setPan(double pan, PanLaw law, std::size_t ramp) noexcept {
    if (!isFinite(pan) || pan < -1 || pan > 1)
        return Status::invalid_argument;
    if (!matrix_.inputChannels())
        return Status::not_prepared;
    const double fraction = (pan + 1) * 0.5;
    std::array<double, 2> gain{};
    if (law == PanLaw::balanced)
        gain = {std::min(1.0, 1 - pan), std::min(1.0, 1 + pan)};
    else
        gain = mixGains(fraction, static_cast<MixLaw>(static_cast<int>(law) - 1));
    const std::array<double, 4> coefficients = matrix_.inputChannels() == 1
                                                   ? std::array<double, 4>{gain[0], gain[1], 0, 0}
                                                   : std::array<double, 4>{gain[0], 0, 0, gain[1]};
    return matrix_.setMatrix(
        std::span<const double>(coefficients.data(), matrix_.inputChannels() * 2), ramp);
}
template <Sample T> Status Panner<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    return matrix_.process(in, out);
}
template <Sample T> struct DryWetMixer<T>::State {
    ProcessSpec spec;
    AudioBuffer<T> dry;
    std::vector<T> history;
    std::size_t ring = 0, position = 0, latency = 0, pending = 0;
    bool hasDry = false;
    std::array<double, 2> gain{0.5, 0.5};
};
template <Sample T> DryWetMixer<T>::DryWetMixer() = default;
template <Sample T> DryWetMixer<T>::~DryWetMixer() = default;
template <Sample T> DryWetMixer<T>::DryWetMixer(DryWetMixer&&) noexcept = default;
template <Sample T> DryWetMixer<T>& DryWetMixer<T>::operator=(DryWetMixer&&) noexcept = default;
template <Sample T>
Status DryWetMixer<T>::prepare(const ProcessSpec& spec, std::size_t maximum) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    if (maximum == std::numeric_limits<std::size_t>::max() ||
        (maximum + 1) > std::numeric_limits<std::size_t>::max() / spec.channels)
        return Status::invalid_argument;
    try {
        auto next = std::make_unique<State>();
        next->spec = spec;
        next->ring = maximum + 1;
        auto status = next->dry.resize(spec.channels, spec.maxBlockFrames);
        if (status != Status::ok)
            return status;
        next->history.resize(next->ring * spec.channels);
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
}
template <Sample T> Status DryWetMixer<T>::setMix(double wet, MixLaw law) noexcept {
    if (!isFinite(wet) || wet < 0 || wet > 1)
        return Status::invalid_argument;
    if (!state_)
        return Status::not_prepared;
    state_->gain = mixGains(wet, law);
    return Status::ok;
}
template <Sample T> Status DryWetMixer<T>::setWetLatency(std::size_t latency) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (latency >= state_->ring)
        return Status::invalid_argument;
    state_->latency = latency;
    return Status::ok;
}
template <Sample T> void DryWetMixer<T>::reset() noexcept {
    if (state_) {
        std::fill(state_->history.begin(), state_->history.end(), T{});
        state_->position = state_->pending = 0;
        state_->hasDry = false;
    }
}
template <Sample T> Status DryWetMixer<T>::pushDrySamples(AudioBlock<const T> in) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (s.hasDry)
        return Status::transition_in_progress;
    if (in.frames() > s.spec.maxBlockFrames)
        return Status::block_too_large;
    auto block = s.dry.block(in.frames());
    auto status = validateProcessBlocks(s.spec, in, block);
    if (status != Status::ok)
        return status;
    for (std::size_t i = 0; i < in.frames(); ++i) {
        const auto read = (s.position + s.ring - s.latency) % s.ring;
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            auto* h = s.history.data() + ch * s.ring;
            h[s.position] = in.channel(ch)[i];
            block.channel(ch)[i] = h[read];
        }
        s.position = (s.position + 1) % s.ring;
    }
    s.pending = in.frames();
    s.hasDry = true;
    return Status::ok;
}
template <Sample T> Status DryWetMixer<T>::mixWetSamples(AudioBlock<T> wet) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (!s.hasDry)
        return Status::not_prepared;
    if (wet.frames() != s.pending)
        return Status::invalid_argument;
    auto status = validateProcessBlocks<T>(s.spec, wet, wet);
    if (status != Status::ok)
        return status;
    auto dry = s.dry.block(s.pending);
    for (std::size_t ch = 0; ch < wet.channels(); ++ch)
        for (std::size_t i = 0; i < wet.frames(); ++i)
            wet.channel(ch)[i] =
                static_cast<T>(s.gain[0] * dry.channel(ch)[i] + s.gain[1] * wet.channel(ch)[i]);
    s.hasDry = false;
    return Status::ok;
}
template <Sample T> Status WaveShaper<T>::prepare(const ProcessSpec& spec) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    spec_ = spec;
    prepared_ = true;
    return Status::ok;
}
template <Sample T> Status WaveShaper<T>::setDrive(double drive) noexcept {
    if (!isFinite(drive) || drive < 0 || drive > 1024)
        return Status::invalid_argument;
    drive_ = drive;
    return Status::ok;
}
template <Sample T>
Status WaveShaper<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!prepared_)
        return Status::not_prepared;
    auto status = validateProcessBlocks(spec_, in, out);
    if (status != Status::ok)
        return status;
    for (std::size_t ch = 0; ch < in.channels(); ++ch)
        for (std::size_t i = 0; i < in.frames(); ++i) {
            const double x = static_cast<double>(in.channel(ch)[i]) * drive_;
            double y = 0;
            if (function_)
                y = function_(static_cast<T>(x), context_);
            else
                switch (shape_) {
                case Shaper::tanh:
                    y = std::tanh(x);
                    break;
                case Shaper::atan:
                    y = 2 / std::numbers::pi * std::atan(x);
                    break;
                case Shaper::hardClip:
                    y = std::clamp(x, -1.0, 1.0);
                    break;
                case Shaper::cubic: {
                    const double v = std::clamp(x, -1.0, 1.0);
                    y = 1.5 * v - 0.5 * v * v * v;
                    break;
                }
                case Shaper::fullWaveRectifier:
                    y = std::abs(x);
                    break;
                }
            out.channel(ch)[i] = static_cast<T>(y);
        }
    return Status::ok;
}
template class Panner<float>;
template class Panner<double>;
template class DryWetMixer<float>;
template class DryWetMixer<double>;
template class WaveShaper<float>;
template class WaveShaper<double>;
} // namespace cydsp
