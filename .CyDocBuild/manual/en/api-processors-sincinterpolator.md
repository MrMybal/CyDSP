---
id: api-processors-sincinterpolator
title: SincInterpolator
description: Prepared windowed-sinc interpolation for fractional positions.
section: API reference
order: 1162
revision: 1
parent: api-processors
symbol: cydsp::SincInterpolator
---

Prepared windowed-sinc interpolation for fractional positions.

## Include {#include}

```cpp
#include <cydsp/processors/SincInterpolator.h>
```

[Usage and contracts](doc:delay) · [Common processing rules](doc:contracts)

## `SincInterpolator` {#symbol-sincinterpolator}

```cpp
template <Sample T> class SincInterpolator;
```

### `SincInterpolator::SincInterpolator<T>` {#symbol-sincinterpolator-sincinterpolator-t}

```cpp
SincInterpolator();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `SincInterpolator::~SincInterpolator<T>` {#symbol-sincinterpolator-sincinterpolator-t-2}

```cpp
~SincInterpolator();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `SincInterpolator::SincInterpolator<T>` {#symbol-sincinterpolator-sincinterpolator-t-3}

```cpp
SincInterpolator(SincInterpolator&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `SincInterpolator<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `SincInterpolator::operator=` {#symbol-sincinterpolator-operator}

```cpp
SincInterpolator& operator=(SincInterpolator&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `SincInterpolator<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `SincInterpolator<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SincInterpolator::prepare` {#symbol-sincinterpolator-prepare}

```cpp
Status prepare(const ProcessSpec& output, double inputRate, std::size_t taps = 64, std::size_t phases = 1024, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `output` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `inputRate` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `taps` | `std::size_t` | `64` | Value/configuration in the declared type; apply the linked module contract. |
| `phases` | `std::size_t` | `1024` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `SincInterpolator::process` {#symbol-sincinterpolator-process}

```cpp
Status process(AudioBlock<const T> source, double firstSourceFrame, AudioBlock<T> output) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `source` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `firstSourceFrame` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `output` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `SincInterpolator::processPositions` {#symbol-sincinterpolator-processpositions}

```cpp
Status processPositions(AudioBlock<const T> source, std::span<const double> positions, AudioBlock<T> output) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `source` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `positions` | `std::span<const double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `output` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `SincInterpolator::backend` {#symbol-sincinterpolator-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
