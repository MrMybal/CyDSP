#include "cydsp/synthesis/SourceBatch.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/math/VectorOps.h"
#include "cydsp/spatial/SpatialRendering.h"
#include "detail/Analysis.h"
#include "detail/VectorTable.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>
namespace cydsp {
template <Sample T> struct SourceBatch<T>::State {
    struct Voice {
        BatchSource p;
        double phase = 0, position = 0;
        std::size_t age = 0, wait = 0;
        bool active = false;
    };
    ProcessSpec spec;
    std::vector<Voice> voices;
    std::vector<const Wavetable<T>*> tables;
    std::vector<const SampleData<T>*> samples;
    std::vector<T> sine, cosine, stepSin, stepCos, a, b, c, d;
    AudioBuffer<T> rendered;
    VectorOps ops;
    std::uint64_t renormalize = 0;
};
template <Sample T> SourceBatch<T>::SourceBatch() = default;
template <Sample T> SourceBatch<T>::~SourceBatch() = default;
template <Sample T> SourceBatch<T>::SourceBatch(SourceBatch&&) noexcept = default;
template <Sample T> SourceBatch<T>& SourceBatch<T>::operator=(SourceBatch&&) noexcept = default;
template <Sample T>
Status SourceBatch<T>::prepare(const ProcessSpec& spec, std::size_t maximum,
                               std::span<const Wavetable<T>* const> tables,
                               std::span<const SampleData<T>* const> samples,
                               CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || !maximum || maximum > 1024 ||
        maximum * spec.maxBlockFrames > (1u << 24) || tables.size() > 256 || samples.size() > 256)
        return Status::invalid_spec;
    for (auto* t : tables)
        if (!t || !t->size())
            return Status::invalid_argument;
    for (auto* t : samples)
        if (!t || t->sampleRate() <= 0)
            return Status::invalid_argument;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->voices.resize(maximum);
        s->tables.assign(tables.begin(), tables.end());
        s->samples.assign(samples.begin(), samples.end());
        for (auto* v : {&s->sine, &s->cosine, &s->stepSin, &s->stepCos, &s->a, &s->b, &s->c, &s->d})
            v->resize(maximum);
        auto status = VectorOps::create(backend, s->ops);
        if (status != Status::ok)
            return status;
        // The internal buffer may have more than the public 64 audio channels.
        status = s->rendered.resize(maximum, spec.maxBlockFrames);
        if (status != Status::ok)
            return status;
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status SourceBatch<T>::start(std::size_t slot, const BatchSource& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (slot >= s.voices.size() || unsigned(p.kind) > unsigned(BatchSourceKind::grain) ||
        !isFinite(p.frequencyHz) || p.frequencyHz < 0 || p.frequencyHz > s.spec.sampleRate * .49 ||
        !isFinite(p.phase) || std::abs(p.phase) > 1e9 || !isFinite(p.gain) ||
        std::abs(p.gain) > 16 || !isFinite(p.samplePosition) ||
        std::abs(p.samplePosition) > (1u << 30) || !isFinite(p.playbackRatio) ||
        std::abs(p.playbackRatio) < .125 || std::abs(p.playbackRatio) > 8 || !p.durationFrames ||
        p.durationFrames > (1u << 30) || p.startOffsetFrames > (1u << 30))
        return Status::invalid_argument;
    for (double g : p.channelGains)
        if (!isFinite(g) || std::abs(g) > 16)
            return Status::invalid_argument;
    if ((p.kind == BatchSourceKind::wavetable && p.asset >= s.tables.size()) ||
        (p.kind == BatchSourceKind::grain && p.asset >= s.samples.size()))
        return Status::invalid_argument;
    if (s.voices[slot].active)
        return Status::would_block;
    auto& v = s.voices[slot];
    v.p = p;
    v.phase = p.phase - std::floor(p.phase);
    v.position = p.samplePosition;
    v.age = 0;
    v.wait = p.startOffsetFrames;
    v.active = true;
    const double angle = 2 * std::numbers::pi * v.phase,
                 step = 2 * std::numbers::pi * p.frequencyHz / s.spec.sampleRate;
    s.sine[slot] = T(std::sin(angle));
    s.cosine[slot] = T(std::cos(angle));
    s.stepSin[slot] = T(std::sin(step));
    s.stepCos[slot] = T(std::cos(step));
    return Status::ok;
}
template <Sample T> Status SourceBatch<T>::stop(std::size_t slot) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (slot >= state_->voices.size())
        return Status::invalid_argument;
    state_->voices[slot].active = false;
    return Status::ok;
}
template <Sample T>
Status SourceBatch<T>::setPosition(std::size_t slot, Vec3 p, int order) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    const double d = length(p);
    if (slot >= s.voices.size() || !isFinite(d) || d < 1e-9 || d > 1e9 || order < -1 || order > 7)
        return Status::invalid_argument;
    std::array<double, 64> gains{};
    const double attenuation = 1 / std::max(1., d);
    if (order >= 0) {
        const auto n = std::size_t((order + 1) * (order + 1));
        if (s.spec.channels != n)
            return Status::channel_mismatch;
        auto st = ambisonicCoefficients(unsigned(order), p, {gains.data(), n});
        if (st != Status::ok)
            return st;
        for (auto& g : gains)
            g *= attenuation;
    } else {
        if (s.spec.channels != 2)
            return Status::channel_mismatch;
        gains[0] = attenuation * std::sqrt(.5 * (1 + p.y / d));
        gains[1] = attenuation * std::sqrt(.5 * (1 - p.y / d));
    }
    s.voices[slot].p.channelGains = gains;
    return Status::ok;
}
template <Sample T> Status SourceBatch<T>::process(AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto st = validateProcessBlocks<T>(s.spec, out, out);
    if (st != Status::ok)
        return st;
    const auto n = s.voices.size();
    const auto& k = detail::VectorAccess::kernels<T>(s.ops);
    for (std::size_t i = 0; i < out.frames(); ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            auto& v = s.voices[j];
            T x{};
            const bool running = v.active && !v.wait;
            if (v.active) {
                if (v.wait)
                    --v.wait;
                else {
                    if (v.p.kind == BatchSourceKind::sine)
                        x = s.sine[j];
                    else if (v.p.kind == BatchSourceKind::wavetable)
                        x = s.tables[v.p.asset]->value(v.phase, v.p.frequencyHz, s.spec.sampleRate);
                    else {
                        const auto* sample = s.samples[v.p.asset];
                        const double ratio =
                            v.p.playbackRatio * sample->sampleRate() / s.spec.sampleRate;
                        x = sample->value(0, v.position, std::abs(ratio));
                        x *= T(.5 - .5 * std::cos(2 * std::numbers::pi * double(v.age) /
                                                  double(v.p.durationFrames)));
                        v.position += ratio;
                    }
                    if (v.p.kind == BatchSourceKind::wavetable) {
                        v.phase += v.p.frequencyHz / s.spec.sampleRate;
                        v.phase -= std::floor(v.phase);
                    }
                    if (++v.age == v.p.durationFrames && v.p.kind == BatchSourceKind::grain)
                        v.active = false;
                }
            }
            s.rendered.block().channel(j)[i] = x * T(v.p.gain);
            // Delayed starts freeze oscillator state; inactive lanes become identity rotations.
            if (!running) {
                s.a[j] = T{1};
                s.b[j] = T{};
            } else {
                s.a[j] = s.stepCos[j];
                s.b[j] = s.stepSin[j];
            }
        }
        // Simultaneous complex rotation through the selected SIMD vector table.
        k.multiply(s.sine.data(), s.a.data(), s.c.data(), n);
        k.multiply(s.cosine.data(), s.b.data(), s.d.data(), n);
        k.addScaled(s.d.data(), s.c.data(), n, T{1});
        k.multiply(s.cosine.data(), s.a.data(), s.d.data(), n);
        k.multiply(s.sine.data(), s.b.data(), s.cosine.data(), n);
        k.addScaled(s.cosine.data(), s.d.data(), n, T{-1});
        k.copy(s.c.data(), s.sine.data(), n);
        k.copy(s.d.data(), s.cosine.data(), n);
        if (++s.renormalize % 1024 == 0)
            for (std::size_t j = 0; j < n; ++j) {
                const double mag = std::hypot(double(s.sine[j]), double(s.cosine[j]));
                if (mag > 0) {
                    s.sine[j] = T(s.sine[j] / mag);
                    s.cosine[j] = T(s.cosine[j] / mag);
                }
            }
    }
    for (std::size_t ch = 0; ch < out.channels(); ++ch) {
        s.ops.fill(out.channel(ch), T{});
        for (std::size_t j = 0; j < n; ++j)
            if (s.voices[j].p.channelGains[ch] != 0)
                k.addScaled(s.rendered.block().channel(j).data(), out.channel(ch).data(),
                            out.frames(), T(s.voices[j].p.channelGains[ch]));
    }
    return Status::ok;
}
template <Sample T> void SourceBatch<T>::reset() noexcept {
    if (state_) {
        for (auto& v : state_->voices)
            v.active = false;
        state_->renormalize = 0;
    }
}
template <Sample T> std::size_t SourceBatch<T>::activeVoices() const noexcept {
    if (!state_)
        return 0;
    std::size_t n = 0;
    for (auto& v : state_->voices)
        n += v.active;
    return n;
}
template <Sample T> CpuBackend SourceBatch<T>::backend() const noexcept {
    return state_ ? state_->ops.backend() : CpuBackend::scalar;
}
template class SourceBatch<float>;
template class SourceBatch<double>;
} // namespace cydsp
