---
id: api-processors-smoothedconvolver
title: SmoothedConvolver
description: Stage a new impulse off the audio thread, prewarm it and crossfade retained plans.
section: API reference
order: 1165
revision: 1
parent: api-processors
symbol: cydsp::SmoothedConvolver
---

Stage a new impulse off the audio thread, prewarm it and crossfade retained plans.

## Include {#include}

```cpp
#include <cydsp/processors/SmoothedConvolver.h>
```

[Usage and contracts](doc:convolution) · [Common processing rules](doc:contracts)

## `SmoothedConvolver` {#symbol-smoothedconvolver}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class SmoothedConvolver;
```

### `SmoothedConvolver::SmoothedConvolver<T>` {#symbol-smoothedconvolver-smoothedconvolver-t}

```cpp
SmoothedConvolver() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `SmoothedConvolver::SmoothedConvolver<T>` {#symbol-smoothedconvolver-smoothedconvolver-t-2}

```cpp
SmoothedConvolver(const SmoothedConvolver&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const SmoothedConvolver<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `SmoothedConvolver::operator=` {#symbol-smoothedconvolver-operator}

```cpp
SmoothedConvolver& operator=(const SmoothedConvolver&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const SmoothedConvolver<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `SmoothedConvolver<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedConvolver::SmoothedConvolver<T>` {#symbol-smoothedconvolver-smoothedconvolver-t-3}

```cpp
SmoothedConvolver(SmoothedConvolver&& other) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `SmoothedConvolver<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `SmoothedConvolver::operator=` {#symbol-smoothedconvolver-operator-2}

```cpp
SmoothedConvolver& operator=(SmoothedConvolver&& other) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `SmoothedConvolver<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `SmoothedConvolver<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedConvolver::prepare` {#symbol-smoothedconvolver-prepare}

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

### `SmoothedConvolver::stageImpulse` {#symbol-smoothedconvolver-stageimpulse}

```cpp
Status stageImpulse(std::span<const T>) noexcept;
```

Stage outside realtime processing; retained assets/contexts must satisfy the transition lifetime.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `SmoothedConvolver::startTransition` {#symbol-smoothedconvolver-starttransition}

```cpp
Status startTransition(std::size_t frames = 256, bool prewarm = true) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `frames` | `std::size_t` | `256` | Frame count or offset; one frame contains one sample per channel. |
| `prewarm` | `bool` | `true` | Explicit true/false option or predicate. |

Return: `Status` — Check Status; ok reports success.

### `SmoothedConvolver::discardStagedImpulse` {#symbol-smoothedconvolver-discardstagedimpulse}

```cpp
void discardStagedImpulse() noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

### `SmoothedConvolver::prepare` {#symbol-smoothedconvolver-prepare-2}

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

### `SmoothedConvolver::configuration` {#symbol-smoothedconvolver-configuration}

```cpp
ConvolutionConfig configuration() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `ConvolutionConfig` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedConvolver::reset` {#symbol-smoothedconvolver-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `SmoothedConvolver::process` {#symbol-smoothedconvolver-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `SmoothedConvolver::hasStagedImpulse` {#symbol-smoothedconvolver-hasstagedimpulse}

```cpp
bool hasStagedImpulse() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedConvolver::isTransitioning` {#symbol-smoothedconvolver-istransitioning}

```cpp
bool isTransitioning() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedConvolver::warmupRemainingFrames` {#symbol-smoothedconvolver-warmupremainingframes}

```cpp
std::size_t warmupRemainingFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedConvolver::transitionRemainingFrames` {#symbol-smoothedconvolver-transitionremainingframes}

```cpp
std::size_t transitionRemainingFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedConvolver::latencyFrames` {#symbol-smoothedconvolver-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedConvolver::tailFrames` {#symbol-smoothedconvolver-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedConvolver::partitionFrames` {#symbol-smoothedconvolver-partitionframes}

```cpp
std::size_t partitionFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedConvolver::backend` {#symbol-smoothedconvolver-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
