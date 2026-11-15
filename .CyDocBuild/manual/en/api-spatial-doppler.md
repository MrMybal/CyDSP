---
id: api-spatial-doppler
title: Doppler
description: Doppler through prepared variable-rate resampling and explicit stream counts.
section: API reference
order: 1171
revision: 1
parent: api-spatial
symbol: cydsp::DopplerResampler
---

Doppler through prepared variable-rate resampling and explicit stream counts.

## Include {#include}

```cpp
#include <cydsp/spatial/Doppler.h>
```

[Usage and contracts](doc:spatial) · [Common processing rules](doc:contracts)

## `DopplerResampler` {#symbol-dopplerresampler}

```cpp
template <Sample T> class DopplerResampler;
```

### `DopplerResampler::prepare` {#symbol-dopplerresampler-prepare}

```cpp
Status prepare(const ResampleSpec& spec, double maximumSemitones = 24, CpuBackend backend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `spec` | `const ResampleSpec &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `maximumSemitones` | `double` | `24` | Value/configuration in the declared type; apply the linked module contract. |
| `backend` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `DopplerResampler::setMotion` {#symbol-dopplerresampler-setmotion}

```cpp
Status setMotion(const RelativeSource& source, const AcousticMedium& medium, std::size_t rampFrames = 0, double intensity = 1) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `source` | `const RelativeSource &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `medium` | `const AcousticMedium &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `rampFrames` | `std::size_t` | `0` | Frame count or offset; one frame contains one sample per channel. |
| `intensity` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `DopplerResampler::process` {#symbol-dopplerresampler-process}

```cpp
ResampleResult process(AudioBlock<const T> input, AudioBlock<T> output) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `input` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `output` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `ResampleResult` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `DopplerResampler::finish` {#symbol-dopplerresampler-finish}

```cpp
ResampleResult finish(AudioBlock<T> output) noexcept;
```

Drain the documented tail and inspect completion; reset before starting a new stream.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `output` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `ResampleResult` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `DopplerResampler::reset` {#symbol-dopplerresampler-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `DopplerResampler::ratio` {#symbol-dopplerresampler-ratio}

```cpp
double ratio() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `DopplerResampler::latencyInputFrames` {#symbol-dopplerresampler-latencyinputframes}

```cpp
double latencyInputFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.
