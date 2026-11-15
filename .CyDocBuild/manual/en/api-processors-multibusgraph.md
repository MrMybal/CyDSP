---
id: api-processors-multibusgraph
title: MultiBusGraph
description: Explicit buses, sidechains, latency alignment and transitions between prepared DAGs.
section: API reference
order: 1150
revision: 1
parent: api-processors
symbol: cydsp::MultiBusGraph
---

Explicit buses, sidechains, latency alignment and transitions between prepared DAGs.

## Include {#include}

```cpp
#include <cydsp/processors/MultiBusGraph.h>
```

[Usage and contracts](doc:graphs) · [Common processing rules](doc:contracts)

## `graphInputNode` {#symbol-graphinputnode}

```cpp
inline constexpr std::uint32_t graphInputNode = 0;
```

## `graphOutputNode` {#symbol-graphoutputnode}

```cpp
inline constexpr std::uint32_t graphOutputNode = 0xffffffffu;
```

## `DspBus` {#symbol-dspbus}

```cpp
struct DspBus;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `id` | `std::uint32_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `channels` | `std::size_t` | `2` | Channel index/count in the declared planar layout. |

## `DspConnection` {#symbol-dspconnection}

```cpp
struct DspConnection;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `sourceNode` | `std::uint32_t` | `graphInputNode` | Value/configuration in the declared type; apply the linked module contract. |
| `sourceBus` | `std::uint32_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `destinationNode` | `std::uint32_t` | `graphOutputNode` | Value/configuration in the declared type; apply the linked module contract. |
| `destinationBus` | `std::uint32_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `gain` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |

## `DspNode` {#symbol-dspnode}

```cpp
template <Sample T> struct DspNode;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `id` | `std::uint32_t` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `context` | `void *` | `nullptr` | Borrowed callback context; the application owns lifetime and synchronization. |
| `process` | `Process` | `nullptr` | Value/configuration in the declared type; apply the linked module contract. |
| `inputs` | `std::span<const DspBus>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `outputs` | `std::span<const DspBus>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `timing` | `DspTiming` | — | Value/configuration in the declared type; apply the linked module contract. |
| `reset` | `void (*)(void *) noexcept` | `nullptr` | Borrowed callback context; the application owns lifetime and synchronization. |

### `DspNode::Process` {#symbol-dspnode-process}

```cpp
using Process = Status (*)(void*, std::span<const AudioBlock<const T>> inputs, std::span<AudioBlock<T>> outputs) noexcept;
```

## `MultiBusGraph` {#symbol-multibusgraph}

```cpp
template <Sample T> class MultiBusGraph;
```

### `MultiBusGraph::MultiBusGraph<T>` {#symbol-multibusgraph-multibusgraph-t}

```cpp
MultiBusGraph();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `MultiBusGraph::~MultiBusGraph<T>` {#symbol-multibusgraph-multibusgraph-t-2}

```cpp
~MultiBusGraph();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `MultiBusGraph::MultiBusGraph<T>` {#symbol-multibusgraph-multibusgraph-t-3}

```cpp
MultiBusGraph(MultiBusGraph&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `MultiBusGraph<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `MultiBusGraph::operator=` {#symbol-multibusgraph-operator}

```cpp
MultiBusGraph& operator=(MultiBusGraph&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `MultiBusGraph<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `MultiBusGraph<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MultiBusGraph::prepare` {#symbol-multibusgraph-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const DspBus> inputs, std::span<const DspBus> outputs, std::span<const DspNode<T>>, std::span<const DspConnection>, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `inputs` | `std::span<const DspBus>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `outputs` | `std::span<const DspBus>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 4` | `std::span<const DspNode<T>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 5` | `std::span<const DspConnection>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 6` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `MultiBusGraph::process` {#symbol-multibusgraph-process}

```cpp
Status process(std::span<const AudioBlock<const T>> inputs, std::span<AudioBlock<T>> outputs) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `inputs` | `std::span<const AudioBlock<const T>>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `outputs` | `std::span<AudioBlock<T>>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `MultiBusGraph::setBypassed` {#symbol-multibusgraph-setbypassed}

```cpp
Status setBypassed(std::uint32_t node, bool bypassed, std::size_t fadeFrames = 64) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `node` | `std::uint32_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `bypassed` | `bool` | — | Explicit true/false option or predicate. |
| `fadeFrames` | `std::size_t` | `64` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `MultiBusGraph::reset` {#symbol-multibusgraph-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `MultiBusGraph::timing` {#symbol-multibusgraph-timing}

```cpp
DspTiming timing() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `DspTiming` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MultiBusGraph::inputBuses` {#symbol-multibusgraph-inputbuses}

```cpp
std::span<const DspBus> inputBuses() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::span<const DspBus>` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MultiBusGraph::outputBuses` {#symbol-multibusgraph-outputbuses}

```cpp
std::span<const DspBus> outputBuses() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::span<const DspBus>` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MultiBusGraph::specification` {#symbol-multibusgraph-specification}

```cpp
ProcessSpec specification() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `ProcessSpec` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MultiBusGraph::prepared` {#symbol-multibusgraph-prepared}

```cpp
bool prepared() const noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `GraphTransition` {#symbol-graphtransition}

```cpp
template <Sample T> class GraphTransition;
```

### `GraphTransition::GraphTransition<T>` {#symbol-graphtransition-graphtransition-t}

```cpp
GraphTransition();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `GraphTransition::~GraphTransition<T>` {#symbol-graphtransition-graphtransition-t-2}

```cpp
~GraphTransition();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `GraphTransition::GraphTransition<T>` {#symbol-graphtransition-graphtransition-t-3}

```cpp
GraphTransition(GraphTransition&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `GraphTransition<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `GraphTransition::operator=` {#symbol-graphtransition-operator}

```cpp
GraphTransition& operator=(GraphTransition&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `GraphTransition<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `GraphTransition<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `GraphTransition::prepare` {#symbol-graphtransition-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const DspBus> inputs, std::span<const DspBus> outputs, std::size_t latencyBudgetFrames, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `inputs` | `std::span<const DspBus>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `outputs` | `std::span<const DspBus>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `latencyBudgetFrames` | `std::size_t` | — | Frame count or offset; one frame contains one sample per channel. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `GraphTransition::setInitial` {#symbol-graphtransition-setinitial}

```cpp
Status setInitial(MultiBusGraph<T>&&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `MultiBusGraph<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

### `GraphTransition::stage` {#symbol-graphtransition-stage}

```cpp
Status stage(MultiBusGraph<T>&&) noexcept;
```

Stage outside realtime processing; retained assets/contexts must satisfy the transition lifetime.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `MultiBusGraph<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

### `GraphTransition::startTransition` {#symbol-graphtransition-starttransition}

```cpp
Status startTransition(std::size_t fadeFrames, std::size_t extraWarmupFrames = 0) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `fadeFrames` | `std::size_t` | — | Frame count or offset; one frame contains one sample per channel. |
| `extraWarmupFrames` | `std::size_t` | `0` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `GraphTransition::process` {#symbol-graphtransition-process}

```cpp
Status process(std::span<const AudioBlock<const T>>, std::span<AudioBlock<T>>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const AudioBlock<const T>>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `std::span<AudioBlock<T>>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `GraphTransition::reset` {#symbol-graphtransition-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `GraphTransition::latencyFrames` {#symbol-graphtransition-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `GraphTransition::transitionRemainingFrames` {#symbol-graphtransition-transitionremainingframes}

```cpp
std::size_t transitionRemainingFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `GraphTransition::warmupRemainingFrames` {#symbol-graphtransition-warmupremainingframes}

```cpp
std::size_t warmupRemainingFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
