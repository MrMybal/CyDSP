---
id: api-processors-convolver
title: Convolver
description: Select a prepared convolution strategy and its partition configuration.
section: API reference
order: 1135
revision: 1
parent: api-processors
symbol: cydsp::Convolver
---

Select a prepared convolution strategy and its partition configuration.

## Include {#include}

```cpp
#include <cydsp/processors/Convolver.h>
```

[Usage and contracts](doc:convolution) · [Common processing rules](doc:contracts)

## `ConvolutionMode` {#symbol-convolutionmode}

```cpp
enum class ConvolutionMode { hybrid, nonuniform, scheduled };
```

## `ConvolutionConfig` {#symbol-convolutionconfig}

```cpp
struct ConvolutionConfig;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `mode` | `ConvolutionMode` | `ConvolutionMode::hybrid` | Value/configuration in the declared type; apply the linked module contract. |
| `earlyPartition` | `std::size_t` | `128` | Value/configuration in the declared type; apply the linked module contract. |
| `latePartition` | `std::size_t` | `1024` | Value/configuration in the declared type; apply the linked module contract. |
| `spectralHead` | `bool` | `false` | Explicit true/false option or predicate. |

### `ConvolutionConfig::operator==` {#symbol-convolutionconfig-operator}

```cpp
bool operator==(const ConvolutionConfig&) const = default;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ConvolutionConfig &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `Convolver` {#symbol-convolver}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class Convolver;
```

### `Convolver::Convolver<T>` {#symbol-convolver-convolver-t}

```cpp
Convolver() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `Convolver::Convolver<T>` {#symbol-convolver-convolver-t-2}

```cpp
Convolver(const Convolver&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const Convolver<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `Convolver::operator=` {#symbol-convolver-operator}

```cpp
Convolver& operator=(const Convolver&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const Convolver<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Convolver<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Convolver::Convolver<T>` {#symbol-convolver-convolver-t-3}

```cpp
Convolver(Convolver&& other) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `Convolver<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `Convolver::operator=` {#symbol-convolver-operator-2}

```cpp
Convolver& operator=(Convolver&& other) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `Convolver<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Convolver<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Convolver::prepare` {#symbol-convolver-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const T>, const ConvolutionConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 3` | `const ConvolutionConfig &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `Convolver::reset` {#symbol-convolver-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `Convolver::process` {#symbol-convolver-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `Convolver::configuration` {#symbol-convolver-configuration}

```cpp
ConvolutionConfig configuration() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `ConvolutionConfig` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Convolver::latencyFrames` {#symbol-convolver-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Convolver::tailFrames` {#symbol-convolver-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Convolver::backend` {#symbol-convolver-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Convolver::partitionFrames` {#symbol-convolver-partitionframes}

```cpp
std::size_t partitionFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
