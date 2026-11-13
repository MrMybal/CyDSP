#include "cydsp/spatial/AcousticRooms.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/math/VectorOps.h"
#include "detail/Analysis.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>
namespace cydsp {
namespace {
bool validRoom(const AcousticRoom& room) {
    if (!isFinite(room.volumeCubicMetres) || room.volumeCubicMetres <= 0 ||
        room.volumeCubicMetres > 1e12 || !isFinite(room.surfaceSquareMetres) ||
        room.surfaceSquareMetres <= 0 || room.surfaceSquareMetres > 1e12)
        return false;
    for (double a : room.meanAbsorption)
        if (!isFinite(a) || a < 0 || a > 1)
            return false;
    return true;
}
bool validResponse(const RoomResponse& r) {
    for (double decay : r.decaySeconds)
        if (!isFinite(decay) || decay < .01 || decay > 120)
            return false;
    return isFinite(r.preDelaySeconds) && r.preDelaySeconds >= 0 && r.preDelaySeconds <= 2 &&
           isFinite(r.diffusion) && r.diffusion >= 0 && r.diffusion <= 1 && isFinite(r.wet) &&
           r.wet >= 0 && r.wet <= 1;
}
} // namespace
Status estimateRoomResponse(const AcousticRoom& room, const AcousticMedium& medium,
                            RoomResponse& output, RoomDecayModel model) noexcept {
    if (!validRoom(room) || !validMedium(medium) ||
        unsigned(model) > unsigned(RoomDecayModel::eyring))
        return Status::invalid_argument;
    RoomResponse response;
    for (std::size_t band = 0; band < 3; ++band) {
        const double alpha = room.meanAbsorption[band],
                     area = room.surfaceSquareMetres * (model == RoomDecayModel::eyring
                                                            ? -std::log(std::max(1e-12, 1 - alpha))
                                                            : alpha),
                     m = medium.absorptionDbPerMetre[band] * std::numbers::ln10 / 10;
        const double loss = area + 4 * room.volumeCubicMetres * m;
        response.decaySeconds[band] =
            loss > 0 ? std::clamp(24 * std::numbers::ln10 * room.volumeCubicMetres /
                                      (medium.speedMetresPerSecond * loss),
                                  .01, 120.)
                     : 120;
    }
    output = response;
    return Status::ok;
}
struct AcousticPortalGraph::State {
    std::vector<AcousticRoom> rooms;
    std::vector<AcousticPortal> portals;
    std::vector<double> cost;
    std::vector<std::size_t> next, portal;
    std::size_t find(std::uint32_t id) const {
        for (std::size_t i = 0; i < rooms.size(); ++i)
            if (rooms[i].id == id)
                return i;
        return rooms.size();
    }
};
AcousticPortalGraph::AcousticPortalGraph() = default;
AcousticPortalGraph::~AcousticPortalGraph() = default;
AcousticPortalGraph::AcousticPortalGraph(AcousticPortalGraph&&) noexcept = default;
AcousticPortalGraph& AcousticPortalGraph::operator=(AcousticPortalGraph&&) noexcept = default;
Status AcousticPortalGraph::prepare(std::span<const AcousticRoom> rooms,
                                    std::span<const AcousticPortal> portals,
                                    double weight) noexcept {
    if (rooms.empty() || rooms.size() > 256 || portals.size() > 2048 || !isFinite(weight) ||
        weight < 0 || weight > 10)
        return Status::invalid_spec;
    for (std::size_t i = 0; i < rooms.size(); ++i) {
        if (!validRoom(rooms[i]))
            return Status::invalid_argument;
        for (std::size_t j = 0; j < i; ++j)
            if (rooms[i].id == rooms[j].id)
                return Status::invalid_argument;
    }
    try {
        auto s = std::make_unique<State>();
        s->rooms.assign(rooms.begin(), rooms.end());
        s->portals.assign(portals.begin(), portals.end());
        const auto n = rooms.size();
        s->cost.resize(n * n, std::numeric_limits<double>::infinity());
        s->next.resize(n * n, n);
        s->portal.resize(n * n, portals.size());
        for (std::size_t i = 0; i < n; ++i)
            s->cost[i * n + i] = 0;
        for (std::size_t i = 0; i < portals.size(); ++i) {
            const auto& p = portals[i];
            const auto a = s->find(p.roomA), b = s->find(p.roomB);
            if (a == n || b == n || a == b || !validMedium(p.medium) || !isFinite(p.lengthMetres) ||
                p.lengthMetres < 0 || p.lengthMetres > 1e9 || !isFinite(p.opening) ||
                p.opening < 0 || p.opening > 1)
                return Status::invalid_argument;
            for (std::size_t j = 0; j < i; ++j)
                if (portals[j].id == p.id)
                    return Status::invalid_argument;
            for (double gain : p.bandGains)
                if (!isFinite(gain) || gain < 0 || gain > 1)
                    return Status::invalid_argument;
            if (p.opening == 0 || *std::max_element(p.bandGains.begin(), p.bandGains.end()) == 0)
                continue;
            const double cost =
                p.lengthMetres / p.medium.speedMetresPerSecond -
                weight * 20 *
                    std::log10(std::sqrt(std::max(1e-300, p.opening *
                                                              (p.bandGains[0] * p.bandGains[0] +
                                                               p.bandGains[1] * p.bandGains[1] +
                                                               p.bandGains[2] * p.bandGains[2]) /
                                                              3)));
            auto assign = [&](std::size_t from, std::size_t to) {
                if (cost < s->cost[from * n + to]) {
                    s->cost[from * n + to] = cost;
                    s->next[from * n + to] = to;
                    s->portal[from * n + to] = i;
                }
            };
            assign(a, b);
            if (p.bidirectional)
                assign(b, a);
        }
        for (std::size_t k = 0; k < n; ++k)
            for (std::size_t i = 0; i < n; ++i)
                for (std::size_t j = 0; j < n; ++j) {
                    const auto cost = s->cost[i * n + k] + s->cost[k * n + j];
                    if (cost < s->cost[i * n + j]) {
                        s->cost[i * n + j] = cost;
                        s->next[i * n + j] = s->next[i * n + k];
                        s->portal[i * n + j] = s->portal[i * n + k];
                    }
                }
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status AcousticPortalGraph::route(std::uint32_t source, std::uint32_t destination,
                                  std::span<std::uint32_t> output, std::size_t& count,
                                  AcousticTravel& travel) const noexcept {
    if (!state_)
        return Status::not_prepared;
    const auto& s = *state_;
    const auto n = s.rooms.size(), a = s.find(source), b = s.find(destination);
    if (a == n || b == n)
        return Status::invalid_argument;
    if (!isFinite(s.cost[a * n + b]))
        return Status::disconnected;
    AcousticTravel result;
    std::array<std::uint32_t, 256> ids{};
    std::size_t used = 0, current = a;
    while (current != b) {
        if (used >= n)
            return Status::invalid_argument;
        const auto p = s.portal[current * n + b];
        if (p >= s.portals.size())
            return Status::disconnected;
        const auto& portal = s.portals[p];
        ids[used++] = portal.id;
        result.seconds += portal.lengthMetres / portal.medium.speedMetresPerSecond;
        for (std::size_t band = 0; band < 3; ++band)
            result.bandGains[band] *=
                portal.bandGains[band] * std::sqrt(portal.opening) *
                std::pow(10., -portal.medium.absorptionDbPerMetre[band] * portal.lengthMetres / 20);
        current = s.next[current * n + b];
    }
    if (output.size() < used)
        return Status::invalid_argument;
    std::copy_n(ids.begin(), used, output.begin());
    count = used;
    travel = result;
    return Status::ok;
}
struct RoomCellGrid::State {
    Vec3 origin, spacing;
    std::array<std::size_t, 3> dimensions{};
    std::vector<RoomResponse> values;
};
RoomCellGrid::RoomCellGrid() = default;
RoomCellGrid::~RoomCellGrid() = default;
RoomCellGrid::RoomCellGrid(RoomCellGrid&&) noexcept = default;
RoomCellGrid& RoomCellGrid::operator=(RoomCellGrid&&) noexcept = default;
Status RoomCellGrid::prepare(Vec3 origin, Vec3 spacing, std::array<std::size_t, 3> dimensions,
                             std::span<const RoomResponse> values) noexcept {
    if (!isFinite(origin.x) || !isFinite(origin.y) || !isFinite(origin.z) ||
        std::abs(origin.x) > 1e9 || std::abs(origin.y) > 1e9 || std::abs(origin.z) > 1e9 ||
        !isFinite(spacing.x) || !isFinite(spacing.y) || !isFinite(spacing.z) || spacing.x < 1e-6 ||
        spacing.y < 1e-6 || spacing.z < 1e-6 || spacing.x > 1e6 || spacing.y > 1e6 ||
        spacing.z > 1e6)
        return Status::invalid_spec;
    std::size_t count = 1;
    for (auto dimension : dimensions) {
        if (!dimension || dimension > 1024 || count > (1u << 20) / dimension)
            return Status::invalid_spec;
        count *= dimension;
    }
    if (values.size() != count)
        return Status::invalid_argument;
    for (const auto& value : values)
        if (!validResponse(value))
            return Status::invalid_argument;
    try {
        auto s = std::make_unique<State>();
        s->origin = origin;
        s->spacing = spacing;
        s->dimensions = dimensions;
        s->values.assign(values.begin(), values.end());
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status RoomCellGrid::sample(Vec3 position, RoomResponse& output) const noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!isFinite(position.x) || !isFinite(position.y) || !isFinite(position.z) ||
        std::abs(position.x) > 1e12 || std::abs(position.y) > 1e12 || std::abs(position.z) > 1e12)
        return Status::invalid_argument;
    const auto& s = *state_;
    const auto p = position - s.origin;
    const std::array<double, 3> point{p.x / s.spacing.x, p.y / s.spacing.y, p.z / s.spacing.z};
    std::array<std::size_t, 3> first{}, second{};
    std::array<double, 3> t{};
    for (std::size_t axis = 0; axis < 3; ++axis) {
        const double value = std::clamp(point[axis], 0., double(s.dimensions[axis] - 1));
        first[axis] = std::size_t(value);
        second[axis] = std::min(first[axis] + 1, s.dimensions[axis] - 1);
        t[axis] = value - double(first[axis]);
    }
    RoomResponse result{{0, 0, 0}, 0, 0, 0};
    for (unsigned corner = 0; corner < 8; ++corner) {
        std::array<std::size_t, 3> index{};
        double weight = 1;
        for (unsigned axis = 0; axis < 3; ++axis) {
            const bool upper = (corner & (1u << axis)) != 0;
            index[axis] = upper ? second[axis] : first[axis];
            weight *= upper ? t[axis] : 1 - t[axis];
        }
        const auto& cell =
            s.values[(index[2] * s.dimensions[1] + index[1]) * s.dimensions[0] + index[0]];
        for (std::size_t band = 0; band < 3; ++band)
            result.decaySeconds[band] += weight * cell.decaySeconds[band];
        result.preDelaySeconds += weight * cell.preDelaySeconds;
        result.diffusion += weight * cell.diffusion;
        result.wet += weight * cell.wet;
    }
    output = result;
    return Status::ok;
}

template <Sample T> struct RoomReverbBank<T>::State {
    ProcessSpec spec;
    VectorOps ops;
    std::vector<FdnReverb<T>> rooms;
    std::vector<AudioBuffer<T>> input, output;
    std::size_t frames = 0, lastFrames = 0;
    bool begun = false;
};
template <Sample T> RoomReverbBank<T>::RoomReverbBank() = default;
template <Sample T> RoomReverbBank<T>::~RoomReverbBank() = default;
template <Sample T> RoomReverbBank<T>::RoomReverbBank(RoomReverbBank&&) noexcept = default;
template <Sample T>
RoomReverbBank<T>& RoomReverbBank<T>::operator=(RoomReverbBank&&) noexcept = default;
template <Sample T>
Status RoomReverbBank<T>::prepare(const ProcessSpec& spec, std::size_t count,
                                  const FdnConfig& config, CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || !count || count > 32)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        auto status = VectorOps::create(backend, s->ops);
        if (status != Status::ok)
            return status;
        s->rooms.resize(count);
        s->input.resize(count);
        s->output.resize(count);
        for (std::size_t room = 0; room < count; ++room) {
            status = s->rooms[room].prepare(spec, config, backend);
            if (status != Status::ok)
                return status;
            for (auto* block : {&s->input[room], &s->output[room]}) {
                status = block->resize(spec.channels, spec.maxBlockFrames);
                if (status != Status::ok)
                    return status;
            }
        }
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status RoomReverbBank<T>::setRoom(std::size_t room, const FdnParameters& p,
                                  std::size_t frames) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (room >= state_->rooms.size())
        return Status::invalid_argument;
    return state_->rooms[room].setParameters(p, frames);
}
template <Sample T> Status RoomReverbBank<T>::beginBlock(std::size_t frames) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (frames > s.spec.maxBlockFrames)
        return Status::block_too_large;
    if (s.begun)
        return Status::transition_in_progress;
    for (auto& block : s.input)
        for (std::size_t ch = 0; ch < s.spec.channels; ++ch)
            s.ops.fill(block.block(frames).channel(ch), T{});
    s.frames = frames;
    s.begun = true;
    return Status::ok;
}
template <Sample T>
Status RoomReverbBank<T>::send(std::size_t room, AudioBlock<const T> input, double gain) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (!s.begun || room >= s.rooms.size() || !isFinite(gain) || std::abs(gain) > 16 ||
        input.frames() != s.frames)
        return Status::invalid_argument;
    auto status = detail::validateAnalysis(s.spec, input);
    if (status != Status::ok)
        return status;
    auto target = s.input[room].block(s.frames);
    for (std::size_t ch = 0; ch < input.channels(); ++ch)
        (void)s.ops.addScaled(input.channel(ch), target.channel(ch), T(gain));
    return Status::ok;
}
template <Sample T> Status RoomReverbBank<T>::render() noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (!s.begun)
        return Status::invalid_argument;
    for (const auto& input : s.input) {
        auto status = detail::validateAnalysis(s.spec, input.block().subBlock(0, s.frames));
        if (status != Status::ok)
            return status;
    }
    for (std::size_t room = 0; room < s.rooms.size(); ++room) {
        auto status =
            s.rooms[room].process(s.input[room].block(s.frames), s.output[room].block(s.frames));
        if (status != Status::ok)
            return status;
    }
    s.lastFrames = s.frames;
    s.begun = false;
    return Status::ok;
}
template <Sample T> Status RoomReverbBank<T>::mix(AudioBlock<T> output) const noexcept {
    if (!state_)
        return Status::not_prepared;
    const auto& s = *state_;
    auto status = validateProcessBlocks<T>(s.spec, output, output);
    if (status != Status::ok)
        return status;
    if (output.frames() != s.lastFrames)
        return Status::invalid_argument;
    for (const auto& block : s.output) {
        status = validateDisjointBlocks(block.block().subBlock(0, s.lastFrames), output);
        if (status != Status::ok)
            return status;
    }
    for (std::size_t ch = 0; ch < output.channels(); ++ch) {
        s.ops.fill(output.channel(ch), T{});
        for (const auto& block : s.output)
            (void)s.ops.addScaled(block.block().subBlock(0, s.lastFrames).channel(ch),
                                  output.channel(ch), T{1});
    }
    return Status::ok;
}
template <Sample T>
AudioBlock<const T> RoomReverbBank<T>::roomOutput(std::size_t room) const noexcept {
    return state_ && room < state_->rooms.size() ? state_->output[room].block(state_->lastFrames)
                                                 : AudioBlock<const T>{};
}
template <Sample T> void RoomReverbBank<T>::reset() noexcept {
    if (state_) {
        auto& s = *state_;
        for (auto& room : s.rooms)
            room.reset();
        s.begun = false;
        s.frames = s.lastFrames = 0;
        for (auto& block : s.output)
            for (std::size_t ch = 0; ch < s.spec.channels; ++ch)
                s.ops.fill(block.block().channel(ch), T{});
    }
}
template class RoomReverbBank<float>;
template class RoomReverbBank<double>;
} // namespace cydsp
