---
id: api-processors-realfft
title: RealFft
description: Real transforms exposing DC through Nyquist bins, with a normalized inverse.
section: API reference
order: 1158
revision: 1
parent: api-processors
symbol: cydsp::RealFft
---

Real transforms exposing DC through Nyquist bins, with a normalized inverse.

## Include {#include}

```cpp
#include <cydsp/processors/RealFft.h>
```

[Usage and contracts](doc:fft) · [Common processing rules](doc:contracts)

## `RealFft` {#symbol-realfft}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class RealFft;
```

### `RealFft::RealFft<T>` {#symbol-realfft-realfft-t}

```cpp
RealFft() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `RealFft::RealFft<T>` {#symbol-realfft-realfft-t-2}

```cpp
RealFft(const RealFft&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const RealFft<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `RealFft::operator=` {#symbol-realfft-operator}

```cpp
RealFft& operator=(const RealFft&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const RealFft<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `RealFft<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `RealFft::RealFft<T>` {#symbol-realfft-realfft-t-3}

```cpp
RealFft(RealFft&& other) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `RealFft<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `RealFft::operator=` {#symbol-realfft-operator-2}

```cpp
RealFft& operator=(RealFft&& other) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `RealFft<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `RealFft<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `RealFft::prepare` {#symbol-realfft-prepare}

```cpp
Status prepare(std::size_t size, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `size` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `RealFft::reset` {#symbol-realfft-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `RealFft::forward` {#symbol-realfft-forward}

```cpp
Status forward(std::span<const T>, std::span<std::complex<T>>) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<std::complex<T>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `RealFft::inverse` {#symbol-realfft-inverse}

```cpp
Status inverse(std::span<const std::complex<T>>, std::span<T>) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const std::complex<T>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `RealFft::size` {#symbol-realfft-size}

```cpp
std::size_t size() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `RealFft::bins` {#symbol-realfft-bins}

```cpp
std::size_t bins() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `RealFft::backend` {#symbol-realfft-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
