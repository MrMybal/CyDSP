---
id: api-processors-hybridconvolver
title: HybridConvolver
description: Immediate direct head plus FFT tail without added block latency.
section: API reference
order: 1147
revision: 1
parent: api-processors
symbol: cydsp::HybridConvolver
---

Immediate direct head plus FFT tail without added block latency.

## Include {#include}

```cpp
#include <cydsp/processors/HybridConvolver.h>
```

[Usage and contracts](doc:convolution) · [Common processing rules](doc:contracts)

## `HybridConvolver` {#symbol-hybridconvolver}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class HybridConvolver;
```

### `HybridConvolver::HybridConvolver<T>` {#symbol-hybridconvolver-hybridconvolver-t}

```cpp
HybridConvolver() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `HybridConvolver::HybridConvolver<T>` {#symbol-hybridconvolver-hybridconvolver-t-2}

```cpp
HybridConvolver(const HybridConvolver&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const HybridConvolver<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `HybridConvolver::operator=` {#symbol-hybridconvolver-operator}

```cpp
HybridConvolver& operator=(const HybridConvolver&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const HybridConvolver<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `HybridConvolver<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `HybridConvolver::HybridConvolver<T>` {#symbol-hybridconvolver-hybridconvolver-t-3}

```cpp
HybridConvolver(HybridConvolver&& other) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `HybridConvolver<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `HybridConvolver::operator=` {#symbol-hybridconvolver-operator-2}

```cpp
HybridConvolver& operator=(HybridConvolver&& other) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `HybridConvolver<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `HybridConvolver<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `HybridConvolver::prepare` {#symbol-hybridconvolver-prepare}

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

### `HybridConvolver::reset` {#symbol-hybridconvolver-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `HybridConvolver::process` {#symbol-hybridconvolver-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `HybridConvolver::latencyFrames` {#symbol-hybridconvolver-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `HybridConvolver::tailFrames` {#symbol-hybridconvolver-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `HybridConvolver::partitionFrames` {#symbol-hybridconvolver-partitionframes}

```cpp
std::size_t partitionFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `HybridConvolver::backend` {#symbol-hybridconvolver-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
