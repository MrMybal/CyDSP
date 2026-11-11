#pragma once
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/math/VectorOps.h"
namespace cydsp {
inline constexpr std::size_t graphExternalInput = std::numeric_limits<std::size_t>::max();
struct GraphInput {
    std::size_t source = graphExternalInput;
    double gain = 1;
};
template <Sample T> struct GraphNode {
    using Process = Status (*)(void*, AudioBlock<const T>, AudioBlock<T>) noexcept;
    Process process = nullptr; // Null means identity; routing gains are applied first.
    void* context = nullptr;
    std::span<const GraphInput> inputs;
};
// Fixed-rate DAG. Contexts are borrowed; definitions/edges are copied during prepare.
// Each node consumes the sum of its inputs, using the common planar channel count.
template <Sample T>
    requires(!std::is_const_v<T>)
class AudioGraph {
  public:
    AudioGraph() = default;
    AudioGraph(const AudioGraph&) = delete;
    AudioGraph& operator=(const AudioGraph&) = delete;
    AudioGraph(AudioGraph&& other) noexcept {
        swap(other);
    }
    AudioGraph& operator=(AudioGraph&& other) noexcept {
        if (this != &other) {
            AudioGraph next(std::move(other));
            swap(next);
        }
        return *this;
    }
    [[nodiscard]] Status prepare(const ProcessSpec&, std::span<const GraphNode<T>>,
                                 std::span<const GraphInput> outputs,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    // Does not reset borrowed processors; the owner resets their histories separately.
    void reset() noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    [[nodiscard]] std::size_t compiledNodeCount() const noexcept {
        return order_.size();
    }
    [[nodiscard]] std::size_t workBufferCount() const noexcept {
        return slots_;
    }

  private:
    struct Node {
        typename GraphNode<T>::Process process = nullptr;
        void* context = nullptr;
        std::vector<GraphInput> inputs;
        std::vector<T> gains;
    };
    void swap(AudioGraph&) noexcept;
    AudioBlock<T> slot(std::size_t, std::size_t) noexcept;
    ProcessSpec spec_{};
    VectorOps ops_;
    std::vector<Node> nodes_;
    std::vector<GraphInput> outputs_;
    std::vector<std::size_t> order_, slotFor_;
    AudioBuffer<T> external_, sum_, work_;
    std::size_t tile_ = 0, slots_ = 0;
};
extern template class AudioGraph<float>;
extern template class AudioGraph<double>;
} // namespace cydsp
