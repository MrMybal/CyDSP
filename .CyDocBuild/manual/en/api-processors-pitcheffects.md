---
id: api-processors-pitcheffects
title: PitchEffects
description: Pitch-target correction and multivoice harmony through delay or spectral engines.
section: API reference
order: 1156
revision: 1
parent: api-processors
symbol: cydsp::PitchEffects
---

Pitch-target correction and multivoice harmony through delay or spectral engines.

## Include {#include}

```cpp
#include <cydsp/processors/PitchEffects.h>
```

[Usage and contracts](doc:pitch) · [Common processing rules](doc:contracts)

## `HarmonyVoice` {#symbol-harmonyvoice}

```cpp
struct HarmonyVoice;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `semitones` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `gain` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `pan` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `delayMs` | `double` | `0` | Time in milliseconds. |

## `PitchCorrectionParameters` {#symbol-pitchcorrectionparameters}

```cpp
struct PitchCorrectionParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `enabled` | `bool` | `false` | Explicit true/false option or predicate. |
| `targetHz` | `double` | `0` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `referenceA4Hz` | `double` | `440` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `retuneMs` | `double` | `20` | Time in milliseconds. |
| `scalePitchClasses` | `unsigned int` | `0xfff` | Value/configuration in the declared type; apply the linked module contract. |

## `PitchEffects` {#symbol-pitcheffects}

```cpp
template <Sample T> class PitchEffects;
```

### `PitchEffects::PitchEffects<T>` {#symbol-pitcheffects-pitcheffects-t}

```cpp
PitchEffects();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `PitchEffects::~PitchEffects<T>` {#symbol-pitcheffects-pitcheffects-t-2}

```cpp
~PitchEffects();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `PitchEffects::PitchEffects<T>` {#symbol-pitcheffects-pitcheffects-t-3}

```cpp
PitchEffects(PitchEffects&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `PitchEffects<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `PitchEffects::operator=` {#symbol-pitcheffects-operator}

```cpp
PitchEffects& operator=(PitchEffects&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `PitchEffects<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `PitchEffects<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `PitchEffects::prepare` {#symbol-pitcheffects-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t maximumVoices = 8, const MusicalAnalysisConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `maximumVoices` | `std::size_t` | `8` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `const MusicalAnalysisConfig &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `PitchEffects::prepareSpectral` {#symbol-pitcheffects-preparespectral}

```cpp
Status prepareSpectral(const ProcessSpec&, std::size_t maximumVoices = 8, const StftConfig& = {}, const MusicalAnalysisConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `maximumVoices` | `std::size_t` | `8` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `const StftConfig &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 4` | `const MusicalAnalysisConfig &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `PitchEffects::latencyFrames` {#symbol-pitcheffects-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `PitchEffects::setVoices` {#symbol-pitcheffects-setvoices}

```cpp
Status setVoices(std::span<const HarmonyVoice>) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const HarmonyVoice>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `PitchEffects::setCorrection` {#symbol-pitcheffects-setcorrection}

```cpp
Status setCorrection(const PitchCorrectionParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const PitchCorrectionParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `PitchEffects::process` {#symbol-pitcheffects-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `PitchEffects::reset` {#symbol-pitcheffects-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `PitchEffects::detectedPitchHz` {#symbol-pitcheffects-detectedpitchhz}

```cpp
double detectedPitchHz() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.
