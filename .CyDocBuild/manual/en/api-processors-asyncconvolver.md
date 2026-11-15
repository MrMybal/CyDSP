---
id: api-processors-asyncconvolver
title: AsyncConvolver
description: One producer stages impulses while one audio consumer uses retained plans without waiting.
section: API reference
order: 1127
revision: 1
parent: api-processors
symbol: cydsp::AsyncConvolver
---

One producer stages impulses while one audio consumer uses retained plans without waiting.

## Include {#include}

```cpp
#include <cydsp/processors/AsyncConvolver.h>
```

[Usage and contracts](doc:threading) · [Common processing rules](doc:contracts)

## `AsyncConvolver` {#symbol-asyncconvolver}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class AsyncConvolver;
```

### `AsyncConvolver::AsyncConvolver<T>` {#symbol-asyncconvolver-asyncconvolver-t}

```cpp
AsyncConvolver() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `AsyncConvolver::AsyncConvolver<T>` {#symbol-asyncconvolver-asyncconvolver-t-2}

```cpp
AsyncConvolver(const AsyncConvolver&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const AsyncConvolver<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `AsyncConvolver::operator=` {#symbol-asyncconvolver-operator}

```cpp
AsyncConvolver& operator=(const AsyncConvolver&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const AsyncConvolver<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `AsyncConvolver<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AsyncConvolver::AsyncConvolver<T>` {#symbol-asyncconvolver-asyncconvolver-t-3}

```cpp
AsyncConvolver(AsyncConvolver&&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AsyncConvolver<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `AsyncConvolver::operator=` {#symbol-asyncconvolver-operator-2}

```cpp
AsyncConvolver& operator=(AsyncConvolver&&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AsyncConvolver<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `AsyncConvolver<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AsyncConvolver::prepare` {#symbol-asyncconvolver-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const T>, std::size_t partition = 128, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `partition` | `std::size_t` | `128` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `AsyncConvolver::stageImpulse` {#symbol-asyncconvolver-stageimpulse}

```cpp
Status stageImpulse(std::span<const T>, std::size_t transitionFrames = 256, bool prewarm = true) noexcept;
```

Stage outside realtime processing; retained assets/contexts must satisfy the transition lifetime.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `transitionFrames` | `std::size_t` | `256` | Frame count or offset; one frame contains one sample per channel. |
| `prewarm` | `bool` | `true` | Explicit true/false option or predicate. |

Return: `Status` — Check Status; ok reports success.

### `AsyncConvolver::prepare` {#symbol-asyncconvolver-prepare-2}

```cpp
Status prepare(const ProcessSpec&, std::span<const T>, const ConvolutionConfig&, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 3` | `const ConvolutionConfig &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `AsyncConvolver::configuration` {#symbol-asyncconvolver-configuration}

```cpp
ConvolutionConfig configuration() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `ConvolutionConfig` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AsyncConvolver::reset` {#symbol-asyncconvolver-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `AsyncConvolver::process` {#symbol-asyncconvolver-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `AsyncConvolver::hasPendingImpulse` {#symbol-asyncconvolver-haspendingimpulse}

```cpp
bool hasPendingImpulse() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AsyncConvolver::isTransitioning` {#symbol-asyncconvolver-istransitioning}

```cpp
bool isTransitioning() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AsyncConvolver::warmupRemainingFrames` {#symbol-asyncconvolver-warmupremainingframes}

```cpp
std::size_t warmupRemainingFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AsyncConvolver::tailFrames` {#symbol-asyncconvolver-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AsyncConvolver::latencyFrames` {#symbol-asyncconvolver-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
