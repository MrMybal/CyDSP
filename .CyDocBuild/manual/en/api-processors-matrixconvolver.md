---
id: api-processors-matrixconvolver
title: MatrixConvolver
description: MIMO impulse routes with shared input transforms and prepared output coverage.
section: API reference
order: 1148
revision: 1
parent: api-processors
symbol: cydsp::MatrixConvolver
---

MIMO impulse routes with shared input transforms and prepared output coverage.

## Include {#include}

```cpp
#include <cydsp/processors/MatrixConvolver.h>
```

[Usage and contracts](doc:convolution-options) · [Common processing rules](doc:contracts)

## `ConvolutionRoute` {#symbol-convolutionroute}

```cpp
template <Sample T> struct ConvolutionRoute;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `inputChannel` | `std::size_t` | `0` | Channel index/count in the declared planar layout. |
| `outputChannel` | `std::size_t` | `0` | Channel index/count in the declared planar layout. |
| `impulse` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

## `MatrixConvolver` {#symbol-matrixconvolver}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class MatrixConvolver;
```

### `MatrixConvolver::MatrixConvolver<T>` {#symbol-matrixconvolver-matrixconvolver-t}

```cpp
MatrixConvolver() noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `MatrixConvolver::~MatrixConvolver<T>` {#symbol-matrixconvolver-matrixconvolver-t-2}

```cpp
~MatrixConvolver();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `MatrixConvolver::MatrixConvolver<T>` {#symbol-matrixconvolver-matrixconvolver-t-3}

```cpp
MatrixConvolver(const MatrixConvolver&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const MatrixConvolver<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `MatrixConvolver::operator=` {#symbol-matrixconvolver-operator}

```cpp
MatrixConvolver& operator=(const MatrixConvolver&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const MatrixConvolver<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `MatrixConvolver<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MatrixConvolver::MatrixConvolver<T>` {#symbol-matrixconvolver-matrixconvolver-t-4}

```cpp
MatrixConvolver(MatrixConvolver&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `MatrixConvolver<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `MatrixConvolver::operator=` {#symbol-matrixconvolver-operator-2}

```cpp
MatrixConvolver& operator=(MatrixConvolver&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `MatrixConvolver<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `MatrixConvolver<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MatrixConvolver::prepare` {#symbol-matrixconvolver-prepare}

```cpp
Status prepare(const ProcessSpec& inputSpec, std::size_t outputChannels, std::span<const ConvolutionRoute<T>>, std::size_t earlyPartition = 128, std::size_t latePartition = 1024, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `inputSpec` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `outputChannels` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `std::span<const ConvolutionRoute<T>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `earlyPartition` | `std::size_t` | `128` | Value/configuration in the declared type; apply the linked module contract. |
| `latePartition` | `std::size_t` | `1024` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 6` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `MatrixConvolver::prepare` {#symbol-matrixconvolver-prepare-2}

```cpp
Status prepare(const ProcessSpec& inputSpec, std::size_t outputChannels, std::span<const ConvolutionRoute<T>>, const ConvolutionConfig&, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `inputSpec` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `outputChannels` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `std::span<const ConvolutionRoute<T>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 4` | `const ConvolutionConfig &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `MatrixConvolver::configuration` {#symbol-matrixconvolver-configuration}

```cpp
ConvolutionConfig configuration() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `ConvolutionConfig` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MatrixConvolver::schedulingActive` {#symbol-matrixconvolver-schedulingactive}

```cpp
bool schedulingActive() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MatrixConvolver::workUnitsPerPartition` {#symbol-matrixconvolver-workunitsperpartition}

```cpp
std::size_t workUnitsPerPartition() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MatrixConvolver::maximumWorkUnitsPerFrame` {#symbol-matrixconvolver-maximumworkunitsperframe}

```cpp
std::size_t maximumWorkUnitsPerFrame() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MatrixConvolver::reset` {#symbol-matrixconvolver-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `MatrixConvolver::process` {#symbol-matrixconvolver-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `MatrixConvolver::latencyFrames` {#symbol-matrixconvolver-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MatrixConvolver::tailFrames` {#symbol-matrixconvolver-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MatrixConvolver::backend` {#symbol-matrixconvolver-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
