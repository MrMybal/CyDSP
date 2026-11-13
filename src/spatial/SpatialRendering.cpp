#include "cydsp/spatial/SpatialRendering.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/core/CircularHistory.h"
#include "cydsp/math/VectorOps.h"
#include "cydsp/processors/MatrixConvolver.h"
#include "detail/Analysis.h"
#include "detail/VectorTable.h"
#include "detail/WindowedSinc.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>
namespace cydsp {
namespace {
bool unit(Vec3 v, Vec3& out) noexcept {
    const double n = length(v);
    if (!isFinite(n) || n < 1e-10 || n > 1e12)
        return false;
    out = v * (1 / n);
    return true;
}
template <Sample T>
Status transformBlocks(const ProcessSpec& spec, AudioBlock<const T> in, AudioBlock<T> out,
                       std::size_t inputs, std::size_t outputs) noexcept {
    auto s = spec;
    s.channels = inputs;
    auto status = detail::validateAnalysis(s, in);
    if (status != Status::ok)
        return status;
    if (!out.isValid() || out.frames() != in.frames())
        return Status::invalid_argument;
    if (out.channels() != outputs)
        return Status::channel_mismatch;
    return validateDisjointBlocks(in, out);
}
Status makeDecoder(unsigned order, std::span<const Vec3> speakers, double regularization,
                   std::vector<double>& decoder) {
    const auto n = std::size_t((order + 1) * (order + 1));
    Status status;
    std::vector<double> basis(n * speakers.size());
    for (std::size_t i = 0; i < speakers.size(); ++i) {
        status = ambisonicCoefficients(order, speakers[i], {basis.data() + i * n, n});
        if (status != Status::ok)
            return status;
    }
    // Regularized least-squares decoder D=Y^T*(Y*Y^T+lambda*I)^-1.
    std::vector<double> inverse(2 * n * n);
    for (std::size_t r = 0; r < n; ++r) {
        for (std::size_t c = 0; c < n; ++c) {
            double v = 0;
            for (std::size_t i = 0; i < speakers.size(); ++i)
                v += basis[i * n + r] * basis[i * n + c];
            inverse[r * 2 * n + c] = v + (r == c ? regularization : 0);
        }
        inverse[r * 2 * n + n + r] = 1;
    }
    for (std::size_t p = 0; p < n; ++p) {
        std::size_t pivot = p;
        for (std::size_t r = p + 1; r < n; ++r)
            if (std::abs(inverse[r * 2 * n + p]) > std::abs(inverse[pivot * 2 * n + p]))
                pivot = r;
        if (std::abs(inverse[pivot * 2 * n + p]) < 1e-12)
            return Status::unsupported_format;
        for (std::size_t c = 0; c < 2 * n; ++c)
            std::swap(inverse[p * 2 * n + c], inverse[pivot * 2 * n + c]);
        const double div = inverse[p * 2 * n + p];
        for (std::size_t c = 0; c < 2 * n; ++c)
            inverse[p * 2 * n + c] /= div;
        for (std::size_t r = 0; r < n; ++r)
            if (r != p) {
                const double f = inverse[r * 2 * n + p];
                for (std::size_t c = 0; c < 2 * n; ++c)
                    inverse[r * 2 * n + c] -= f * inverse[p * 2 * n + c];
            }
    }
    decoder.assign(n * speakers.size(), 0);
    for (std::size_t i = 0; i < speakers.size(); ++i)
        for (std::size_t c = 0; c < n; ++c)
            for (std::size_t k = 0; k < n; ++k)
                decoder[i * n + c] += basis[i * n + k] * inverse[k * 2 * n + n + c];
    return Status::ok;
}
} // namespace
template <Sample T> struct AnalyticBinaural<T>::State {
    struct Ear {
        double delay = 0, gain = 1, pole = 0, targetDelay = 0, targetGain = 1, targetPole = 0,
               low = 0;
    };
    ProcessSpec spec;
    double radius = 0, speed = 0;
    std::size_t remaining = 0;
    CircularHistory<T> history;
    std::vector<std::array<Ear, 2>> ears;
};
template <Sample T> AnalyticBinaural<T>::AnalyticBinaural() = default;
template <Sample T> AnalyticBinaural<T>::~AnalyticBinaural() = default;
template <Sample T> AnalyticBinaural<T>::AnalyticBinaural(AnalyticBinaural&&) noexcept = default;
template <Sample T>
AnalyticBinaural<T>& AnalyticBinaural<T>::operator=(AnalyticBinaural&&) noexcept = default;
template <Sample T>
Status AnalyticBinaural<T>::prepare(const ProcessSpec& spec, double radius, double speed) noexcept {
    if (!detail::validAnalysisSpec(spec) || spec.channels > 32 || !isFinite(radius) ||
        radius < .02 || radius > .3 || !isFinite(speed) || speed < 100 || speed > 2000)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->radius = radius;
        s->speed = speed;
        s->ears.resize(spec.channels);
        auto status = s->history.prepare(
            spec.channels, std::size_t(std::ceil(3 * radius / speed * spec.sampleRate)) + 8);
        if (status != Status::ok)
            return status;
        for (auto& pair : s->ears)
            for (auto& e : pair)
                e.delay = e.targetDelay = 2;
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status AnalyticBinaural<T>::setEmitters(std::span<const BinauralEmitter> emitters,
                                        std::size_t ramp) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (emitters.size() != s.spec.channels || ramp > (1u << 30))
        return Status::invalid_argument;
    for (const auto& e : emitters) {
        Vec3 d;
        if (!unit(e.position, d) || !isFinite(e.gain) || std::abs(e.gain) > 16 ||
            !isFinite(e.radiusMetres) || e.radiusMetres < 0 || e.radiusMetres > 100)
            return Status::invalid_argument;
    }
    for (std::size_t ch = 0; ch < emitters.size(); ++ch) {
        const auto& e = emitters[ch];
        Vec3 d;
        (void)unit(e.position, d);
        const double distance = length(e.position),
                     width = std::clamp(e.radiusMetres / distance, 0., 1.);
        // Spherical-head ITD (Woodworth), rear/upper coloration and near-ear distance.
        const double angle = std::asin(std::clamp(d.y, -1., 1.));
        const double itd =
            s.radius / s.speed * (angle + std::sin(angle)) * (1 - width) * s.spec.sampleRate;
        for (std::size_t ear = 0; ear < 2; ++ear) {
            auto& a = s.ears[ch][ear];
            const double side = ear == 0 ? 1. : -1.;
            const double shadow = std::max(0., -side * d.y) * (1 - width);
            const double cutoff = std::min(s.spec.sampleRate * .45,
                                           18000. / (1 + 8 * shadow + 1.5 * std::max(0., -d.x)));
            const Vec3 earPos{0, side * s.radius, 0};
            const double nearGain =
                std::clamp(distance / std::max(.02, length(e.position - earPos)), .25, 4.);
            a.targetDelay = 2 + std::max(0., -side * itd);
            a.targetGain = e.gain * (1 - .45 * shadow) * ((1 - width) * nearGain + width);
            a.targetPole = std::exp(-2 * std::numbers::pi * cutoff / s.spec.sampleRate);
            if (!ramp) {
                a.delay = a.targetDelay;
                a.gain = a.targetGain;
                a.pole = a.targetPole;
            }
        }
    }
    s.remaining = ramp;
    return Status::ok;
}
template <Sample T>
Status AnalyticBinaural<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = transformBlocks(s.spec, in, out, s.spec.channels, 2);
    if (status != Status::ok)
        return status;
    std::array<T, 32> frame{};
    for (std::size_t i = 0; i < in.frames(); ++i) {
        for (std::size_t ch = 0; ch < in.channels(); ++ch)
            frame[ch] = in.channel(ch)[i];
        (void)s.history.pushFrame({frame.data(), in.channels()});
        std::array<double, 2> sum{};
        for (std::size_t ch = 0; ch < in.channels(); ++ch)
            for (std::size_t ear = 0; ear < 2; ++ear) {
                auto& a = s.ears[ch][ear];
                if (s.remaining) {
                    const double t = 1. / double(s.remaining);
                    a.delay += (a.targetDelay - a.delay) * t;
                    a.gain += (a.targetGain - a.gain) * t;
                    a.pole += (a.targetPole - a.pole) * t;
                }
                const double x = s.history.read(ch, a.delay, HistoryInterpolation::lagrange3);
                a.low = (1 - a.pole) * x + a.pole * a.low;
                sum[ear] += a.gain * a.low;
            }
        if (s.remaining)
            --s.remaining;
        out.channel(0)[i] = T(sum[0]);
        out.channel(1)[i] = T(sum[1]);
    }
    return Status::ok;
}
template <Sample T> void AnalyticBinaural<T>::reset() noexcept {
    if (state_) {
        auto& s = *state_;
        s.history.reset();
        s.remaining = 0;
        for (auto& p : s.ears)
            for (auto& a : p) {
                a.low = 0;
                a.delay = a.targetDelay;
                a.gain = a.targetGain;
                a.pole = a.targetPole;
            }
    }
}

template <Sample T> struct HrtfSet<T>::State {
    struct Point {
        Vec3 direction;
        std::array<double, 2> delays{};
        std::vector<double> fir;
    };
    std::vector<Point> points;
    double rate = 0, width = .7;
    std::size_t taps = 0, tail = 0, latency = 0;
    bool truncated = false;
};
template <Sample T>
Status HrtfSet<T>::prepare(std::span<const HrirPoint<T>> points,
                           const HrtfPreparation& config) noexcept {
    if (points.empty() || points.size() > 2048 || !isFinite(config.sourceSampleRate) ||
        !isFinite(config.targetSampleRate) || config.sourceSampleRate < 8000 ||
        config.sourceSampleRate > 768000 || config.targetSampleRate < 8000 ||
        config.targetSampleRate > 768000 ||
        config.targetSampleRate / config.sourceSampleRate < .125 ||
        config.targetSampleRate / config.sourceSampleRate > 8 || !config.maximumTaps ||
        config.maximumTaps > 2048 || !isFinite(config.onsetThresholdDb) ||
        config.onsetThresholdDb < -100 || config.onsetThresholdDb > -3 ||
        !isFinite(config.angularWidthRadians) || config.angularWidthRadians < .05 ||
        config.angularWidthRadians > 3.2)
        return Status::invalid_spec;
    try {
        auto s = std::make_shared<State>();
        s->rate = config.targetSampleRate;
        s->width = config.angularWidthRadians;
        const double ratio = config.sourceSampleRate / config.targetSampleRate;
        const double half = 32 * std::max(1., ratio);
        s->latency = ratio == 1 ? 0 : std::size_t(std::ceil(half / ratio));
        for (const auto& p : points) {
            typename State::Point q;
            if (!unit(p.direction, q.direction))
                return Status::invalid_argument;
            for (auto& prior : s->points)
                if (dot(prior.direction, q.direction) > 1 - 1e-12)
                    return Status::invalid_argument;
            const std::array<std::span<const T>, 2> ears{p.left, p.right};
            const std::array<double, 2> delays{p.leftDelayFrames, p.rightDelayFrames};
            std::array<std::vector<double>, 2> converted;
            for (std::size_t ear = 0; ear < 2; ++ear) {
                auto ir = ears[ear];
                if (ir.empty() || ir.size() > 16384 || !isFinite(delays[ear]) || delays[ear] < 0 ||
                    delays[ear] > config.sourceSampleRate * .02)
                    return Status::invalid_argument;
                double peak = 0;
                for (T x : ir) {
                    if (!isFinite(x) || std::abs(double(x)) > 64)
                        return Status::invalid_argument;
                    peak = std::max(peak, std::abs(double(x)));
                }
                std::size_t onset = 0;
                if (config.detectOnsets && peak > 0) {
                    const double threshold = peak * std::pow(10., config.onsetThresholdDb / 20);
                    while (onset + 1 < ir.size() && std::abs(double(ir[onset])) < threshold)
                        ++onset;
                }
                q.delays[ear] = (delays[ear] + double(onset)) / ratio;
                const auto natural = std::size_t(
                    std::ceil((double(ir.size() - onset) + (ratio == 1 ? 0 : half)) / ratio) +
                    double(s->latency));
                const auto count = std::min(config.maximumTaps, natural);
                s->truncated = s->truncated || natural > count;
                converted[ear].resize(count);
                const double cutoff = .95 / std::max(1., ratio);
                for (std::size_t i = 0; i < count; ++i) {
                    if (ratio == 1) {
                        converted[ear][i] = ir[onset + i];
                        continue;
                    }
                    const double position = (double(i) - double(s->latency)) * ratio;
                    const auto first = std::ptrdiff_t(std::ceil(position - half)),
                               last = std::ptrdiff_t(std::floor(position + half));
                    double value = 0;
                    for (auto j = first; j <= last; ++j)
                        if (j >= 0 && std::size_t(j) + onset < ir.size())
                            value += double(ir[onset + std::size_t(j)]) *
                                     detail::windowedSinc(position - double(j), cutoff, half);
                    converted[ear][i] =
                        value * ratio; // IR conversion preserves DC gain, not waveform energy.
                }
                s->taps = std::max(s->taps, count);
            }
            const auto count = std::max(converted[0].size(), converted[1].size());
            q.fir.resize(2 * count);
            std::copy(converted[0].begin(), converted[0].end(), q.fir.begin());
            std::copy(converted[1].begin(), converted[1].end(), q.fir.begin() + count);
            // Temporarily retain each point's own stride; pad once the global stride is known.
            s->points.push_back(std::move(q));
        }
        for (auto& q : s->points) {
            const auto old = q.fir.size() / 2;
            if (old != s->taps) {
                std::vector<double> padded(2 * s->taps);
                std::copy_n(q.fir.begin(), old, padded.begin());
                std::copy_n(q.fir.begin() + old, old, padded.begin() + s->taps);
                q.fir.swap(padded);
            }
            for (double d : q.delays)
                s->tail = std::max(s->tail, s->taps + std::size_t(std::ceil(d)));
        }
        state_ = std::move(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> std::size_t HrtfSet<T>::points() const noexcept {
    return state_ ? state_->points.size() : 0;
}
template <Sample T> std::size_t HrtfSet<T>::taps() const noexcept {
    return state_ ? state_->taps : 0;
}
template <Sample T> double HrtfSet<T>::sampleRate() const noexcept {
    return state_ ? state_->rate : 0;
}
template <Sample T> std::size_t HrtfSet<T>::latencyFrames() const noexcept {
    return state_ ? state_->latency : 0;
}
template <Sample T> bool HrtfSet<T>::truncated() const noexcept {
    return state_ && state_->truncated;
}

template <Sample T> struct HrirBinaural<T>::State {
    ProcessSpec spec;
    std::shared_ptr<const typename HrtfSet<T>::State> data;
    std::vector<double> current, next, scratch;
    CircularHistory<T> history;
    VectorOps ops;
    std::array<double, 2> delay{}, nextDelay{};
    std::size_t taps = 0, tail = 0, remaining = 0, total = 0;
    double convolve(std::size_t ear, bool target) noexcept {
        auto& fir = target ? next : current;
        const double age = (target ? nextDelay : delay)[ear];
        const auto first = std::ptrdiff_t(std::floor(age));
        const double mix = age - double(first);
        (void)history.readWindow(0, first, scratch);
        const auto& kernels = detail::VectorAccess::kernels<double>(ops);
        const auto* h = fir.data() + ear * taps;
        const double a = kernels.dot(scratch.data(), h, taps);
        return mix == 0 ? a : a + (kernels.dot(scratch.data() + 1, h, taps) - a) * mix;
    }
};
template <Sample T> HrirBinaural<T>::HrirBinaural() = default;
template <Sample T> HrirBinaural<T>::~HrirBinaural() = default;
template <Sample T> HrirBinaural<T>::HrirBinaural(HrirBinaural&&) noexcept = default;
template <Sample T> HrirBinaural<T>& HrirBinaural<T>::operator=(HrirBinaural&&) noexcept = default;
template <Sample T>
Status HrirBinaural<T>::prepare(const ProcessSpec& spec, std::span<const HrirPoint<T>> points,
                                CpuBackend backend) noexcept {
    // Preserve the raw overload's original tap bound: no silent truncation.
    // The asset overload makes larger input/truncation an explicit host choice.
    for (const auto& point : points)
        if (point.left.size() > 2048 || point.right.size() > 2048)
            return Status::invalid_argument;
    HrtfSet<T> data;
    HrtfPreparation config;
    config.sourceSampleRate = config.targetSampleRate = spec.sampleRate;
    config.detectOnsets = false;
    auto status = data.prepare(points, config);
    if (status != Status::ok)
        return status;
    return prepare(spec, data, backend);
}
template <Sample T>
Status HrirBinaural<T>::prepare(const ProcessSpec& spec, const HrtfSet<T>& data,
                                CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || spec.channels != 1 || !data.state_ ||
        data.sampleRate() != spec.sampleRate)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->data = data.state_;
        s->taps = s->data->taps;
        s->tail = s->data->tail;
        s->current.resize(2 * s->taps);
        s->next.resize(2 * s->taps);
        s->scratch.resize(s->taps + 1);
        auto status = VectorOps::create(backend, s->ops);
        if (status != Status::ok)
            return status;
        status = s->history.prepare(1, s->tail + 2);
        if (status != Status::ok)
            return status;
        s->current = s->next = s->data->points.front().fir;
        s->delay = s->nextDelay = s->data->points.front().delays;
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}

template <Sample T> Status HrirBinaural<T>::setDirection(Vec3 dir, std::size_t fade) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    Vec3 d;
    if (!unit(dir, d) || fade > (1u << 30))
        return Status::invalid_argument;
    if (s.remaining)
        return Status::transition_in_progress;
    double sum = 0, closest = 1e100;
    std::size_t exact = s.data->points.size();
    std::array<double, 2048> weights{};
    for (std::size_t i = 0; i < s.data->points.size(); ++i) {
        const double angle = std::acos(std::clamp(dot(d, s.data->points[i].direction), -1., 1.));
        if (angle < 1e-7) {
            exact = i;
            break;
        }
        const double distance = angle * angle;
        weights[i] = distance;
        closest = std::min(closest, distance);
    }
    if (exact == s.data->points.size())
        for (std::size_t i = 0; i < s.data->points.size(); ++i) {
            const double distance = weights[i];
            weights[i] =
                std::exp(-(distance - closest) / (s.data->width * s.data->width)) / distance;
            sum += weights[i];
        }
    std::fill(s.next.begin(), s.next.end(), 0.);
    s.nextDelay = {};
    if (exact < s.data->points.size()) {
        std::copy(s.data->points[exact].fir.begin(), s.data->points[exact].fir.end(),
                  s.next.begin());
        s.nextDelay = s.data->points[exact].delays;
    } else
        for (std::size_t j = 0; j < s.data->points.size(); ++j) {
            const auto& p = s.data->points[j];
            const double w = weights[j] / sum;
            (void)s.ops.addScaled(std::span<const double>(p.fir), std::span<double>(s.next), w);
            for (std::size_t e = 0; e < 2; ++e)
                s.nextDelay[e] += w * p.delays[e];
        }
    s.remaining = s.total = fade;
    if (!fade) {
        std::copy(s.next.begin(), s.next.end(), s.current.begin());
        s.delay = s.nextDelay;
    }
    return Status::ok;
}
template <Sample T>
Status HrirBinaural<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = transformBlocks(s.spec, in, out, 1, 2);
    if (status != Status::ok)
        return status;
    for (std::size_t i = 0; i < in.frames(); ++i) {
        const T x = in.channel(0)[i];
        (void)s.history.pushFrame({&x, 1});
        const double t = s.remaining ? double(s.total - s.remaining + 1) / double(s.total) : 0;
        for (std::size_t ear = 0; ear < 2; ++ear) {
            const double a = s.convolve(ear, false);
            out.channel(ear)[i] = T(s.remaining ? a + (s.convolve(ear, true) - a) * t : a);
        }
        if (s.remaining && !--s.remaining) {
            std::copy(s.next.begin(), s.next.end(), s.current.begin());
            s.delay = s.nextDelay;
        }
    }
    return Status::ok;
}
template <Sample T> void HrirBinaural<T>::reset() noexcept {
    if (state_) {
        auto& s = *state_;
        s.history.reset();
        if (s.remaining) {
            std::copy(s.next.begin(), s.next.end(), s.current.begin());
            s.delay = s.nextDelay;
        }
        s.remaining = 0;
    }
}
template <Sample T> std::size_t HrirBinaural<T>::latencyFrames() const noexcept {
    return state_ ? state_->data->latency : 0;
}
template <Sample T> std::size_t HrirBinaural<T>::tailFrames() const noexcept {
    return state_ ? state_->tail : 0;
}

struct VbapLayout::State {
    struct Basis {
        std::array<std::size_t, 3> index{};
        std::array<Vec3, 3> inverse{};
    };
    std::vector<Vec3> speakers;
    std::vector<Basis> bases;
    bool three = false, fallback = false;
};
VbapLayout::VbapLayout() = default;
VbapLayout::~VbapLayout() = default;
VbapLayout::VbapLayout(VbapLayout&&) noexcept = default;
VbapLayout& VbapLayout::operator=(VbapLayout&&) noexcept = default;
Status VbapLayout::prepare(std::span<const Vec3> speakers, bool three, bool fallback) noexcept {
    if (speakers.size() < (three ? 3u : 2u) || speakers.size() > 64)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->three = three;
        s->fallback = fallback;
        for (auto v : speakers) {
            Vec3 d;
            if (!unit(v, d) || (!three && std::abs(d.z) > 1e-6))
                return Status::invalid_argument;
            for (auto prior : s->speakers)
                if (dot(prior, d) > 1 - 1e-10)
                    return Status::invalid_argument;
            s->speakers.push_back(d);
        }
        const auto n = speakers.size();
        if (!three) {
            std::vector<std::size_t> indices(n);
            for (std::size_t i = 0; i < n; ++i)
                indices[i] = i;
            std::sort(indices.begin(), indices.end(), [&](auto a, auto b) {
                return std::atan2(s->speakers[a].y, s->speakers[a].x) <
                       std::atan2(s->speakers[b].y, s->speakers[b].x);
            });
            for (std::size_t i = 0; i < n; ++i) {
                auto a = indices[i], b = indices[(i + 1) % n];
                const auto x = s->speakers[a], y = s->speakers[b];
                const double det = x.x * y.y - x.y * y.x;
                if (det <= 1e-9)
                    continue;
                s->bases.push_back(
                    {{a, b, 0}, {{{y.y / det, -y.x / det, 0}, {-x.y / det, x.x / det, 0}, {}}}});
            }
        } else
            for (std::size_t i = 0; i < n; ++i)
                for (std::size_t j = i + 1; j < n; ++j)
                    for (std::size_t k = j + 1; k < n; ++k) {
                        const auto a = s->speakers[i], b = s->speakers[j], c = s->speakers[k];
                        const double det = dot(a, cross(b, c));
                        if (std::abs(det) < 1e-9)
                            continue;
                        const auto normal = cross(b - a, c - a);
                        const double plane = dot(normal, a);
                        bool hull = true;
                        for (auto v : s->speakers)
                            if ((dot(normal, v) - plane) * plane > 1e-8) {
                                hull = false;
                                break;
                            }
                        if (hull)
                            s->bases.push_back({{i, j, k},
                                                {cross(b, c) * (1 / det), cross(c, a) * (1 / det),
                                                 cross(a, b) * (1 / det)}});
                    }
        if (s->bases.empty())
            return Status::unsupported_format;
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status VbapLayout::gains(Vec3 direction, std::span<double> gains) const noexcept {
    if (!state_)
        return Status::not_prepared;
    const auto& s = *state_;
    Vec3 d;
    if (!unit(direction, d) || gains.size() != s.speakers.size())
        return Status::invalid_argument;
    std::array<double, 64> result{};
    bool found = false;
    for (const auto& basis : s.bases) {
        std::array<double, 3> g{};
        bool valid = true;
        double energy = 0;
        for (std::size_t i = 0; i < (s.three ? 3u : 2u); ++i) {
            g[i] = dot(basis.inverse[i], d);
            if (g[i] < -1e-9)
                valid = false;
            g[i] = std::max(0., g[i]);
            energy += g[i] * g[i];
        }
        if (valid && energy > 1e-15) {
            for (std::size_t i = 0; i < (s.three ? 3u : 2u); ++i)
                result[basis.index[i]] = g[i] / std::sqrt(energy);
            found = true;
            break;
        }
    }
    if (!found) {
        if (!s.fallback)
            return Status::unsupported_format;
        std::size_t best = 0;
        for (std::size_t i = 1; i < s.speakers.size(); ++i)
            if (dot(d, s.speakers[i]) > dot(d, s.speakers[best]))
                best = i;
        result[best] = 1;
    }
    std::copy_n(result.begin(), gains.size(), gains.begin());
    return Status::ok;
}
std::size_t VbapLayout::speakers() const noexcept {
    return state_ ? state_->speakers.size() : 0;
}
Status ambisonicCoefficients(unsigned order, Vec3 direction, std::span<double> output) noexcept {
    if (order > 7 || output.size() != (order + 1) * (order + 1))
        return Status::invalid_argument;
    Vec3 d;
    if (!unit(direction, d))
        return Status::invalid_argument;
    std::array<double, 64> result{};
    const double phi = std::atan2(d.y, d.x), z = d.z, r = std::sqrt(std::max(0., 1 - z * z));
    for (unsigned m = 0; m <= order; ++m) {
        double pmm = 1;
        for (unsigned k = 1; k <= m; ++k)
            pmm *= double(2 * k - 1) * r;
        double previous = 0, current = pmm;
        for (unsigned l = m; l <= order; ++l) {
            if (l > m) {
                const double next =
                    (double(2 * l - 1) * z * current - double(l + m - 1) * previous) /
                    double(l - m);
                previous = current;
                current = next;
            }
            double ratio = 1;
            for (unsigned k = l - m + 1; k <= l + m; ++k)
                ratio /= double(k);
            const double v = std::sqrt((m ? 2. : 1.) * ratio) * current;
            const auto center = l * (l + 1);
            result[center + m] = v * std::cos(double(m) * phi);
            if (m)
                result[center - m] = v * std::sin(double(m) * phi);
        }
    }
    std::copy_n(result.begin(), output.size(), output.begin());
    return Status::ok;
}
Status ambisonicRotation(unsigned order, Quaternion rotation, std::span<double> output) noexcept {
    const auto n = (order + 1) * (order + 1);
    if (order > 7 || output.size() != n * n)
        return Status::invalid_argument;
    Quaternion q;
    auto status = normalize(rotation, q);
    if (status != Status::ok)
        return status;
    std::array<double, 4096> result{};
    std::array<double, 64> a{}, b{};
    // Eight-point Gauss-Legendre x 32 azimuths is exact for products through order 7.
    constexpr std::array<double, 8> z{-.9602898564975363, -.7966664774136267, -.5255324099163290,
                                      -.1834346424956498, .1834346424956498,  .5255324099163290,
                                      .7966664774136267,  .9602898564975363};
    constexpr std::array<double, 8> w{.1012285362903763, .2223810344533745, .3137066458778873,
                                      .3626837833783620, .3626837833783620, .3137066458778873,
                                      .2223810344533745, .1012285362903763};
    for (std::size_t iz = 0; iz < 8; ++iz)
        for (unsigned ip = 0; ip < 32; ++ip) {
            const double phi = 2 * std::numbers::pi * ip / 32, r = std::sqrt(1 - z[iz] * z[iz]);
            const Vec3 d{r * std::cos(phi), r * std::sin(phi), z[iz]};
            (void)ambisonicCoefficients(order, d, {a.data(), n});
            (void)ambisonicCoefficients(order, rotate(q, d), {b.data(), n});
            for (unsigned row = 0; row < n; ++row)
                for (unsigned col = 0; col < n; ++col) {
                    const auto degree = unsigned(std::sqrt(double(col)));
                    result[row * n + col] += b[row] * a[col] * w[iz] * double(2 * degree + 1) / 64.;
                }
        }
    std::copy_n(result.begin(), output.size(), output.begin());
    return Status::ok;
}
template <Sample T> struct AmbisonicRenderer<T>::State {
    ProcessSpec spec;
    unsigned order = 0;
    std::size_t count = 0, speakers = 0;
    std::array<double, 64> direction{};
    std::array<double, 4096> rotation{};
    std::vector<double> decoder;
    VectorOps ops;
};
template <Sample T> AmbisonicRenderer<T>::AmbisonicRenderer() = default;
template <Sample T> AmbisonicRenderer<T>::~AmbisonicRenderer() = default;
template <Sample T> AmbisonicRenderer<T>::AmbisonicRenderer(AmbisonicRenderer&&) noexcept = default;
template <Sample T>
AmbisonicRenderer<T>& AmbisonicRenderer<T>::operator=(AmbisonicRenderer&&) noexcept = default;
template <Sample T>
Status AmbisonicRenderer<T>::prepare(const ProcessSpec& spec, unsigned order,
                                     std::span<const Vec3> speakers, double regularization,
                                     CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || spec.channels != 1 || order > 7 || speakers.empty() ||
        speakers.size() > 64 || !isFinite(regularization) || regularization < 0 ||
        regularization > 1)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->order = order;
        s->count = (order + 1) * (order + 1);
        s->speakers = speakers.size();
        const auto n = s->count;
        auto status = VectorOps::create(backend, s->ops);
        if (status != Status::ok)
            return status;
        status = makeDecoder(order, speakers, regularization, s->decoder);
        if (status != Status::ok)
            return status;
        (void)ambisonicCoefficients(order, {1, 0, 0}, {s->direction.data(), n});
        (void)ambisonicRotation(order, {}, {s->rotation.data(), n * n});
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status AmbisonicRenderer<T>::setDirection(Vec3 d) noexcept {
    return state_
               ? ambisonicCoefficients(state_->order, d, {state_->direction.data(), state_->count})
               : Status::not_prepared;
}
template <Sample T> Status AmbisonicRenderer<T>::setRotation(Quaternion q) noexcept {
    return state_ ? ambisonicRotation(state_->order, q,
                                      {state_->rotation.data(), state_->count * state_->count})
                  : Status::not_prepared;
}
template <Sample T>
Status AmbisonicRenderer<T>::encode(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = transformBlocks(s.spec, in, out, 1, s.count);
    if (status != Status::ok)
        return status;
    for (std::size_t ch = 0; ch < s.count; ++ch)
        (void)s.ops.scale(in.channel(0), out.channel(ch), T(s.direction[ch]));
    return Status::ok;
}
template <Sample T>
Status AmbisonicRenderer<T>::rotateField(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = transformBlocks(s.spec, in, out, s.count, s.count);
    if (status != Status::ok)
        return status;
    for (std::size_t ch = 0; ch < s.count; ++ch) {
        s.ops.fill(out.channel(ch), T{});
        for (std::size_t k = 0; k < s.count; ++k)
            (void)s.ops.addScaled(in.channel(k), out.channel(ch), T(s.rotation[ch * s.count + k]));
    }
    return Status::ok;
}
template <Sample T>
Status AmbisonicRenderer<T>::decode(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = transformBlocks(s.spec, in, out, s.count, s.speakers);
    if (status != Status::ok)
        return status;
    for (std::size_t ch = 0; ch < s.speakers; ++ch) {
        s.ops.fill(out.channel(ch), T{});
        for (std::size_t k = 0; k < s.count; ++k)
            (void)s.ops.addScaled(in.channel(k), out.channel(ch), T(s.decoder[ch * s.count + k]));
    }
    return Status::ok;
}
template <Sample T> void AmbisonicRenderer<T>::reset() noexcept {}
template <Sample T> std::size_t AmbisonicRenderer<T>::channels() const noexcept {
    return state_ ? state_->count : 0;
}
template <Sample T> struct HoaBinaural<T>::State {
    ProcessSpec spec;
    unsigned order = 0;
    std::size_t tail = 0, latency = 0;
    std::array<double, 4096> rotation{};
    bool identity = true;
    VectorOps ops;
    AudioBuffer<T> rotated;
    MatrixConvolver<T> convolution;
};
template <Sample T> HoaBinaural<T>::HoaBinaural() = default;
template <Sample T> HoaBinaural<T>::~HoaBinaural() = default;
template <Sample T> HoaBinaural<T>::HoaBinaural(HoaBinaural&&) noexcept = default;
template <Sample T> HoaBinaural<T>& HoaBinaural<T>::operator=(HoaBinaural&&) noexcept = default;
template <Sample T>
Status HoaBinaural<T>::prepare(const ProcessSpec& spec, unsigned order, const HrtfSet<T>& data,
                               double regularization, CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || order > 7 ||
        spec.channels != (order + 1) * (order + 1) || !data.state_ ||
        data.sampleRate() != spec.sampleRate || !isFinite(regularization) || regularization < 0 ||
        regularization > 1)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->order = order;
        s->tail = data.state_->tail + 1;
        s->latency = data.latencyFrames();
        std::vector<Vec3> directions;
        for (auto& p : data.state_->points)
            directions.push_back(p.direction);
        std::vector<double> decoder;
        auto status = makeDecoder(order, directions, regularization, decoder);
        if (status != Status::ok)
            return status;
        status = VectorOps::create(backend, s->ops);
        if (status != Status::ok)
            return status;
        status = s->rotated.resize(spec.channels, spec.maxBlockFrames);
        if (status != Status::ok)
            return status;
        std::vector<std::vector<T>> irs(2 * spec.channels, std::vector<T>(s->tail));
        std::vector<ConvolutionRoute<T>> routes;
        routes.reserve(irs.size());
        for (std::size_t ch = 0; ch < spec.channels; ++ch)
            for (std::size_t ear = 0; ear < 2; ++ear) {
                auto& ir = irs[2 * ch + ear];
                for (std::size_t i = 0; i < directions.size(); ++i) {
                    const auto& p = data.state_->points[i];
                    const double weight = decoder[i * spec.channels + ch];
                    const auto delay = std::size_t(p.delays[ear]);
                    const double fraction = p.delays[ear] - double(delay);
                    for (std::size_t t = 0; t < data.taps(); ++t) {
                        const double h = p.fir[ear * data.taps() + t] * weight;
                        ir[delay + t] += T(h * (1 - fraction));
                        ir[delay + t + 1] += T(h * fraction);
                    }
                }
                routes.push_back({ch, ear, ir});
            }
        status = s->convolution.prepare(spec, 2, routes, 128, 1024, backend);
        if (status != Status::ok)
            return status;
        (void)ambisonicRotation(order, {}, {s->rotation.data(), spec.channels * spec.channels});
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status HoaBinaural<T>::setRotation(Quaternion q) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto st = ambisonicRotation(s.order, q, {s.rotation.data(), s.spec.channels * s.spec.channels});
    if (st != Status::ok)
        return st;
    s.identity = true;
    for (std::size_t r = 0; r < s.spec.channels; ++r)
        for (std::size_t c = 0; c < s.spec.channels; ++c)
            if (std::abs(s.rotation[r * s.spec.channels + c] - double(r == c)) > 1e-12)
                s.identity = false;
    return Status::ok;
}
template <Sample T>
Status HoaBinaural<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto st = transformBlocks(s.spec, in, out, s.spec.channels, 2);
    if (st != Status::ok)
        return st;
    if (s.identity)
        return s.convolution.process(in, out);
    auto temp = s.rotated.block(in.frames());
    for (std::size_t ch = 0; ch < in.channels(); ++ch) {
        s.ops.fill(temp.channel(ch), T{});
        for (std::size_t k = 0; k < in.channels(); ++k)
            (void)s.ops.addScaled(in.channel(k), temp.channel(ch),
                                  T(s.rotation[ch * in.channels() + k]));
    }
    return s.convolution.process(temp, out);
}
template <Sample T> void HoaBinaural<T>::reset() noexcept {
    if (state_)
        state_->convolution.reset();
}
template <Sample T> std::size_t HoaBinaural<T>::latencyFrames() const noexcept {
    return state_ ? state_->latency : 0;
}
template <Sample T> std::size_t HoaBinaural<T>::tailFrames() const noexcept {
    return state_ ? state_->tail : 0;
}
template class HrtfSet<float>;
template class HrtfSet<double>;
template class HoaBinaural<float>;
template class HoaBinaural<double>;

template class AnalyticBinaural<float>;
template class AnalyticBinaural<double>;
template class HrirBinaural<float>;
template class HrirBinaural<double>;
template class AmbisonicRenderer<float>;
template class AmbisonicRenderer<double>;
} // namespace cydsp
