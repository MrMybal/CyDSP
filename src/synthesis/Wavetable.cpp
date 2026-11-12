#include "cydsp/synthesis/Wavetable.h"
#include "cydsp/processors/ComplexFft.h"
#include <algorithm>
#include <cmath>
namespace cydsp {
template <Sample T> struct Wavetable<T>::State {
    std::size_t size = 0, levels = 0;
    std::vector<T> data;
};
template <Sample T> Wavetable<T>::Wavetable() = default;
template <Sample T> Wavetable<T>::~Wavetable() = default;
template <Sample T> Wavetable<T>::Wavetable(Wavetable&&) noexcept = default;
template <Sample T> Wavetable<T>& Wavetable<T>::operator=(Wavetable&&) noexcept = default;
template <Sample T>
Status Wavetable<T>::prepare(std::span<const T> cycle, CpuBackend backend) noexcept {
    if (cycle.size() < 16 || cycle.size() > 65536 || !isPowerOfTwo(cycle.size()) ||
        !std::all_of(cycle.begin(), cycle.end(), [](T v) { return isFinite(v); }))
        return Status::invalid_argument;
    try {
        auto next = std::make_unique<State>();
        next->size = cycle.size();
        for (auto n = cycle.size() / 2; n; n /= 2)
            ++next->levels;
        next->data.resize(next->levels * (cycle.size() + 1));
        ComplexFft<T> fft;
        auto result = fft.prepare(cycle.size(), backend);
        if (result != Status::ok)
            return result;
        std::vector<std::complex<T>> input(cycle.size()), spectrum(cycle.size()),
            filtered(cycle.size()), out(cycle.size());
        for (std::size_t i = 0; i < cycle.size(); ++i)
            input[i] = {cycle[i], 0};
        result = fft.process(input, spectrum, FftDirection::forward);
        if (result != Status::ok)
            return result;
        auto harmonics = cycle.size() / 2;
        for (std::size_t level = 0; level < next->levels; ++level, harmonics /= 2) {
            filtered = spectrum;
            for (std::size_t i = harmonics + 1; i < cycle.size() - harmonics; ++i)
                filtered[i] = {};
            result = fft.process(filtered, out, FftDirection::inverse);
            if (result != Status::ok)
                return result;
            const auto offset = level * (cycle.size() + 1);
            for (std::size_t i = 0; i < cycle.size(); ++i)
                next->data[offset + i] = out[i].real();
            next->data[offset + cycle.size()] = next->data[offset];
        }
        state_.swap(next);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> T Wavetable<T>::value(double phase, double hz, double rate) const noexcept {
    if (!state_ || !isFinite(phase) || !isFinite(hz) || !isFinite(rate) || rate <= 0 || hz < 0 ||
        hz > rate * .5)
        return T{};
    const auto& s = *state_;
    phase -= std::floor(phase);
    const auto position = phase * double(s.size), fraction = position - std::floor(position);
    const auto index = std::size_t(position);
    auto read = [&](std::size_t level) {
        const auto offset = level * (s.size + 1) + index;
        return double(s.data[offset]) + (double(s.data[offset + 1]) - s.data[offset]) * fraction;
    };
    // Conservative crossfade: both selected banks fit below .45 * sampleRate.
    const auto harmonicRatio = hz * double(s.size / 2) / (rate * .45);
    if (harmonicRatio <= .5)
        return static_cast<T>(read(0));
    const auto level = std::max(0.0, std::log2(harmonicRatio) + 1);
    const auto first = std::min(std::size_t(level), s.levels - 1),
               second = std::min(first + 1, s.levels - 1);
    const auto mix = std::min(1.0, level - double(first));
    return static_cast<T>(read(first) * (1 - mix) + read(second) * mix);
}
template <Sample T> std::size_t Wavetable<T>::size() const noexcept {
    return state_ ? state_->size : 0;
}
template <Sample T> std::size_t Wavetable<T>::levels() const noexcept {
    return state_ ? state_->levels : 0;
}
template <Sample T>
Status WavetableOscillator<T>::prepare(const ProcessSpec& s, const Wavetable<T>& t) noexcept {
    if (!s.isValid() || !t.size())
        return Status::invalid_spec;
    spec_ = s;
    table_ = &t;
    frequency_ = std::min(440., s.sampleRate * .25);
    phase_ = 0;
    return Status::ok;
}
template <Sample T> Status WavetableOscillator<T>::setFrequency(double hz) noexcept {
    if (!table_)
        return Status::not_prepared;
    if (!isFinite(hz) || hz < 0 || hz > spec_.sampleRate * .5)
        return Status::invalid_argument;
    frequency_ = hz;
    return Status::ok;
}
template <Sample T> void WavetableOscillator<T>::reset(double p) noexcept {
    phase_ = isFinite(p) ? p - std::floor(p) : 0;
}
template <Sample T> T WavetableOscillator<T>::next() noexcept {
    if (!table_)
        return T{};
    const auto v = table_->value(phase_, frequency_, spec_.sampleRate);
    phase_ += frequency_ / spec_.sampleRate;
    phase_ -= std::floor(phase_);
    return v;
}
template <Sample T> Status WavetableOscillator<T>::process(AudioBlock<T> out, bool add) noexcept {
    if (!table_)
        return Status::not_prepared;
    auto r = validateProcessBlocks<T>(spec_, out, out);
    if (r != Status::ok)
        return r;
    for (std::size_t i = 0; i < out.frames(); ++i) {
        const auto v = next();
        for (std::size_t ch = 0; ch < out.channels(); ++ch) {
            if (add)
                out.channel(ch)[i] += v;
            else
                out.channel(ch)[i] = v;
        }
    }
    return Status::ok;
}
template class Wavetable<float>;
template class Wavetable<double>;
template class WavetableOscillator<float>;
template class WavetableOscillator<double>;
} // namespace cydsp
