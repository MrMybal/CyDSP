#include "cydsp/synthesis/Granulator.h"
#include "cydsp/math/VectorOps.h"
#include "cydsp/math/Window.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>
namespace cydsp {
template <Sample T> struct Granulator<T>::State {
    struct Grain {
        bool used = false;
        std::size_t wait = 0, age = 0, length = 0;
        double position = 0, step = 1;
        std::array<T, 2> gains{};
        GrainParameters parameters;
    };
    ProcessSpec spec;
    const SampleData<T>* source = nullptr;
    std::size_t sourceChannels = 0, sourceFrames = 0;
    double sourceRate = 0, countdown = 0;
    VectorOps ops;
    std::vector<Grain> grains;
    std::array<std::vector<T>, 3> windows;
    std::vector<T> envelope, scratch;
    GrainCloudParameters cloud;
    std::uint64_t random = 1, dropped = 0;
    bool valid(const GrainParameters& p) const noexcept {
        const double frames = p.durationMs * spec.sampleRate / 1000;
        return isFinite(p.positionSeconds) && std::abs(p.positionSeconds) <= 1e9 &&
               (p.wrap || (p.positionSeconds >= 0 &&
                           p.positionSeconds < double(sourceFrames) / sourceRate)) &&
               isFinite(p.durationMs) && p.durationMs <= 60000 && frames >= 2 &&
               isFinite(p.pitchRatio) && std::abs(p.pitchRatio) >= 1. / 64 &&
               std::abs(p.pitchRatio) <= 64 && isFinite(p.gain) && std::abs(p.gain) <= 16 &&
               isFinite(p.pan) && std::abs(p.pan) <= 1 &&
               unsigned(p.envelope) <= unsigned(GrainEnvelope::rectangular) &&
               unsigned(p.interpolation) <= unsigned(SampleInterpolation::bandLimited);
    }
    double uniform() noexcept {
        random ^= random >> 12;
        random ^= random << 25;
        random ^= random >> 27;
        return double((random * 2685821657736338717ULL) >> 11) * 0x1.0p-53 * 2 - 1;
    }
    Status launch(const GrainParameters& p, std::size_t wait) noexcept {
        auto it =
            std::find_if(grains.begin(), grains.end(), [](const Grain& g) { return !g.used; });
        if (it == grains.end())
            return Status::update_queue_full;
        Grain g;
        g.used = true;
        g.wait = wait;
        g.parameters = p;
        g.position = p.positionSeconds * sourceRate;
        g.step = p.pitchRatio * sourceRate / spec.sampleRate;
        g.length = std::size_t(std::llround(p.durationMs * spec.sampleRate / 1000));
        if (spec.channels == 1)
            g.gains[0] = T(p.gain);
        else {
            const double angle = (p.pan + 1) * std::numbers::pi / 4;
            const double scale = sourceChannels == 1 ? 1. : std::sqrt(2.);
            g.gains = {T(p.gain * scale * std::cos(angle)), T(p.gain * scale * std::sin(angle))};
        }
        *it = g;
        return Status::ok;
    }
    void render(AudioBlock<T> out) noexcept {
        for (auto& g : grains) {
            if (!g.used)
                continue;
            if (g.wait >= out.frames()) {
                g.wait -= out.frames();
                continue;
            }
            const auto start = g.wait;
            g.wait = 0;
            const auto count = std::min(out.frames() - start, g.length - g.age);
            const auto& window = windows[std::size_t(g.parameters.envelope)];
            for (std::size_t i = 0; i < count; ++i) {
                const double pos =
                    double(g.age + i) * double(window.size() - 1) / double(g.length - 1);
                const auto index = std::min(std::size_t(pos), window.size() - 2);
                envelope[i] =
                    T(double(window[index]) +
                      (double(window[index + 1]) - window[index]) * (pos - double(index)));
            }
            double endPosition = g.position;
            const auto first = source->loopEnd() ? source->loopStart() : 0;
            const auto end = source->loopEnd() ? source->loopEnd() : sourceFrames;
            const auto span = double(end - first);
            // Mono grains share their sampled/enveloped signal across both outputs.
            const auto reads = sourceChannels == 1 ? std::size_t{1} : spec.channels;
            for (std::size_t ch = 0; ch < reads; ++ch) {
                double position = g.position;
                for (std::size_t i = 0; i < count; ++i) {
                    if (g.parameters.wrap && (position < double(first) || position >= double(end)))
                        position = double(first) + (position - double(first)) -
                                   std::floor((position - double(first)) / span) * span;
                    // Rounding at very large wrapped offsets can land on the endpoint.
                    if (g.parameters.wrap)
                        position = std::clamp(position, double(first),
                                              std::nextafter(double(end), double(first)));
                    if (spec.channels == 1 && sourceChannels == 2)
                        scratch[i] = T((double(source->value(0, position, std::abs(g.step),
                                                             g.parameters.interpolation)) +
                                        double(source->value(1, position, std::abs(g.step),
                                                             g.parameters.interpolation))) *
                                       .5);
                    else
                        scratch[i] = source->value(sourceChannels == 1 ? 0 : ch, position,
                                                   std::abs(g.step), g.parameters.interpolation);
                    position += g.step;
                }
                auto values = std::span<T>(scratch).first(count);
                (void)ops.multiply(values, std::span<const T>(envelope).first(count), values);
                if (sourceChannels == 1) {
                    for (std::size_t output = 0; output < spec.channels; ++output)
                        (void)ops.addScaled(values, out.channel(output).subspan(start, count),
                                            g.gains[output]);
                } else
                    (void)ops.addScaled(values, out.channel(ch).subspan(start, count), g.gains[ch]);
                endPosition = position;
            }
            g.position = endPosition;
            g.age += count;
            if (g.age == g.length)
                g.used = false;
        }
    }
};
template <Sample T> Granulator<T>::Granulator() = default;
template <Sample T> Granulator<T>::~Granulator() = default;
template <Sample T> Granulator<T>::Granulator(Granulator&&) noexcept = default;
template <Sample T> Granulator<T>& Granulator<T>::operator=(Granulator&&) noexcept = default;
template <Sample T>
Status Granulator<T>::prepare(const ProcessSpec& spec, const SampleData<T>& source,
                              std::size_t maximumGrains, CpuBackend backend) noexcept {
    if (!spec.isValid() || spec.sampleRate < 8000 || spec.sampleRate > 768000 ||
        spec.channels > 2 || spec.maxBlockFrames > 65536 || maximumGrains == 0 ||
        maximumGrains > 4096 || source.samples().channels() == 0 ||
        source.samples().channels() > 2 || !source.samples().frames())
        return Status::invalid_spec;
    for (std::size_t ch = 0; ch < source.samples().channels(); ++ch)
        for (T v : source.samples().channel(ch))
            if (!isFinite(v))
                return Status::invalid_argument;
    try {
        auto next = std::make_unique<State>();
        next->spec = spec;
        next->source = &source;
        next->sourceChannels = source.samples().channels();
        next->sourceFrames = source.samples().frames();
        next->sourceRate = source.sampleRate();
        auto status = VectorOps::create(backend, next->ops);
        if (status != Status::ok)
            return status;
        next->grains.resize(maximumGrains);
        next->envelope.resize(spec.maxBlockFrames);
        next->scratch.resize(spec.maxBlockFrames);
        const std::array<WindowType, 3> types{WindowType::hann, WindowType::bartlett,
                                              WindowType::rectangular};
        for (std::size_t i = 0; i < 3; ++i) {
            next->windows[i].resize(2049);
            status = makeWindow(types[i], std::span<T>(next->windows[i]));
            if (status != Status::ok)
                return status;
        }
        state_.swap(next);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status Granulator<T>::trigger(const GrainParameters& p, std::size_t offset) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (!s.valid(p) || offset >= s.spec.maxBlockFrames)
        return Status::invalid_argument;
    return s.launch(p, offset);
}
template <Sample T> Status Granulator<T>::setCloud(const GrainCloudParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (!s.valid(p.grain) || !isFinite(p.grainsPerSecond) || p.grainsPerSecond < 0 ||
        p.grainsPerSecond > s.spec.sampleRate ||
        (p.grainsPerSecond > 0 && !isFinite(s.spec.sampleRate / p.grainsPerSecond)) ||
        !isFinite(p.positionSpreadSeconds) || p.positionSpreadSeconds < 0 ||
        p.positionSpreadSeconds > 1e6 || !isFinite(p.pitchSpreadSemitones) ||
        p.pitchSpreadSemitones < 0 || p.pitchSpreadSemitones > 48 || !isFinite(p.panSpread) ||
        p.panSpread < 0 || p.panSpread > 2)
        return Status::invalid_argument;
    if (s.cloud.grainsPerSecond != p.grainsPerSecond)
        s.countdown = 0;
    s.cloud = p;
    return Status::ok;
}
template <Sample T> void Granulator<T>::reset(std::uint64_t seed) noexcept {
    if (!state_)
        return;
    for (auto& g : state_->grains)
        g.used = false;
    state_->countdown = 0;
    state_->random = seed ? seed : 1;
    state_->dropped = 0;
}
template <Sample T> Status Granulator<T>::process(AudioBlock<T> out, bool add) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks<T>(s.spec, out, out);
    if (status != Status::ok || !out.frames())
        return status;
    if (s.source->samples().channels() != s.sourceChannels ||
        s.source->samples().frames() != s.sourceFrames || s.source->sampleRate() != s.sourceRate)
        return Status::not_prepared;
    status = validateDisjointBlocks(s.source->samples(), out);
    if (status != Status::ok)
        return status;
    if (!add)
        for (std::size_t ch = 0; ch < out.channels(); ++ch)
            s.ops.fill(out.channel(ch), T{});
    std::size_t at = 0;
    while (at < out.frames()) {
        if (s.cloud.grainsPerSecond > 0 && s.countdown <= 0) {
            auto p = s.cloud.grain;
            p.positionSeconds = std::clamp(
                p.positionSeconds + s.uniform() * s.cloud.positionSpreadSeconds, -1e9, 1e9);
            if (!p.wrap)
                p.positionSeconds =
                    std::clamp(p.positionSeconds, 0.,
                               std::nextafter(double(s.sourceFrames) / s.sourceRate, 0.));
            const double pitch =
                p.pitchRatio * std::exp2(s.uniform() * s.cloud.pitchSpreadSemitones / 12);
            p.pitchRatio = std::copysign(std::clamp(std::abs(pitch), 1. / 64, 64.), pitch);
            p.pan = std::clamp(p.pan + s.uniform() * s.cloud.panSpread, -1., 1.);
            if (s.launch(p, 0) != Status::ok)
                ++s.dropped;
            s.countdown += s.spec.sampleRate / s.cloud.grainsPerSecond;
        }
        const auto remaining = out.frames() - at;
        const auto count =
            s.cloud.grainsPerSecond > 0
                ? std::size_t(std::min(double(remaining), std::max(1., std::ceil(s.countdown))))
                : remaining;
        s.render(out.subBlock(at, count));
        at += count;
        if (s.cloud.grainsPerSecond > 0)
            s.countdown -= double(count);
    }
    return Status::ok;
}
template <Sample T> std::size_t Granulator<T>::activeGrains() const noexcept {
    return state_ ? std::size_t(std::count_if(state_->grains.begin(), state_->grains.end(),
                                              [](const auto& g) { return g.used; }))
                  : 0;
}
template <Sample T> std::uint64_t Granulator<T>::droppedGrains() const noexcept {
    return state_ ? state_->dropped : 0;
}
template <Sample T> CpuBackend Granulator<T>::backend() const noexcept {
    return state_ ? state_->ops.backend() : CpuBackend::scalar;
}
template class Granulator<float>;
template class Granulator<double>;
} // namespace cydsp
