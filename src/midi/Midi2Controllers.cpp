#include "cydsp/midi/Midi2Controllers.h"
#include <algorithm>
namespace cydsp {
UmpPacket umpParameter(bool registered, unsigned g, unsigned c, unsigned b, unsigned i,
                       std::uint32_t v, bool relative) noexcept {
    if (g > 15 || c < 1 || c > 16 || b > 127 || i > 127)
        return {};
    UmpPacket p;
    p.count = 2;
    p.words = {0x40000000u | (g << 24) |
                   ((unsigned(registered ? 2 : 3) + (relative ? 2u : 0u)) << 20) | ((c - 1) << 16) |
                   (b << 8) | i,
               v, 0, 0};
    return p;
}
UmpPacket umpPerNoteManagement(unsigned g, unsigned c, unsigned n, bool detach,
                               bool reset) noexcept {
    if (g > 15 || c < 1 || c > 16 || n > 127 || (!detach && !reset))
        return {};
    UmpPacket p;
    p.count = 2;
    p.words = {0x40f00000u | (g << 24) | ((c - 1) << 16) | (n << 8) | (detach ? 2u : 0u) |
                   (reset ? 1u : 0u),
               0, 0, 0};
    return p;
}
struct Midi2Controllers::State {
    struct Parameter {
        std::uint32_t key = 0, value = 0;
    };
    std::array<Midi2ChannelControllers, 256> channels;
    std::vector<Parameter> parameters;
    std::size_t used = 0;
};
namespace {
std::uint32_t key(bool reg, unsigned g, unsigned c, unsigned b, unsigned i) noexcept {
    return (reg ? 0x80000000u : 0) | (g << 24) | ((c - 1) << 16) | (b << 8) | i;
}
void defaults(Midi2ChannelControllers& c) noexcept {
    c = {};
    c.cc[7] = c.cc[11] = 0xffffffffu;
    c.cc[10] = 0x80000000u;
}
} // namespace
Midi2Controllers::Midi2Controllers() = default;
Midi2Controllers::~Midi2Controllers() = default;
Midi2Controllers::Midi2Controllers(Midi2Controllers&&) noexcept = default;
Midi2Controllers& Midi2Controllers::operator=(Midi2Controllers&&) noexcept = default;
Status Midi2Controllers::prepare(std::size_t max) noexcept {
    if (!max || max > 65536)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->parameters.resize(max);
        state_.swap(s);
        reset();
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
void Midi2Controllers::reset() noexcept {
    if (state_) {
        for (auto& c : state_->channels)
            defaults(c);
        state_->used = 0;
    }
}
const Midi2ChannelControllers* Midi2Controllers::channel(unsigned g, unsigned c) const noexcept {
    return state_ && g < 16 && c >= 1 && c <= 16 ? &state_->channels[g * 16 + c - 1] : nullptr;
}
Status Midi2Controllers::parameter(bool reg, unsigned g, unsigned c, unsigned b, unsigned i,
                                   std::uint32_t& value) const noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!umpParameter(reg, g, c, b, i, 0).valid())
        return Status::invalid_argument;
    const auto k = key(reg, g, c, b, i);
    for (std::size_t n = 0; n < state_->used; ++n)
        if (state_->parameters[n].key == k) {
            value = state_->parameters[n].value;
            return Status::ok;
        }
    return Status::would_block;
}
Status Midi2Controllers::process(const UmpPacket& p, Callback callback, void* ctx) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!p.valid() || p.type() != 4)
        return Status::invalid_argument;
    auto& s = *state_;
    auto& c = s.channels[p.group() * 16 + p.channel() - 1];
    const auto st = p.status(), index = (p.words[0] >> 8) & 127;
    auto value = p.words[1];
    if (st == 11) {
        // MIDI 2 uses atomic parameter/bank messages for these compound MIDI 1 CCs.
        if (index == 0 || index == 32 || index == 6 || index == 38 || (index >= 98 && index <= 101))
            return Status::ok;
        c.cc[index] = value;
        if (index == 121) {
            const auto range = c.pitchRange, pn = c.perNotePitchRange, fine = c.fineTuning,
                       coarse = c.coarseTuning;
            const auto program = c.program, msb = c.bankMsb, lsb = c.bankLsb;
            defaults(c);
            c.pitchRange = range;
            c.perNotePitchRange = pn;
            c.fineTuning = fine;
            c.coarseTuning = coarse;
            c.program = program;
            c.bankMsb = msb;
            c.bankLsb = lsb;
        }
    } else if (st >= 2 && st <= 5) {
        const bool reg = st == 2 || st == 4;
        const auto bank = (p.words[0] >> 8) & 127, idx = p.words[0] & 127;
        if (p.words[0] & 0x8080)
            return Status::invalid_argument;
        const auto k = key(reg, p.group(), p.channel(), bank, idx);
        auto n = std::size_t(0);
        while (n < s.used && s.parameters[n].key != k)
            ++n;
        if (n == s.used && s.used == s.parameters.size())
            return Status::update_queue_full;
        auto previous = n < s.used ? s.parameters[n].value : 0u;
        if (n == s.used && reg && bank == 0) {
            if (idx == 0)
                previous = 2u << 25;
            else if (idx == 1 || idx == 2)
                previous = 0x80000000u;
            else if (idx == 7)
                previous = 48u << 25;
        }
        if (st >= 4) {
            const auto delta =
                value < 0x80000000u ? std::int64_t(value) : std::int64_t(value) - 4294967296LL;
            value = std::uint32_t(
                std::clamp<std::int64_t>(std::int64_t(previous) + delta, 0, 4294967295LL));
        }
        s.parameters[n] = {k, value};
        if (n == s.used)
            ++s.used;
        if (reg && bank == 0) {
            if (idx == 0)
                c.pitchRange = double(value >> 25) + double((value >> 18) & 127) / 100;
            else if (idx == 1)
                c.fineTuning = (double(value) - 2147483648.0) / 2147483648.0;
            else if (idx == 2)
                c.coarseTuning = double(value >> 25) - 64;
            else if (idx == 3)
                c.tuningProgram = value >> 25;
            else if (idx == 4)
                c.tuningBank = value >> 25;
            else if (idx == 6 && (p.channel() == 1 || p.channel() == 16))
                c.mpeMembers = std::min(15u, value >> 25);
            else if (idx == 7)
                c.perNotePitchRange = double(value) / 33554432.0;
        }
    } else if (st == 12) {
        c.program = (value >> 24) & 127;
        if (p.words[0] & 1) {
            c.bankMsb = (value >> 8) & 127;
            c.bankLsb = value & 127;
        }
    }
    if (callback)
        callback(ctx, {p, value});
    return Status::ok;
}
Status CiProfileDispatcher::prepare(std::span<const CiProfileBinding> bindings,
                                    std::size_t count) noexcept {
    if (bindings.size() > 128 || !count || count > 65536)
        return Status::invalid_spec;
    for (std::size_t i = 0; i < bindings.size(); ++i) {
        if (!bindings[i].apply)
            return Status::invalid_argument;
        for (auto b : bindings[i].id)
            if (b > 127)
                return Status::invalid_argument;
        for (std::size_t j = 0; j < i; ++j)
            if (bindings[i].id == bindings[j].id)
                return Status::invalid_argument;
    }
    try {
        std::vector<CiProfileBinding> next(bindings.begin(), bindings.end());
        auto r = queue_.prepare(count);
        if (r != Status::ok)
            return r;
        bindings_.swap(next);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status CiProfileDispatcher::notify(unsigned g, const CiProfile& p) noexcept {
    if (!queue_.capacity())
        return Status::not_prepared;
    if (g > 15 || (p.address > 15 && p.address != 0x7e && p.address != 0x7f) ||
        (p.address <= 15 && p.channels > 16 - p.address))
        return Status::invalid_argument;
    for (std::size_t i = 0; i < bindings_.size(); ++i)
        if (bindings_[i].id == p.id)
            return queue_.push({g, i, p}) ? Status::ok : Status::update_queue_full;
    return Status::unsupported_format;
}
Status CiProfileDispatcher::applyPending() noexcept {
    if (!queue_.capacity())
        return Status::not_prepared;
    Update u;
    // Bound work even when the producer keeps supplying updates.
    for (std::size_t i = 0; i < queue_.capacity() && queue_.pop(u); ++i) {
        auto& b = bindings_[u.binding];
        b.apply(b.context, u.group, u.profile);
    }
    return Status::ok;
}
} // namespace cydsp
