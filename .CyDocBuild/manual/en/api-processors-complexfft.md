---
id: api-processors-complexfft
title: ComplexFft
description: Complex transforms with explicit forward/inverse direction and backend selection.
section: API reference
order: 1134
revision: 1
parent: api-processors
symbol: cydsp::ComplexFft
---

Complex transforms with explicit forward/inverse direction and backend selection.

## Include {#include}

```cpp
#include <cydsp/processors/ComplexFft.h>
```

[Usage and contracts](doc:fft) · [Common processing rules](doc:contracts)

## `FftDirection` {#symbol-fftdirection}

```cpp
enum class FftDirection { forward, inverse };
```

## `ComplexFft` {#symbol-complexfft}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class ComplexFft;
```

### `ComplexFft::ComplexFft<T>` {#symbol-complexfft-complexfft-t}

```cpp
ComplexFft() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `ComplexFft::ComplexFft<T>` {#symbol-complexfft-complexfft-t-2}

```cpp
ComplexFft(const ComplexFft&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ComplexFft<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `ComplexFft::operator=` {#symbol-complexfft-operator}

```cpp
ComplexFft& operator=(const ComplexFft&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ComplexFft<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `ComplexFft<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ComplexFft::ComplexFft<T>` {#symbol-complexfft-complexfft-t-3}

```cpp
ComplexFft(ComplexFft&& other) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `ComplexFft<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `ComplexFft::operator=` {#symbol-complexfft-operator-2}

```cpp
ComplexFft& operator=(ComplexFft&& other) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `ComplexFft<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `ComplexFft<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ComplexFft::prepare` {#symbol-complexfft-prepare}

```cpp
Status prepare(std::size_t size, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `size` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `ComplexFft::reset` {#symbol-complexfft-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `ComplexFft::process` {#symbol-complexfft-process}

```cpp
Status process(std::span<const std::complex<T>>, std::span<std::complex<T>>, FftDirection) const noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const std::complex<T>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<std::complex<T>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 3` | `FftDirection` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `ComplexFft::size` {#symbol-complexfft-size}

```cpp
std::size_t size() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ComplexFft::backend` {#symbol-complexfft-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
