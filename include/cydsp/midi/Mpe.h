#pragma once
#include "cydsp/midi/Midi.h"
namespace cydsp {
struct MidiParameter {
    unsigned channel = 0, number = 0, value = 0;
    bool registered = true;
};
class MidiParameterParser {
  public:
    void reset() noexcept;
    bool process(MidiMessageView, MidiParameter&) noexcept;

  private:
    struct State {
        unsigned msb = 127, lsb = 127, dataMsb = 0, dataLsb = 0;
        bool registered = true;
    };
    std::array<State, 16> channels_{};
};
struct MpeNote {
    std::uint64_t id = 0;
    unsigned channel = 0, note = 0;
    double velocity = 0, releaseVelocity = 0, pitchSemitones = 0, pressure = 0, timbre = .5,
           masterPressure = 0;
    bool keyDown = false, active = false;
};
enum class MpeChange { noteOn, noteOff, expression, allSoundOff };
using MpeCallback = void (*)(void*, MpeChange, const MpeNote&, std::uint32_t frame) noexcept;
// Bounded channel/note state. Owner serializes configuration and process calls.
class MpeInstrument {
  public:
    MpeInstrument() noexcept;
    Status setZones(unsigned lowerMembers, unsigned upperMembers) noexcept;
    void reset() noexcept;
    Status process(MidiMessageView, std::uint32_t frame, MpeCallback, void*) noexcept;
    [[nodiscard]] const MpeNote* note(unsigned channel, unsigned key) const noexcept;
    [[nodiscard]] unsigned lowerMembers() const noexcept {
        return lower_;
    }
    [[nodiscard]] unsigned upperMembers() const noexcept {
        return upper_;
    }

  private:
    struct Channel {
        double bend = 0, range = 48, pressure = 0, timbre = .5;
        bool sustain = false, sostenuto = false;
    };
    std::array<Channel, 16> channels_{};
    std::array<std::array<MpeNote, 128>, 16> notes_{};
    std::array<std::array<std::uint16_t, 128>, 16> sostenuto_{};
    MidiParameterParser parameters_;
    unsigned lower_ = 15, upper_ = 0;
    std::uint64_t serial_ = 0;
    int master(unsigned channel) const noexcept;
    void expression(unsigned channel, std::uint32_t, MpeCallback, void*) noexcept;
    void release(unsigned channel, bool force, std::uint32_t, MpeCallback, void*) noexcept;
};
} // namespace cydsp
