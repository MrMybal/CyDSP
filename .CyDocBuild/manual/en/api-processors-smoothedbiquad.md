---
id: api-processors-smoothedbiquad
title: SmoothedBiquad
description: Crossfade between stable filter states rather than interpolating potentially unstable coefficients.
section: API reference
order: 1163
revision: 1
parent: api-processors
symbol: cydsp::SmoothedBiquad
---

Crossfade between stable filter states rather than interpolating potentially unstable coefficients.

## Include {#include}

```cpp
#include <cydsp/processors/SmoothedBiquad.h>
```

[Usage and contracts](doc:filters) · [Common processing rules](doc:contracts)

## `SmoothedBiquad` {#symbol-smoothedbiquad}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class SmoothedBiquad;
```

### `SmoothedBiquad::SmoothedBiquad<T>` {#symbol-smoothedbiquad-smoothedbiquad-t}

```cpp
SmoothedBiquad() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `SmoothedBiquad::SmoothedBiquad<T>` {#symbol-smoothedbiquad-smoothedbiquad-t-2}

```cpp
SmoothedBiquad(const SmoothedBiquad&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const SmoothedBiquad<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `SmoothedBiquad::operator=` {#symbol-smoothedbiquad-operator}

```cpp
SmoothedBiquad& operator=(const SmoothedBiquad&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const SmoothedBiquad<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `SmoothedBiquad<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedBiquad::SmoothedBiquad<T>` {#symbol-smoothedbiquad-smoothedbiquad-t-3}

```cpp
SmoothedBiquad(SmoothedBiquad&& other) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `SmoothedBiquad<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `SmoothedBiquad::operator=` {#symbol-smoothedbiquad-operator-2}

```cpp
SmoothedBiquad& operator=(SmoothedBiquad&& other) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `SmoothedBiquad<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `SmoothedBiquad<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedBiquad::prepare` {#symbol-smoothedbiquad-prepare}

```cpp
Status prepare(const ProcessSpec&, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `SmoothedBiquad::setCoefficients` {#symbol-smoothedbiquad-setcoefficients}

```cpp
Status setCoefficients(const BiquadCoefficients&, std::size_t transitionFrames = 64) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const BiquadCoefficients &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `transitionFrames` | `std::size_t` | `64` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `SmoothedBiquad::reset` {#symbol-smoothedbiquad-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `SmoothedBiquad::process` {#symbol-smoothedbiquad-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `SmoothedBiquad::isTransitioning` {#symbol-smoothedbiquad-istransitioning}

```cpp
bool isTransitioning() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedBiquad::hasQueuedTransition` {#symbol-smoothedbiquad-hasqueuedtransition}

```cpp
bool hasQueuedTransition() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedBiquad::transitionRemainingFrames` {#symbol-smoothedbiquad-transitionremainingframes}

```cpp
std::size_t transitionRemainingFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedBiquad::targetCoefficients` {#symbol-smoothedbiquad-targetcoefficients}

```cpp
const BiquadCoefficients& targetCoefficients() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `const BiquadCoefficients &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedBiquad::backend` {#symbol-smoothedbiquad-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
