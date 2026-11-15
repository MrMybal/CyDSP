---
id: api-processors-channelmatrix
title: ChannelMatrix
description: Output-major routing matrices, gain ramps and exact cross-channel aliases.
section: API reference
order: 1133
revision: 1
parent: api-processors
symbol: cydsp::ChannelMatrix
---

Output-major routing matrices, gain ramps and exact cross-channel aliases.

## Include {#include}

```cpp
#include <cydsp/processors/ChannelMatrix.h>
```

[Usage and contracts](doc:mixing) · [Common processing rules](doc:contracts)

## `defaultChannelMatrix` {#symbol-defaultchannelmatrix}

```cpp
Status defaultChannelMatrix(std::size_t inputChannels, std::size_t outputChannels, std::span<double> coefficients) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `inputChannels` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `outputChannels` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `coefficients` | `std::span<double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

## `ChannelMatrix` {#symbol-channelmatrix}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class ChannelMatrix;
```

### `ChannelMatrix::ChannelMatrix<T>` {#symbol-channelmatrix-channelmatrix-t}

```cpp
ChannelMatrix() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `ChannelMatrix::ChannelMatrix<T>` {#symbol-channelmatrix-channelmatrix-t-2}

```cpp
ChannelMatrix(const ChannelMatrix&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ChannelMatrix<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `ChannelMatrix::operator=` {#symbol-channelmatrix-operator}

```cpp
ChannelMatrix& operator=(const ChannelMatrix&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ChannelMatrix<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `ChannelMatrix<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ChannelMatrix::ChannelMatrix<T>` {#symbol-channelmatrix-channelmatrix-t-3}

```cpp
ChannelMatrix(ChannelMatrix&& other) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `ChannelMatrix<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `ChannelMatrix::operator=` {#symbol-channelmatrix-operator-2}

```cpp
ChannelMatrix& operator=(ChannelMatrix&& other) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `ChannelMatrix<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `ChannelMatrix<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ChannelMatrix::prepare` {#symbol-channelmatrix-prepare}

```cpp
Status prepare(const ProcessSpec& inputSpec, std::size_t outputChannels, std::span<const double> coefficients, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `inputSpec` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `outputChannels` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `coefficients` | `std::span<const double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `ChannelMatrix::setMatrix` {#symbol-channelmatrix-setmatrix}

```cpp
Status setMatrix(std::span<const double>, std::size_t rampFrames = 0) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `rampFrames` | `std::size_t` | `0` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `ChannelMatrix::reset` {#symbol-channelmatrix-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `ChannelMatrix::process` {#symbol-channelmatrix-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `ChannelMatrix::inputChannels` {#symbol-channelmatrix-inputchannels}

```cpp
std::size_t inputChannels() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ChannelMatrix::outputChannels` {#symbol-channelmatrix-outputchannels}

```cpp
std::size_t outputChannels() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ChannelMatrix::rampRemainingFrames` {#symbol-channelmatrix-rampremainingframes}

```cpp
std::size_t rampRemainingFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ChannelMatrix::backend` {#symbol-channelmatrix-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
