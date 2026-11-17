---
id: api-spectral-spectralpitch
title: SpectralPitch
description: Phase-vocoder pitch shifting with linked channel phases, formant compensation and transient protection.
section: API reference
order: 1180
revision: 1
parent: api-spectral
symbol: cydsp::SpectralPitchParameters
---

Phase-vocoder pitch shifting with linked channel phases, formant compensation and transient protection.

## Include {#include}

```cpp
#include <cydsp/spectral/SpectralPitch.h>
```

[Usage and contracts](doc:pitch) · [Common processing rules](doc:contracts)

## `SpectralPitchParameters` {#symbol-spectralpitchparameters}

```cpp
struct SpectralPitchParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `ratio` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `preserveFormants` | `bool` | `true` | Explicit true/false option or predicate. |
| `protectTransients` | `bool` | `true` | Explicit true/false option or predicate. |
| `lockSpectralPeaks` | `bool` | `true` | Explicit true/false option or predicate. |
| `envelopeWidthHz` | `double` | `150` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `transientFluxThreshold` | `double` | `.35` | Value/configuration in the declared type; apply the linked module contract. |

## `SpectralPitchShifter` {#symbol-spectralpitchshifter}

```cpp
template <Sample T> class SpectralPitchShifter;
```

### `SpectralPitchShifter::SpectralPitchShifter<T>` {#symbol-spectralpitchshifter-spectralpitchshifter-t}

```cpp
SpectralPitchShifter();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `SpectralPitchShifter::~SpectralPitchShifter<T>` {#symbol-spectralpitchshifter-spectralpitchshifter-t-2}

```cpp
~SpectralPitchShifter();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `SpectralPitchShifter::SpectralPitchShifter<T>` {#symbol-spectralpitchshifter-spectralpitchshifter-t-3}

```cpp
SpectralPitchShifter(SpectralPitchShifter&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `SpectralPitchShifter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `SpectralPitchShifter::operator=` {#symbol-spectralpitchshifter-operator}

```cpp
SpectralPitchShifter& operator=(SpectralPitchShifter&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `SpectralPitchShifter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `SpectralPitchShifter<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SpectralPitchShifter::prepare` {#symbol-spectralpitchshifter-prepare}

```cpp
Status prepare(const ProcessSpec&, const StftConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `const StftConfig &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `SpectralPitchShifter::setParameters` {#symbol-spectralpitchshifter-setparameters}

```cpp
Status setParameters(const SpectralPitchParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const SpectralPitchParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `SpectralPitchShifter::process` {#symbol-spectralpitchshifter-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `SpectralPitchShifter::reset` {#symbol-spectralpitchshifter-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `SpectralPitchShifter::latencyFrames` {#symbol-spectralpitchshifter-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
