#pragma once
#include "cydsp/control/Parameters.h"
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include <cstdint>
#include <memory>
namespace cydsp {
inline constexpr std::uint32_t graphInputNode = 0, graphOutputNode = 0xffffffffu;
struct DspBus {
    std::uint32_t id = 0;
    std::size_t channels = 2;
};
struct DspConnection {
    std::uint32_t sourceNode = graphInputNode, sourceBus = 0, destinationNode = graphOutputNode,
                  destinationBus = 0;
    double gain = 1;
};
template <Sample T> struct DspNode {
    using Process = Status (*)(void*, std::span<const AudioBlock<const T>> inputs,
                               std::span<AudioBlock<T>> outputs) noexcept;
    std::uint32_t id = 1;
    void* context = nullptr;
    Process process = nullptr;
    std::span<const DspBus> inputs, outputs;
    DspTiming timing;
    void (*reset)(void*) noexcept = nullptr;
};
// Prepared DAG with typed bus widths and fixed node timings. Sources, callback
// contexts and callback code are borrowed; topology and bus definitions are copied.
// All input buses (including sidechains) align at each node. All external outputs
// align to the graph's slowest path. Callback errors silence external outputs;
// already executed node states cannot be rolled back.
template <Sample T> class MultiBusGraph {
  public:
    MultiBusGraph();
    ~MultiBusGraph();
    MultiBusGraph(MultiBusGraph&&) noexcept;
    MultiBusGraph& operator=(MultiBusGraph&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, std::span<const DspBus> inputs,
                                 std::span<const DspBus> outputs, std::span<const DspNode<T>>,
                                 std::span<const DspConnection>,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status process(std::span<const AudioBlock<const T>> inputs,
                                 std::span<AudioBlock<T>> outputs) noexcept;
    [[nodiscard]] Status setBypassed(std::uint32_t node, bool bypassed,
                                     std::size_t fadeFrames = 64) noexcept;
    void reset() noexcept;
    [[nodiscard]] DspTiming timing() const noexcept;
    [[nodiscard]] std::span<const DspBus> inputBuses() const noexcept;
    [[nodiscard]] std::span<const DspBus> outputBuses() const noexcept;
    [[nodiscard]] ProcessSpec specification() const noexcept;
    [[nodiscard]] bool prepared() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
// Two retained prepared graphs, no graph destruction during processing. Stage is
// non-real-time and owner-serialized; startTransition/process/reset retain storage.
// A fixed latency budget aligns both graphs, also outside transitions. A staged
// graph receives live input during warmup before the fade begins.
template <Sample T> class GraphTransition {
  public:
    GraphTransition();
    ~GraphTransition();
    GraphTransition(GraphTransition&&) noexcept;
    GraphTransition& operator=(GraphTransition&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, std::span<const DspBus> inputs,
                                 std::span<const DspBus> outputs, std::size_t latencyBudgetFrames,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setInitial(MultiBusGraph<T>&&) noexcept;
    [[nodiscard]] Status stage(MultiBusGraph<T>&&) noexcept;
    [[nodiscard]] Status startTransition(std::size_t fadeFrames,
                                         std::size_t extraWarmupFrames = 0) noexcept;
    [[nodiscard]] Status process(std::span<const AudioBlock<const T>>,
                                 std::span<AudioBlock<T>>) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept;
    [[nodiscard]] std::size_t transitionRemainingFrames() const noexcept;
    [[nodiscard]] std::size_t warmupRemainingFrames() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class MultiBusGraph<float>;
extern template class MultiBusGraph<double>;
extern template class GraphTransition<float>;
extern template class GraphTransition<double>;
} // namespace cydsp
