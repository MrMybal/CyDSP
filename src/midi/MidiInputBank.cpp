#include "cydsp/midi/MidiInputBank.h"
#include <atomic>
namespace cydsp {
struct MidiInputBank::State {
    struct Port {
        MidiEventCollector collector;
        MidiStreamParser parser;
        MidiBuffer midi;
        UmpBuffer ump;
        std::atomic<std::uint32_t> generation{1};
        std::uint32_t seen = 1;
        std::uint64_t arrival = 0;
        Status error = Status::ok;
        bool release = false;
        static void message(void* context, std::span<const std::uint8_t> data, double) noexcept {
            auto& p = *static_cast<Port*>(context);
            const auto r = p.collector.pushMidi(p.arrival, data);
            if (r != Status::ok)
                p.error = r;
        }
        void releaseMidi() noexcept {
            midi.reset();
            ump.reset();
            release = true;
            for (unsigned channel = 1; channel <= 16; ++channel)
                for (unsigned cc : {64u, 123u, 120u})
                    (void)midi.add(0, midiController(channel, cc, 0).bytes());
        }
    };
    std::vector<std::unique_ptr<Port>> ports;
};
MidiInputBank::MidiInputBank() = default;
MidiInputBank::~MidiInputBank() = default;
MidiInputBank::MidiInputBank(MidiInputBank&&) noexcept = default;
MidiInputBank& MidiInputBank::operator=(MidiInputBank&&) noexcept = default;
Status MidiInputBank::prepare(const MidiInputBankOptions& o) noexcept {
    if (!o.ports || o.ports > 64 || !o.messagesPerPort || o.messagesPerPort > 65536 ||
        !o.maximumMessageBytes || o.maximumMessageBytes > 1048576 ||
        o.messagesPerPort > 268435456 / o.maximumMessageBytes / o.ports ||
        static_cast<unsigned>(o.late) > static_cast<unsigned>(LateMidiPolicy::drop))
        return Status::invalid_spec;
    try {
        auto next = std::make_unique<State>();
        for (std::size_t i = 0; i < o.ports; ++i) {
            auto p = std::make_unique<State::Port>();
            auto r = p->collector.prepare({o.messagesPerPort, o.maximumMessageBytes, o.late});
            if (r != Status::ok)
                return r;
            r = p->parser.prepare(o.maximumMessageBytes);
            if (r != Status::ok)
                return r;
            r = p->midi.prepare(o.messagesPerPort + 48,
                                o.messagesPerPort * o.maximumMessageBytes + 144);
            if (r != Status::ok)
                return r;
            r = p->ump.prepare(o.messagesPerPort);
            if (r != Status::ok)
                return r;
            next->ports.push_back(std::move(p));
        }
        state_.swap(next);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status MidiInputBank::ingest(std::size_t id, std::uint64_t time,
                             std::span<const std::uint8_t> fragments) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (id >= ports())
        return Status::invalid_argument;
    auto& p = *state_->ports[id];
    p.arrival = time;
    p.error = Status::ok;
    const auto r = p.parser.feed(fragments, 0, &State::Port::message, &p);
    return r == Status::ok ? p.error : r;
}
Status MidiInputBank::ingestUmp(std::size_t id, std::uint64_t time,
                                const UmpPacket& packet) noexcept {
    if (!state_)
        return Status::not_prepared;
    return id < ports() ? state_->ports[id]->collector.pushUmp(time, packet)
                        : Status::invalid_argument;
}
Status MidiInputBank::invalidate(std::size_t id) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (id >= ports())
        return Status::invalid_argument;
    auto& p = *state_->ports[id];
    p.parser.reset();
    p.collector.invalidate();
    p.generation.fetch_add(1, std::memory_order_release);
    return Status::ok;
}
Status MidiInputBank::collect(const AudioClockAnchor& anchor, std::uint32_t frames) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!isFinite(anchor.sampleRate) || anchor.sampleRate <= 0 || anchor.sampleRate > 1e9 ||
        anchor.firstFrame > std::numeric_limits<std::uint64_t>::max() - frames)
        return Status::invalid_argument;
    if (!frames)
        return Status::ok;
    Status result = Status::ok;
    for (auto& value : state_->ports) {
        auto& p = *value;
        p.midi.reset();
        p.ump.reset();
        p.release = false;
        const auto before = p.generation.load(std::memory_order_acquire);
        if (before != p.seen) {
            p.releaseMidi();
            p.seen = before;
        }
        const auto r = p.collector.collect(anchor, frames, p.midi, p.ump);
        const auto after = p.generation.load(std::memory_order_acquire);
        if (after != before) {
            p.releaseMidi();
            p.seen = after;
        }
        if (r != Status::ok)
            result = r;
    }
    return result;
}
MidiInputPortBlock MidiInputBank::block(std::size_t id) const noexcept {
    if (!state_ || id >= ports())
        return {};
    const auto& p = *state_->ports[id];
    return {&p.midi, &p.ump, p.seen, p.release};
}
std::size_t MidiInputBank::ports() const noexcept {
    return state_ ? state_->ports.size() : 0;
}
} // namespace cydsp
