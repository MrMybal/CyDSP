---
id: api-processors-fir
title: Fir
description: Causal FIR filtering; tap zero multiplies the current sample.
section: API reference
order: 1143
revision: 1
parent: api-processors
symbol: cydsp::Fir
---

Causal FIR filtering; tap zero multiplies the current sample.

## Include {#include}

```cpp
#include <cydsp/processors/Fir.h>
```

[Usage and contracts](doc:filters) · [Common processing rules](doc:contracts)

## `Fir` {#symbol-fir}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class Fir;
```

### `Fir::Fir<T>` {#symbol-fir-fir-t}

```cpp
Fir() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `Fir::Fir<T>` {#symbol-fir-fir-t-2}

```cpp
Fir(const Fir&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const Fir<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `Fir::operator=` {#symbol-fir-operator}

```cpp
Fir& operator=(const Fir&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const Fir<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Fir<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Fir::Fir<T>` {#symbol-fir-fir-t-3}

```cpp
Fir(Fir&& other) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `Fir<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `Fir::operator=` {#symbol-fir-operator-2}

```cpp
Fir& operator=(Fir&& other) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `Fir<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Fir<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Fir::prepare` {#symbol-fir-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const T> coefficients, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `coefficients` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `Fir::reset` {#symbol-fir-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `Fir::process` {#symbol-fir-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `Fir::taps` {#symbol-fir-taps}

```cpp
std::size_t taps() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Fir::backend` {#symbol-fir-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
