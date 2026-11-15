---
id: api-processors-diffusion
title: Diffusion
description: Schroeder allpass diffusion and orthogonal Hadamard/Householder mixing.
section: API reference
order: 1137
revision: 1
parent: api-processors
symbol: cydsp::OrthogonalMix
---

Schroeder allpass diffusion and orthogonal Hadamard/Householder mixing.

## Include {#include}

```cpp
#include <cydsp/processors/Diffusion.h>
```

[Usage and contracts](doc:reverb) · [Common processing rules](doc:contracts)

## `OrthogonalMix` {#symbol-orthogonalmix}

```cpp
enum class OrthogonalMix { hadamard, householder };
```

## `orthogonalMix` {#symbol-orthogonalmix-2}

```cpp
template <Sample T> [[nodiscard]] Status orthogonalMix(std::span<const T> input, std::span<T> output, OrthogonalMix kind = OrthogonalMix::hadamard) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `input` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `output` | `std::span<T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `kind` | `OrthogonalMix` | `OrthogonalMix::hadamard` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

## `DiffusionStage` {#symbol-diffusionstage}

```cpp
struct DiffusionStage;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `delayFrames` | `std::size_t` | `1` | Frame count or offset; one frame contains one sample per channel. |
| `feedback` | `double` | `.5` | Value/configuration in the declared type; apply the linked module contract. |

## `Diffuser` {#symbol-diffuser}

```cpp
template <Sample T> class Diffuser;
```

### `Diffuser::Diffuser<T>` {#symbol-diffuser-diffuser-t}

```cpp
Diffuser();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `Diffuser::~Diffuser<T>` {#symbol-diffuser-diffuser-t-2}

```cpp
~Diffuser();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `Diffuser::Diffuser<T>` {#symbol-diffuser-diffuser-t-3}

```cpp
Diffuser(Diffuser&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Diffuser<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `Diffuser::operator=` {#symbol-diffuser-operator}

```cpp
Diffuser& operator=(Diffuser&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Diffuser<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Diffuser<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Diffuser::prepare` {#symbol-diffuser-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const DiffusionStage>) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `std::span<const DiffusionStage>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `Diffuser::process` {#symbol-diffuser-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `Diffuser::reset` {#symbol-diffuser-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.
