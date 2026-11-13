#include "cydsp/processors/MultiBusGraph.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/core/Memory.h"
#include "cydsp/math/VectorOps.h"
#include "detail/Analysis.h"
#include <algorithm>
#include <array>
#include <vector>
namespace cydsp {
namespace {
bool validBuses(std::span<const DspBus> buses, bool allowEmpty = false) {
    if ((buses.empty() && !allowEmpty) || buses.size() > 32)
        return false;
    std::size_t channels = 0;
    for (std::size_t i = 0; i < buses.size(); ++i) {
        if (!buses[i].channels || buses[i].channels > 64)
            return false;
        channels += buses[i].channels;
        for (std::size_t j = 0; j < i; ++j)
            if (buses[j].id == buses[i].id)
                return false;
    }
    return channels <= 256;
}
std::size_t busIndex(std::span<const DspBus> buses, std::uint32_t id) {
    for (std::size_t i = 0; i < buses.size(); ++i)
        if (buses[i].id == id)
            return i;
    return buses.size();
}
bool sameBuses(std::span<const DspBus> a, std::span<const DspBus> b) {
    if (a.size() != b.size())
        return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (a[i].id != b[i].id || a[i].channels != b[i].channels)
            return false;
    return true;
}
template <Sample T>
Status validateGraphBlocks(const ProcessSpec& spec, std::span<const DspBus> ins,
                           std::span<const DspBus> outs, std::span<const AudioBlock<const T>> input,
                           std::span<AudioBlock<T>> output) {
    if (input.size() != ins.size() || output.size() != outs.size())
        return Status::channel_mismatch;
    const auto frames = output.empty() ? std::size_t{0} : output[0].frames();
    if (frames > spec.maxBlockFrames)
        return Status::block_too_large;
    for (std::size_t i = 0; i < input.size(); ++i) {
        if (input[i].frames() != frames)
            return Status::invalid_argument;
        auto local = spec;
        local.channels = ins[i].channels;
        auto s = detail::validateAnalysis(local, input[i]);
        if (s != Status::ok)
            return s;
    }
    for (std::size_t i = 0; i < output.size(); ++i) {
        if (!output[i].isValid() || output[i].frames() != frames)
            return Status::invalid_argument;
        if (output[i].channels() != outs[i].channels)
            return Status::channel_mismatch;
        auto local = spec;
        local.channels = outs[i].channels;
        auto s = validateProcessBlocks<T>(local, output[i], output[i]);
        if (s != Status::ok)
            return s;
        for (std::size_t j = 0; j < i; ++j) {
            s = validateDisjointBlocks(AudioBlock<const T>(output[j]), output[i]);
            if (s != Status::ok)
                return s;
        }
    }
    return Status::ok;
}
} // namespace
template <Sample T> struct MultiBusGraph<T>::State {
    struct Node {
        std::uint32_t id = 0;
        void* context = nullptr;
        typename DspNode<T>::Process process = nullptr;
        void (*reset)(void*) noexcept = nullptr;
        DspTiming timing;
        std::vector<DspBus> ins, outs;
        std::vector<AudioBuffer<T>> input, output;
        std::vector<AudioBlock<const T>> inViews;
        std::vector<AudioBlock<T>> outViews;
        std::vector<T> dryHistory;
        std::size_t dryHead = 0, totalOutputs = 0;
        double bypass = 0, target = 0, step = 0;
        std::size_t fade = 0, arrival = 0, tail = 0;
    };
    struct Route {
        std::size_t source = 0, sourceBus = 0, destination = 0, destinationBus = 0, delay = 0,
                    head = 0, channels = 0;
        T gain = T{1};
        std::vector<T> history;
    };
    ProcessSpec spec;
    std::vector<DspBus> ins, outs;
    std::vector<AudioBuffer<T>> external, results;
    std::vector<Node> nodes;
    std::vector<Route> routes;
    std::vector<std::size_t> order;
    VectorOps ops;
    DspTiming timing;
    std::size_t find(std::uint32_t id) const {
        for (std::size_t i = 0; i < nodes.size(); ++i)
            if (nodes[i].id == id)
                return i;
        return nodes.size();
    }
    void route(Route& r, std::size_t frames) noexcept {
        const auto source = r.source == nodes.size()
                                ? external[r.sourceBus].block(frames)
                                : nodes[r.source].output[r.sourceBus].block(frames);
        auto dest = r.destination == nodes.size()
                        ? results[r.destinationBus].block(frames)
                        : nodes[r.destination].input[r.destinationBus].block(frames);
        if (!r.delay) {
            for (std::size_t ch = 0; ch < r.channels; ++ch)
                (void)ops.addScaled(source.channel(ch), dest.channel(ch), r.gain);
            return;
        }
        const auto capacity = r.delay + 1;
        for (std::size_t i = 0; i < frames; ++i) {
            const auto read = (r.head + 1) % capacity;
            for (std::size_t ch = 0; ch < r.channels; ++ch) {
                auto* line = r.history.data() + ch * capacity;
                line[r.head] = source.channel(ch)[i];
                dest.channel(ch)[i] += line[read] * r.gain;
            }
            r.head = read;
        }
    }
    void bypass(Node& node, std::size_t frames) noexcept {
        const auto capacity = node.timing.latencyFrames + 1;
        for (std::size_t i = 0; i < frames; ++i) {
            if (node.fade) {
                node.bypass += node.step;
                if (!--node.fade)
                    node.bypass = node.target;
            }
            std::size_t channel = 0;
            for (std::size_t bus = 0; bus < node.output.size(); ++bus) {
                auto out = node.output[bus].block(frames);
                for (std::size_t ch = 0; ch < out.channels(); ++ch, ++channel) {
                    T dry{};
                    if (!node.input.empty() && ch < node.input[0].block().channels())
                        dry = node.input[0].block(frames).channel(ch)[i];
                    if (node.timing.latencyFrames) {
                        auto* history = node.dryHistory.data() + channel * capacity;
                        history[node.dryHead] = dry;
                        dry = history[(node.dryHead + 1) % capacity];
                    }
                    out.channel(ch)[i] = T(double(out.channel(ch)[i]) * (1 - node.bypass) +
                                           double(dry) * node.bypass);
                }
            }
            if (++node.dryHead == capacity)
                node.dryHead = 0;
        }
    }
};
template <Sample T> MultiBusGraph<T>::MultiBusGraph() = default;
template <Sample T> MultiBusGraph<T>::~MultiBusGraph() = default;
template <Sample T> MultiBusGraph<T>::MultiBusGraph(MultiBusGraph&&) noexcept = default;
template <Sample T>
MultiBusGraph<T>& MultiBusGraph<T>::operator=(MultiBusGraph&&) noexcept = default;
template <Sample T>
Status
MultiBusGraph<T>::prepare(const ProcessSpec& spec, std::span<const DspBus> ins,
                          std::span<const DspBus> outs, std::span<const DspNode<T>> definitions,
                          std::span<const DspConnection> connections, CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || !validBuses(ins, true) || !validBuses(outs) ||
        definitions.size() > 256 || connections.size() > 4096)
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->ins.assign(ins.begin(), ins.end());
        s->outs.assign(outs.begin(), outs.end());
        auto status = VectorOps::create(backend, s->ops);
        if (status != Status::ok)
            return status;
        s->external.resize(ins.size());
        s->results.resize(outs.size());
        for (std::size_t i = 0; i < ins.size(); ++i) {
            status = s->external[i].resize(ins[i].channels, spec.maxBlockFrames);
            if (status != Status::ok)
                return status;
        }
        for (std::size_t i = 0; i < outs.size(); ++i) {
            status = s->results[i].resize(outs[i].channels, spec.maxBlockFrames);
            if (status != Status::ok)
                return status;
        }
        s->nodes.resize(definitions.size());
        std::size_t memory = 0;
        for (std::size_t i = 0; i < definitions.size(); ++i) {
            const auto& d = definitions[i];
            if (d.id == graphInputNode || d.id == graphOutputNode || !d.process ||
                !validBuses(d.inputs, true) || !validBuses(d.outputs) ||
                d.timing.latencyFrames > (1u << 20) ||
                (d.timing.tailFrames > (1u << 30) &&
                 d.timing.tailFrames != std::numeric_limits<std::size_t>::max()))
                return Status::invalid_argument;
            for (std::size_t j = 0; j < i; ++j)
                if (definitions[j].id == d.id)
                    return Status::invalid_argument;
            auto& n = s->nodes[i];
            n.id = d.id;
            n.context = d.context;
            n.process = d.process;
            n.reset = d.reset;
            n.timing = d.timing;
            n.ins.assign(d.inputs.begin(), d.inputs.end());
            n.outs.assign(d.outputs.begin(), d.outputs.end());
            n.input.resize(d.inputs.size());
            n.output.resize(d.outputs.size());
            n.inViews.resize(d.inputs.size());
            n.outViews.resize(d.outputs.size());
            for (std::size_t b = 0; b < d.inputs.size(); ++b) {
                status = n.input[b].resize(d.inputs[b].channels, spec.maxBlockFrames);
                if (status != Status::ok)
                    return status;
                memory += d.inputs[b].channels * spec.maxBlockFrames;
            }
            for (std::size_t b = 0; b < d.outputs.size(); ++b) {
                status = n.output[b].resize(d.outputs[b].channels, spec.maxBlockFrames);
                if (status != Status::ok)
                    return status;
                n.totalOutputs += d.outputs[b].channels;
                memory += d.outputs[b].channels * spec.maxBlockFrames;
            }
            if (n.totalOutputs * (n.timing.latencyFrames + 1) >
                (1u << 27) - std::min(memory, std::size_t(1u << 27)))
                return Status::invalid_spec;
            memory += n.totalOutputs * (n.timing.latencyFrames + 1);
            if (memory > (1u << 27))
                return Status::invalid_spec;
            n.dryHistory.resize(
                n.timing.latencyFrames ? n.totalOutputs * (n.timing.latencyFrames + 1) : 0);
        }
        std::vector<std::size_t> incoming(definitions.size());
        for (const auto& c : connections) {
            if (!isFinite(c.gain) || std::abs(c.gain) > 64 || c.sourceNode == graphOutputNode ||
                c.destinationNode == graphInputNode)
                return Status::invalid_argument;
            const auto src =
                           c.sourceNode == graphInputNode ? s->nodes.size() : s->find(c.sourceNode),
                       dst = c.destinationNode == graphOutputNode ? s->nodes.size()
                                                                  : s->find(c.destinationNode);
            if ((src == s->nodes.size() && c.sourceNode != graphInputNode) ||
                (dst == s->nodes.size() && c.destinationNode != graphOutputNode))
                return Status::invalid_argument;
            const auto source = src == s->nodes.size()
                                    ? std::span<const DspBus>(s->ins)
                                    : std::span<const DspBus>(s->nodes[src].outs),
                       destination = dst == s->nodes.size()
                                         ? std::span<const DspBus>(s->outs)
                                         : std::span<const DspBus>(s->nodes[dst].ins);
            const auto a = busIndex(source, c.sourceBus),
                       b = busIndex(destination, c.destinationBus);
            if (a == source.size() || b == destination.size() ||
                source[a].channels != destination[b].channels)
                return Status::channel_mismatch;
            typename State::Route route;
            route.source = src;
            route.sourceBus = a;
            route.destination = dst;
            route.destinationBus = b;
            route.channels = source[a].channels;
            route.gain = T(c.gain);
            s->routes.push_back(std::move(route));
            if (src < s->nodes.size() && dst < s->nodes.size())
                ++incoming[dst];
        }
        for (std::size_t i = 0; i < incoming.size(); ++i)
            if (!incoming[i])
                s->order.push_back(i);
        for (std::size_t at = 0; at < s->order.size(); ++at)
            for (const auto& route : s->routes)
                if (route.source == s->order[at] && route.destination < s->nodes.size() &&
                    !--incoming[route.destination])
                    s->order.push_back(route.destination);
        if (s->order.size() != s->nodes.size())
            return Status::invalid_argument;
        for (auto index : s->order) {
            auto& node = s->nodes[index];
            std::size_t arrival = 0, tail = 0;
            for (const auto& route : s->routes)
                if (route.destination == index && route.source < s->nodes.size()) {
                    arrival = std::max(arrival, s->nodes[route.source].arrival);
                    tail = std::max(tail, s->nodes[route.source].tail);
                }
            const bool infinite = tail == std::numeric_limits<std::size_t>::max() ||
                                  node.timing.tailFrames == std::numeric_limits<std::size_t>::max();
            if (arrival > (1u << 20) - node.timing.latencyFrames ||
                (!infinite && tail > (1u << 30) - node.timing.tailFrames))
                return Status::invalid_spec;
            node.arrival = arrival + node.timing.latencyFrames;
            node.tail =
                infinite ? std::numeric_limits<std::size_t>::max() : tail + node.timing.tailFrames;
        }
        for (const auto& route : s->routes)
            if (route.destination == s->nodes.size() && route.source < s->nodes.size()) {
                s->timing.latencyFrames =
                    std::max(s->timing.latencyFrames, s->nodes[route.source].arrival);
                s->timing.tailFrames = std::max(s->timing.tailFrames, s->nodes[route.source].tail);
            }
        for (auto& route : s->routes) {
            const auto arrival =
                route.source < s->nodes.size() ? s->nodes[route.source].arrival : 0;
            const auto target = route.destination < s->nodes.size()
                                    ? s->nodes[route.destination].arrival -
                                          s->nodes[route.destination].timing.latencyFrames
                                    : s->timing.latencyFrames;
            route.delay = target - arrival;
            if (route.delay) {
                const auto amount = route.channels * (route.delay + 1);
                if (amount > (1u << 27) - memory)
                    return Status::invalid_spec;
                memory += amount;
                route.history.resize(amount);
            }
        }
        if (s->timing.tailFrames != std::numeric_limits<std::size_t>::max())
            s->timing.tailFrames += s->timing.latencyFrames;
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status MultiBusGraph<T>::process(std::span<const AudioBlock<const T>> input,
                                 std::span<AudioBlock<T>> output) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateGraphBlocks(s.spec, s.ins, s.outs, input, output);
    if (status != Status::ok)
        return status;
    const auto frames = output[0].frames();
    if (!frames)
        return Status::ok;
    for (std::size_t b = 0; b < input.size(); ++b)
        for (std::size_t ch = 0; ch < input[b].channels(); ++ch)
            (void)s.ops.copy(input[b].channel(ch), s.external[b].block(frames).channel(ch));
    for (auto& b : s.results)
        for (std::size_t ch = 0; ch < b.block().channels(); ++ch)
            s.ops.fill(b.block(frames).channel(ch), T{});
    for (auto index : s.order) {
        auto& node = s.nodes[index];
        for (auto& b : node.input)
            for (std::size_t ch = 0; ch < b.block().channels(); ++ch)
                s.ops.fill(b.block(frames).channel(ch), T{});
        for (auto& route : s.routes)
            if (route.destination == index)
                s.route(route, frames);
        for (std::size_t b = 0; b < node.input.size(); ++b)
            node.inViews[b] = node.input[b].block(frames);
        for (std::size_t b = 0; b < node.output.size(); ++b) {
            node.outViews[b] = node.output[b].block(frames);
            for (std::size_t ch = 0; ch < node.outViews[b].channels(); ++ch)
                s.ops.fill(node.outViews[b].channel(ch), T{});
        }
        status = node.process(node.context, node.inViews, node.outViews);
        if (status != Status::ok) {
            for (auto block : output)
                for (std::size_t ch = 0; ch < block.channels(); ++ch)
                    s.ops.fill(block.channel(ch), T{});
            return status;
        }
        s.bypass(node, frames);
    }
    for (auto& route : s.routes)
        if (route.destination == s.nodes.size())
            s.route(route, frames);
    for (std::size_t b = 0; b < output.size(); ++b)
        for (std::size_t ch = 0; ch < output[b].channels(); ++ch)
            (void)s.ops.copy(s.results[b].block(frames).channel(ch), output[b].channel(ch));
    return Status::ok;
}
template <Sample T>
Status MultiBusGraph<T>::setBypassed(std::uint32_t id, bool bypass, std::size_t frames) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    const auto index = s.find(id);
    if (index == s.nodes.size() || frames > (1u << 30))
        return Status::invalid_argument;
    auto& node = s.nodes[index];
    node.target = bypass ? 1 : 0;
    node.fade = frames;
    if (!frames)
        node.bypass = node.target;
    else
        node.step = (node.target - node.bypass) / double(frames);
    return Status::ok;
}
template <Sample T> void MultiBusGraph<T>::reset() noexcept {
    if (state_) {
        auto& s = *state_;
        for (auto& route : s.routes) {
            std::fill(route.history.begin(), route.history.end(), T{});
            route.head = 0;
        }
        for (auto& node : s.nodes) {
            std::fill(node.dryHistory.begin(), node.dryHistory.end(), T{});
            node.dryHead = 0;
            node.bypass = node.target;
            node.fade = 0;
            if (node.reset)
                node.reset(node.context);
        }
    }
}
template <Sample T> DspTiming MultiBusGraph<T>::timing() const noexcept {
    return state_ ? state_->timing : DspTiming{};
}
template <Sample T> std::span<const DspBus> MultiBusGraph<T>::inputBuses() const noexcept {
    return state_ ? std::span<const DspBus>(state_->ins) : std::span<const DspBus>{};
}
template <Sample T> std::span<const DspBus> MultiBusGraph<T>::outputBuses() const noexcept {
    return state_ ? std::span<const DspBus>(state_->outs) : std::span<const DspBus>{};
}
template <Sample T> ProcessSpec MultiBusGraph<T>::specification() const noexcept {
    return state_ ? state_->spec : ProcessSpec{0, 0, 0};
}
template <Sample T> bool MultiBusGraph<T>::prepared() const noexcept {
    return bool(state_);
}

template <Sample T> struct GraphTransition<T>::State {
    ProcessSpec spec;
    std::vector<DspBus> ins, outs;
    std::size_t budget = 0, current = 0, fade = 0, total = 0, warmup = 0;
    bool initial = false, staged = false, running = false;
    VectorOps ops;
    std::array<MultiBusGraph<T>, 2> graphs;
    std::array<std::vector<AudioBuffer<T>>, 2> scratch;
    std::array<std::vector<AudioBlock<T>>, 2> views;
    std::array<std::vector<T>, 2> history;
    std::array<std::size_t, 2> head{};
    bool accepts(const MultiBusGraph<T>& graph) const {
        const auto p = graph.specification();
        return graph.prepared() && p.sampleRate == spec.sampleRate &&
               p.maxBlockFrames >= spec.maxBlockFrames && graph.timing().latencyFrames <= budget &&
               sameBuses(ins, graph.inputBuses()) && sameBuses(outs, graph.outputBuses());
    }
    void align(std::size_t slot, std::size_t frames) noexcept {
        const auto delay = budget - graphs[slot].timing().latencyFrames;
        if (!delay)
            return;
        const auto capacity = budget + 1;
        for (std::size_t i = 0; i < frames; ++i) {
            const auto read = (head[slot] + capacity - delay) % capacity;
            std::size_t channel = 0;
            for (auto block : views[slot])
                for (std::size_t ch = 0; ch < block.channels(); ++ch, ++channel) {
                    auto* line = history[slot].data() + channel * capacity;
                    line[head[slot]] = block.channel(ch)[i];
                    block.channel(ch)[i] = line[read];
                }
            if (++head[slot] == capacity)
                head[slot] = 0;
        }
    }
};
template <Sample T> GraphTransition<T>::GraphTransition() = default;
template <Sample T> GraphTransition<T>::~GraphTransition() = default;
template <Sample T> GraphTransition<T>::GraphTransition(GraphTransition&&) noexcept = default;
template <Sample T>
GraphTransition<T>& GraphTransition<T>::operator=(GraphTransition&&) noexcept = default;
template <Sample T>
Status GraphTransition<T>::prepare(const ProcessSpec& spec, std::span<const DspBus> ins,
                                   std::span<const DspBus> outs, std::size_t budget,
                                   CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || !validBuses(ins, true) || !validBuses(outs) ||
        budget > (1u << 20))
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->ins.assign(ins.begin(), ins.end());
        s->outs.assign(outs.begin(), outs.end());
        s->budget = budget;
        auto status = VectorOps::create(backend, s->ops);
        if (status != Status::ok)
            return status;
        std::size_t channels = 0;
        for (auto bus : outs)
            channels += bus.channels;
        if (channels * (budget + 1) > (1u << 26))
            return Status::invalid_spec;
        for (std::size_t slot = 0; slot < 2; ++slot) {
            s->scratch[slot].resize(outs.size());
            s->views[slot].resize(outs.size());
            s->history[slot].resize(channels * (budget + 1));
            for (std::size_t b = 0; b < outs.size(); ++b) {
                status = s->scratch[slot][b].resize(outs[b].channels, spec.maxBlockFrames);
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
template <Sample T> Status GraphTransition<T>::setInitial(MultiBusGraph<T>&& graph) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    if (!s.accepts(graph))
        return Status::invalid_argument;
    if (s.initial)
        return Status::transition_in_progress;
    s.graphs[0] = std::move(graph);
    s.current = 0;
    s.initial = true;
    return Status::ok;
}
template <Sample T> Status GraphTransition<T>::stage(MultiBusGraph<T>&& graph) noexcept {
    if (!state_ || !state_->initial)
        return Status::not_prepared;
    auto& s = *state_;
    if (s.running)
        return Status::transition_in_progress;
    if (!s.accepts(graph))
        return Status::invalid_argument;
    const auto slot = 1 - s.current;
    s.graphs[slot] = std::move(graph);
    std::fill(s.history[slot].begin(), s.history[slot].end(), T{});
    s.head[slot] = 0;
    s.staged = true;
    return Status::ok;
}
template <Sample T>
Status GraphTransition<T>::startTransition(std::size_t fade, std::size_t extraWarmup) noexcept {
    if (!state_ || !state_->initial)
        return Status::not_prepared;
    auto& s = *state_;
    if (s.running)
        return Status::transition_in_progress;
    if (!s.staged || fade > (1u << 30) || extraWarmup > (1u << 30))
        return Status::invalid_argument;
    s.total = s.fade = fade;
    s.warmup = s.budget + extraWarmup;
    s.running = true;
    return Status::ok;
}
template <Sample T>
Status GraphTransition<T>::process(std::span<const AudioBlock<const T>> input,
                                   std::span<AudioBlock<T>> output) noexcept {
    if (!state_ || !state_->initial)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateGraphBlocks(s.spec, s.ins, s.outs, input, output);
    if (status != Status::ok)
        return status;
    const auto frames = output[0].frames();
    if (!frames)
        return Status::ok;
    const auto a = s.current, b = 1 - a;
    for (std::size_t slot = 0; slot < 2; ++slot) {
        if (slot != a && !s.running)
            continue;
        for (std::size_t bus = 0; bus < s.outs.size(); ++bus)
            s.views[slot][bus] = s.scratch[slot][bus].block(frames);
        status = s.graphs[slot].process(input, s.views[slot]);
        if (status != Status::ok) {
            for (auto block : output)
                for (std::size_t ch = 0; ch < block.channels(); ++ch)
                    s.ops.fill(block.channel(ch), T{});
            return status;
        }
        s.align(slot, frames);
    }
    for (std::size_t i = 0; i < frames; ++i) {
        double mix = 0;
        if (s.running) {
            if (s.warmup)
                --s.warmup;
            else {
                mix = s.fade ? double(s.total - s.fade + 1) / double(s.total) : 1;
                if (s.fade)
                    --s.fade;
                if (!s.fade) {
                    s.current = b;
                    s.running = false;
                    s.staged = false;
                }
            }
        } else if (s.current == b)
            mix = 1;
        for (std::size_t bus = 0; bus < output.size(); ++bus)
            for (std::size_t ch = 0; ch < output[bus].channels(); ++ch) {
                const double old = s.views[a][bus].channel(ch)[i];
                const double next = mix > 0 ? double(s.views[b][bus].channel(ch)[i]) : 0;
                output[bus].channel(ch)[i] = T(old * (1 - mix) + next * mix);
            }
    }
    return Status::ok;
}
template <Sample T> void GraphTransition<T>::reset() noexcept {
    if (state_) {
        auto& s = *state_;
        for (auto& graph : s.graphs)
            graph.reset();
        for (auto& h : s.history)
            std::fill(h.begin(), h.end(), T{});
        s.head = {};
        s.running = false;
        s.fade = s.warmup = 0;
    }
}
template <Sample T> std::size_t GraphTransition<T>::latencyFrames() const noexcept {
    return state_ ? state_->budget : 0;
}
template <Sample T> std::size_t GraphTransition<T>::transitionRemainingFrames() const noexcept {
    return state_ ? state_->fade : 0;
}
template <Sample T> std::size_t GraphTransition<T>::warmupRemainingFrames() const noexcept {
    return state_ ? state_->warmup : 0;
}
template class MultiBusGraph<float>;
template class MultiBusGraph<double>;
template class GraphTransition<float>;
template class GraphTransition<double>;
} // namespace cydsp
