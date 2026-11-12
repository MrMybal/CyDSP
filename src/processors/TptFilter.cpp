#include "cydsp/processors/TptFilter.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/processors/BiquadCascade.h"
#include "detail/BiquadAlgorithm.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <stdexcept>
#include <vector>
namespace cydsp {
namespace {
bool frequencyValid(double frequency, double rate) noexcept {
    return isFinite(frequency) && frequency > 0 && frequency < rate * 0.5;
}
struct Integrators {
    double a = 0, b = 0;
};
} // namespace
template <Sample T> struct TptFilter<T>::State {
    ProcessSpec spec;
    std::vector<Integrators> memory;
    double g = 0, k = 1, a = 1;
    TptFilterMode mode = TptFilterMode::lowPass;
};
#define CYDSP_LIFETIME(Class)                                                                      \
    template <Sample T> Class<T>::Class() = default;                                               \
    template <Sample T> Class<T>::~Class() = default;                                              \
    template <Sample T> Class<T>::Class(Class&&) noexcept = default;                               \
    template <Sample T> Class<T>& Class<T>::operator=(Class&&) noexcept = default;
CYDSP_LIFETIME(TptFilter)
template <Sample T> Status TptFilter<T>::prepare(const ProcessSpec& spec) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    try {
        auto next = std::make_unique<State>();
        next->spec = spec;
        next->memory.resize(spec.channels);
        next->g = std::tan(std::numbers::pi * 0.1);
        next->k = std::sqrt(2.0);
        next->a = 1 / (1 + next->g * (next->g + next->k));
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
}
template <Sample T>
Status TptFilter<T>::setParameters(double frequency, double q, TptFilterMode mode) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!frequencyValid(frequency, state_->spec.sampleRate) || !isFinite(q) || q <= 0 || q > 1000)
        return Status::invalid_argument;
    const double g = std::tan(std::numbers::pi * frequency / state_->spec.sampleRate), k = 1 / q,
                 a = 1 / (1 + g * (g + k));
    if (!isFinite(g) || !isFinite(a))
        return Status::invalid_argument;
    state_->g = g;
    state_->k = k;
    state_->a = a;
    state_->mode = mode;
    return Status::ok;
}
template <Sample T> void TptFilter<T>::reset() noexcept {
    if (state_)
        std::fill(state_->memory.begin(), state_->memory.end(), Integrators{});
}
template <Sample T>
Status TptFilter<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    const auto status = validateProcessBlocks(s.spec, in, out);
    if (status != Status::ok)
        return status;
    for (std::size_t ch = 0; ch < in.channels(); ++ch) {
        auto& z = s.memory[ch];
        for (std::size_t i = 0; i < in.frames(); ++i) {
            const double x = in.channel(ch)[i];
            double y = 0;
            if (s.mode == TptFilterMode::firstOrderLowPass ||
                s.mode == TptFilterMode::firstOrderHighPass ||
                s.mode == TptFilterMode::firstOrderAllPass) {
                const double v = (x - z.a) * s.g / (1 + s.g), low = v + z.a;
                z.a = low + v;
                y = s.mode == TptFilterMode::firstOrderLowPass    ? low
                    : s.mode == TptFilterMode::firstOrderHighPass ? x - low
                                                                  : 2 * low - x;
            } else {
                const double v3 = x - z.b, v1 = s.a * (z.a + s.g * v3), v2 = z.b + s.g * v1;
                z.a = 2 * v1 - z.a;
                z.b = 2 * v2 - z.b;
                const double high = x - s.k * v1 - v2;
                switch (s.mode) {
                case TptFilterMode::lowPass:
                    y = v2;
                    break;
                case TptFilterMode::bandPass:
                    y = v1;
                    break;
                case TptFilterMode::highPass:
                    y = high;
                    break;
                case TptFilterMode::notch:
                    y = x - s.k * v1;
                    break;
                case TptFilterMode::allPass:
                    y = x - 2 * s.k * v1;
                    break;
                default:
                    y = 0;
                    break;
                }
            }
            out.channel(ch)[i] = static_cast<T>(y);
        }
    }
    return Status::ok;
}
// Two cascaded second-order Butterworth sections per branch (LR4).
template <Sample T> struct LinkwitzRiley<T>::State {
    ProcessSpec spec;
    BiquadCascade<T> low, high;
    AudioBuffer<T> input;
};
CYDSP_LIFETIME(LinkwitzRiley)
template <Sample T> Status LinkwitzRiley<T>::prepare(const ProcessSpec& spec) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    try {
        auto next = std::make_unique<State>();
        next->spec = spec;
        auto status = next->input.resize(spec.channels, spec.maxBlockFrames);
        if (status != Status::ok)
            return status;
        std::array<BiquadCoefficients, 2> low{}, high{};
        status = designBiquad(BiquadType::lowPass, spec.sampleRate, spec.sampleRate * 0.1,
                              std::sqrt(0.5), low[0]);
        if (status != Status::ok)
            return status;
        low[1] = low[0];
        status = designBiquad(BiquadType::highPass, spec.sampleRate, spec.sampleRate * 0.1,
                              std::sqrt(0.5), high[0]);
        if (status != Status::ok)
            return status;
        high[1] = high[0];
        status = next->low.prepare(spec, low);
        if (status != Status::ok)
            return status;
        status = next->high.prepare(spec, high);
        if (status != Status::ok)
            return status;
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status LinkwitzRiley<T>::setFrequency(double frequency) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (!frequencyValid(frequency, s.spec.sampleRate))
        return Status::invalid_argument;
    std::array<BiquadCoefficients, 2> lo{}, hi{};
    auto status =
        designBiquad(BiquadType::lowPass, s.spec.sampleRate, frequency, std::sqrt(0.5), lo[0]);
    if (status != Status::ok)
        return status;
    status =
        designBiquad(BiquadType::highPass, s.spec.sampleRate, frequency, std::sqrt(0.5), hi[0]);
    if (status != Status::ok)
        return status;
    // Preflight both branches at sample precision before committing any section.
    BiquadCoefficients roundedLow, roundedHigh;
    if (!detail::roundBiquad<T>(lo[0], roundedLow) || !detail::roundBiquad<T>(hi[0], roundedHigh))
        return Status::invalid_argument;
    lo[0] = lo[1] = roundedLow;
    hi[0] = hi[1] = roundedHigh;
    for (std::size_t i = 0; i < 2; ++i) {
        (void)s.low.setSection(i, lo[i]);
        (void)s.high.setSection(i, hi[i]);
    }
    return Status::ok;
}
template <Sample T> void LinkwitzRiley<T>::reset() noexcept {
    if (state_) {
        state_->low.reset();
        state_->high.reset();
    }
}
template <Sample T>
Status LinkwitzRiley<T>::process(AudioBlock<const T> in, AudioBlock<T> low,
                                 AudioBlock<T> high) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, in, low);
    if (status != Status::ok)
        return status;
    status = validateProcessBlocks(s.spec, in, high);
    if (status != Status::ok)
        return status;
    const auto bytes = in.frames() * sizeof(T);
    for (std::size_t a = 0; a < low.channels(); ++a)
        for (std::size_t b = 0; b < high.channels(); ++b) {
            if (!bytes)
                continue;
            const auto x = reinterpret_cast<std::uintptr_t>(low.channel(a).data()),
                       y = reinterpret_cast<std::uintptr_t>(high.channel(b).data());
            if ((x <= y && y - x < bytes) || (y < x && x - y < bytes))
                return Status::partial_overlap;
        }
    auto captured = s.input.block(in.frames());
    for (std::size_t ch = 0; ch < in.channels(); ++ch)
        std::copy(in.channel(ch).begin(), in.channel(ch).end(), captured.channel(ch).begin());
    status = s.low.process(captured, low);
    if (status != Status::ok)
        return status;
    return s.high.process(captured, high);
}
template <Sample T> struct LadderFilter<T>::State {
    ProcessSpec spec;
    std::vector<std::array<double, 4>> poles;
    double g = 0, resonance = 0, drive = 1;
};
CYDSP_LIFETIME(LadderFilter)
template <Sample T> Status LadderFilter<T>::prepare(const ProcessSpec& spec) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    try {
        auto next = std::make_unique<State>();
        next->spec = spec;
        next->poles.resize(spec.channels);
        const double g = std::tan(std::numbers::pi * 0.1);
        next->g = g / (1 + g);
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
}
template <Sample T>
Status LadderFilter<T>::setParameters(double frequency, double resonance, double drive) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!frequencyValid(frequency, state_->spec.sampleRate) || !isFinite(resonance) ||
        resonance < 0 || resonance > 1 || !isFinite(drive) || drive <= 0 || drive > 64)
        return Status::invalid_argument;
    const double g = std::tan(std::numbers::pi * frequency / state_->spec.sampleRate);
    if (!isFinite(g))
        return Status::invalid_argument;
    state_->g = g / (1 + g);
    state_->resonance = 3.8 * resonance;
    state_->drive = drive;
    return Status::ok;
}
template <Sample T> void LadderFilter<T>::reset() noexcept {
    if (state_)
        std::fill(state_->poles.begin(), state_->poles.end(), std::array<double, 4>{});
}
template <Sample T>
Status LadderFilter<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, in, out);
    if (status != Status::ok)
        return status;
    const double G = s.g * s.g * s.g * s.g;
    for (std::size_t ch = 0; ch < in.channels(); ++ch) {
        auto& z = s.poles[ch];
        for (std::size_t i = 0; i < in.frames(); ++i) {
            const double offset =
                (1 - s.g) * (s.g * s.g * s.g * z[0] + s.g * s.g * z[1] + s.g * z[2] + z[3]);
            const double x = std::tanh(static_cast<double>(in.channel(ch)[i]) * s.drive);
            const double u = (x - s.resonance * offset) / (1 + s.resonance * G);
            double y = u;
            for (auto& memory : z) {
                const double v = (y - memory) * s.g;
                y = v + memory;
                memory = y + v;
            }
            out.channel(ch)[i] = static_cast<T>(y / s.drive);
        }
    }
    return Status::ok;
}
#undef CYDSP_LIFETIME
Status designButterworth(BiquadType type, unsigned order, double rate, double cutoff,
                         std::span<BiquadCoefficients> output, std::size_t& count) noexcept {
    if ((type != BiquadType::lowPass && type != BiquadType::highPass) || order < 1 || order > 32 ||
        !isFinite(rate) || rate <= 0 || !frequencyValid(cutoff, rate) ||
        output.size() < (order + 1) / 2)
        return Status::invalid_argument;
    std::array<BiquadCoefficients, 16> next{};
    std::size_t n = 0;
    if (order % 2) {
        const double g = std::tan(std::numbers::pi * cutoff / rate), a = 1 / (1 + g);
        auto& c = next[n++];
        c.b0 = type == BiquadType::lowPass ? g * a : a;
        c.b1 = type == BiquadType::lowPass ? c.b0 : -c.b0;
        c.b2 = 0;
        c.a1 = (g - 1) * a;
        c.a2 = 0;
    }
    for (unsigned i = 0; i < order / 2; ++i) {
        const double angle = std::numbers::pi * (2 * static_cast<double>(i) + 1) / (2 * order);
        const double q = 1 / (2 * std::sin(angle));
        auto status = designBiquad(type, rate, cutoff, q, next[n++]);
        if (status != Status::ok)
            return status;
    }
    for (std::size_t i = 0; i < n; ++i)
        if (!next[i].isStable())
            return Status::invalid_argument;
    std::copy_n(next.begin(), n, output.begin());
    count = n;
    return Status::ok;
}
namespace {
double besselI0(double x) noexcept {
    double sum = 1, term = 1;
    for (unsigned n = 1; n < 100; ++n) {
        term *= x * x / (4 * static_cast<double>(n) * n);
        sum += term;
        if (term < sum * 1e-16)
            break;
    }
    return sum;
}
} // namespace
Status designWindowedFir(std::span<double> taps, double cutoff, bool highPass,
                         double attenuation) noexcept {
    if (taps.empty() || (highPass && taps.size() == 1) || taps.size() > 1048575 ||
        taps.size() % 2 == 0 || !isFinite(cutoff) || cutoff <= 0 || cutoff >= 0.5 ||
        !isFinite(attenuation) || attenuation < 0 || attenuation > 200)
        return Status::invalid_argument;
    const double beta = attenuation > 50    ? 0.1102 * (attenuation - 8.7)
                        : attenuation >= 21 ? 0.5842 * std::pow(attenuation - 21, 0.4) +
                                                  0.07886 * (attenuation - 21)
                                            : 0;
    const double denom = besselI0(beta), middle = static_cast<double>(taps.size() - 1) * 0.5;
    auto coefficient = [&](std::size_t i) {
        const double x = static_cast<double>(i) - middle, r = middle ? x / middle : 0;
        const double h = x == 0
                             ? 2 * cutoff
                             : std::sin(2 * std::numbers::pi * cutoff * x) / (std::numbers::pi * x);
        return h * besselI0(beta * std::sqrt(std::max(0.0, 1 - r * r))) / denom;
    };
    double sum = 0;
    for (std::size_t i = 0; i < taps.size(); ++i)
        sum += coefficient(i);
    if (!isFinite(sum) || sum == 0)
        return Status::invalid_argument;
    for (std::size_t i = 0; i < taps.size(); ++i)
        taps[i] = coefficient(i) / sum;
    if (highPass) {
        for (auto& value : taps)
            value = -value;
        taps[taps.size() / 2] += 1;
    }
    return Status::ok;
}
template class TptFilter<float>;
template class TptFilter<double>;
template class LinkwitzRiley<float>;
template class LinkwitzRiley<double>;
template class LadderFilter<float>;
template class LadderFilter<double>;
} // namespace cydsp
