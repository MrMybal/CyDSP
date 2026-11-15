---
id: api-processors-polynomialinterpolator
title: PolynomialInterpolator
description: Lagrange/Catmull–Rom polynomial sample readers.
section: API reference
order: 1157
revision: 1
parent: api-processors
symbol: cydsp::PolynomialInterpolator
---

Lagrange/Catmull–Rom polynomial sample readers.

## Include {#include}

```cpp
#include <cydsp/processors/PolynomialInterpolator.h>
```

[Usage and contracts](doc:delay) · [Common processing rules](doc:contracts)

## `PolynomialInterpolation` {#symbol-polynomialinterpolation}

```cpp
enum class PolynomialInterpolation { lagrange, catmullRom };
```

## `PolynomialInterpolator` {#symbol-polynomialinterpolator}

```cpp
template <Sample T> class PolynomialInterpolator;
```

### `PolynomialInterpolator::prepare` {#symbol-polynomialinterpolator-prepare}

```cpp
Status prepare(const ProcessSpec&, double inputRate, PolynomialInterpolation = PolynomialInterpolation::lagrange, unsigned order = 3, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `inputRate` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `PolynomialInterpolation` | `PolynomialInterpolation::lagrange` | Value/configuration in the declared type; apply the linked module contract. |
| `order` | `unsigned int` | `3` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `PolynomialInterpolator::process` {#symbol-polynomialinterpolator-process}

```cpp
Status process(AudioBlock<const T> source, double firstFrame, AudioBlock<T> output) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `source` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `firstFrame` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `output` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.
