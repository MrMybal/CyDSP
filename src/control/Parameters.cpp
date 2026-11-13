#include "cydsp/control/Parameters.h"
#include "cydsp/core/Memory.h"
#include "cydsp/math/DspMath.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
namespace cydsp {
namespace {
bool valid(const ParameterDescriptor& d) noexcept {
    return isFinite(d.minimum) && isFinite(d.maximum) && d.maximum > d.minimum &&
           isFinite(d.maximum - d.minimum) && isFinite(d.initial) && d.initial >= d.minimum &&
           d.initial <= d.maximum && unsigned(d.scale) <= unsigned(ParameterScale::logarithmic) &&
           unsigned(d.unit) <= unsigned(ParameterUnit::degrees) &&
           (d.scale != ParameterScale::logarithmic || d.minimum > 0);
}
} // namespace
double parameterToNormalized(const ParameterDescriptor& d, double v) noexcept {
    if (!valid(d) || !isFinite(v))
        return 0;
    v = std::clamp(v, d.minimum, d.maximum);
    return d.scale == ParameterScale::logarithmic
               ? (std::log(v) - std::log(d.minimum)) / (std::log(d.maximum) - std::log(d.minimum))
               : (v - d.minimum) / (d.maximum - d.minimum);
}
double parameterFromNormalized(const ParameterDescriptor& d, double v) noexcept {
    if (!valid(d) || !isFinite(v))
        return 0;
    v = std::clamp(v, 0., 1.);
    return d.scale == ParameterScale::logarithmic
               ? std::exp(std::log(d.minimum) + v * (std::log(d.maximum) - std::log(d.minimum)))
               : d.minimum + v * (d.maximum - d.minimum);
}
struct ParameterBank::State {
    struct Parameter {
        ParameterDescriptor descriptor;
        std::string name;
        SmoothedValue ramp;
    };
    std::vector<Parameter> parameters;
    std::vector<std::pair<std::uint32_t, std::size_t>> index;
    std::size_t maximum = 0;
    std::size_t find(std::uint32_t id) const noexcept {
        const auto it = std::lower_bound(index.begin(), index.end(), id,
                                         [](const auto& p, auto v) { return p.first < v; });
        return it != index.end() && it->first == id ? it->second : parameters.size();
    }
};
ParameterBank::ParameterBank() = default;
ParameterBank::~ParameterBank() = default;
ParameterBank::ParameterBank(ParameterBank&&) noexcept = default;
ParameterBank& ParameterBank::operator=(ParameterBank&&) noexcept = default;
Status ParameterBank::prepare(std::span<const ParameterDescriptor> descriptors,
                              std::size_t maximum) noexcept {
    if (descriptors.empty() || descriptors.size() > 4096 || !maximum || maximum > 65536)
        return Status::invalid_spec;
    for (const auto& d : descriptors)
        if (!valid(d) || d.name.size() > 4096)
            return Status::invalid_argument;
    try {
        auto n = std::make_unique<State>();
        n->maximum = maximum;
        n->parameters.resize(descriptors.size());
        for (std::size_t i = 0; i < descriptors.size(); ++i) {
            auto& p = n->parameters[i];
            p.descriptor = descriptors[i];
            p.name = std::string(descriptors[i].name);
            p.descriptor.name = p.name;
            (void)p.ramp.reset(p.descriptor.initial);
            n->index.push_back({p.descriptor.id, i});
        }
        std::sort(n->index.begin(), n->index.end());
        for (std::size_t i = 1; i < n->index.size(); ++i)
            if (n->index[i - 1].first == n->index[i].first)
                return Status::invalid_argument;
        state_.swap(n);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status ParameterBank::render(std::span<const ParameterEvent> events,
                             AudioBlock<double> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (!out.isValid())
        return Status::invalid_argument;
    if (out.channels() != s.parameters.size())
        return Status::channel_mismatch;
    if (out.frames() > s.maximum)
        return Status::block_too_large;
    auto result = validateProcessBlocks<double>({1, s.maximum, out.channels()}, out, out);
    if (result != Status::ok)
        return result;
    if (events.size() > 65536)
        return Status::invalid_argument;
    for (std::size_t ch = 0; ch < out.channels(); ++ch)
        if (memory::overlaps(events.data(), events.size_bytes(), out.channel(ch).data(),
                             out.channel(ch).size_bytes()))
            return Status::partial_overlap;
    std::size_t last = 0;
    for (const auto& event : events) {
        const auto index = s.find(event.id);
        if (index == s.parameters.size() || event.frameOffset >= out.frames() ||
            event.frameOffset < last || !isFinite(event.value) || event.rampFrames > (1u << 30))
            return Status::invalid_argument;
        const auto& d = s.parameters[index].descriptor;
        if (!d.modulatable || event.value < d.minimum || event.value > d.maximum)
            return Status::invalid_argument;
        last = event.frameOffset;
    }
    std::size_t next = 0;
    for (std::size_t i = 0; i < out.frames(); ++i) {
        while (next < events.size() && events[next].frameOffset == i) {
            const auto& e = events[next++];
            (void)s.parameters[s.find(e.id)].ramp.setTarget(e.value, e.rampFrames);
        }
        for (std::size_t ch = 0; ch < out.channels(); ++ch)
            out.channel(ch)[i] = s.parameters[ch].ramp.next();
    }
    return Status::ok;
}
void ParameterBank::reset() noexcept {
    if (state_)
        for (auto& p : state_->parameters)
            (void)p.ramp.reset(p.descriptor.initial);
}
double ParameterBank::value(std::uint32_t id) const noexcept {
    if (!state_)
        return 0;
    const auto i = state_->find(id);
    return i < state_->parameters.size() ? state_->parameters[i].ramp.current() : 0;
}
std::size_t ParameterBank::size() const noexcept {
    return state_ ? state_->parameters.size() : 0;
}
} // namespace cydsp
