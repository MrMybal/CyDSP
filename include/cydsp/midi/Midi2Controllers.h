#pragma once
#include "cydsp/core/SpscQueue.h"
#include "cydsp/midi/MidiCi.h"
#include <memory>
namespace cydsp {
UmpPacket umpParameter(bool registered, unsigned group, unsigned channel, unsigned bank,
                       unsigned index, std::uint32_t value, bool relative = false) noexcept;
UmpPacket umpPerNoteManagement(unsigned group, unsigned channel, unsigned note, bool detach,
                               bool reset) noexcept;
struct Midi2ChannelControllers {
    std::array<std::uint32_t, 128> cc{};
    double pitchRange = 2, perNotePitchRange = 48, fineTuning = 0, coarseTuning = 0;
    unsigned program = 0, bankMsb = 0, bankLsb = 0, tuningProgram = 0, tuningBank = 0;
    unsigned mpeMembers = 0;
};
struct Midi2ControllerChange {
    UmpPacket packet;
    std::uint32_t value = 0; // Absolute value, including resolved relative operations.
};
// Single owner. Retains all CCs and bounded sparse RPN/NRPN values. Per-note
// operations are delivered verbatim for voice-specific reset/detach handling.
class Midi2Controllers {
  public:
    using Callback = void (*)(void*, const Midi2ControllerChange&) noexcept;
    Midi2Controllers();
    ~Midi2Controllers();
    Midi2Controllers(Midi2Controllers&&) noexcept;
    Midi2Controllers& operator=(Midi2Controllers&&) noexcept;
    Status prepare(std::size_t maximumParameters = 1024) noexcept;
    Status process(const UmpPacket&, Callback = nullptr, void* = nullptr) noexcept;
    [[nodiscard]] const Midi2ChannelControllers* channel(unsigned group,
                                                         unsigned channel) const noexcept;
    Status parameter(bool registered, unsigned group, unsigned channel, unsigned bank,
                     unsigned index, std::uint32_t& value) const noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
struct CiProfileBinding {
    std::array<std::uint8_t, 5> id{};
    void* context = nullptr;
    void (*apply)(void*, unsigned group, const CiProfile&) noexcept = nullptr;
};
// Explicit application bindings: no behavior is invented for unknown profiles.
// notify is the CI/control producer; applyPending is the audio consumer.
class CiProfileDispatcher {
  public:
    Status prepare(std::span<const CiProfileBinding>, std::size_t updates = 64) noexcept;
    Status notify(unsigned group, const CiProfile&) noexcept;
    Status applyPending() noexcept;

  private:
    struct Update {
        unsigned group;
        std::size_t binding;
        CiProfile profile;
        Update() noexcept : group(0), binding(0) {}
        Update(unsigned g, std::size_t b, CiProfile p) noexcept
            : group(g), binding(b), profile(p) {}
    };
    std::vector<CiProfileBinding> bindings_;
    SpscQueue<Update> queue_;
};
} // namespace cydsp
