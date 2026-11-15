---
id: api-processors-smoothedbiquadcascade
title: SmoothedBiquadCascade
description: Prepared cascade transitions with a coalesced following target.
section: API reference
order: 1164
revision: 1
parent: api-processors
symbol: cydsp::SmoothedBiquadCascade
---

Prepared cascade transitions with a coalesced following target.

## Include {#include}

```cpp
#include <cydsp/processors/SmoothedBiquadCascade.h>
```

[Usage and contracts](doc:filters) · [Common processing rules](doc:contracts)

## `SmoothedBiquadCascade` {#symbol-smoothedbiquadcascade}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class SmoothedBiquadCascade;
```

### `SmoothedBiquadCascade::SmoothedBiquadCascade<T>` {#symbol-smoothedbiquadcascade-smoothedbiquadcascade-t}

```cpp
SmoothedBiquadCascade() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `SmoothedBiquadCascade::SmoothedBiquadCascade<T>` {#symbol-smoothedbiquadcascade-smoothedbiquadcascade-t-2}

```cpp
SmoothedBiquadCascade(const SmoothedBiquadCascade&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const SmoothedBiquadCascade<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `SmoothedBiquadCascade::operator=` {#symbol-smoothedbiquadcascade-operator}

```cpp
SmoothedBiquadCascade& operator=(const SmoothedBiquadCascade&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const SmoothedBiquadCascade<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `SmoothedBiquadCascade<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedBiquadCascade::SmoothedBiquadCascade<T>` {#symbol-smoothedbiquadcascade-smoothedbiquadcascade-t-3}

```cpp
SmoothedBiquadCascade(SmoothedBiquadCascade&& other) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `SmoothedBiquadCascade<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `SmoothedBiquadCascade::operator=` {#symbol-smoothedbiquadcascade-operator-2}

```cpp
SmoothedBiquadCascade& operator=(SmoothedBiquadCascade&& other) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `SmoothedBiquadCascade<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `SmoothedBiquadCascade<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedBiquadCascade::prepare` {#symbol-smoothedbiquadcascade-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t maxSections = 16, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `maxSections` | `std::size_t` | `16` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `SmoothedBiquadCascade::setCoefficients` {#symbol-smoothedbiquadcascade-setcoefficients}

```cpp
Status setCoefficients(std::span<const BiquadCoefficients>, std::size_t transitionFrames = 128) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const BiquadCoefficients>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `transitionFrames` | `std::size_t` | `128` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `SmoothedBiquadCascade::reset` {#symbol-smoothedbiquadcascade-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `SmoothedBiquadCascade::process` {#symbol-smoothedbiquadcascade-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `SmoothedBiquadCascade::maxSections` {#symbol-smoothedbiquadcascade-maxsections}

```cpp
std::size_t maxSections() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedBiquadCascade::targetSections` {#symbol-smoothedbiquadcascade-targetsections}

```cpp
std::size_t targetSections() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedBiquadCascade::isTransitioning` {#symbol-smoothedbiquadcascade-istransitioning}

```cpp
bool isTransitioning() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedBiquadCascade::transitionRemainingFrames` {#symbol-smoothedbiquadcascade-transitionremainingframes}

```cpp
std::size_t transitionRemainingFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedBiquadCascade::backend` {#symbol-smoothedbiquadcascade-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
