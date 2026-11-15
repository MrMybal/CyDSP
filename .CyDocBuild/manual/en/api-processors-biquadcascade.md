---
id: api-processors-biquadcascade
title: BiquadCascade
description: Cascade of validated biquad sections with independent channel histories.
section: API reference
order: 1132
revision: 1
parent: api-processors
symbol: cydsp::BiquadCascade
---

Cascade of validated biquad sections with independent channel histories.

## Include {#include}

```cpp
#include <cydsp/processors/BiquadCascade.h>
```

[Usage and contracts](doc:filters) · [Common processing rules](doc:contracts)

## `BiquadCascade` {#symbol-biquadcascade}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class BiquadCascade;
```

### `BiquadCascade::BiquadCascade<T>` {#symbol-biquadcascade-biquadcascade-t}

```cpp
BiquadCascade() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `BiquadCascade::BiquadCascade<T>` {#symbol-biquadcascade-biquadcascade-t-2}

```cpp
BiquadCascade(const BiquadCascade&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const BiquadCascade<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `BiquadCascade::operator=` {#symbol-biquadcascade-operator}

```cpp
BiquadCascade& operator=(const BiquadCascade&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const BiquadCascade<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `BiquadCascade<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `BiquadCascade::BiquadCascade<T>` {#symbol-biquadcascade-biquadcascade-t-3}

```cpp
BiquadCascade(BiquadCascade&& other) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `BiquadCascade<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `BiquadCascade::operator=` {#symbol-biquadcascade-operator-2}

```cpp
BiquadCascade& operator=(BiquadCascade&& other) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `BiquadCascade<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `BiquadCascade<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `BiquadCascade::prepare` {#symbol-biquadcascade-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const BiquadCoefficients>) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `std::span<const BiquadCoefficients>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `BiquadCascade::setSection` {#symbol-biquadcascade-setsection}

```cpp
Status setSection(std::size_t, const BiquadCoefficients&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `const BiquadCoefficients &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `BiquadCascade::reset` {#symbol-biquadcascade-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `BiquadCascade::process` {#symbol-biquadcascade-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `BiquadCascade::sections` {#symbol-biquadcascade-sections}

```cpp
std::size_t sections() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
