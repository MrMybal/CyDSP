#include "cydsp/synthesis/Synthesizer.h"
#include "cydsp/math/VectorOps.h"
#include "cydsp/midi/Midi2Controllers.h"
#include "cydsp/processors/ChannelMatrix.h"
#include "cydsp/processors/SincInterpolator.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
namespace cydsp {
Status validateSynthParameters(const SynthParameters& p) noexcept {
    Adsr check;
    auto status = check.setParameters(p.envelope);
    if (status != Status::ok)
        return status;
    if (!isFinite(p.gain) || p.gain < 0 || p.gain > 16 || !isFinite(p.pan) || p.pan < -1 ||
        p.pan > 1 || !isFinite(p.pressureToGain) || p.pressureToGain < 0 || p.pressureToGain > 1 ||
        p.stealFadeFrames > 1048576 ||
        static_cast<unsigned>(p.waveform) > static_cast<unsigned>(Waveform::whiteNoise) ||
        static_cast<unsigned>(p.sampleInterpolation) >
            static_cast<unsigned>(SampleInterpolation::bandLimited))
        return Status::invalid_argument;
    return Status::ok;
}
template <Sample T>
Status SampleData<T>::prepare(AudioBlock<const T> source, double rate, unsigned root,
                              CpuBackend backend) noexcept {
    if (!source.isValid() || !source.channels() || source.channels() > 256 || !source.frames() ||
        !isFinite(rate) || rate <= 0 || rate > 768000 || root > 127)
        return Status::invalid_argument;
    VectorOps ops;
    auto status = VectorOps::create(backend, ops);
    if (status != Status::ok)
        return status;
    AudioBuffer<T> next;
    status = next.resize(source.channels(), source.frames());
    if (status != Status::ok)
        return status;
    for (std::size_t ch = 0; ch < source.channels(); ++ch)
        (void)ops.copy(source.channel(ch), next.block().channel(ch));
    std::vector<AudioBuffer<T>> mips;
    try {
        AudioBlock<const T> current = next.block();
        for (unsigned level = 0; level < 16 && current.frames() >= 16; ++level) {
            const auto frames = (current.frames() + 1) / 2;
            AudioBuffer<T> padded, reduced;
            status = padded.resize(current.channels(), current.frames() + 64);
            if (status != Status::ok)
                return status;
            status = reduced.resize(current.channels(), frames);
            if (status != Status::ok)
                return status;
            for (std::size_t ch = 0; ch < current.channels(); ++ch) {
                ops.fill(padded.block().channel(ch), T{});
                (void)ops.copy(current.channel(ch),
                               padded.block().channel(ch).subspan(31, current.frames()));
            }
            SincInterpolator<T> converter;
            const auto block = std::min<std::size_t>(4096, frames);
            status = converter.prepare({48000, block, current.channels()}, 96000, 64, 256, backend);
            if (status != Status::ok)
                return status;
            for (std::size_t at = 0; at < frames; at += block) {
                status =
                    converter.process(padded.block(), 31 + double(at) * 2,
                                      reduced.block().subBlock(at, std::min(block, frames - at)));
                if (status != Status::ok)
                    return status;
            }
            mips.push_back(std::move(reduced));
            current = mips.back().block();
        }
    } catch (...) {
        return Status::allocation_failed;
    }
    audio_.swap(next);
    mips_.swap(mips);
    rate_ = rate;
    root_ = root;
    first_ = end_ = crossfade_ = 0;
    return Status::ok;
}
template <Sample T>
Status SampleData<T>::setLoop(std::size_t first, std::size_t end, std::size_t crossfade) noexcept {
    if (!first && !end && !crossfade) {
        first_ = end_ = crossfade_ = 0;
        return Status::ok;
    }
    if (first >= end || end > audio_.block().frames() || crossfade > (end - first) / 2)
        return Status::invalid_argument;
    first_ = first;
    end_ = end;
    crossfade_ = crossfade;
    return Status::ok;
}
template <Sample T>
T SampleData<T>::value(std::size_t channel, double position, double ratio,
                       SampleInterpolation mode) const noexcept {
    if (channel >= audio_.block().channels() || !isFinite(position) || position < 0 ||
        position >= double(audio_.block().frames()) || !isFinite(ratio) || ratio <= 0)
        return T{};
    std::size_t level = 0;
    double scale = 1;
    if (mode == SampleInterpolation::bandLimited)
        while (ratio / scale > 1 && level < mips_.size()) {
            ++level;
            scale *= 2;
        }
    auto data = level ? mips_[level - 1].block() : audio_.block();
    auto read = [&](double pos) {
        pos /= scale;
        const auto a = std::size_t(pos);
        if (a >= data.frames())
            return 0.0;
        const auto fraction = pos - double(a);
        auto b = a + 1;
        if (end_ && double(b) * scale >= double(end_))
            b = std::min(std::size_t(double(first_) / scale), data.frames() - 1);
        const auto next = b < data.frames() ? double(data.channel(channel)[b]) : 0;
        return double(data.channel(channel)[a]) + (next - data.channel(channel)[a]) * fraction;
    };
    double result = read(position);
    if (crossfade_ && position >= double(end_ - crossfade_) && position < double(end_)) {
        const auto offset = position - double(end_ - crossfade_), mix = offset / double(crossfade_);
        result = result * (1 - mix) + read(double(first_) + offset) * mix;
    }
    return static_cast<T>(result);
}
template <Sample T> struct Synthesizer<T>::State {
    struct Voice {
        SynthVoiceInfo info;
        Oscillator<T> oscillator;
        double wavePhase = 0;
        Adsr envelope;
        double position = 0, pitch = 0, perNoteBend = 0, baseNote = 60;
        std::array<double, 256> last{}, stolen{};
        std::size_t fade = 0;
        bool keyDown = false, fromMpe = false, detached = false, attributePitch = false;
        std::uint64_t age = 0;
    };
    struct Channel {
        double bend = 0, range = 2, perNoteRange = 48, volume = 1, expression = 1, pan = 0,
               tuningOffset = 0;
        bool sustain = false;
        std::array<double, 128> tuning{};
        std::array<bool, 128> tuned{};
    };
    ProcessSpec spec;
    SynthParameters p;
    VectorOps ops;
    AudioBuffer<T> mono, envelopes, sampleBuffer;
    ChannelMatrix<T> sampleMatrix;
    std::vector<Voice> voices;
    std::vector<MpeInstrument> mpe;
    std::array<Channel, 256> channels{};
    Midi2Controllers controllers;
    const SampleData<T>* sample = nullptr;
    const Wavetable<T>* wavetable = nullptr;
    SynthVoiceRenderer<T> renderer;
    std::uint64_t serial = 0, stolen = 0;
    void frequency(Voice& v) noexcept {
        const auto& c = channels[v.info.group * 16 + v.info.channel - 1];
        v.info.frequency =
            440 * std::exp2((v.baseNote - 69 + v.pitch + v.perNoteBend * c.perNoteRange +
                             c.bend * c.range + c.tuningOffset) /
                            12);
        (void)v.oscillator.setFrequency(std::clamp(v.info.frequency, 0.0, spec.sampleRate / 2));
    }
    void stop(Voice& v) noexcept {
        if (v.info.active && renderer.stop)
            renderer.stop(renderer.context, v.info);
        v.info.active = false;
        v.keyDown = false;
        v.envelope.reset();
    }
    Voice* start(unsigned group, unsigned channel, unsigned note, double velocity,
                 std::uint64_t id = 0, double pitch = 0, double pressure = 0, double timbre = .5,
                 double base = -1) noexcept {
        auto chosen = voices.end();
        for (auto i = voices.begin(); i != voices.end(); ++i) {
            if (!i->info.active) {
                chosen = i;
                break;
            }
            if (chosen == voices.end() || (i->info.releasing && !chosen->info.releasing) ||
                (i->info.releasing == chosen->info.releasing && i->age < chosen->age))
                chosen = i;
        }
        auto& v = *chosen;
        const auto held = v.last;
        if (v.info.active) {
            stop(v);
            ++stolen;
            v.stolen = held;
            v.fade = p.stealFadeFrames;
        } else {
            v.stolen.fill(0);
            v.fade = 0;
        }
        const auto& c = channels[group * 16 + channel - 1];
        v.info = {static_cast<std::size_t>(chosen - voices.begin()),
                  id ? id : serial + 1,
                  group,
                  channel,
                  note,
                  440,
                  velocity,
                  pressure,
                  timbre,
                  true,
                  false};
        v.age = ++serial;
        v.fromMpe = id != 0;
        v.detached = false;
        v.attributePitch = base >= 0;
        v.keyDown = true;
        v.position = 0;
        v.wavePhase = 0;
        v.pitch = pitch;
        v.perNoteBend = 0;
        v.baseNote = base >= 0 ? base : c.tuned[note] ? c.tuning[note] : note;
        v.last.fill(0);
        v.oscillator.reset();
        v.oscillator.setWaveform(p.waveform);
        v.envelope.reset();
        (void)v.envelope.setParameters(p.envelope);
        v.envelope.noteOn();
        frequency(v);
        if (renderer.start)
            renderer.start(renderer.context, v.info);
        return &v;
    }
    struct MpeContext {
        State* state;
        unsigned group;
    };
    static void mpeEvent(void* ptr, MpeChange change, const MpeNote& n, std::uint32_t) noexcept {
        auto& context = *static_cast<MpeContext*>(ptr);
        auto& s = *context.state;
        if (change == MpeChange::noteOn) {
            s.start(context.group, n.channel, n.note, n.velocity,
                    (std::uint64_t(context.group) << 60) | n.id, n.pitchSemitones, n.pressure,
                    n.timbre);
            return;
        }
        for (auto& v : s.voices)
            if (v.info.active && v.fromMpe && v.info.group == context.group &&
                v.info.channel == n.channel && v.info.note == n.note &&
                (v.info.id & 0x0fffffffffffffffull) == n.id) {
                if (change == MpeChange::allSoundOff)
                    s.stop(v);
                else if (change == MpeChange::noteOff) {
                    v.keyDown = false;
                    v.info.releasing = true;
                    v.envelope.noteOff();
                } else {
                    v.pitch = n.pitchSemitones;
                    v.info.pressure = n.pressure;
                    v.info.timbre = n.timbre;
                    s.frequency(v);
                }
            }
    }
    void midi(unsigned group, MidiMessageView m) noexcept {
        if (m.valid() && m.channel() && (m.bytes()[0] & 0xf0) == 0xb0) {
            auto& channel = channels[group * 16 + m.channel() - 1];
            const auto cc = m.bytes()[1], value = m.bytes()[2];
            if (cc == 7)
                channel.volume = static_cast<double>(value) / 127;
            else if (cc == 11)
                channel.expression = static_cast<double>(value) / 127;
            else if (cc == 10)
                channel.pan = value < 64 ? (static_cast<double>(value) - 64) / 64
                                         : (static_cast<double>(value) - 64) / 63;
            else if (cc == 121) {
                channel.volume = channel.expression = 1;
                channel.pan = 0;
            }
        }
        MpeContext context{this, group};
        (void)mpe[group].process(m, 0, &mpeEvent, &context);
    }
    Status ump(const UmpPacket& packet) noexcept {
        const auto group = packet.group(), channel = packet.channel();
        if (packet.type() == 2) {
            const std::array<std::uint8_t, 3> d{static_cast<std::uint8_t>(packet.words[0] >> 16),
                                                static_cast<std::uint8_t>(packet.words[0] >> 8),
                                                static_cast<std::uint8_t>(packet.words[0])};
            midi(group, MidiMessageView({d.data(), (d[0] & 0xe0) == 0xc0 ? 2u : 3u}));
            return Status::ok;
        }
        if (packet.type() != 4)
            return Status::ok;
        auto& c = channels[group * 16 + channel - 1];
        const auto status = packet.status(), key = (packet.words[0] >> 8) & 127,
                   value = packet.words[1];
        if (status == 9) {
            const auto attr = packet.words[0] & 255;
            const auto base = attr == 3 ? static_cast<double>(value & 65535) / 512 : -1;
            start(group, channel, key, static_cast<double>(value >> 16) / 65535, 0, 0, 0, .5, base);
            return Status::ok;
        }
        if (status == 8) {
            for (auto& v : voices)
                if (v.info.active && v.info.group == group && v.info.channel == channel &&
                    v.info.note == key && v.keyDown) {
                    v.keyDown = false;
                    if (!c.sustain) {
                        v.info.releasing = true;
                        v.envelope.noteOff();
                    }
                    break;
                }
            return Status::ok;
        }
        const auto controlStatus = controllers.process(packet);
        if (controlStatus != Status::ok)
            return controlStatus;
        if (status == 11 &&
            (key == 0 || key == 32 || key == 6 || key == 38 || (key >= 98 && key <= 101)))
            return Status::ok;
        const auto* control = controllers.channel(group, channel);
        c.range = control->pitchRange;
        c.perNoteRange = control->perNotePitchRange;
        c.tuningOffset = control->fineTuning + control->coarseTuning;
        if (status >= 2 && status <= 5) {
            for (auto& v : voices)
                if (v.info.active && v.info.group == group && v.info.channel == channel)
                    frequency(v);
            return Status::ok;
        }
        const double normalized = static_cast<double>(value) / 4294967295.0;
        if (status == 14)
            c.bend = value < 0x80000000u
                         ? (static_cast<double>(value) - 2147483648.0) / 2147483648.0
                         : (static_cast<double>(value) - 2147483648.0) / 2147483647.0;
        if (status == 0 && (packet.words[0] & 255) == 3) {
            c.tuning[key] = static_cast<double>(value) / 33554432.0;
            c.tuned[key] = true;
        }
        if (status == 15 && (packet.words[0] & 1))
            c.tuned[key] = false;
        if (status == 11) {
            if (key == 7)
                c.volume = normalized;
            else if (key == 10)
                c.pan = normalized * 2 - 1;
            else if (key == 11)
                c.expression = normalized;
            else if (key == 64)
                c.sustain = value >= 0x80000000u;
            else if (key == 121) {
                c.volume = c.expression = 1;
                c.pan = c.bend = 0;
                c.sustain = false;
            }
        }
        for (auto& v : voices)
            if (v.info.active && v.info.group == group && v.info.channel == channel) {
                if (status == 14)
                    frequency(v);
                else if (status == 6 && v.info.note == key && !v.detached) {
                    v.perNoteBend = (static_cast<double>(value) - 2147483648.0) / 2147483648.0;
                    frequency(v);
                } else if ((status == 10 && v.info.note == key && !v.detached) || status == 13)
                    v.info.pressure = normalized;
                else if ((status == 0 || status == 1) && v.info.note == key && !v.detached) {
                    const auto index = packet.words[0] & 255;
                    if (status == 0 && index == 3 && !v.attributePitch) {
                        v.baseNote = c.tuning[key];
                        frequency(v);
                    } else if (index == 74)
                        v.info.timbre = normalized;
                } else if (status == 15 && v.info.note == key) {
                    if ((packet.words[0] & 2) && !v.detached) {
                        v.pitch += v.perNoteBend * c.perNoteRange;
                        v.perNoteBend = 0;
                        v.detached = true;
                    }
                    if ((packet.words[0] & 1) && !v.detached) {
                        v.pitch = v.perNoteBend = 0;
                        if (!v.attributePitch)
                            v.baseNote = v.info.note;
                        v.info.pressure = 0;
                        v.info.timbre = .5;
                        frequency(v);
                    }
                } else if (status == 11) {
                    if (key == 121) {
                        if (!v.detached) {
                            v.pitch = v.perNoteBend = 0;
                            v.info.pressure = 0;
                            v.info.timbre = .5;
                        }
                        frequency(v);
                    }
                    if (key == 74)
                        v.info.timbre = normalized;
                    else if (key == 120)
                        stop(v);
                    else if (key == 123) {
                        v.keyDown = false;
                        if (!c.sustain) {
                            v.info.releasing = true;
                            v.envelope.noteOff();
                        }
                    } else if ((key == 64 || key == 121) && !c.sustain && !v.keyDown) {
                        v.info.releasing = true;
                        v.envelope.noteOff();
                    }
                }
            }
        return Status::ok;
    }
    Status render(AudioBlock<T> output, std::size_t first, std::size_t count) noexcept {
        if (!count)
            return Status::ok;
        for (auto& v : voices)
            if (v.info.active) {
                auto full = sample ? sampleBuffer.block(count) : mono.block(count);
                AudioBlock<T> wave(full.channelData(), sample ? sample->samples().channels() : 1,
                                   count);
                auto env = envelopes.block(count);
                const auto& c = channels[v.info.group * 16 + v.info.channel - 1];
                if (renderer.render) {
                    for (std::size_t ch = 0; ch < wave.channels(); ++ch)
                        ops.fill(wave.channel(ch), T{});
                    const auto status = renderer.render(renderer.context, v.info, wave);
                    if (status != Status::ok)
                        return status;
                } else if (!sample && wavetable) {
                    const auto frequency = std::clamp(v.info.frequency, 0.0, spec.sampleRate * .5);
                    for (std::size_t i = 0; i < count; ++i) {
                        wave.channel(0)[i] =
                            wavetable->value(v.wavePhase, frequency, spec.sampleRate);
                        v.wavePhase += frequency / spec.sampleRate;
                        v.wavePhase -= std::floor(v.wavePhase);
                    }
                } else if (!sample) {
                    const auto status = v.oscillator.process(wave);
                    if (status != Status::ok)
                        return status;
                } else {
                    const auto data = sample->samples();
                    const auto ratio = std::min(
                        1e6, sample->sampleRate() / spec.sampleRate *
                                 std::exp2((v.baseNote - static_cast<double>(sample->rootNote()) +
                                            v.pitch + v.perNoteBend * c.perNoteRange +
                                            c.bend * c.range + c.tuningOffset) /
                                           12));
                    for (std::size_t i = 0; i < count; ++i) {
                        const auto loopEnd = sample->loopEnd();
                        if (loopEnd && v.position >= static_cast<double>(loopEnd))
                            v.position = double(sample->loopStart() + sample->loopCrossfade()) +
                                         std::fmod(v.position - double(loopEnd),
                                                   double(loopEnd - sample->loopStart() -
                                                          sample->loopCrossfade()));
                        for (std::size_t ch = 0; ch < wave.channels(); ++ch) {
                            T value{};
                            if (v.position < static_cast<double>(data.frames())) {
                                value = sample->value(std::min(ch, data.channels() - 1), v.position,
                                                      ratio, p.sampleInterpolation);
                            } else if (v.keyDown) {
                                v.envelope.noteOff();
                                v.keyDown = false;
                                v.info.releasing = true;
                            }
                            wave.channel(ch)[i] = value;
                        }
                        env.channel(0)[i] = static_cast<T>(v.envelope.next());
                        v.position += ratio;
                    }
                }
                if (!sample)
                    for (std::size_t i = 0; i < count; ++i) {
                        env.channel(0)[i] = static_cast<T>(v.envelope.next());
                    }
                if (sample) {
                    const auto status = sampleMatrix.process(wave, mono.block(count));
                    if (status != Status::ok)
                        return status;
                    wave = mono.block(count);
                }
                for (std::size_t ch = 0; ch < wave.channels(); ++ch)
                    (void)ops.multiply(wave.channel(ch), env.channel(0), wave.channel(ch));
                double managerVolume = 1, managerPan = 0;
                if (v.fromMpe) {
                    const auto& zone = mpe[v.info.group];
                    const auto ch = v.info.channel;
                    const auto manager =
                        zone.lowerMembers() && ch >= 2 && ch <= zone.lowerMembers() + 1     ? 1u
                        : zone.upperMembers() && ch >= 16 - zone.upperMembers() && ch <= 15 ? 16u
                                                                                            : 0u;
                    if (manager) {
                        const auto& master = channels[v.info.group * 16 + manager - 1];
                        managerVolume = master.volume * master.expression;
                        managerPan = master.pan;
                    }
                }
                const auto velocity = v.info.velocity * p.gain * c.volume * c.expression *
                                      managerVolume *
                                      (1 - p.pressureToGain + p.pressureToGain * v.info.pressure);
                const auto pan = std::clamp(p.pan + c.pan + managerPan, -1.0, 1.0);
                const auto fadeCount = std::min(v.fade, count);
                for (std::size_t ch = 0; ch < output.channels(); ++ch) {
                    const auto gain = static_cast<T>(
                        velocity * (output.channels() == 2
                                        ? std::sqrt(ch == 0 ? (1 - pan) / 2 : (1 + pan) / 2)
                                        : 1.0));
                    const auto signal = wave.channel(wave.channels() == 1 ? 0 : ch);
                    auto destination = output.channel(ch).subspan(first, count);
                    for (std::size_t i = 0; i < fadeCount; ++i) {
                        const auto mix = 1 - static_cast<double>(v.fade - i) /
                                                 static_cast<double>(p.stealFadeFrames);
                        const auto contribution =
                            static_cast<T>(signal[i] * gain * mix + v.stolen[ch] * (1 - mix));
                        destination[i] += contribution;
                        if (i + 1 == count)
                            v.last[ch] = contribution;
                    }
                    if (fadeCount < count) {
                        (void)ops.addScaled(signal.subspan(fadeCount),
                                            destination.subspan(fadeCount), gain);
                        v.last[ch] = static_cast<T>(signal.back() * gain);
                    }
                }
                v.fade -= fadeCount;
                if (!v.envelope.active())
                    stop(v);
            }
        return Status::ok;
    }
};
template <Sample T> Synthesizer<T>::Synthesizer() = default;
template <Sample T> Synthesizer<T>::~Synthesizer() = default;
template <Sample T> Synthesizer<T>::Synthesizer(Synthesizer&&) noexcept = default;
template <Sample T> Synthesizer<T>& Synthesizer<T>::operator=(Synthesizer&&) noexcept = default;
template <Sample T>
Status Synthesizer<T>::prepare(const ProcessSpec& spec, std::size_t count,
                               CpuBackend backend) noexcept {
    if (!spec.isValid() || spec.sampleRate < 8000 || spec.sampleRate > 768000 ||
        spec.channels > 256 || !count || count > 1024)
        return Status::invalid_spec;
    try {
        auto next = std::make_unique<State>();
        next->spec = spec;
        auto status = VectorOps::create(backend, next->ops);
        if (status != Status::ok)
            return status;
        status = next->controllers.prepare();
        if (status != Status::ok)
            return status;
        next->voices.resize(count);
        next->mpe.resize(16);
        status = next->mono.resize(spec.channels, spec.maxBlockFrames);
        if (status != Status::ok)
            return status;
        status = next->envelopes.resize(1, spec.maxBlockFrames);
        if (status != Status::ok)
            return status;
        for (auto& v : next->voices) {
            status = v.oscillator.prepare({spec.sampleRate, spec.maxBlockFrames, 1});
            if (status != Status::ok)
                return status;
            status = v.envelope.prepare(spec.sampleRate);
            if (status != Status::ok)
                return status;
        }
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_spec;
    }
}
template <Sample T> Status Synthesizer<T>::setParameters(const SynthParameters& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    const auto status = validateSynthParameters(p);
    if (status != Status::ok)
        return status;
    if (p.stealFadeFrames != state_->p.stealFadeFrames)
        for (auto& v : state_->voices)
            v.fade = 0;
    state_->p = p;
    for (auto& v : state_->voices) {
        (void)v.envelope.setParameters(p.envelope);
        v.oscillator.setWaveform(p.waveform);
    }
    return Status::ok;
}
template <Sample T> Status Synthesizer<T>::setMpeZones(unsigned lower, unsigned upper) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (activeVoices())
        return Status::transition_in_progress;
    if (lower > 15 || upper > 15 || (lower && upper && lower + upper > 14))
        return Status::invalid_argument;
    for (auto& m : state_->mpe) {
        m.reset();
        (void)m.setZones(lower, upper);
    }
    return Status::ok;
}
template <Sample T> Status Synthesizer<T>::setSample(const SampleData<T>* sample) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (activeVoices())
        return Status::transition_in_progress;
    if (sample && (!sample->samples().frames() || !sample->samples().channels()))
        return Status::invalid_argument;
    if (sample) {
        const auto source = sample->samples();
        AudioBuffer<T> buffer;
        auto status = buffer.resize(source.channels(), state_->spec.maxBlockFrames);
        if (status != Status::ok)
            return status;
        try {
            std::vector<double> coefficients(source.channels() * state_->spec.channels, 0);
            status = defaultChannelMatrix(source.channels(), state_->spec.channels, coefficients);
            if (status != Status::ok)
                return status;
            ChannelMatrix<T> matrix;
            status = matrix.prepare(
                {state_->spec.sampleRate, state_->spec.maxBlockFrames, source.channels()},
                state_->spec.channels, coefficients, state_->ops.backend());
            if (status != Status::ok)
                return status;
            state_->sampleBuffer.swap(buffer);
            state_->sampleMatrix = std::move(matrix);
        } catch (const std::bad_alloc&) {
            return Status::allocation_failed;
        } catch (const std::length_error&) {
            return Status::invalid_argument;
        }
    }
    state_->sample = sample;
    state_->wavetable = nullptr;
    state_->renderer = {};
    return Status::ok;
}
template <Sample T> Status Synthesizer<T>::setRenderer(SynthVoiceRenderer<T> renderer) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (activeVoices())
        return Status::transition_in_progress;
    state_->renderer = renderer;
    state_->sample = nullptr;
    state_->wavetable = nullptr;
    return Status::ok;
}
template <Sample T> Status Synthesizer<T>::setWavetable(const Wavetable<T>* table) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (activeVoices())
        return Status::transition_in_progress;
    if (table && !table->size())
        return Status::invalid_argument;
    state_->wavetable = table;
    state_->sample = nullptr;
    state_->renderer = {};
    return Status::ok;
}
template <Sample T>
Status Synthesizer<T>::process(AudioBlock<T> out, const MidiBuffer* midi, const UmpBuffer* ump,
                               bool add) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks<T>(s.spec, out, out);
    if (status != Status::ok)
        return status;
    if (midi)
        for (std::size_t i = 0; i < midi->size(); ++i)
            if (midi->event(i).frame >= out.frames())
                return Status::invalid_argument;
    if (ump)
        for (const auto& e : ump->events())
            if (e.frame >= out.frames())
                return Status::invalid_argument;
    if (!add)
        for (std::size_t ch = 0; ch < out.channels(); ++ch)
            s.ops.fill(out.channel(ch), T{});
    std::size_t a = 0, b = 0, position = 0;
    const auto events = ump ? ump->events() : std::span<const UmpEvent>{};
    while ((midi && a < midi->size()) || b < events.size()) {
        const auto mf = midi && a < midi->size() ? midi->event(a).frame
                                                 : std::numeric_limits<std::uint32_t>::max();
        const auto uf =
            b < events.size() ? events[b].frame : std::numeric_limits<std::uint32_t>::max();
        const auto next = std::min(mf, uf);
        status = s.render(out, position, next - position);
        if (status != Status::ok)
            return status;
        position = next;
        if (mf <= uf) {
            s.midi(0, MidiMessageView(midi->event(a++).bytes));
        } else
            status = s.ump(events[b++].packet);
        if (status != Status::ok)
            return status;
    }
    return s.render(out, position, out.frames() - position);
}
template <Sample T> void Synthesizer<T>::reset() noexcept {
    if (state_) {
        for (auto& v : state_->voices) {
            state_->stop(v);
            v.oscillator.reset();
            v.position = 0;
            v.last.fill(0);
            v.fade = 0;
        }
        for (auto& m : state_->mpe)
            m.reset();
        state_->channels.fill(typename State::Channel{});
        state_->controllers.reset();
        state_->serial = state_->stolen = 0;
    }
}
template <Sample T> std::size_t Synthesizer<T>::activeVoices() const noexcept {
    if (!state_)
        return 0;
    return static_cast<std::size_t>(std::count_if(state_->voices.begin(), state_->voices.end(),
                                                  [](const auto& v) { return v.info.active; }));
}
template <Sample T> std::uint64_t Synthesizer<T>::stolenVoices() const noexcept {
    return state_ ? state_->stolen : 0;
}
template <Sample T> CpuBackend Synthesizer<T>::backend() const noexcept {
    return state_ ? state_->ops.backend() : CpuBackend::scalar;
}
template <Sample T> SynthVoiceInfo Synthesizer<T>::voice(std::size_t n) const noexcept {
    return state_ && n < state_->voices.size() ? state_->voices[n].info : SynthVoiceInfo{};
}
template class SampleData<float>;
template class SampleData<double>;
template class Synthesizer<float>;
template class Synthesizer<double>;
} // namespace cydsp
