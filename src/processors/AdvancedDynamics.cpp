#include "cydsp/processors/AdvancedDynamics.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/processors/Biquad.h"
#include "cydsp/processors/FilterBank.h"
#include "cydsp/processors/Oversampler.h"
#include "detail/Analysis.h"
#include "detail/AudioMath.h"
#include "detail/VectorTable.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
#include <vector>
namespace cydsp {
namespace {
bool validSaturation(const SaturationParameters& p) noexcept {
    return unsigned(p.math) <= unsigned(MathAccuracy::fast) &&
           unsigned(p.shape) <= unsigned(AdaaShape::hysteresis) && isFinite(p.drive) &&
           p.drive >= .001 && p.drive <= 64 && isFinite(p.bias) && std::abs(p.bias) <= 4 &&
           isFinite(p.outputGain) && std::abs(p.outputGain) <= 16 && isFinite(p.hysteresisWidth) &&
           p.hysteresisWidth >= 0 && p.hysteresisWidth <= 4 && isFinite(p.hysteresisAmount) &&
           p.hysteresisAmount >= 0 && p.hysteresisAmount <= 4;
}
// The driven input is clamped to +/-1e6. This power-of-two reduction
// is exact there and avoids the scalar libm remainder call. The +/-2 tie
// selects the same triangle value and even primitive on either side.
double foldedPhase(double x) noexcept {
    return x - 4 * std::floor(x * .25 + .5);
}
double shape(AdaaShape type, double x) noexcept {
    switch (type) {
    case AdaaShape::atan:
        return 2 / std::numbers::pi * std::atan(x);
    case AdaaShape::cubic:
        return std::abs(x) <= 1 ? x - x * x * x / 3 : std::copysign(2. / 3, x);
    case AdaaShape::hardClip:
        return std::clamp(x, -1., 1.);
    case AdaaShape::wavefolder: {
        const double r = foldedPhase(x);
        return std::abs(r) <= 1 ? r : std::copysign(2 - std::abs(r), r);
    }
    default:
        return std::tanh(x);
    }
}
double primitive(AdaaShape type, double x) noexcept {
    const double a = std::abs(x);
    switch (type) {
    case AdaaShape::atan:
        return 2 / std::numbers::pi * (x * std::atan(x) - .5 * std::log1p(x * x));
    case AdaaShape::cubic:
        return a <= 1 ? .5 * x * x - x * x * x * x / 12 : 2 * a / 3 - .25;
    case AdaaShape::hardClip:
        return a <= 1 ? .5 * x * x : a - .5;
    case AdaaShape::wavefolder: {
        const double r = std::abs(foldedPhase(x));
        return r <= 1 ? .5 * r * r : 2 * r - .5 * r * r - 1;
    }
    default:
        return a + std::log1p(std::exp(-2 * a)) - std::numbers::ln2;
    }
}
double pole(double ms, double rate) noexcept {
    return ms <= 0 ? 0 : std::exp(-1000 / (ms * rate));
}
struct SectionState {
    double z1 = 0, z2 = 0;
    double run(double x, const BiquadCoefficients& c) noexcept {
        const double y = c.b0 * x + z1;
        z1 = c.b1 * x - c.a1 * y + z2;
        z2 = c.b2 * x - c.a2 * y;
        return y;
    }
};
} // namespace
template <Sample T> struct AdaaSaturator<T>::State {
    ProcessSpec spec;
    SaturationParameters p;
    VectorOps ops;
    std::array<double, 64> foldInput{}, foldOutput{};
    std::vector<double> previous, memory, previousPrimitive;
    std::vector<unsigned char> active, primitiveValid;
};
template <Sample T> AdaaSaturator<T>::AdaaSaturator() = default;
template <Sample T> AdaaSaturator<T>::~AdaaSaturator() = default;
template <Sample T> AdaaSaturator<T>::AdaaSaturator(AdaaSaturator&&) noexcept = default;
template <Sample T>
AdaaSaturator<T>& AdaaSaturator<T>::operator=(AdaaSaturator&&) noexcept = default;
template <Sample T> Status AdaaSaturator<T>::prepare(const ProcessSpec& spec) noexcept {
    if (!detail::validAnalysisSpec(spec))
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        const auto status = VectorOps::create(CpuBackend::automatic, s->ops);
        if (status != Status::ok)
            return status;
        s->previous.resize(spec.channels);
        s->memory.resize(spec.channels);
        s->previousPrimitive.resize(spec.channels);
        s->primitiveValid.resize(spec.channels);
        s->active.resize(spec.channels);
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> Status AdaaSaturator<T>::setParameters(const SaturationParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!validSaturation(p))
        return Status::invalid_argument;
    // The primitive depends on the shape; the stored driven sample remains valid
    // across drive/bias/AA edits and must not be reset by automation.
    if (state_->p.shape != p.shape)
        std::fill(state_->primitiveValid.begin(), state_->primitiveValid.end(),
                  static_cast<unsigned char>(0));
    state_->p = p;
    return Status::ok;
}
template <Sample T>
Status AdaaSaturator<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    return processImpl<false>(in, out, {}, 0, 1);
}
template <Sample T>
Status AdaaSaturator<T>::processMixed(AudioBlock<const T> in, AudioBlock<T> out,
                                      const SaturationInputStage& input, double dryGain,
                                      double wetGain) noexcept {
    // Snapshot controls before any caller-owned output can be written.
    const auto inputSnapshot = input;
    if (!isFinite(inputSnapshot.gain) || std::abs(inputSnapshot.gain) > 1e4 ||
        !isFinite(inputSnapshot.bias) || std::abs(inputSnapshot.bias) > 1e3 ||
        !isFinite(inputSnapshot.scale) || inputSnapshot.scale < 0 || inputSnapshot.scale > 64 ||
        !isFinite(dryGain) || !isFinite(wetGain) || std::abs(dryGain) > 16 ||
        std::abs(wetGain) > 16)
        return Status::invalid_argument;
    return processImpl<true>(in, out, inputSnapshot, dryGain, wetGain);
}
template <Sample T>
template <bool Mixed>
Status AdaaSaturator<T>::processImpl(AudioBlock<const T> in, AudioBlock<T> out,
                                     const SaturationInputStage& inputStage, double dryGain,
                                     double wetGain) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, in, out);
    if (status != Status::ok)
        return status;
    status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    const auto p = s.p;
    const auto driven = [&](T value) noexcept {
        double sample = double(value);
        if constexpr (Mixed)
            sample = (sample * inputStage.gain + inputStage.bias) * inputStage.scale;
        return std::clamp(sample * p.drive + p.bias, -1e6, 1e6);
    };
    const auto publish = [&]([[maybe_unused]] T dry, double value) noexcept {
        if constexpr (Mixed)
            return T(dryGain * double(dry) + wetGain * double(T(value)));
        else
            return T(value);
    };
    const double dc = shape(p.shape, p.bias);
    const auto& table = detail::VectorAccess::table(s.ops);
    const auto cubicDirect = detail::VectorAccess::kernels<T>(s.ops).cubicDirect;
    if (!p.antialias && p.shape == AdaaShape::cubic && cubicDirect) {
        const std::array<double, 5> stage{inputStage.gain, inputStage.bias, inputStage.scale,
                                          p.drive, p.bias};
        for (std::size_t ch = 0; ch < in.channels() && in.frames(); ++ch) {
            const auto source = in.channel(ch);
            const auto last = driven(source.back());
            cubicDirect(source.data(), out.channel(ch).data(), in.frames(), stage.data(), dc,
                        p.outputGain, dryGain, wetGain, Mixed);
            s.previous[ch] = last;
            s.active[ch] = 1;
            s.primitiveValid[ch] = 0;
        }
        return Status::ok;
    }
    if constexpr (!Mixed) {
        const auto direct = detail::VectorAccess::kernels<T>(s.ops).clipDirect;
        if (!p.antialias && p.shape == AdaaShape::hardClip && direct) {
            for (std::size_t ch = 0; ch < in.channels(); ++ch) {
                const auto input = in.channel(ch);
                auto output = out.channel(ch);
                if (input.empty())
                    continue;
                const double last = driven(input.back());
                direct(input.data(), output.data(), input.size(), p.drive, p.bias, dc,
                       p.outputGain);
                s.previous[ch] = last;
                s.active[ch] = 1;
                s.primitiveValid[ch] = 0;
            }
            return Status::ok;
        }
    }
    // The play-memory recurrence retains its original sample order. Only its
    // memoryless tanh law and the typed input/output passes are time-parallel.
    if (p.shape == AdaaShape::hysteresis && table.shapeTanh) {
        const auto& kernels = detail::VectorAccess::kernels<T>(s.ops);
        const std::array<double, 5> stage{inputStage.gain, inputStage.bias, inputStage.scale,
                                          p.drive, p.bias};
        for (std::size_t ch = 0; ch < in.channels() && in.frames(); ++ch) {
            const auto sourceBlock = in.channel(ch);
            if (in.frames() >= 8 &&
                std::all_of(sourceBlock.begin() + 1, sourceBlock.end(),
                            [first = sourceBlock[0]](T v) { return v == first; })) {
                const auto x = driven(sourceBlock[0]);
                const auto memory =
                    std::clamp(s.memory[ch], x - p.hysteresisWidth, x + p.hysteresisWidth);
                const auto argument = x + p.hysteresisAmount * memory;
                double wet{};
                table.shapeTanh(&argument, &wet, 1, dc, p.outputGain);
                std::fill(out.channel(ch).begin(), out.channel(ch).end(),
                          publish(sourceBlock[0], wet));
                s.previous[ch] = x;
                s.memory[ch] = memory;
                s.active[ch] = 1;
                s.primitiveValid[ch] = 0;
                continue;
            }
            auto memory = s.memory[ch];
            double last = s.previous[ch];
            const auto* source = in.channel(ch).data();
            auto* destination = out.channel(ch).data();
            for (std::size_t offset = 0; offset < in.frames();) {
                const auto count = std::min(s.foldInput.size(), in.frames() - offset);
                kernels.saturationDrive(source + offset, s.foldInput.data(), count, stage.data(),
                                        Mixed);
                for (std::size_t i = 0; i < count; ++i) {
                    const auto x = s.foldInput[i];
                    memory = std::clamp(memory, x - p.hysteresisWidth, x + p.hysteresisWidth);
                    s.foldInput[i] = x + p.hysteresisAmount * memory;
                    last = x;
                }
                table.shapeTanh(s.foldInput.data(), s.foldOutput.data(), count, dc, p.outputGain);
                kernels.saturationPublish(source + offset, s.foldOutput.data(),
                                          destination + offset, count, dryGain, wetGain, Mixed);
                offset += count;
            }
            s.previous[ch] = last;
            s.memory[ch] = memory;
            s.active[ch] = 1;
            s.primitiveValid[ch] = 0;
        }
        return Status::ok;
    }
    using Secant =
        void (*)(const double*, double*, std::size_t, double, bool, double, double) noexcept;
    using Memoryless = void (*)(const double*, double*, std::size_t, double, double) noexcept;
    Secant secant = nullptr;
    Memoryless memoryless = nullptr;
    switch (p.shape) {
    case AdaaShape::tanh:
        secant = table.adaaTanh;
        memoryless = table.shapeTanh;
        break;
    case AdaaShape::atan:
        // The vector kernel is exact to rounding, so both math policies share it.
        secant = table.adaaAtan;
        memoryless = table.shapeAtan;
        break;
    case AdaaShape::cubic:
        // Two-lane NEON is faster with the cached scalar primitive on Quest 3.
        // Keep the vector law available for direct verification; SVE stays VLA.
        secant = table.backend == CpuBackend::neon ? nullptr : table.adaaCubic;
        memoryless = table.shapeCubic;
        break;
    case AdaaShape::hardClip:
        secant = table.adaaClip;
        break;
    case AdaaShape::wavefolder:
        secant = table.adaaFold;
        break;
    default:
        break;
    }
    if (p.antialias ? secant != nullptr : memoryless != nullptr) {
        // Tiles of driven doubles: input stage, shaping and publication are each
        // one vector pass. Cost does not depend on the signal: there is no
        // silence, stationarity or proximity branch into scalar code.
        const auto& kernels = detail::VectorAccess::kernels<T>(s.ops);
        const std::array<double, 5> stage{inputStage.gain, inputStage.bias, inputStage.scale,
                                          p.drive, p.bias};
        for (std::size_t ch = 0; ch < in.channels() && in.frames(); ++ch) {
            const auto* source = in.channel(ch).data();
            auto* destination = out.channel(ch).data();
            auto previous = s.previous[ch];
            bool active = s.active[ch] != 0;
            // A constant channel (digital silence, DC) has one transition from the
            // retained history and then a steady value. Both come from the same
            // kernel as the general path, so the output is identical; a varying
            // signal leaves this test at its first differing sample.
            if (in.frames() >= 8 &&
                std::all_of(source + 1, source + in.frames(),
                            [first = source[0]](T value) { return value == first; })) {
                const T dry = source[0];
                const double x = driven(dry);
                double first = 0, steady = 0;
                if (p.antialias) {
                    secant(&x, &first, 1, previous, active, dc, p.outputGain);
                    secant(&x, &steady, 1, x, true, dc, p.outputGain);
                } else {
                    memoryless(&x, &first, 1, dc, p.outputGain);
                    steady = first;
                }
                destination[0] = publish(dry, first);
                std::fill(destination + 1, destination + in.frames(), publish(dry, steady));
                s.previous[ch] = x;
                s.active[ch] = 1;
                s.primitiveValid[ch] = 0;
                continue;
            }
            for (std::size_t offset = 0; offset < in.frames();) {
                const auto count = std::min(s.foldInput.size(), in.frames() - offset);
                if (kernels.saturationDrive)
                    kernels.saturationDrive(source + offset, s.foldInput.data(), count,
                                            stage.data(), Mixed);
                else
                    for (std::size_t i = 0; i < count; ++i)
                        s.foldInput[i] = driven(source[offset + i]);
                if (p.antialias)
                    secant(s.foldInput.data(), s.foldOutput.data(), count, previous, active, dc,
                           p.outputGain);
                else
                    memoryless(s.foldInput.data(), s.foldOutput.data(), count, dc, p.outputGain);
                if (kernels.saturationPublish)
                    kernels.saturationPublish(source + offset, s.foldOutput.data(),
                                              destination + offset, count, dryGain, wetGain, Mixed);
                else
                    for (std::size_t i = 0; i < count; ++i)
                        destination[offset + i] = publish(source[offset + i], s.foldOutput[i]);
                previous = s.foldInput[count - 1];
                active = true;
                offset += count;
            }
            s.previous[ch] = previous;
            s.active[ch] = active;
            s.primitiveValid[ch] = 0;
        }
        return Status::ok;
    }
    // Select the curve once, allowing each inner loop to inline its primitive.
    if (!p.antialias && p.shape != AdaaShape::hysteresis) {
        const auto directRun = [&]<AdaaShape type>() noexcept {
            for (std::size_t ch = 0; ch < in.channels(); ++ch) {
                const auto input = in.channel(ch);
                auto output = out.channel(ch);
                if (input.empty())
                    continue;
                const double last = driven(input.back());
                if constexpr (type == AdaaShape::hardClip) {
                    bool identity = false;
                    if constexpr (!Mixed)
                        identity = p.drive == 1 && p.bias == 0 && p.outputGain == 1;
                    if (identity) {
                        for (std::size_t i = 0; i < input.size(); ++i)
                            output[i] = std::clamp(input[i] + T{0}, T{-1}, T{1});
                    } else {
                        for (std::size_t i = 0; i < input.size(); ++i) {
                            const double x = driven(input[i]);
                            output[i] = publish(input[i], (shape(type, x) - dc) * p.outputGain);
                        }
                    }
                } else {
                    for (std::size_t i = 0; i < input.size(); ++i) {
                        const double x = driven(input[i]);
                        output[i] = publish(input[i], (shape(type, x) - dc) * p.outputGain);
                    }
                }
                s.previous[ch] = last;
                s.primitiveValid[ch] = false;
                s.active[ch] = true;
            }
            return;
        };
        switch (p.shape) {
        case AdaaShape::atan:
            directRun.template operator()<AdaaShape::atan>();
            break;
        case AdaaShape::cubic:
            directRun.template operator()<AdaaShape::cubic>();
            break;
        case AdaaShape::hardClip:
            directRun.template operator()<AdaaShape::hardClip>();
            break;
        case AdaaShape::wavefolder:
            directRun.template operator()<AdaaShape::wavefolder>();
            break;
        default:
            directRun.template operator()<AdaaShape::tanh>();
            break;
        }
        return Status::ok;
    }
    const auto run = [&]<AdaaShape type>() noexcept {
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            auto previous = s.previous[ch], previousPrimitive = s.previousPrimitive[ch];
            bool active = s.active[ch] != 0, primitiveValid = s.primitiveValid[ch] != 0;
            auto memory = s.memory[ch];
            const auto input = in.channel(ch);
            auto output = out.channel(ch);
            for (std::size_t i = 0; i < in.frames(); ++i) {
                const double x = driven(input[i]);
                double y;
                if constexpr (type == AdaaShape::hysteresis) {
                    memory = std::clamp(memory, x - p.hysteresisWidth, x + p.hysteresisWidth);
                    y = std::tanh(x + p.hysteresisAmount * memory) - dc;
                    primitiveValid = false;
                } else if (p.antialias && active) {
                    const double delta = x - previous;
                    if (std::abs(delta) > 1e-5 * std::max({1., std::abs(x), std::abs(previous)})) {
                        const auto currentPrimitive = primitive(type, x);
                        if (!primitiveValid)
                            previousPrimitive = primitive(type, previous);
                        y = (currentPrimitive - previousPrimitive) / delta;
                        previousPrimitive = currentPrimitive;
                        primitiveValid = true;
                    } else {
                        y = shape(type, .5 * (x + previous));
                        // Avoid evaluating an expensive primitive for a constant segment.
                        primitiveValid = false;
                    }
                    y -= dc;
                } else {
                    y = shape(type, x) - dc;
                    primitiveValid = false;
                }
                previous = x;
                active = true;
                output[i] = publish(input[i], y * p.outputGain);
            }
            s.previous[ch] = previous;
            s.previousPrimitive[ch] = previousPrimitive;
            s.primitiveValid[ch] = primitiveValid;
            s.active[ch] = active;
            s.memory[ch] = memory;
        }
    };
    switch (p.shape) {
    case AdaaShape::atan:
        run.template operator()<AdaaShape::atan>();
        break;
    case AdaaShape::cubic:
        run.template operator()<AdaaShape::cubic>();
        break;
    case AdaaShape::hardClip:
        run.template operator()<AdaaShape::hardClip>();
        break;
    case AdaaShape::wavefolder:
        run.template operator()<AdaaShape::wavefolder>();
        break;
    case AdaaShape::hysteresis:
        run.template operator()<AdaaShape::hysteresis>();
        break;
    default:
        run.template operator()<AdaaShape::tanh>();
        break;
    }
    return Status::ok;
}
template <Sample T> void AdaaSaturator<T>::reset() noexcept {
    if (state_) {
        std::fill(state_->previous.begin(), state_->previous.end(), 0);
        std::fill(state_->memory.begin(), state_->memory.end(), 0);
        std::fill(state_->active.begin(), state_->active.end(), static_cast<unsigned char>(0));
        std::fill(state_->primitiveValid.begin(), state_->primitiveValid.end(),
                  static_cast<unsigned char>(0));
    }
}

template <Sample T> struct DynamicEqualizer<T>::State {
    struct Band {
        DynamicEqBand p;
        BiquadCoefficients coefficients;
        double attack = 0, release = 0;
        std::vector<SectionState> audio, detector;
        std::vector<double> envelope;
    };
    ProcessSpec spec;
    VectorOps ops;
    std::array<double, 64> signal{}, envelope{}, weight{};
    std::vector<Band> bands;
    std::size_t active = 0;
};
template <Sample T> DynamicEqualizer<T>::DynamicEqualizer() = default;
template <Sample T> DynamicEqualizer<T>::~DynamicEqualizer() = default;
template <Sample T> DynamicEqualizer<T>::DynamicEqualizer(DynamicEqualizer&&) noexcept = default;
template <Sample T>
DynamicEqualizer<T>& DynamicEqualizer<T>::operator=(DynamicEqualizer&&) noexcept = default;
template <Sample T>
Status DynamicEqualizer<T>::prepare(const ProcessSpec& spec, std::size_t maximum) noexcept {
    if (!detail::validAnalysisSpec(spec) || !maximum || maximum > 16)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        const auto status = VectorOps::create(CpuBackend::automatic, s->ops);
        if (status != Status::ok)
            return status;
        s->bands.resize(maximum);
        for (auto& b : s->bands) {
            b.audio.resize(spec.channels);
            b.detector.resize(spec.channels);
            b.envelope.resize(spec.channels);
        }
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status DynamicEqualizer<T>::setBands(std::span<const DynamicEqBand> bands) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (bands.size() > s.bands.size())
        return Status::invalid_argument;
    std::array<BiquadCoefficients, 16> coefficients{};
    for (std::size_t i = 0; i < bands.size(); ++i) {
        const auto& p = bands[i];
        if (!isFinite(p.thresholdDb) || p.thresholdDb < -160 || p.thresholdDb > 24 ||
            !isFinite(p.rangeDb) || std::abs(p.rangeDb) > 48 || !isFinite(p.staticGainDb) ||
            std::abs(p.staticGainDb) > 36 || !isFinite(p.attackMs) || p.attackMs < 0 ||
            p.attackMs > 10000 || !isFinite(p.releaseMs) || p.releaseMs < 0 || p.releaseMs > 10000)
            return Status::invalid_argument;
        auto status = designBiquad(BiquadType::bandPass, s.spec.sampleRate, p.frequencyHz, p.q,
                                   coefficients[i]);
        if (status != Status::ok)
            return status;
    }
    for (std::size_t i = 0; i < bands.size(); ++i) {
        auto& b = s.bands[i];
        b.p = bands[i];
        b.coefficients = coefficients[i];
        b.attack = pole(b.p.attackMs, s.spec.sampleRate);
        b.release = pole(b.p.releaseMs, s.spec.sampleRate);
    }
    s.active = bands.size();
    return Status::ok;
}
template <Sample T>
Status DynamicEqualizer<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    return process(in, in, out);
}
template <Sample T>
Status DynamicEqualizer<T>::process(AudioBlock<const T> in, AudioBlock<const T> side,
                                    AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, in, out);
    if (status != Status::ok)
        return status;
    status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    auto spec = s.spec;
    spec.channels = side.channels();
    if (side.channels() != 1 && side.channels() != in.channels())
        return Status::channel_mismatch;
    status = detail::validateAnalysis(spec, side);
    if (status != Status::ok)
        return status;
    if (side.frames() != in.frames())
        return Status::invalid_argument;
    const bool same = side.channelData() == in.channelData() &&
                      side.frameOffset() == in.frameOffset() && side.channels() == in.channels();
    if (!same) {
        status = validateDisjointBlocks(side, out);
        if (status != Status::ok)
            return status;
    }
    if (const auto weights = detail::VectorAccess::table(s.ops).dynamicEqGain) {
        // Each band's detector reads only the sidechain, so its filter and
        // envelope recurrences can run over a whole tile before the cascaded
        // audio corrections. The decibel weight of the tile is one vector pass;
        // every recurrence keeps its original sample order and arithmetic.
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            const auto* source = in.channel(ch).data();
            const auto* detector = side.channel(side.channels() == 1 ? 0 : ch).data();
            auto* destination = out.channel(ch).data();
            for (std::size_t offset = 0; offset < in.frames();) {
                const auto count = std::min(s.signal.size(), in.frames() - offset);
                for (std::size_t i = 0; i < count; ++i)
                    s.signal[i] = source[offset + i];
                for (std::size_t b = 0; b < s.active; ++b) {
                    auto& band = s.bands[b];
                    auto filter = band.detector[ch];
                    double envelope = band.envelope[ch];
                    for (std::size_t i = 0; i < count; ++i) {
                        const double level =
                            std::abs(filter.run(double(detector[offset + i]), band.coefficients));
                        envelope = level + (level > envelope ? band.attack : band.release) *
                                               (envelope - level);
                        s.envelope[i] = envelope;
                    }
                    band.detector[ch] = filter;
                    band.envelope[ch] = envelope;
                    weights(s.envelope.data(), s.weight.data(), count, band.p.thresholdDb,
                            band.p.staticGainDb, band.p.rangeDb);
                    auto audio = band.audio[ch];
                    for (std::size_t i = 0; i < count; ++i)
                        s.signal[i] += s.weight[i] * audio.run(s.signal[i], band.coefficients);
                    band.audio[ch] = audio;
                }
                for (std::size_t i = 0; i < count; ++i)
                    destination[offset + i] = T(s.signal[i]);
                offset += count;
            }
        }
        return Status::ok;
    }
    for (std::size_t ch = 0; ch < in.channels(); ++ch)
        for (std::size_t i = 0; i < in.frames(); ++i) {
            double x = in.channel(ch)[i];
            const double detector = side.channel(side.channels() == 1 ? 0 : ch)[i];
            for (std::size_t b = 0; b < s.active; ++b) {
                auto& band = s.bands[b];
                const double level = std::abs(band.detector[ch].run(detector, band.coefficients));
                auto& env = band.envelope[ch];
                env = level + (level > env ? band.attack : band.release) * (env - level);
                const double over = 20 * std::log10(std::max(1e-12, env)) - band.p.thresholdDb;
                const double db =
                    band.p.staticGainDb + band.p.rangeDb * std::clamp(over / 24., 0., 1.);
                x += (std::pow(10., db / 20) - 1) * band.audio[ch].run(x, band.coefficients);
            }
            out.channel(ch)[i] = T(x);
        }
    return Status::ok;
}
template <Sample T> void DynamicEqualizer<T>::reset() noexcept {
    if (state_)
        for (auto& b : state_->bands) {
            std::fill(b.audio.begin(), b.audio.end(), SectionState{});
            std::fill(b.detector.begin(), b.detector.end(), SectionState{});
            std::fill(b.envelope.begin(), b.envelope.end(), 0);
        }
}

template <Sample T> struct TransientShaper<T>::State {
    ProcessSpec spec;
    TransientParameters p;
    VectorOps ops;
    std::array<double, 64> fastTile{}, slowTile{}, gainTile{};
    double fast = 0, slow = 0, release = 0;
    std::vector<double> f, s;
};
template <Sample T> TransientShaper<T>::TransientShaper() = default;
template <Sample T> TransientShaper<T>::~TransientShaper() = default;
template <Sample T> TransientShaper<T>::TransientShaper(TransientShaper&&) noexcept = default;
template <Sample T>
TransientShaper<T>& TransientShaper<T>::operator=(TransientShaper&&) noexcept = default;
template <Sample T> Status TransientShaper<T>::prepare(const ProcessSpec& spec) noexcept {
    if (!detail::validAnalysisSpec(spec))
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        const auto status = VectorOps::create(CpuBackend::automatic, s->ops);
        if (status != Status::ok)
            return status;
        s->f.resize(spec.channels);
        s->s.resize(spec.channels);
        s->fast = pole(1, spec.sampleRate);
        s->slow = pole(30, spec.sampleRate);
        s->release = pole(150, spec.sampleRate);
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status TransientShaper<T>::setParameters(const TransientParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!isFinite(p.attack) || std::abs(p.attack) > 1 || !isFinite(p.sustain) ||
        std::abs(p.sustain) > 1 || !isFinite(p.fastMs) || p.fastMs < 0 || p.fastMs > 1000 ||
        !isFinite(p.slowMs) || p.slowMs < p.fastMs || p.slowMs > 10000 || !isFinite(p.releaseMs) ||
        p.releaseMs < 0 || p.releaseMs > 10000 || !isFinite(p.maximumGainDb) ||
        p.maximumGainDb < 0 || p.maximumGainDb > 48 ||
        unsigned(p.math) > unsigned(MathAccuracy::fast))
        return Status::invalid_argument;
    auto& s = *state_;
    if (s.p.fastMs != p.fastMs)
        s.fast = pole(p.fastMs, s.spec.sampleRate);
    if (s.p.slowMs != p.slowMs)
        s.slow = pole(p.slowMs, s.spec.sampleRate);
    if (s.p.releaseMs != p.releaseMs)
        s.release = pole(p.releaseMs, s.spec.sampleRate);
    s.p = p;
    return Status::ok;
}
template <Sample T>
Status TransientShaper<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    return processImpl(in, out, false);
}
template <Sample T>
Status TransientShaper<T>::processGain(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    return processImpl(in, out, true);
}
template <Sample T>
Status TransientShaper<T>::processImpl(AudioBlock<const T> in, AudioBlock<T> out,
                                       bool gainOnly) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, in, out);
    if (status != Status::ok)
        return status;
    status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    const auto p = s.p;
    const auto fast = s.fast, slowCoefficient = s.slow, release = s.release;
    const auto frames = in.frames();
    if (const auto curve = detail::VectorAccess::table(s.ops).transientGain;
        curve && p.math == MathAccuracy::fast) {
        // The two envelope recurrences stay scalar and ordered. The gain law of a
        // tile (two quotients, the decibel mix and its exponential) is one
        // double-precision vector pass followed by a vector gain application.
        // Precise keeps the reference decimal power below, bit for bit.
        const auto& kernels = detail::VectorAccess::kernels<T>(s.ops);
        const std::array<double, 3> parameters{p.attack, p.sustain, p.maximumGainDb};
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            auto fastEnvelope = s.f[ch], slowEnvelope = s.s[ch];
            const auto* source = in.channel(ch).data();
            auto* destination = out.channel(ch).data();
            for (std::size_t offset = 0; offset < frames;) {
                const auto count = std::min(s.fastTile.size(), frames - offset);
                for (std::size_t i = 0; i < count; ++i) {
                    const double level = std::abs(double(source[offset + i]));
                    fastEnvelope =
                        level + (level > fastEnvelope ? fast : release) * (fastEnvelope - level);
                    slowEnvelope = level + (level > slowEnvelope ? slowCoefficient : release) *
                                               (slowEnvelope - level);
                    s.fastTile[i] = fastEnvelope;
                    s.slowTile[i] = slowEnvelope;
                }
                curve(s.fastTile.data(), s.slowTile.data(), s.gainTile.data(), count,
                      parameters.data());
                if (gainOnly) {
                    if (kernels.saturationPublish)
                        kernels.saturationPublish(source + offset, s.gainTile.data(),
                                                  destination + offset, count, 0, 1, false);
                    else
                        for (std::size_t i = 0; i < count; ++i)
                            destination[offset + i] = T(s.gainTile[i]);
                } else if (kernels.applyGains)
                    kernels.applyGains(source + offset, s.gainTile.data(), destination + offset,
                                       count, 1, std::numeric_limits<double>::infinity(),
                                       source + offset, 0, 1, false);
                else
                    for (std::size_t i = 0; i < count; ++i)
                        destination[offset + i] = T(double(source[offset + i]) * s.gainTile[i]);
                offset += count;
            }
            s.f[ch] = fastEnvelope;
            s.s[ch] = slowEnvelope;
        }
        return Status::ok;
    }
    const auto run = [&]<bool Fast, bool GainOnly>() noexcept {
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            auto fastEnvelope = s.f[ch], slowEnvelope = s.s[ch];
            const auto* source = in.channel(ch).data();
            auto* destination = out.channel(ch).data();
            for (std::size_t i = 0; i < frames; ++i) {
                const double x = source[i], level = std::abs(x);
                fastEnvelope =
                    level + (level > fastEnvelope ? fast : release) * (fastEnvelope - level);
                slowEnvelope = level + (level > slowEnvelope ? slowCoefficient : release) *
                                           (slowEnvelope - level);
                const double attack = std::clamp((fastEnvelope - slowEnvelope) /
                                                     std::max(1e-12, fastEnvelope),
                                                 0., 1.),
                             sustain =
                                 std::clamp(slowEnvelope / std::max(1e-12, fastEnvelope), 0., 1.) *
                                 (1 - attack);
                const double db =
                    p.maximumGainDb * std::clamp(p.attack * attack + p.sustain * sustain, -1., 1.);
                double gain;
                if constexpr (Fast)
                    gain = detail::audioExp(db * (std::numbers::ln10 / 20));
                else
                    // pow(10, +/-0) is exactly one: neutral settings and silence
                    // skip the library call without changing any result.
                    gain = db == 0 ? 1. : std::pow(10., db / 20);
                if constexpr (GainOnly)
                    destination[i] = T(gain);
                else
                    destination[i] = T(x * gain);
            }
            s.f[ch] = fastEnvelope;
            s.s[ch] = slowEnvelope;
        }
    };
    if (p.math == MathAccuracy::fast) {
        if (gainOnly)
            run.template operator()<true, true>();
        else
            run.template operator()<true, false>();
    } else {
        if (gainOnly)
            run.template operator()<false, true>();
        else
            run.template operator()<false, false>();
    }
    return Status::ok;
}
template <Sample T> void TransientShaper<T>::reset() noexcept {
    if (state_) {
        std::fill(state_->f.begin(), state_->f.end(), 0);
        std::fill(state_->s.begin(), state_->s.end(), 0);
    }
}

template <Sample T> struct MultibandDynamics<T>::State {
    ProcessSpec spec;
    FirFilterBank<T> bank;
    AudioBuffer<T> buffer;
    std::vector<Dynamics<T>> dynamics;
    std::vector<AdaaSaturator<T>> saturation;
    std::vector<MultibandMode> modes;
};
template <Sample T> MultibandDynamics<T>::MultibandDynamics() = default;
template <Sample T> MultibandDynamics<T>::~MultibandDynamics() = default;
template <Sample T> MultibandDynamics<T>::MultibandDynamics(MultibandDynamics&&) noexcept = default;
template <Sample T>
MultibandDynamics<T>& MultibandDynamics<T>::operator=(MultibandDynamics&&) noexcept = default;
template <Sample T>
Status MultibandDynamics<T>::prepare(const ProcessSpec& spec, std::span<const double> crossovers,
                                     std::size_t taps, CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || crossovers.size() > 15 ||
        (crossovers.size() + 1) * spec.channels > 256)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        auto status = s->bank.prepare(spec, crossovers, taps, backend);
        if (status != Status::ok)
            return status;
        const auto n = s->bank.bands();
        status = s->buffer.resize(n * spec.channels, spec.maxBlockFrames);
        if (status != Status::ok)
            return status;
        s->dynamics.resize(n);
        s->saturation.resize(n);
        s->modes.resize(n);
        for (std::size_t i = 0; i < n; ++i) {
            status = s->dynamics[i].prepare(spec, DynamicsMode::compressor, 0);
            if (status != Status::ok)
                return status;
            status = s->saturation[i].prepare(spec);
            if (status != Status::ok)
                return status;
        }
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status MultibandDynamics<T>::setBand(std::size_t index, MultibandMode mode,
                                     const DynamicsParameters& d,
                                     const SaturationParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (index >= state_->modes.size() || unsigned(mode) > unsigned(MultibandMode::saturator) ||
        !validSaturation(p))
        return Status::invalid_argument;
    auto status = state_->dynamics[index].setParameters(d);
    if (status != Status::ok)
        return status;
    (void)state_->saturation[index].setParameters(p);
    state_->modes[index] = mode;
    return Status::ok;
}
template <Sample T>
Status MultibandDynamics<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, in, out);
    if (status != Status::ok)
        return status;
    auto bands = s.buffer.block(in.frames());
    status = s.bank.process(in, bands);
    if (status != Status::ok)
        return status;
    std::array<T*, 64> pointers{};
    for (std::size_t i = 0; i < s.modes.size(); ++i) {
        for (std::size_t ch = 0; ch < s.spec.channels; ++ch)
            pointers[ch] = bands.channel(i * s.spec.channels + ch).data();
        AudioBlock<T> block{pointers.data(), s.spec.channels, in.frames()};
        if (s.modes[i] == MultibandMode::compressor)
            status = s.dynamics[i].process(block, block);
        else if (s.modes[i] == MultibandMode::saturator)
            status = s.saturation[i].process(block, block);
        if (status != Status::ok)
            return status;
    }
    return s.bank.sumBands(bands, out);
}
template <Sample T> void MultibandDynamics<T>::reset() noexcept {
    if (state_) {
        state_->bank.reset();
        for (auto& d : state_->dynamics)
            d.reset();
        for (auto& s : state_->saturation)
            s.reset();
    }
}
template <Sample T> std::size_t MultibandDynamics<T>::latencyFrames() const noexcept {
    return state_ ? state_->bank.latencyFrames() : 0;
}

template <Sample T> struct TruePeakLimiter<T>::State {
    ProcessSpec spec;
    Oversampler<T> oversampler;
    Dynamics<T> limiter;
    double lookAhead = 0;
    unsigned factor = 4;
};
template <Sample T> TruePeakLimiter<T>::TruePeakLimiter() = default;
template <Sample T> TruePeakLimiter<T>::~TruePeakLimiter() = default;
template <Sample T> TruePeakLimiter<T>::TruePeakLimiter(TruePeakLimiter&&) noexcept = default;
template <Sample T>
TruePeakLimiter<T>& TruePeakLimiter<T>::operator=(TruePeakLimiter&&) noexcept = default;
template <Sample T>
Status TruePeakLimiter<T>::prepare(const ProcessSpec& spec, unsigned factor, double ahead,
                                   CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || (factor != 4 && factor != 8) || !isFinite(ahead) ||
        ahead < 0 || ahead > 100)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->lookAhead = ahead;
        s->factor = factor;
        auto status = s->oversampler.prepare(spec, factor, backend);
        if (status != Status::ok)
            return status;
        auto high = spec;
        high.sampleRate *= factor;
        high.maxBlockFrames *= factor;
        status = s->limiter.prepare(high, DynamicsMode::limiter, ahead);
        if (status != Status::ok)
            return status;
        DynamicsParameters p;
        p.attackMs = 0;
        p.releaseMs = 100;
        p.thresholdDb = -1.5;
        p.kneeDb = 0;
        p.lookAheadMs = ahead;
        p.ratio = 1000;
        status = s->limiter.setParameters(p);
        if (status != Status::ok)
            return status;
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status TruePeakLimiter<T>::setParameters(double ceiling, double release, double margin) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!isFinite(ceiling) || ceiling < -60 || ceiling > 0 || !isFinite(release) || release < 0 ||
        release > 10000 || !isFinite(margin) || margin < 0 || margin > 12)
        return Status::invalid_argument;
    DynamicsParameters p;
    p.attackMs = 0;
    p.releaseMs = release;
    p.thresholdDb = ceiling - margin;
    p.kneeDb = 0;
    p.lookAheadMs = state_->lookAhead;
    p.ratio = 1000;
    return state_->limiter.setParameters(p);
}
template <Sample T>
Status TruePeakLimiter<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, in, out);
    if (status != Status::ok)
        return status;
    status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    AudioBlock<T> high;
    status = s.oversampler.processUp(in, high);
    if (status != Status::ok)
        return status;
    status = s.limiter.process(high, high);
    if (status != Status::ok)
        return status;
    return s.oversampler.processDown(out);
}
template <Sample T> void TruePeakLimiter<T>::reset() noexcept {
    if (state_) {
        state_->oversampler.reset();
        state_->limiter.reset();
    }
}
template <Sample T> double TruePeakLimiter<T>::latencyFrames() const noexcept {
    return state_ ? double(state_->oversampler.latencyFrames()) +
                        double(state_->limiter.latencyFrames()) / state_->factor
                  : 0;
}
template class AdaaSaturator<float>;
template class AdaaSaturator<double>;
template class DynamicEqualizer<float>;
template class DynamicEqualizer<double>;
template class TransientShaper<float>;
template class TransientShaper<double>;
template class MultibandDynamics<float>;
template class MultibandDynamics<double>;
template class TruePeakLimiter<float>;
template class TruePeakLimiter<double>;
} // namespace cydsp
