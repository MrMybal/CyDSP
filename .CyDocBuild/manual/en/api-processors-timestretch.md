---
id: api-processors-timestretch
title: TimeStretch
description: Optional time, pitch and formant stretching in the separate CyDSP::Stretch target.
section: API reference
order: 1166
revision: 1
parent: api-processors
symbol: cydsp::TimeStretch
---

Optional time, pitch and formant stretching in the separate CyDSP::Stretch target.

## Include {#include}

```cpp
#include <cydsp/processors/TimeStretch.h>
```

[Usage and contracts](doc:pitch) · [Common processing rules](doc:contracts)

## `TimeStretch` {#symbol-timestretch}

```cpp
template <Sample T> class TimeStretch;
```

### `TimeStretch::TimeStretch<T>` {#symbol-timestretch-timestretch-t}

```cpp
TimeStretch();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `TimeStretch::~TimeStretch<T>` {#symbol-timestretch-timestretch-t-2}

```cpp
~TimeStretch();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `TimeStretch::TimeStretch<T>` {#symbol-timestretch-timestretch-t-3}

```cpp
TimeStretch(TimeStretch&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `TimeStretch<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `TimeStretch::operator=` {#symbol-timestretch-operator}

```cpp
TimeStretch& operator=(TimeStretch&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `TimeStretch<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `TimeStretch<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `TimeStretch::prepare` {#symbol-timestretch-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t maximumOutputFrames, bool cheaper = false, bool splitComputation = true, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `maximumOutputFrames` | `std::size_t` | — | Frame count or offset; one frame contains one sample per channel. |
| `cheaper` | `bool` | `false` | Explicit true/false option or predicate. |
| `splitComputation` | `bool` | `true` | Explicit true/false option or predicate. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `TimeStretch::setPitchSemitones` {#symbol-timestretch-setpitchsemitones}

```cpp
Status setPitchSemitones(double semitones, double tonalityLimitHz = 0) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `semitones` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `tonalityLimitHz` | `double` | `0` | Frequency in Hz; use the supported sample-rate/Nyquist range. |

Return: `Status` — Check Status; ok reports success.

### `TimeStretch::setFormant` {#symbol-timestretch-setformant}

```cpp
Status setFormant(double multiplier, bool compensatePitch = false, double fundamentalHz = 0) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `multiplier` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `compensatePitch` | `bool` | `false` | Explicit true/false option or predicate. |
| `fundamentalHz` | `double` | `0` | Frequency in Hz; use the supported sample-rate/Nyquist range. |

Return: `Status` — Check Status; ok reports success.

### `TimeStretch::process` {#symbol-timestretch-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `TimeStretch::prime` {#symbol-timestretch-prime}

```cpp
Status prime(AudioBlock<const T> history) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `history` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |

Return: `Status` — Check Status; ok reports success.

### `TimeStretch::flush` {#symbol-timestretch-flush}

```cpp
Status flush(AudioBlock<T>) noexcept;
```

Drain the documented tail and inspect completion; reset before starting a new stream.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `TimeStretch::reset` {#symbol-timestretch-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `TimeStretch::backend` {#symbol-timestretch-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `TimeStretch::inputLatency` {#symbol-timestretch-inputlatency}

```cpp
std::size_t inputLatency() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `TimeStretch::outputLatency` {#symbol-timestretch-outputlatency}

```cpp
std::size_t outputLatency() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
