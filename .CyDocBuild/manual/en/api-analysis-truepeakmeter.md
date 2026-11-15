---
id: api-analysis-truepeakmeter
title: TruePeakMeter
description: Oversampled intersample-peak estimation with explicit reconstruction delay.
section: API reference
order: 1107
revision: 1
parent: api-analysis
symbol: cydsp::TruePeakMeter
---

Oversampled intersample-peak estimation with explicit reconstruction delay.

## Include {#include}

```cpp
#include <cydsp/analysis/TruePeakMeter.h>
```

[Usage and contracts](doc:metering) · [Common processing rules](doc:contracts)

## `TruePeakMeter` {#symbol-truepeakmeter}

```cpp
template <Sample T> class TruePeakMeter;
```

### `TruePeakMeter::TruePeakMeter<T>` {#symbol-truepeakmeter-truepeakmeter-t}

```cpp
TruePeakMeter();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `TruePeakMeter::~TruePeakMeter<T>` {#symbol-truepeakmeter-truepeakmeter-t-2}

```cpp
~TruePeakMeter();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `TruePeakMeter::TruePeakMeter<T>` {#symbol-truepeakmeter-truepeakmeter-t-3}

```cpp
TruePeakMeter(TruePeakMeter&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `TruePeakMeter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `TruePeakMeter::operator=` {#symbol-truepeakmeter-operator}

```cpp
TruePeakMeter& operator=(TruePeakMeter&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `TruePeakMeter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `TruePeakMeter<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `TruePeakMeter::prepare` {#symbol-truepeakmeter-prepare}

```cpp
Status prepare(const ProcessSpec&, unsigned oversampling = 4, std::size_t tapsPerPhase = 64, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `oversampling` | `unsigned int` | `4` | Value/configuration in the declared type; apply the linked module contract. |
| `tapsPerPhase` | `std::size_t` | `64` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `TruePeakMeter::process` {#symbol-truepeakmeter-process}

```cpp
Status process(AudioBlock<const T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |

Return: `Status` — Check Status; ok reports success.

### `TruePeakMeter::flush` {#symbol-truepeakmeter-flush}

```cpp
void flush() noexcept;
```

Drain the documented tail and inspect completion; reset before starting a new stream.

### `TruePeakMeter::reset` {#symbol-truepeakmeter-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `TruePeakMeter::samplePeak` {#symbol-truepeakmeter-samplepeak}

```cpp
double samplePeak(std::size_t channel) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Channel index/count in the declared planar layout. |

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `TruePeakMeter::truePeak` {#symbol-truepeakmeter-truepeak}

```cpp
double truePeak(std::size_t channel) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Channel index/count in the declared planar layout. |

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `TruePeakMeter::truePeakDbtp` {#symbol-truepeakmeter-truepeakdbtp}

```cpp
double truePeakDbtp(std::size_t channel) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Channel index/count in the declared planar layout. |

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `TruePeakMeter::latencyFrames` {#symbol-truepeakmeter-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `TruePeakMeter::backend` {#symbol-truepeakmeter-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
