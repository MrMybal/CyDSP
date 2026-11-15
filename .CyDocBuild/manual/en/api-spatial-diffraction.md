---
id: api-spatial-diffraction
title: Diffraction
description: Thin-screen edge/aperture diffraction gains and a prepared band filter.
section: API reference
order: 1170
revision: 1
parent: api-spatial
symbol: cydsp::DiffractionPath
---

Thin-screen edge/aperture diffraction gains and a prepared band filter.

## Include {#include}

```cpp
#include <cydsp/spatial/Diffraction.h>
```

[Usage and contracts](doc:acoustics) · [Common processing rules](doc:contracts)

## `DiffractionPath` {#symbol-diffractionpath}

```cpp
struct DiffractionPath;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `sourceToEdgeMetres` | `double` | `1` | Length in metres. |
| `edgeToListenerMetres` | `double` | `1` | Length in metres. |
| `edgeOffsetMetres` | `double` | `0` | Length in metres. |
| `soundSpeed` | `double` | `343` | Value/configuration in the declared type; apply the linked module contract. |
| `openingWidthMetres` | `double` | `0` | Length in metres. |
| `openingAngleRadians` | `double` | `0` | Angle in radians. |

## `diffractionGain` {#symbol-diffractiongain}

```cpp
Status diffractionGain(const DiffractionPath&, double frequencyHz, double& linearGain) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const DiffractionPath &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `frequencyHz` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `linearGain` | `double &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `DiffractionFilter` {#symbol-diffractionfilter}

```cpp
template <Sample T> class DiffractionFilter;
```

### `DiffractionFilter::DiffractionFilter<T>` {#symbol-diffractionfilter-diffractionfilter-t}

```cpp
DiffractionFilter();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `DiffractionFilter::~DiffractionFilter<T>` {#symbol-diffractionfilter-diffractionfilter-t-2}

```cpp
~DiffractionFilter();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `DiffractionFilter::DiffractionFilter<T>` {#symbol-diffractionfilter-diffractionfilter-t-3}

```cpp
DiffractionFilter(DiffractionFilter&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `DiffractionFilter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `DiffractionFilter::operator=` {#symbol-diffractionfilter-operator}

```cpp
DiffractionFilter& operator=(DiffractionFilter&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `DiffractionFilter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `DiffractionFilter<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `DiffractionFilter::prepare` {#symbol-diffractionfilter-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |

Return: `Status` — Check Status; ok reports success.

### `DiffractionFilter::setPath` {#symbol-diffractionfilter-setpath}

```cpp
Status setPath(const DiffractionPath&, std::size_t rampFrames = 256) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const DiffractionPath &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `rampFrames` | `std::size_t` | `256` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `DiffractionFilter::process` {#symbol-diffractionfilter-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `DiffractionFilter::reset` {#symbol-diffractionfilter-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.
