---
id: api-processors-nonuniformconvolver
title: NonuniformConvolver
description: Early and late partition sizes to reduce the cost of long impulse responses.
section: API reference
order: 1153
revision: 1
parent: api-processors
symbol: cydsp::NonuniformConvolver
---

Early and late partition sizes to reduce the cost of long impulse responses.

## Include {#include}

```cpp
#include <cydsp/processors/NonuniformConvolver.h>
```

[Usage and contracts](doc:convolution) · [Common processing rules](doc:contracts)

## `NonuniformConvolver` {#symbol-nonuniformconvolver}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class NonuniformConvolver;
```

### `NonuniformConvolver::NonuniformConvolver<T>` {#symbol-nonuniformconvolver-nonuniformconvolver-t}

```cpp
NonuniformConvolver() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `NonuniformConvolver::NonuniformConvolver<T>` {#symbol-nonuniformconvolver-nonuniformconvolver-t-2}

```cpp
NonuniformConvolver(const NonuniformConvolver&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const NonuniformConvolver<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `NonuniformConvolver::operator=` {#symbol-nonuniformconvolver-operator}

```cpp
NonuniformConvolver& operator=(const NonuniformConvolver&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const NonuniformConvolver<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `NonuniformConvolver<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `NonuniformConvolver::NonuniformConvolver<T>` {#symbol-nonuniformconvolver-nonuniformconvolver-t-3}

```cpp
NonuniformConvolver(NonuniformConvolver&& other) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `NonuniformConvolver<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `NonuniformConvolver::operator=` {#symbol-nonuniformconvolver-operator-2}

```cpp
NonuniformConvolver& operator=(NonuniformConvolver&& other) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `NonuniformConvolver<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `NonuniformConvolver<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `NonuniformConvolver::prepare` {#symbol-nonuniformconvolver-prepare}

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

### `NonuniformConvolver::reset` {#symbol-nonuniformconvolver-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `NonuniformConvolver::process` {#symbol-nonuniformconvolver-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `NonuniformConvolver::latencyFrames` {#symbol-nonuniformconvolver-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `NonuniformConvolver::tailFrames` {#symbol-nonuniformconvolver-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `NonuniformConvolver::backend` {#symbol-nonuniformconvolver-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
