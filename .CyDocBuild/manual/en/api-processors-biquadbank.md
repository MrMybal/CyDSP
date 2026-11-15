---
id: api-processors-biquadbank
title: BiquadBank
description: Independent coefficients and histories per voice, with SIMD across independent voices.
section: API reference
order: 1131
revision: 1
parent: api-processors
symbol: cydsp::BiquadBank
---

Independent coefficients and histories per voice, with SIMD across independent voices.

## Include {#include}

```cpp
#include <cydsp/processors/BiquadBank.h>
```

[Usage and contracts](doc:filters) · [Common processing rules](doc:contracts)

## `BiquadBank` {#symbol-biquadbank}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class BiquadBank;
```

### `BiquadBank::BiquadBank<T>` {#symbol-biquadbank-biquadbank-t}

```cpp
BiquadBank() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `BiquadBank::BiquadBank<T>` {#symbol-biquadbank-biquadbank-t-2}

```cpp
BiquadBank(const BiquadBank&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const BiquadBank<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `BiquadBank::operator=` {#symbol-biquadbank-operator}

```cpp
BiquadBank& operator=(const BiquadBank&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const BiquadBank<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `BiquadBank<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `BiquadBank::BiquadBank<T>` {#symbol-biquadbank-biquadbank-t-3}

```cpp
BiquadBank(BiquadBank&& other) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `BiquadBank<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `BiquadBank::operator=` {#symbol-biquadbank-operator-2}

```cpp
BiquadBank& operator=(BiquadBank&& other) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `BiquadBank<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `BiquadBank<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `BiquadBank::prepare` {#symbol-biquadbank-prepare}

```cpp
Status prepare(const ProcessSpec&, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `BiquadBank::setCoefficients` {#symbol-biquadbank-setcoefficients}

```cpp
Status setCoefficients(std::size_t voice, const BiquadCoefficients&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `voice` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `const BiquadCoefficients &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `BiquadBank::setAllCoefficients` {#symbol-biquadbank-setallcoefficients}

```cpp
Status setAllCoefficients(const BiquadCoefficients&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const BiquadCoefficients &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `BiquadBank::reset` {#symbol-biquadbank-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `BiquadBank::process` {#symbol-biquadbank-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `BiquadBank::voices` {#symbol-biquadbank-voices}

```cpp
std::size_t voices() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `BiquadBank::backend` {#symbol-biquadbank-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
