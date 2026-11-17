// Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
#include "cydsp/spatial/NeuralHrtf.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/math/VectorOps.h"
#include "cydsp/processors/MultiTapDelay.h"
#include "detail/Alias.h"
#include "detail/VectorTable.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <numbers>
namespace cydsp {
namespace {
constexpr double pi = std::numbers::pi;
bool unitDirection(Vec3 v, Vec3& out) noexcept {
    const double norm = std::hypot(v.x, v.y, v.z);
    if (!isFinite(norm) || norm <= 0)
        return false;
    out = {v.x / norm, v.y / norm, v.z / norm};
    return true;
}
struct Reader {
    std::span<const std::byte> bytes;
    std::size_t position = 0;
    bool valid = true;
    std::uint32_t integer() noexcept {
        if (position > bytes.size() || bytes.size() - position < 4) {
            valid = false;
            return 0;
        }
        std::uint32_t v = 0;
        for (unsigned i = 0; i < 4; ++i)
            v |= std::uint32_t(std::to_integer<unsigned char>(bytes[position++])) << (8 * i);
        return v;
    }
    float number() noexcept {
        return std::bit_cast<float>(integer());
    }
};
struct Layer {
    std::size_t inputs = 0, outputs = 0;
    std::vector<float> weights, bias, columns;
    void read(Reader& r, std::size_t in, std::size_t out) {
        inputs = in;
        outputs = out;
        weights.resize(in * out);
        bias.resize(out);
        for (auto& x : weights)
            x = r.number();
        for (auto& x : bias)
            x = r.number();
    }
    void prepareColumns() {
        columns.resize(weights.size());
        for (std::size_t row = 0; row < outputs; ++row)
            for (std::size_t column = 0; column < inputs; ++column)
                columns[column * outputs + row] = weights[row * inputs + column];
        std::vector<float>().swap(weights);
    }
    void apply(std::span<const float> in, std::span<float> out, const VectorOps& ops,
               bool activate) const noexcept {
        (void)ops.copy(std::span<const float>(bias), out);
        for (std::size_t column = 0; column < inputs; ++column)
            (void)ops.addScaled({columns.data() + column * outputs, outputs}, out, in[column]);
        if (activate)
            for (auto& value : out)
                value = std::tanh(value);
    }
};
bool validLayer(const Layer& l) noexcept {
    for (const auto x : l.weights)
        if (!isFinite(x) || std::abs(x) > 10000)
            return false;
    for (const auto x : l.bias)
        if (!isFinite(x) || std::abs(x) > 10000)
            return false;
    return true;
}
} // namespace
struct NeuralHrtfModel::State {
    NeuralHrtfInfo info;
    Layer first, second, last;
    std::array<float, neuralHrtfMaxSections> centers{}, shifts{};
    std::array<float, 6> phases{};
    VectorOps ops;
    void encode(Vec3 dir, float* encoded) const noexcept {
        const std::array<float, 3> xyz{float(dir.x), float(dir.y), float(dir.z)};
        const auto bands = info.featureBands;
        std::copy(xyz.begin(), xyz.end(), encoded);
        for (std::size_t axis = 0; axis < 3; ++axis)
            for (std::size_t band = 0; band < bands; ++band) {
                const auto i = axis * bands + band;
                const float phase = xyz[axis] * phases[band];
                encoded[3 + i] = std::sin(phase);
                encoded[3 + 3 * bands + i] = std::cos(phase);
            }
    }
    void decode(const float* raw, NeuralHrtfParameters& out) const noexcept {
        const auto filters = info.sectionsPerEar;
        NeuralHrtfParameters p;
        p.sampleRate = info.sampleRate;
        p.sectionCount = filters;
        for (std::size_t ear = 0; ear < 2; ++ear) {
            const auto base = ear * (filters * 3 + 1);
            for (std::size_t i = 0; i < filters; ++i) {
                const auto j = base + 3 * i;
                p.sections[ear][i] = {centers[i] * std::exp(shifts[i] * std::tanh(raw[j])),
                                      .3f + 11.7f / (1.f + std::exp(-raw[j + 1])),
                                      24.f * std::tanh(raw[j + 2])};
            }
            p.levelDb[ear] = 30.f * std::tanh(raw[base + filters * 3]) + info.magnitudeOffsetDb;
        }
        p.itdMicroseconds = 1200.f * std::tanh(raw[last.outputs - 1]);
        out = p;
    }
};
Status NeuralHrtfModel::load(std::span<const std::byte> bytes, CpuBackend backend) noexcept {
    constexpr std::array<unsigned char, 8> magic{'C', 'Y', 'H', 'R', 'T', 'F', '0', '2'};
    if (bytes.size() < 36 || bytes.size() > 2 * 1024 * 1024)
        return Status::unsupported_format;
    for (std::size_t i = 0; i < magic.size(); ++i)
        if (std::to_integer<unsigned char>(bytes[i]) != magic[i])
            return Status::unsupported_format;
    Reader r{bytes, 8};
    const auto inputs = r.integer(), width = r.integer(), outputs = r.integer(),
               filters = r.integer(), bands = r.integer(), rate = r.integer();
    const float offset = r.number();
    if (!bands || bands > 6 || inputs != 3 + 6 * bands || width < 16 || width > 256 ||
        filters < 4 || filters > neuralHrtfMaxSections || outputs != 2 * (3 * filters + 1) + 1 ||
        rate < 44100 || rate > 192000 || !isFinite(offset) || offset < -140 || offset > 100)
        return Status::unsupported_format;
    const std::size_t parameters =
        inputs * width + width + width * width + width + width * outputs + outputs;
    if (bytes.size() != 36 + 4 * (parameters + 2 * filters + bands))
        return Status::unsupported_format;
    try {
        auto s = std::make_shared<State>();
        auto status = VectorOps::create(backend, s->ops);
        if (status != Status::ok)
            return status;
        s->info = {width, bands, filters, parameters, double(rate), offset, s->ops.backend()};
        s->first.read(r, inputs, width);
        s->second.read(r, width, width);
        s->last.read(r, width, outputs);
        for (std::size_t i = 0; i < filters; ++i)
            s->centers[i] = r.number();
        for (std::size_t i = 0; i < filters; ++i)
            s->shifts[i] = r.number();
        for (std::size_t i = 0; i < bands; ++i)
            s->phases[i] = r.number();
        if (!r.valid || !validLayer(s->first) || !validLayer(s->second) || !validLayer(s->last))
            return Status::unsupported_format;
        for (std::size_t i = 0; i < filters; ++i) {
            const double c = s->centers[i], shift = s->shifts[i];
            if (!isFinite(c) || !isFinite(shift) || c < 20 || shift < 0 || shift > 1 ||
                c * std::exp(shift) >= rate * .5)
                return Status::unsupported_format;
        }
        for (std::size_t i = 0; i < bands; ++i)
            if (!isFinite(s->phases[i]) ||
                std::abs(double(s->phases[i]) - std::ldexp(pi, int(i))) > 1e-5)
                return Status::unsupported_format;
        s->first.prepareColumns();
        s->second.prepareColumns();
        s->last.prepareColumns();
        state_ = std::move(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
NeuralHrtfInfo NeuralHrtfModel::info() const noexcept {
    return state_ ? state_->info : NeuralHrtfInfo{};
}
Status NeuralHrtfModel::predict(Vec3 dir, NeuralHrtfParameters& out) const noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!unitDirection(dir, dir))
        return Status::invalid_argument;
    const auto& s = *state_;
    std::array<float, 39> encoded{};
    std::array<float, 256> first{}, second{};
    std::array<float, 195> raw{};
    const auto width = s.info.width;
    s.encode(dir, encoded.data());
    s.first.apply({encoded.data(), s.first.inputs}, {first.data(), width}, s.ops, true);
    s.second.apply({first.data(), width}, {second.data(), width}, s.ops, true);
    s.last.apply({second.data(), width}, {raw.data(), s.last.outputs}, s.ops, false);
    s.decode(raw.data(), out);
    return Status::ok;
}
Status NeuralHrtfModel::predict(std::span<const Vec3> dirs,
                                std::span<NeuralHrtfParameters> out) const noexcept {
    if (!state_)
        return Status::not_prepared;
    if (dirs.size() != out.size())
        return Status::invalid_argument;
    Vec3 normalized;
    for (const auto d : dirs)
        if (!unitDirection(d, normalized))
            return Status::invalid_argument;
    if (detail::overlaps(dirs.data(), dirs.size_bytes(), out.data(), out.size_bytes()))
        return Status::partial_overlap;
    for (std::size_t i = 0; i < dirs.size(); ++i)
        (void)predict(dirs[i], out[i]);
    return Status::ok;
}
Status designNeuralHrtf(const NeuralHrtfParameters& p, NeuralHrtfDesign& out) noexcept {
    if (!isFinite(p.sampleRate) || p.sampleRate < 44100 || p.sampleRate > 192000 ||
        !p.sectionCount || p.sectionCount > neuralHrtfMaxSections || !isFinite(p.itdMicroseconds) ||
        std::abs(p.itdMicroseconds) > 1200)
        return Status::invalid_argument;
    NeuralHrtfDesign next;
    next.sectionCount = p.sectionCount;
    next.sampleRate = p.sampleRate;
    next.desiredItdMicroseconds = p.itdMicroseconds;
    for (std::size_t ear = 0; ear < 2; ++ear) {
        if (!isFinite(p.levelDb[ear]) || p.levelDb[ear] < -170 || p.levelDb[ear] > 130)
            return Status::invalid_argument;
        next.gain[ear] = std::pow(10., p.levelDb[ear] / 20.);
        for (std::size_t i = 0; i < p.sectionCount; ++i) {
            const auto& section = p.sections[ear][i];
            if (!isFinite(section.gainDb) || std::abs(section.gainDb) > 24 ||
                !isFinite(section.q) || section.q < .299 || section.q > 12.001)
                return Status::invalid_argument;
            const auto status = designPeakingBiquad(p.sampleRate, section.frequencyHz, section.q,
                                                    section.gainDb, next.sections[ear][i]);
            if (status != Status::ok)
                return status;
        }
    }
    // Algebraically identical RBJ section phase: combine two atan2 terms into
    // one, caching section constants. Both operands have the same real part and
    // positive imaginary part, so the phase difference cannot cross +/-pi.
    std::array<std::array<double, neuralHrtfMaxSections>, 2> cosine{}, alphaSquared{}, phaseScale{};
    for (std::size_t ear = 0; ear < 2; ++ear)
        for (std::size_t i = 0; i < p.sectionCount; ++i) {
            const auto& v = p.sections[ear][i];
            const double w0 = 2 * pi * v.frequencyHz / p.sampleRate;
            const double alpha = std::sin(w0) / (2 * v.q);
            const double amplitude = std::pow(10., v.gainDb / 40.);
            cosine[ear][i] = std::cos(w0);
            alphaSquared[ear][i] = alpha * alpha;
            phaseScale[ear][i] = alpha * (amplitude - 1 / amplitude);
        }
    const auto first = std::size_t(std::ceil(200 * 4096 / p.sampleRate)),
               last = std::size_t(std::floor(1500 * 4096 / p.sampleRate));
    const double mean = double(first + last) * .5;
    double numerator = 0, denominator = 0;
    for (auto k = first; k <= last; ++k) {
        const double w = 2 * pi * double(k) / 4096, sine = std::sin(w), cosw = std::cos(w);
        double phase[2]{};
        for (std::size_t ear = 0; ear < 2; ++ear)
            for (std::size_t i = 0; i < p.sectionCount; ++i) {
                const double delta = cosw - cosine[ear][i];
                phase[ear] += std::atan2(delta * sine * phaseScale[ear][i],
                                         delta * delta + sine * sine * alphaSquared[ear][i]);
            }
        const double centered = 2 * pi * (double(k) - mean) * p.sampleRate / 4096;
        numerator += (phase[0] - phase[1]) * centered;
        denominator += centered * centered;
    }
    next.intrinsicItdMicroseconds = numerator / denominator * 1e6;
    const double residualFrames =
        (p.itdMicroseconds - next.intrinsicItdMicroseconds) * p.sampleRate / 1e6;
    next.delayFrames = {std::max(-residualFrames, 0.), std::max(residualFrames, 0.)};
    out = next;
    return Status::ok;
}

template <Sample T> struct NeuralBinaural<T>::State {
    static constexpr std::size_t groupVoices = 4, tileFrames = 128;
    struct Bank {
        // Section-major, field-major stereo lanes; compatible with CyDSP's bank kernels.
        std::array<std::array<double, 10>, neuralHrtfMaxSections> coefficients{};
        std::array<std::array<double, 4>, neuralHrtfMaxSections> states{};
        AudioBuffer<double> scratch;
        std::array<std::vector<double>, 2> history;
        std::array<ThiranDelay<double>, 2> fractional;
        std::array<std::size_t, 2> integer{};
        std::array<bool, 2> hasFraction{};
        NeuralHrtfDesign design;
        std::size_t write = 0;
        void reset() noexcept {
            for (auto& s : states)
                s.fill(0);
            for (auto& h : history)
                std::fill(h.begin(), h.end(), 0.);
            for (auto& f : fractional)
                f.reset();
            write = 0;
        }
        void configure(const NeuralHrtfDesign& d) noexcept {
            design = d;
            for (std::size_t e = 0; e < 2; ++e) {
                for (std::size_t i = 0; i < d.sectionCount; ++i) {
                    const auto& c = d.sections[e][i];
                    coefficients[i][e] = c.b0;
                    coefficients[i][2 + e] = c.b1;
                    coefficients[i][4 + e] = c.b2;
                    coefficients[i][6 + e] = c.a1;
                    coefficients[i][8 + e] = c.a2;
                }
                integer[e] = std::size_t(std::floor(d.delayFrames[e]));
                const double frac = d.delayFrames[e] - double(integer[e]);
                hasFraction[e] = frac > 1e-9;
                if (hasFraction[e])
                    (void)fractional[e].setDelay(frac);
            }
            reset();
        }
    };
    NeuralHrtfModel model;
    VectorOps ops, stereoOps;
    ProcessSpec spec;
    std::array<Bank, 2> banks;
    std::vector<T> inputHistory;
    std::size_t inputWrite = 0, inputCount = 0;
    std::size_t active = 0, remaining = 0, total = 0, maximumDelay = 0;
    bool configured = false;

    Status validate(const NeuralHrtfDesign& d, std::size_t fade) const noexcept {
        if (d.sampleRate != spec.sampleRate || !d.sectionCount ||
            d.sectionCount > model.info().sectionsPerEar || fade > (1u << 30))
            return Status::invalid_argument;
        for (std::size_t e = 0; e < 2; ++e) {
            if (!isFinite(d.gain[e]) || d.gain[e] < 0 || d.gain[e] > 1e7 ||
                !isFinite(d.delayFrames[e]) || d.delayFrames[e] < 0 ||
                d.delayFrames[e] > double(maximumDelay))
                return Status::invalid_argument;
            for (std::size_t j = 0; j < d.sectionCount; ++j)
                if (!d.sections[e][j].isStable())
                    return Status::invalid_argument;
        }
        return remaining ? Status::transition_in_progress : Status::ok;
    }
    void record(const T* input, std::size_t frames) noexcept {
        if (inputHistory.empty())
            return;
        for (std::size_t i = 0; i < frames; ++i) {
            inputHistory[inputWrite] = input[i];
            if (++inputWrite == inputHistory.size())
                inputWrite = 0;
        }
        inputCount = std::min(inputHistory.size(), inputCount + frames);
    }
    static void runBanks(const std::array<Bank*, groupVoices>& group,
                         const std::array<const T*, groupVoices>& input, std::size_t voices,
                         std::size_t frames, const State& dispatch) noexcept {
        constexpr auto maximumLanes = 2 * groupVoices;
        std::array<double, tileFrames * maximumLanes> packed;
        std::array<double, 5 * maximumLanes> coefficients;
        std::array<double, 2 * maximumLanes> states;
        const auto lanes = 2 * voices;
        std::size_t sections = 0;
        for (std::size_t v = 0; v < voices; ++v) {
            const auto* b = group[v];
            if (b)
                sections = std::max(sections, b->design.sectionCount);
            for (std::size_t f = 0; f < frames; ++f)
                for (std::size_t e = 0; e < 2; ++e)
                    packed[f * lanes + 2 * v + e] =
                        b ? double(input[v][f]) * b->design.gain[e] : 0.;
        }
        const auto kernel =
            detail::VectorAccess::kernels<double>(voices == 1 ? dispatch.stereoOps : dispatch.ops)
                .biquadBank;
        if (voices == 1 && group[0]) {
            auto& bank = *group[0];
            for (std::size_t section = 0; section < sections; ++section)
                kernel(packed.data(), 2, frames, bank.coefficients[section].data(),
                       bank.states[section].data());
        } else
            for (std::size_t section = 0; section < sections; ++section) {
                for (std::size_t v = 0; v < voices; ++v) {
                    auto* b = group[v];
                    const bool present = b && section < b->design.sectionCount;
                    for (std::size_t e = 0; e < 2; ++e) {
                        for (std::size_t field = 0; field < 5; ++field)
                            coefficients[field * lanes + 2 * v + e] =
                                present ? b->coefficients[section][2 * field + e]
                                        : (field == 0 ? 1. : 0.);
                        states[2 * v + e] = present ? b->states[section][e] : 0.;
                        states[lanes + 2 * v + e] = present ? b->states[section][2 + e] : 0.;
                    }
                }
                kernel(packed.data(), lanes, frames, coefficients.data(), states.data());
                for (std::size_t v = 0; v < voices; ++v)
                    if (auto* b = group[v]; b && section < b->design.sectionCount)
                        for (std::size_t e = 0; e < 2; ++e) {
                            b->states[section][e] = states[2 * v + e];
                            b->states[section][2 + e] = states[lanes + 2 * v + e];
                        }
            }
        for (std::size_t v = 0; v < voices; ++v) {
            auto* b = group[v];
            if (!b)
                continue;
            const auto capacity = b->history[0].size();
            auto audio = b->scratch.block(frames);
            for (std::size_t f = 0; f < frames; ++f) {
                for (std::size_t e = 0; e < 2; ++e) {
                    b->history[e][b->write] = packed[f * lanes + 2 * v + e];
                    const auto read = b->write >= b->integer[e]
                                          ? b->write - b->integer[e]
                                          : b->write + capacity - b->integer[e];
                    double sample = b->history[e][read];
                    if (b->hasFraction[e])
                        sample = b->fractional[e].processSample(sample);
                    audio.channel(e)[f] = sample;
                }
                if (++b->write == capacity)
                    b->write = 0;
            }
        }
    }
    // Replay bounded raw input under the NEW coefficients, gain and delay.
    // A finite replay approximates an infinite IIR past; it does not copy old states.
    static void warm(const std::array<State*, groupVoices>& group, std::size_t voices) noexcept {
        std::size_t frames = 0;
        std::array<Bank*, groupVoices> banks{};
        for (std::size_t v = 0; v < voices; ++v)
            if (group[v]->remaining) {
                banks[v] = &group[v]->banks[group[v]->active ^ 1u];
                frames = std::max(frames, group[v]->inputCount);
            }
        std::array<std::array<T, tileFrames>, groupVoices> history{};
        std::array<const T*, groupVoices> input{};
        for (std::size_t offset = 0; offset < frames;) {
            const auto n = std::min(tileFrames, frames - offset);
            for (std::size_t v = 0; v < voices; ++v) {
                const auto& s = *group[v];
                input[v] = history[v].data();
                for (std::size_t i = 0; i < n; ++i) {
                    const auto age = frames - offset - i;
                    history[v][i] = T{};
                    if (banks[v] && age <= s.inputCount) {
                        const auto index = s.inputWrite >= age
                                               ? s.inputWrite - age
                                               : s.inputWrite + s.inputHistory.size() - age;
                        history[v][i] = s.inputHistory[index];
                    }
                }
            }
            runBanks(banks, input, voices, n, *group[0]);
            offset += n;
        }
    }
};

template <Sample T> NeuralBinaural<T>::NeuralBinaural() = default;
template <Sample T> NeuralBinaural<T>::~NeuralBinaural() = default;
template <Sample T> NeuralBinaural<T>::NeuralBinaural(NeuralBinaural&&) noexcept = default;
template <Sample T>
NeuralBinaural<T>& NeuralBinaural<T>::operator=(NeuralBinaural&&) noexcept = default;

template <Sample T>
Status NeuralBinaural<T>::prepare(const ProcessSpec& spec, const NeuralHrtfModel& model,
                                  const NeuralBinauralOptions& options) noexcept {
    const auto info = model.info();
    if (!spec.isValid() || spec.channels != 1 || !info.sectionsPerEar ||
        spec.sampleRate != info.sampleRate)
        return Status::invalid_spec;
    if (!isFinite(options.maximumDelaySeconds) || options.maximumDelaySeconds < 0 ||
        options.maximumDelaySeconds > .1 || options.transitionHistoryFrames > 65536)
        return Status::invalid_argument;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->model = model;
        auto status = VectorOps::create(info.backend, s->ops);
        if (status != Status::ok)
            return status;
        const auto smallBackend =
            info.backend == CpuBackend::avx512 || info.backend == CpuBackend::avx2
                ? CpuBackend::sse2
                : info.backend;
        status = VectorOps::create(smallBackend, s->stereoOps);
        if (status != Status::ok)
            return status;
        s->inputHistory.resize(options.transitionHistoryFrames);
        s->maximumDelay = std::size_t(std::ceil(spec.sampleRate * options.maximumDelaySeconds));
        for (auto& bank : s->banks) {
            status = bank.scratch.resize(2, State::tileFrames);
            if (status != Status::ok)
                return status;
            for (auto& h : bank.history)
                h.resize(s->maximumDelay + 1);
        }
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}

template <Sample T>
Status NeuralBinaural<T>::setDesigns(std::span<NeuralBinaural> voices,
                                     std::span<const NeuralHrtfDesign> designs,
                                     std::size_t frames) noexcept {
    if (voices.empty() || voices.size() != designs.size())
        return Status::invalid_argument;
    for (std::size_t v = 0; v < voices.size(); ++v) {
        if (!voices[v].state_)
            return Status::not_prepared;
        const auto& s = *voices[v].state_;
        if (s.spec.sampleRate != voices[0].state_->spec.sampleRate ||
            s.ops.backend() != voices[0].state_->ops.backend())
            return Status::invalid_spec;
        const auto status = s.validate(designs[v], frames);
        if (status != Status::ok)
            return status;
    }
    for (std::size_t first = 0; first < voices.size(); first += State::groupVoices) {
        const auto count = std::min(State::groupVoices, voices.size() - first);
        std::array<State*, State::groupVoices> group{};
        for (std::size_t v = 0; v < count; ++v) {
            auto& s = *voices[first + v].state_;
            group[v] = &s;
            if (!s.configured || !frames) {
                s.banks[s.active].configure(designs[first + v]);
                s.configured = true;
            } else {
                s.banks[s.active ^ 1u].configure(designs[first + v]);
                s.remaining = s.total = frames;
            }
        }
        State::warm(group, count);
    }
    return Status::ok;
}

template <Sample T>
Status NeuralBinaural<T>::setDesign(const NeuralHrtfDesign& design, std::size_t frames) noexcept {
    return setDesigns({this, 1}, {&design, 1}, frames);
}

template <Sample T>
Status NeuralBinaural<T>::setDirection(Vec3 direction, std::size_t frames) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (state_->remaining)
        return Status::transition_in_progress;
    NeuralHrtfParameters parameters;
    auto status = state_->model.predict(direction, parameters);
    if (status != Status::ok)
        return status;
    NeuralHrtfDesign design;
    status = designNeuralHrtf(parameters, design);
    return status == Status::ok ? setDesign(design, frames) : status;
}

template <Sample T>
Status NeuralBinaural<T>::processBatch(std::span<NeuralBinaural> voices, AudioBlock<const T> in,
                                       AudioBlock<T> out) noexcept {
    if (voices.empty() || voices.size() > std::numeric_limits<std::size_t>::max() / 2)
        return Status::invalid_argument;
    if (!in.isValid() || !out.isValid())
        return Status::invalid_argument;
    if (in.channels() != voices.size() || out.channels() != 2 * voices.size())
        return Status::channel_mismatch;
    if (in.frames() != out.frames())
        return Status::invalid_argument;
    for (const auto& voice : voices) {
        if (!voice.state_ || !voice.state_->configured)
            return Status::not_prepared;
        if (in.frames() > voice.state_->spec.maxBlockFrames)
            return Status::block_too_large;
        if (voice.state_->spec.sampleRate != voices[0].state_->spec.sampleRate ||
            voice.state_->ops.backend() != voices[0].state_->ops.backend())
            return Status::invalid_spec;
    }
    const auto status = validateDisjointBlocks(in, out);
    if (status != Status::ok)
        return status;
    for (std::size_t first = 0; first < voices.size(); first += State::groupVoices) {
        const auto count = std::min(State::groupVoices, voices.size() - first);
        for (std::size_t offset = 0; offset < in.frames();) {
            auto frames = std::min(State::tileFrames, in.frames() - offset);
            std::array<typename State::Bank*, State::groupVoices> current{}, next{};
            std::array<const T*, State::groupVoices> input{};
            bool fading = false;
            for (std::size_t v = 0; v < count; ++v) {
                auto& s = *voices[first + v].state_;
                if (s.remaining)
                    frames = std::min(frames, s.remaining);
                current[v] = &s.banks[s.active];
                if (s.remaining) {
                    next[v] = &s.banks[s.active ^ 1u];
                    fading = true;
                }
                input[v] = in.channel(first + v).data() + offset;
            }
            State::runBanks(current, input, count, frames, *voices[first].state_);
            if (fading)
                State::runBanks(next, input, count, frames, *voices[first].state_);
            for (std::size_t v = 0; v < count; ++v) {
                auto& s = *voices[first + v].state_;
                const auto from = current[v]->scratch.block(frames);
                for (std::size_t i = 0; i < frames; ++i) {
                    const double blend =
                        s.remaining ? double(s.total - s.remaining + i + 1) / double(s.total) : 0.;
                    for (std::size_t ear = 0; ear < 2; ++ear) {
                        const double a = from.channel(ear)[i];
                        const double b =
                            s.remaining ? next[v]->scratch.block(frames).channel(ear)[i] : a;
                        out.channel(2 * (first + v) + ear)[offset + i] =
                            T(s.remaining ? a + (b - a) * blend : a);
                    }
                }
                s.record(input[v], frames);
                if (s.remaining) {
                    s.remaining -= frames;
                    if (!s.remaining)
                        s.active ^= 1u;
                }
            }
            offset += frames;
        }
    }
    return Status::ok;
}

template <Sample T>
Status NeuralBinaural<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    return processBatch({this, 1}, in, out);
}

template <Sample T> void NeuralBinaural<T>::reset() noexcept {
    if (!state_)
        return;
    auto& s = *state_;
    if (s.remaining)
        s.active ^= 1u;
    s.remaining = 0;
    s.inputWrite = s.inputCount = 0;
    for (auto& bank : s.banks)
        bank.reset();
}

template <Sample T> std::size_t NeuralBinaural<T>::transitionRemainingFrames() const noexcept {
    return state_ ? state_->remaining : 0;
}

template class NeuralBinaural<float>;
template class NeuralBinaural<double>;
} // namespace cydsp
