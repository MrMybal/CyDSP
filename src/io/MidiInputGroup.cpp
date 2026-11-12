#include "cydsp/io/MidiInputGroup.h"
#include <atomic>
namespace cydsp {
struct MidiInputGroup::State {
    struct Port {
        MidiPortSelector selection;
        MidiInput input;
        std::string identity;
        std::uint32_t index = 0;
        std::atomic<std::uint32_t> connected{0}, status{std::uint32_t(Status::device_unavailable)};
    };
    MidiInputBank bank;
    std::vector<std::unique_ptr<Port>> ports;
    std::vector<std::uint8_t> scratch;
    MidiInputGroupOptions options;
    std::uint64_t lastScan = 0;
    bool scanned = false;
    void disconnect(std::size_t id, Status why) noexcept {
        auto& p = *ports[id];
        if (p.connected.exchange(0, std::memory_order_acq_rel)) {
            p.input.close();
            (void)bank.invalidate(id);
        }
        p.status.store(std::uint32_t(why), std::memory_order_release);
    }
};
MidiInputGroup::MidiInputGroup() = default;
MidiInputGroup::~MidiInputGroup() = default;
MidiInputGroup::MidiInputGroup(MidiInputGroup&&) noexcept = default;
MidiInputGroup& MidiInputGroup::operator=(MidiInputGroup&&) noexcept = default;
Status MidiInputGroup::prepare(std::span<const MidiPortSelector> selections,
                               const MidiInputGroupOptions& o) noexcept {
    if (selections.empty() || selections.size() > 64 || !o.scanIntervalMs ||
        !o.maximumMessagesPerPoll || o.maximumMessagesPerPoll > 65536 ||
        !o.input.nativeBufferCount || o.input.nativeBufferCount > 64)
        return Status::invalid_argument;
    try {
        auto next = std::make_unique<State>();
        next->options = o;
        auto r = next->bank.prepare(
            {selections.size(), o.input.queuedMessages, o.input.maximumMessageBytes});
        if (r != Status::ok)
            return r;
        next->scratch.resize(o.input.maximumMessageBytes);
        for (const auto& selection : selections) {
            if (!selection.input || (selection.name.empty() && selection.persistentId.empty()))
                return Status::invalid_argument;
            auto p = std::make_unique<State::Port>();
            p->selection = selection;
            next->ports.push_back(std::move(p));
        }
        state_.swap(next);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status MidiInputGroup::poll(std::uint64_t now) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    try {
        if (!s.scanned || now < s.lastScan || now - s.lastScan >= s.options.scanIntervalMs) {
            MidiDevicePorts list;
            const auto listed = enumerateMidiPorts(list);
            s.lastScan = now;
            s.scanned = true;
            for (std::size_t id = 0; id < s.ports.size(); ++id) {
                auto& p = *s.ports[id];
                std::uint32_t index = 0;
                const auto resolved =
                    listed == Status::ok ? resolveMidiPort(list, p.selection, index) : listed;
                if (resolved != Status::ok) {
                    s.disconnect(id, resolved);
                    continue;
                }
                const MidiPortInfo* found = nullptr;
                for (const auto& info : list.inputs)
                    if (info.index == index) {
                        found = &info;
                        break;
                    }
                if (!found) {
                    s.disconnect(id, Status::device_unavailable);
                    continue;
                }
                const auto identity = found->persistentIdentity ? found->persistentId
                                                                : list.backend + "/" + found->name;
                if (p.connected.load(std::memory_order_acquire) &&
                    (p.identity != identity || p.index != index))
                    s.disconnect(id, Status::disconnected);
                if (!p.connected.load(std::memory_order_acquire)) {
                    const auto r = p.input.open(index, s.options.input);
                    p.status.store(std::uint32_t(r), std::memory_order_release);
                    if (r == Status::ok) {
                        p.identity = identity;
                        p.index = index;
                        p.connected.store(1, std::memory_order_release);
                    }
                }
            }
        }
        Status result = Status::ok;
        for (std::size_t id = 0; id < s.ports.size(); ++id) {
            auto& p = *s.ports[id];
            if (!p.connected.load(std::memory_order_acquire))
                continue;
            for (std::size_t n = 0; n < s.options.maximumMessagesPerPoll; ++n) {
                std::size_t size = 0;
                MidiInputTimestamp stamp;
                const auto r = p.input.readTimed(s.scratch, size, stamp);
                if (r == Status::end_of_stream)
                    break;
                if (r != Status::ok) {
                    s.disconnect(id, r);
                    result = r;
                    break;
                }
                const auto pushed =
                    s.bank.ingest(id, stamp.arrivalNanoseconds, {s.scratch.data(), size});
                if (pushed != Status::ok)
                    result = pushed;
            }
        }
        return result;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status MidiInputGroup::disconnect(std::size_t id) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (id >= state_->ports.size())
        return Status::invalid_argument;
    state_->disconnect(id, Status::disconnected);
    state_->scanned = false;
    return Status::ok;
}
Status MidiInputGroup::collect(const AudioClockAnchor& clock, std::uint32_t frames) noexcept {
    return state_ ? state_->bank.collect(clock, frames) : Status::not_prepared;
}
MidiInputPortBlock MidiInputGroup::block(std::size_t id) const noexcept {
    return state_ ? state_->bank.block(id) : MidiInputPortBlock{};
}
bool MidiInputGroup::connected(std::size_t id) const noexcept {
    return state_ && id < state_->ports.size() &&
           state_->ports[id]->connected.load(std::memory_order_acquire);
}
Status MidiInputGroup::portStatus(std::size_t id) const noexcept {
    if (!state_)
        return Status::not_prepared;
    return id < state_->ports.size()
               ? Status(state_->ports[id]->status.load(std::memory_order_acquire))
               : Status::invalid_argument;
}
void MidiInputGroup::close() noexcept {
    state_.reset();
}
} // namespace cydsp
