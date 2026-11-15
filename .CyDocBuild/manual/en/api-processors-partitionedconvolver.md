---
id: api-processors-partitionedconvolver
title: PartitionedConvolver
description: Uniform FFT partitions with explicit block latency and finite tail.
section: API reference
order: 1155
revision: 1
parent: api-processors
symbol: cydsp::PartitionedConvolver
---

Uniform FFT partitions with explicit block latency and finite tail.

## Include {#include}

```cpp
#include <cydsp/processors/PartitionedConvolver.h>
```

[Usage and contracts](doc:convolution) · [Common processing rules](doc:contracts)

## `PartitionedConvolver` {#symbol-partitionedconvolver}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class PartitionedConvolver;
```

### `PartitionedConvolver::PartitionedConvolver<T>` {#symbol-partitionedconvolver-partitionedconvolver-t}

```cpp
PartitionedConvolver() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `PartitionedConvolver::PartitionedConvolver<T>` {#symbol-partitionedconvolver-partitionedconvolver-t-2}

```cpp
PartitionedConvolver(const PartitionedConvolver&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const PartitionedConvolver<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `PartitionedConvolver::operator=` {#symbol-partitionedconvolver-operator}

```cpp
PartitionedConvolver& operator=(const PartitionedConvolver&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const PartitionedConvolver<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `PartitionedConvolver<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `PartitionedConvolver::PartitionedConvolver<T>` {#symbol-partitionedconvolver-partitionedconvolver-t-3}

```cpp
PartitionedConvolver(PartitionedConvolver&& other) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `PartitionedConvolver<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `PartitionedConvolver::operator=` {#symbol-partitionedconvolver-operator-2}

```cpp
PartitionedConvolver& operator=(PartitionedConvolver&& other) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `PartitionedConvolver<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `PartitionedConvolver<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `PartitionedConvolver::prepare` {#symbol-partitionedconvolver-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const T> impulse, std::size_t partitionFrames = 128, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `impulse` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `partitionFrames` | `std::size_t` | `128` | Frame count or offset; one frame contains one sample per channel. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `PartitionedConvolver::reset` {#symbol-partitionedconvolver-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `PartitionedConvolver::process` {#symbol-partitionedconvolver-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `PartitionedConvolver::latencyFrames` {#symbol-partitionedconvolver-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `PartitionedConvolver::tailFrames` {#symbol-partitionedconvolver-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `PartitionedConvolver::backend` {#symbol-partitionedconvolver-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
