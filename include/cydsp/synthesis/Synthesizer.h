#pragma once
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/core/Cpu.h"
#include "cydsp/midi/Mpe.h"
#include "cydsp/midi/Ump.h"
#include "cydsp/processors/Generators.h"
#include "cydsp/synthesis/Wavetable.h"
#include <memory>
namespace cydsp {
enum class SampleInterpolation { linear, bandLimited };
template <Sample T> class SampleData {
  public:
    Status prepare(AudioBlock<const T>, double sampleRate, unsigned rootNote = 60,
                   CpuBackend = CpuBackend::automatic) noexcept;
    Status setLoop(std::size_t first, std::size_t end,
                   std::size_t crossfadeFrames = 0) noexcept; // [first,end), 0/0 disables
    [[nodiscard]] AudioBlock<const T> samples() const noexcept {
        return audio_.block();
    }
    [[nodiscard]] double sampleRate() const noexcept {
        return rate_;
    }
    [[nodiscard]] unsigned rootNote() const noexcept {
        return root_;
    }
    [[nodiscard]] std::size_t loopStart() const noexcept {
        return first_;
    }
    [[nodiscard]] std::size_t loopEnd() const noexcept {
        return end_;
    }

    [[nodiscard]] std::size_t loopCrossfade() const noexcept {
        return crossfade_;
    }
    [[nodiscard]] std::size_t mipLevels() const noexcept {
        return mips_.size();
    }
    [[nodiscard]] T value(std::size_t channel, double position, double ratio = 1,
                          SampleInterpolation = SampleInterpolation::bandLimited) const noexcept;

  private:
    AudioBuffer<T> audio_;
    std::vector<AudioBuffer<T>> mips_;
    double rate_ = 0;
    unsigned root_ = 60;
    std::size_t first_ = 0, end_ = 0, crossfade_ = 0;
};
struct SynthVoiceInfo {
    std::size_t index = 0;
    std::uint64_t id = 0;
    unsigned group = 0, channel = 1, note = 60;
    double frequency = 440, velocity = 0, pressure = 0, timbre = .5;
    bool active = false, releasing = false;
};
struct SynthParameters {
    AdsrParameters envelope{};
    Waveform waveform = Waveform::sine;
    double gain = .2, pan = 0, pressureToGain = 0;
    std::size_t stealFadeFrames = 32;
    SampleInterpolation sampleInterpolation = SampleInterpolation::bandLimited;
};
Status validateSynthParameters(const SynthParameters&) noexcept;
template <Sample T> struct SynthVoiceRenderer {
    void* context = nullptr;
    void (*start)(void*, const SynthVoiceInfo&) noexcept = nullptr;
    void (*stop)(void*, const SynthVoiceInfo&) noexcept = nullptr;
    Status (*render)(void*, const SynthVoiceInfo&, AudioBlock<T> mono) noexcept = nullptr;
};
// Prepared polyphonic oscillator/sampler/custom voices, sample-accurate MIDI1/MPE/UMP events.
template <Sample T> class Synthesizer {
  public:
    Synthesizer();
    ~Synthesizer();
    Synthesizer(Synthesizer&&) noexcept;
    Synthesizer& operator=(Synthesizer&&) noexcept;
    Status prepare(const ProcessSpec&, std::size_t voices = 64,
                   CpuBackend = CpuBackend::automatic) noexcept;
    Status setParameters(const SynthParameters&) noexcept;
    Status setMpeZones(unsigned lowerMembers, unsigned upperMembers) noexcept;
    // Configuration and sample lifetime are control-thread responsibilities; voices must be idle.
    Status setSample(const SampleData<T>*) noexcept;
    Status setRenderer(SynthVoiceRenderer<T>) noexcept;
    Status setWavetable(const Wavetable<T>*) noexcept;
    Status process(AudioBlock<T>, const MidiBuffer* midi = nullptr, const UmpBuffer* ump = nullptr,
                   bool add = false) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t activeVoices() const noexcept;
    [[nodiscard]] std::uint64_t stolenVoices() const noexcept;
    [[nodiscard]] CpuBackend backend() const noexcept;
    [[nodiscard]] SynthVoiceInfo voice(std::size_t) const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class SampleData<float>;
extern template class SampleData<double>;
extern template class Synthesizer<float>;
extern template class Synthesizer<double>;
} // namespace cydsp
