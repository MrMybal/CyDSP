---
id: api-processors-scheduledconvolver
title: ScheduledConvolver
description: Spread late FFT/MAC work across input frames while retaining an immediate onset.
section: API reference
order: 1161
revision: 1
parent: api-processors
symbol: cydsp::ScheduledConvolver
---

Spread late FFT/MAC work across input frames while retaining an immediate onset.

## Include {#include}

```cpp
#include <cydsp/processors/ScheduledConvolver.h>
```

[Usage and contracts](doc:convolution-options) · [Common processing rules](doc:contracts)

## `ScheduledConvolver` {#symbol-scheduledconvolver}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class ScheduledConvolver;
```

### `ScheduledConvolver::ScheduledConvolver<T>` {#symbol-scheduledconvolver-scheduledconvolver-t}

```cpp
ScheduledConvolver() noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `ScheduledConvolver::~ScheduledConvolver<T>` {#symbol-scheduledconvolver-scheduledconvolver-t-2}

```cpp
~ScheduledConvolver();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `ScheduledConvolver::ScheduledConvolver<T>` {#symbol-scheduledconvolver-scheduledconvolver-t-3}

```cpp
ScheduledConvolver(const ScheduledConvolver&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ScheduledConvolver<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `ScheduledConvolver::operator=` {#symbol-scheduledconvolver-operator}

```cpp
ScheduledConvolver& operator=(const ScheduledConvolver&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ScheduledConvolver<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `ScheduledConvolver<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ScheduledConvolver::ScheduledConvolver<T>` {#symbol-scheduledconvolver-scheduledconvolver-t-4}

```cpp
ScheduledConvolver(ScheduledConvolver&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `ScheduledConvolver<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `ScheduledConvolver::operator=` {#symbol-scheduledconvolver-operator-2}

```cpp
ScheduledConvolver& operator=(ScheduledConvolver&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `ScheduledConvolver<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `ScheduledConvolver<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ScheduledConvolver::prepare` {#symbol-scheduledconvolver-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const T>, std::size_t earlyPartition = 128, std::size_t latePartition = 1024, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `earlyPartition` | `std::size_t` | `128` | Value/configuration in the declared type; apply the linked module contract. |
| `latePartition` | `std::size_t` | `1024` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `ScheduledConvolver::reset` {#symbol-scheduledconvolver-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `ScheduledConvolver::process` {#symbol-scheduledconvolver-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `ScheduledConvolver::latencyFrames` {#symbol-scheduledconvolver-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ScheduledConvolver::tailFrames` {#symbol-scheduledconvolver-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ScheduledConvolver::backend` {#symbol-scheduledconvolver-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ScheduledConvolver::schedulingActive` {#symbol-scheduledconvolver-schedulingactive}

```cpp
bool schedulingActive() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ScheduledConvolver::workUnitsPerPartition` {#symbol-scheduledconvolver-workunitsperpartition}

```cpp
std::size_t workUnitsPerPartition() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ScheduledConvolver::maximumWorkUnitsPerFrame` {#symbol-scheduledconvolver-maximumworkunitsperframe}

```cpp
std::size_t maximumWorkUnitsPerFrame() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
