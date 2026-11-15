---
id: api-processors-hilbert
title: Hilbert
description: Matched-delay FIR quadrature signals for analytic-signal processing.
section: API reference
order: 1146
revision: 1
parent: api-processors
symbol: cydsp::HilbertTransform
---

Matched-delay FIR quadrature signals for analytic-signal processing.

## Include {#include}

```cpp
#include <cydsp/processors/Hilbert.h>
```

[Usage and contracts](doc:spectral) · [Common processing rules](doc:contracts)

## `HilbertTransform` {#symbol-hilberttransform}

```cpp
template <Sample T> class HilbertTransform;
```

### `HilbertTransform::HilbertTransform<T>` {#symbol-hilberttransform-hilberttransform-t}

```cpp
HilbertTransform();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `HilbertTransform::~HilbertTransform<T>` {#symbol-hilberttransform-hilberttransform-t-2}

```cpp
~HilbertTransform();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `HilbertTransform::HilbertTransform<T>` {#symbol-hilberttransform-hilberttransform-t-3}

```cpp
HilbertTransform(HilbertTransform&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `HilbertTransform<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `HilbertTransform::operator=` {#symbol-hilberttransform-operator}

```cpp
HilbertTransform& operator=(HilbertTransform&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `HilbertTransform<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `HilbertTransform<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `HilbertTransform::prepare` {#symbol-hilberttransform-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t taps = 129, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `taps` | `std::size_t` | `129` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `HilbertTransform::process` {#symbol-hilberttransform-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T> inPhase, AudioBlock<T> quadrature) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `inPhase` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |
| `quadrature` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `HilbertTransform::reset` {#symbol-hilberttransform-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `HilbertTransform::latencyFrames` {#symbol-hilberttransform-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
