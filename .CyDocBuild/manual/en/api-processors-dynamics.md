---
id: api-processors-dynamics
title: Dynamics
description: Compressor, expander and gate with peak/RMS detection, sidechain and prepared look-ahead.
section: API reference
order: 1138
revision: 1
parent: api-processors
symbol: cydsp::Dynamics
---

Compressor, expander and gate with peak/RMS detection, sidechain and prepared look-ahead.

## Include {#include}

```cpp
#include <cydsp/processors/Dynamics.h>
```

[Usage and contracts](doc:dynamics) · [Common processing rules](doc:contracts)

## `DetectorMode` {#symbol-detectormode}

```cpp
enum class DetectorMode { peak, rms };
```

## `DynamicsMode` {#symbol-dynamicsmode}

```cpp
enum class DynamicsMode { compressor, expander, noiseGate, limiter };
```

## `DynamicsParameters` {#symbol-dynamicsparameters}

```cpp
struct DynamicsParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `thresholdDb` | `double` | `-18` | Logarithmic level in the indicated dB reference. |
| `ratio` | `double` | `4` | Value/configuration in the declared type; apply the linked module contract. |
| `kneeDb` | `double` | `6` | Logarithmic level in the indicated dB reference. |
| `attackMs` | `double` | `10` | Time in milliseconds. |
| `releaseMs` | `double` | `100` | Time in milliseconds. |
| `makeupDb` | `double` | `0` | Logarithmic level in the indicated dB reference. |
| `linkedChannels` | `bool` | `true` | Explicit true/false option or predicate. |
| `detector` | `DetectorMode` | `DetectorMode::peak` | Value/configuration in the declared type; apply the linked module contract. |
| `holdMs` | `double` | `0` | Time in milliseconds. |
| `hysteresisDb` | `double` | `0` | Logarithmic level in the indicated dB reference. |
| `lookAheadMs` | `double` | `0` | Time in milliseconds. |

## `BallisticsFilter` {#symbol-ballisticsfilter}

```cpp
template <Sample T> class BallisticsFilter;
```

### `BallisticsFilter::BallisticsFilter<T>` {#symbol-ballisticsfilter-ballisticsfilter-t}

```cpp
BallisticsFilter();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `BallisticsFilter::~BallisticsFilter<T>` {#symbol-ballisticsfilter-ballisticsfilter-t-2}

```cpp
~BallisticsFilter();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `BallisticsFilter::BallisticsFilter<T>` {#symbol-ballisticsfilter-ballisticsfilter-t-3}

```cpp
BallisticsFilter(BallisticsFilter&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `BallisticsFilter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `BallisticsFilter::operator=` {#symbol-ballisticsfilter-operator}

```cpp
BallisticsFilter& operator=(BallisticsFilter&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `BallisticsFilter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `BallisticsFilter<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `BallisticsFilter::prepare` {#symbol-ballisticsfilter-prepare}

```cpp
Status prepare(const ProcessSpec&, DetectorMode = DetectorMode::peak) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `DetectorMode` | `DetectorMode::peak` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `BallisticsFilter::setTimes` {#symbol-ballisticsfilter-settimes}

```cpp
Status setTimes(double attackMs, double releaseMs) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `attackMs` | `double` | — | Time in milliseconds. |
| `releaseMs` | `double` | — | Time in milliseconds. |

Return: `Status` — Check Status; ok reports success.

### `BallisticsFilter::reset` {#symbol-ballisticsfilter-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `BallisticsFilter::process` {#symbol-ballisticsfilter-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

## `Dynamics` {#symbol-dynamics}

```cpp
template <Sample T> class Dynamics;
```

### `Dynamics::Dynamics<T>` {#symbol-dynamics-dynamics-t}

```cpp
Dynamics();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `Dynamics::~Dynamics<T>` {#symbol-dynamics-dynamics-t-2}

```cpp
~Dynamics();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `Dynamics::Dynamics<T>` {#symbol-dynamics-dynamics-t-3}

```cpp
Dynamics(Dynamics&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Dynamics<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `Dynamics::operator=` {#symbol-dynamics-operator}

```cpp
Dynamics& operator=(Dynamics&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Dynamics<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Dynamics<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Dynamics::prepare` {#symbol-dynamics-prepare}

```cpp
Status prepare(const ProcessSpec&, DynamicsMode = DynamicsMode::compressor, double maximumLookAheadMs = 20) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `DynamicsMode` | `DynamicsMode::compressor` | Value/configuration in the declared type; apply the linked module contract. |
| `maximumLookAheadMs` | `double` | `20` | Time in milliseconds. |

Return: `Status` — Check Status; ok reports success.

### `Dynamics::setParameters` {#symbol-dynamics-setparameters}

```cpp
Status setParameters(const DynamicsParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const DynamicsParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `Dynamics::reset` {#symbol-dynamics-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `Dynamics::process` {#symbol-dynamics-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `Dynamics::process` {#symbol-dynamics-process-2}

```cpp
Status process(AudioBlock<const T> program, AudioBlock<const T> sidechain, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `program` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `sidechain` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 3` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `Dynamics::processMixed` {#symbol-dynamics-processmixed}

```cpp
Status processMixed(AudioBlock<const T>, AudioBlock<T>, double dryGain, double wetGain) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |
| `dryGain` | `double` | — | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `wetGain` | `double` | — | Linear amplitude multiplier; follow the processor bounds and mixing law. |

Return: `Status` — Check Status; ok reports success.

### `Dynamics::processMixed` {#symbol-dynamics-processmixed-2}

```cpp
Status processMixed(AudioBlock<const T>, AudioBlock<const T> sidechain, AudioBlock<T>, double dryGain, double wetGain) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `sidechain` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 3` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |
| `dryGain` | `double` | — | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `wetGain` | `double` | — | Linear amplitude multiplier; follow the processor bounds and mixing law. |

Return: `Status` — Check Status; ok reports success.

### `Dynamics::gainReductionDb` {#symbol-dynamics-gainreductiondb}

```cpp
double gainReductionDb() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Dynamics::minimumAppliedGain` {#symbol-dynamics-minimumappliedgain}

```cpp
double minimumAppliedGain() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Dynamics::latencyFrames` {#symbol-dynamics-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
