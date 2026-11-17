---
id: api-spectral-spectraleffects
title: SpectralEffects
description: STFT freeze, vocoder, morph/filter effects and a Hilbert-based frequency shifter.
section: API reference
order: 1179
revision: 1
parent: api-spectral
symbol: cydsp::SpectralEffects
---

STFT freeze, vocoder, morph/filter effects and a Hilbert-based frequency shifter.

## Include {#include}

```cpp
#include <cydsp/spectral/SpectralEffects.h>
```

[Usage and contracts](doc:spectral) · [Common processing rules](doc:contracts)

## `SpectralEffectMode` {#symbol-spectraleffectmode}

```cpp
enum class SpectralEffectMode { bypass, freeze, vocoder, morph, filter };
```

## `SpectralEffectParameters` {#symbol-spectraleffectparameters}

```cpp
struct SpectralEffectParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `mode` | `SpectralEffectMode` | `SpectralEffectMode::bypass` | Value/configuration in the declared type; apply the linked module contract. |
| `mix` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `envelopeRadiusBins` | `std::size_t` | `8` | Value/configuration in the declared type; apply the linked module contract. |

## `SpectralEffects` {#symbol-spectraleffects}

```cpp
template <Sample T> class SpectralEffects;
```

### `SpectralEffects::SpectralEffects<T>` {#symbol-spectraleffects-spectraleffects-t}

```cpp
SpectralEffects();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `SpectralEffects::~SpectralEffects<T>` {#symbol-spectraleffects-spectraleffects-t-2}

```cpp
~SpectralEffects();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `SpectralEffects::SpectralEffects<T>` {#symbol-spectraleffects-spectraleffects-t-3}

```cpp
SpectralEffects(SpectralEffects&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `SpectralEffects<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `SpectralEffects::operator=` {#symbol-spectraleffects-operator}

```cpp
SpectralEffects& operator=(SpectralEffects&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `SpectralEffects<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `SpectralEffects<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SpectralEffects::prepare` {#symbol-spectraleffects-prepare}

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

### `SpectralEffects::setParameters` {#symbol-spectraleffects-setparameters}

```cpp
Status setParameters(const SpectralEffectParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const SpectralEffectParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `SpectralEffects::setBinGains` {#symbol-spectraleffects-setbingains}

```cpp
Status setBinGains(std::span<const double>) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `SpectralEffects::process` {#symbol-spectraleffects-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `SpectralEffects::process` {#symbol-spectraleffects-process-2}

```cpp
Status process(AudioBlock<const T>, AudioBlock<const T> modulator, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `modulator` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 3` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `SpectralEffects::reset` {#symbol-spectraleffects-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `SpectralEffects::latencyFrames` {#symbol-spectraleffects-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `FrequencyShifter` {#symbol-frequencyshifter}

```cpp
template <Sample T> class FrequencyShifter;
```

### `FrequencyShifter::FrequencyShifter<T>` {#symbol-frequencyshifter-frequencyshifter-t}

```cpp
FrequencyShifter();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `FrequencyShifter::~FrequencyShifter<T>` {#symbol-frequencyshifter-frequencyshifter-t-2}

```cpp
~FrequencyShifter();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `FrequencyShifter::FrequencyShifter<T>` {#symbol-frequencyshifter-frequencyshifter-t-3}

```cpp
FrequencyShifter(FrequencyShifter&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `FrequencyShifter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `FrequencyShifter::operator=` {#symbol-frequencyshifter-operator}

```cpp
FrequencyShifter& operator=(FrequencyShifter&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `FrequencyShifter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `FrequencyShifter<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `FrequencyShifter::prepare` {#symbol-frequencyshifter-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t hilbertTaps = 129, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `hilbertTaps` | `std::size_t` | `129` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `FrequencyShifter::setFrequency` {#symbol-frequencyshifter-setfrequency}

```cpp
Status setFrequency(double hz) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `hz` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |

Return: `Status` — Check Status; ok reports success.

### `FrequencyShifter::process` {#symbol-frequencyshifter-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `FrequencyShifter::reset` {#symbol-frequencyshifter-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `FrequencyShifter::latencyFrames` {#symbol-frequencyshifter-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
