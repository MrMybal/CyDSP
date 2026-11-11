#include "cydsp/processors/AudioGraph.h"
#include "detail/CheckedSize.h"
#include "detail/MatrixAlgorithm.h"
#include "detail/VectorTable.h"
#include <algorithm>
namespace cydsp {
template <Sample T>
    requires(!std::is_const_v<T>)
void AudioGraph<T>::swap(AudioGraph& other) noexcept {
    std::swap(spec_, other.spec_);
    std::swap(ops_, other.ops_);
    nodes_.swap(other.nodes_);
    outputs_.swap(other.outputs_);
    order_.swap(other.order_);
    slotFor_.swap(other.slotFor_);
    external_.swap(other.external_);
    sum_.swap(other.sum_);
    work_.swap(other.work_);
    std::swap(tile_, other.tile_);
    std::swap(slots_, other.slots_);
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status AudioGraph<T>::prepare(const ProcessSpec& spec, std::span<const GraphNode<T>> nodes,
                              std::span<const GraphInput> outputs, CpuBackend backend) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    if (nodes.size() > 4096)
        return Status::invalid_argument;
    const auto valid = [&](const GraphInput& edge) {
        return (edge.source == graphExternalInput || edge.source < nodes.size()) &&
               detail::validMatrixCoefficient<T>(edge.gain);
    };
    for (const auto& node : nodes)
        for (const auto& edge : node.inputs)
            if (!valid(edge))
                return Status::invalid_argument;
    for (const auto& edge : outputs)
        if (!valid(edge))
            return Status::invalid_argument;
    AudioGraph next;
    auto status = VectorOps::create(backend, next.ops_);
    if (status != Status::ok)
        return status;
    try {
        next.nodes_.resize(nodes.size());
        next.outputs_.assign(outputs.begin(), outputs.end());
        std::vector<std::size_t> indegree(nodes.size()), position(nodes.size()), last(nodes.size()),
            free;
        std::vector<bool> live(nodes.size());
        next.slotFor_.resize(nodes.size());
        next.order_.reserve(nodes.size());
        for (std::size_t id = 0; id < nodes.size(); ++id) {
            auto& owned = next.nodes_[id];
            owned.process = nodes[id].process;
            owned.context = nodes[id].context;
            owned.inputs.assign(nodes[id].inputs.begin(), nodes[id].inputs.end());
            for (auto& edge : owned.inputs) {
                edge.gain = static_cast<double>(static_cast<T>(edge.gain));
                if (edge.source != graphExternalInput)
                    ++indegree[id];
            }
        }
        for (auto& edge : next.outputs_)
            edge.gain = static_cast<double>(static_cast<T>(edge.gain));
        for (std::size_t id = 0; id < nodes.size(); ++id)
            if (!indegree[id])
                next.order_.push_back(id);
        for (std::size_t at = 0; at < next.order_.size(); ++at) {
            const auto source = next.order_[at];
            for (std::size_t id = 0; id < nodes.size(); ++id)
                for (const auto& edge : next.nodes_[id].inputs)
                    if (edge.source == source && --indegree[id] == 0)
                        next.order_.push_back(id);
        }
        if (next.order_.size() != nodes.size())
            return Status::invalid_argument;
        // Fuse uniquely consumed linear routing chains without multiplying gains together.
        // Each stage retains its sample-precision rounding, zero-gain mute and sum-with-zero.
        std::vector<std::size_t> uses(nodes.size());
        std::vector<bool> removed(nodes.size());
        for (const auto& node : next.nodes_)
            for (const auto& edge : node.inputs)
                if (edge.source != graphExternalInput)
                    ++uses[edge.source];
        for (const auto& edge : next.outputs_)
            if (edge.source != graphExternalInput)
                ++uses[edge.source];
        for (const auto id : next.order_) {
            auto& node = next.nodes_[id];
            if (node.process || node.inputs.size() != 1)
                continue;
            const auto gain = static_cast<T>(node.inputs[0].gain);
            const auto source = node.inputs[0].source;
            if (source != graphExternalInput && uses[source] == 1 &&
                !next.nodes_[source].gains.empty()) {
                node.gains = next.nodes_[source].gains;
                node.inputs[0].source = next.nodes_[source].inputs[0].source;
                removed[source] = true;
            }
            node.gains.push_back(gain);
            node.inputs[0].gain = 1;
        }
        std::erase_if(next.order_, [&](auto id) { return removed[id]; });
        for (std::size_t at = 0; at < next.order_.size(); ++at) {
            auto id = next.order_[at];
            position[id] = last[id] = at;
        }
        for (const auto id : next.order_)
            for (const auto& edge : next.nodes_[id].inputs)
                if (edge.source != graphExternalInput)
                    last[edge.source] = std::max(last[edge.source], position[id]);
        for (const auto& edge : next.outputs_)
            if (edge.source != graphExternalInput)
                last[edge.source] = next.order_.size();
        for (std::size_t at = 0; at < next.order_.size(); ++at) {
            for (std::size_t id = 0; id < nodes.size(); ++id)
                if (live[id] && last[id] <= at) {
                    free.push_back(next.slotFor_[id]);
                    live[id] = false;
                }
            const auto id = next.order_[at];
            if (free.empty())
                next.slotFor_[id] = next.slots_++;
            else {
                next.slotFor_[id] = free.back();
                free.pop_back();
            }
            live[id] = true;
        }
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
    next.tile_ = std::min(spec.maxBlockFrames, std::size_t{64});
    next.spec_ = spec;
    status = next.external_.resize(spec.channels, next.tile_);
    if (status != Status::ok)
        return status;
    status = next.sum_.resize(spec.channels, next.tile_);
    if (status != Status::ok)
        return status;
    if (next.slots_) {
        std::size_t channels{};
        if (!detail::checkedMultiply(next.slots_, spec.channels, channels))
            return Status::invalid_spec;
        status = next.work_.resize(channels, next.tile_);
        if (status != Status::ok)
            return status;
    }
    swap(next);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
AudioBlock<T> AudioGraph<T>::slot(std::size_t id, std::size_t frames) noexcept {
    return {work_.block().channelData() + slotFor_[id] * spec_.channels, spec_.channels, frames};
}
template <Sample T>
    requires(!std::is_const_v<T>)
void AudioGraph<T>::reset() noexcept {
    for (auto* buffer : {&external_, &sum_, &work_}) {
        auto view = buffer->block();
        for (std::size_t ch = 0; ch < view.channels(); ++ch)
            ops_.fill(view.channel(ch), T{});
    }
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status AudioGraph<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    if (!tile_)
        return Status::not_prepared;
    const auto validation = validateProcessBlocks(spec_, input, output);
    if (validation != Status::ok)
        return validation;
    std::size_t offset = 0;
    while (offset < input.frames()) {
        const auto count = std::min(tile_, input.frames() - offset);
        auto external = external_.block(count), sum = sum_.block(count);
        for (std::size_t ch = 0; ch < spec_.channels; ++ch)
            (void)ops_.copy(input.channel(ch).subspan(offset, count), external.channel(ch));
        const auto mix = [&](std::span<const GraphInput> edges, AudioBlock<T> target) noexcept {
            for (std::size_t ch = 0; ch < spec_.channels; ++ch)
                ops_.fill(target.channel(ch), T{});
            for (const auto& edge : edges) {
                const auto gain = static_cast<T>(edge.gain);
                if (gain == T{})
                    continue;
                const auto source =
                    edge.source == graphExternalInput ? external : slot(edge.source, count);
                for (std::size_t ch = 0; ch < spec_.channels; ++ch)
                    (void)ops_.addScaled(source.channel(ch), target.channel(ch), gain);
            }
        };
        for (const auto id : order_) {
            auto destination = slot(id, count);
            const auto& node = nodes_[id];
            if (!node.gains.empty()) {
                const auto source = node.inputs[0].source == graphExternalInput
                                        ? external
                                        : slot(node.inputs[0].source, count);
                const auto& table = detail::VectorAccess::table(ops_);
                for (std::size_t ch = 0; ch < spec_.channels; ++ch) {
                    if constexpr (std::same_as<T, float>)
                        table.f32.gainChain(source.channel(ch).data(),
                                            destination.channel(ch).data(), count,
                                            node.gains.data(), node.gains.size());
                    else
                        table.f64.gainChain(source.channel(ch).data(),
                                            destination.channel(ch).data(), count,
                                            node.gains.data(), node.gains.size());
                }
                continue;
            }
            mix(node.inputs, sum);
            if (node.process) {
                const auto status = node.process(node.context, sum, destination);
                if (status != Status::ok)
                    return status;
            } else
                for (std::size_t ch = 0; ch < spec_.channels; ++ch)
                    (void)ops_.copy(sum.channel(ch), destination.channel(ch));
        }
        mix(outputs_, output.subBlock(offset, count));
        offset += count;
    }
    return Status::ok;
}
template class AudioGraph<float>;
template class AudioGraph<double>;
} // namespace cydsp
