#include "cydsp/midi/MidiEventCollector.h"
#include "cydsp/core/SpscQueue.h"
#include <algorithm>
#include <atomic>
#include <cmath>
namespace cydsp {
struct MidiEventCollector::State {
    struct Slot {
        std::vector<std::uint8_t> bytes;
        std::size_t size = 0;
        UmpPacket packet;
        std::uint64_t time = 0, sequence = 0;
        bool ump = false;
        std::uint32_t epoch = 0;
    };
    SpscQueue<std::size_t> free, ready;
    std::vector<Slot> slots;
    std::vector<std::size_t> heap;
    std::size_t used = 0;
    std::uint64_t sequence = 0;
    std::atomic<std::uint32_t> overflows{0}, late{0}, epoch{0};
    LateMidiPolicy policy = LateMidiPolicy::clampToFirstFrame;
    bool after(std::size_t a, std::size_t b) const noexcept {
        return slots[a].time > slots[b].time ||
               (slots[a].time == slots[b].time && slots[a].sequence > slots[b].sequence);
    }
    void drain() noexcept {
        std::size_t id;
        while (used < heap.size() && ready.pop(id)) {
            heap[used++] = id;
            std::push_heap(heap.begin(), heap.begin() + std::ptrdiff_t(used),
                           [&](auto a, auto b) { return after(a, b); });
        }
    }
    void consume() noexcept {
        std::pop_heap(heap.begin(), heap.begin() + std::ptrdiff_t(used),
                      [&](auto a, auto b) { return after(a, b); });
        (void)free.push(heap[--used]);
    }
};
MidiEventCollector::MidiEventCollector() = default;
MidiEventCollector::~MidiEventCollector() = default;
MidiEventCollector::MidiEventCollector(MidiEventCollector&&) noexcept = default;
MidiEventCollector& MidiEventCollector::operator=(MidiEventCollector&&) noexcept = default;
Status MidiEventCollector::prepare(const MidiCollectorOptions& options) noexcept {
    if (!options.messages || options.messages > 65536 || !options.maximumMessageBytes ||
        options.maximumMessageBytes > 1048576 ||
        options.messages > 1073741824 / options.maximumMessageBytes ||
        static_cast<unsigned>(options.late) > static_cast<unsigned>(LateMidiPolicy::drop))
        return Status::invalid_spec;
    try {
        auto next = std::make_unique<State>();
        next->slots.resize(options.messages);
        next->heap.resize(options.messages);
        next->policy = options.late;
        auto r = next->free.prepare(options.messages);
        if (r != Status::ok)
            return r;
        r = next->ready.prepare(options.messages);
        if (r != Status::ok)
            return r;
        for (std::size_t i = 0; i < options.messages; ++i) {
            next->slots[i].bytes.resize(options.maximumMessageBytes);
            (void)next->free.push(i);
        }
        state_.swap(next);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status MidiEventCollector::pushMidi(std::uint64_t time,
                                    std::span<const std::uint8_t> data) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (!MidiMessageView(data).valid())
        return Status::invalid_argument;
    if (data.size() > s.slots[0].bytes.size())
        return Status::block_too_large;
    std::size_t id;
    if (!s.free.pop(id)) {
        ++s.overflows;
        return Status::update_queue_full;
    }
    auto& slot = s.slots[id];
    slot.epoch = s.epoch.load(std::memory_order_relaxed);
    slot.time = time;
    slot.sequence = ++s.sequence;
    slot.ump = false;
    slot.size = data.size();
    std::copy(data.begin(), data.end(), slot.bytes.begin());
    (void)s.ready.push(id);
    return Status::ok;
}
Status MidiEventCollector::pushUmp(std::uint64_t time, const UmpPacket& packet) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!packet.valid())
        return Status::invalid_argument;
    auto& s = *state_;
    std::size_t id;
    if (!s.free.pop(id)) {
        ++s.overflows;
        return Status::update_queue_full;
    }
    auto& slot = s.slots[id];
    slot.epoch = s.epoch.load(std::memory_order_relaxed);
    slot.time = time;
    slot.sequence = ++s.sequence;
    slot.ump = true;
    slot.packet = packet;
    (void)s.ready.push(id);
    return Status::ok;
}
Status MidiEventCollector::collect(const AudioClockAnchor& clock, std::uint32_t frames,
                                   MidiBuffer& midi, UmpBuffer& ump) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!isFinite(clock.sampleRate) || clock.sampleRate <= 0 || clock.sampleRate > 1e9 ||
        clock.firstFrame > std::numeric_limits<std::uint64_t>::max() - frames)
        return Status::invalid_argument;
    if (!frames)
        return Status::ok;
    auto& s = *state_;
    s.drain();
    const auto epoch = s.epoch.load(std::memory_order_acquire);
    auto kept = std::size_t(0);
    for (std::size_t i = 0; i < s.used; ++i) {
        const auto id = s.heap[i];
        if (s.slots[id].epoch == epoch)
            s.heap[kept++] = id;
        else
            (void)s.free.push(id);
    }
    s.used = kept;
    std::make_heap(s.heap.begin(), s.heap.begin() + std::ptrdiff_t(s.used),
                   [&](auto a, auto b) { return s.after(a, b); });
    for (std::size_t emitted = 0; s.used && emitted < s.slots.size(); ++emitted) {
        const auto& slot = s.slots[s.heap[0]];
        const bool late = slot.time < clock.monotonicNanoseconds;
        const auto offset =
            late ? 0.0L
                 : std::floor(static_cast<long double>(slot.time - clock.monotonicNanoseconds) *
                              clock.sampleRate / 1000000000.0L);
        if (offset >= frames)
            break;
        if (late && s.policy == LateMidiPolicy::drop) {
            ++s.late;
            s.consume();
            continue;
        }
        const auto frame = static_cast<std::uint32_t>(offset);
        const auto r = slot.ump ? ump.add(frame, slot.packet)
                                : midi.add(frame, {slot.bytes.data(), slot.size});
        if (r != Status::ok)
            return r;
        if (late)
            ++s.late;
        s.consume();
    }
    return Status::ok;
}
void MidiEventCollector::invalidate() noexcept {
    if (state_)
        state_->epoch.fetch_add(1, std::memory_order_release);
}
void MidiEventCollector::reset() noexcept {
    if (!state_)
        return;
    auto& s = *state_;
    s.drain();
    while (s.used)
        s.consume();
    s.sequence = 0;
    s.overflows.store(0);
    s.late.store(0);
}
std::uint32_t MidiEventCollector::overflows() const noexcept {
    return state_ ? state_->overflows.load(std::memory_order_relaxed) : 0;
}
std::uint32_t MidiEventCollector::lateMessages() const noexcept {
    return state_ ? state_->late.load(std::memory_order_relaxed) : 0;
}
} // namespace cydsp
