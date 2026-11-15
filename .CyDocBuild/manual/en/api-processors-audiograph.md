---
id: api-processors-audiograph
title: AudioGraph
description: Prepared single-bus DAG processing, retained scratch and borrowed callbacks.
section: API reference
order: 1128
revision: 1
parent: api-processors
symbol: cydsp::AudioGraph
---

Prepared single-bus DAG processing, retained scratch and borrowed callbacks.

## Include {#include}

```cpp
#include <cydsp/processors/AudioGraph.h>
```

[Usage and contracts](doc:graphs) · [Common processing rules](doc:contracts)

## `graphExternalInput` {#symbol-graphexternalinput}

```cpp
inline constexpr std::size_t graphExternalInput = std::numeric_limits<std::size_t>::max();
```

## `GraphInput` {#symbol-graphinput}

```cpp
struct GraphInput;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `source` | `std::size_t` | `graphExternalInput` | Value/configuration in the declared type; apply the linked module contract. |
| `gain` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |

## `GraphNode` {#symbol-graphnode}

```cpp
template <Sample T> struct GraphNode;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `process` | `Process` | `nullptr` | Value/configuration in the declared type; apply the linked module contract. |
| `context` | `void *` | `nullptr` | Borrowed callback context; the application owns lifetime and synchronization. |
| `inputs` | `std::span<const GraphInput>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

### `GraphNode::Process` {#symbol-graphnode-process}

```cpp
using Process = Status (*)(void*, AudioBlock<const T>, AudioBlock<T>) noexcept;
```

## `AudioGraph` {#symbol-audiograph}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class AudioGraph;
```

### `AudioGraph::AudioGraph<T>` {#symbol-audiograph-audiograph-t}

```cpp
AudioGraph() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `AudioGraph::AudioGraph<T>` {#symbol-audiograph-audiograph-t-2}

```cpp
AudioGraph(const AudioGraph&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const AudioGraph<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `AudioGraph::operator=` {#symbol-audiograph-operator}

```cpp
AudioGraph& operator=(const AudioGraph&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const AudioGraph<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `AudioGraph<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AudioGraph::AudioGraph<T>` {#symbol-audiograph-audiograph-t-3}

```cpp
AudioGraph(AudioGraph&& other) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `AudioGraph<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `AudioGraph::operator=` {#symbol-audiograph-operator-2}

```cpp
AudioGraph& operator=(AudioGraph&& other) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `AudioGraph<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `AudioGraph<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AudioGraph::prepare` {#symbol-audiograph-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const GraphNode<T>>, std::span<const GraphInput> outputs, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `std::span<const GraphNode<T>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `outputs` | `std::span<const GraphInput>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `AudioGraph::reset` {#symbol-audiograph-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `AudioGraph::process` {#symbol-audiograph-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `AudioGraph::compiledNodeCount` {#symbol-audiograph-compilednodecount}

```cpp
std::size_t compiledNodeCount() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AudioGraph::workBufferCount` {#symbol-audiograph-workbuffercount}

```cpp
std::size_t workBufferCount() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
