---
id: api-processors-advancedeffects
title: AdvancedEffects
description: Filtered/modulated multitap echoes, amplitude modulation, plate and spring reverbs.
section: API reference
order: 1125
revision: 1
parent: api-processors
symbol: cydsp::DelayEffectMode
---

Filtered/modulated multitap echoes, amplitude modulation, plate and spring reverbs.

## Include {#include}

```cpp
#include <cydsp/processors/AdvancedEffects.h>
```

[Usage and contracts](doc:effects) · [Common processing rules](doc:contracts)

## `DelayEffectMode` {#symbol-delayeffectmode}

```cpp
enum class DelayEffectMode { echo, flanger, vibrato, wowFlutter };
```

## `EchoTap` {#symbol-echotap}

```cpp
struct EchoTap;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `delayMs` | `double` | `250` | Time in milliseconds. |
| `gain` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |

## `DelayEffectParameters` {#symbol-delayeffectparameters}

```cpp
struct DelayEffectParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `mode` | `DelayEffectMode` | `DelayEffectMode::echo` | Value/configuration in the declared type; apply the linked module contract. |
| `delayMs` | `double` | `250` | Time in milliseconds. |
| `depthMs` | `double` | `0` | Time in milliseconds. |
| `rateHz` | `double` | `.5` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `feedback` | `double` | `.3` | Value/configuration in the declared type; apply the linked module contract. |
| `lowPassHz` | `double` | `10000` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `feedbackDrive` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `duckDb` | `double` | `0` | Logarithmic level in the indicated dB reference. |
| `wet` | `double` | `.5` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `dry` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `pingPong` | `bool` | `false` | Explicit true/false option or predicate. |

## `DelayEffect` {#symbol-delayeffect}

```cpp
template <Sample T> class DelayEffect;
```

### `DelayEffect::DelayEffect<T>` {#symbol-delayeffect-delayeffect-t}

```cpp
DelayEffect();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `DelayEffect::~DelayEffect<T>` {#symbol-delayeffect-delayeffect-t-2}

```cpp
~DelayEffect();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `DelayEffect::DelayEffect<T>` {#symbol-delayeffect-delayeffect-t-3}

```cpp
DelayEffect(DelayEffect&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `DelayEffect<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `DelayEffect::operator=` {#symbol-delayeffect-operator}

```cpp
DelayEffect& operator=(DelayEffect&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `DelayEffect<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `DelayEffect<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `DelayEffect::prepare` {#symbol-delayeffect-prepare}

```cpp
Status prepare(const ProcessSpec&, double maximumDelayMs = 2000) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `maximumDelayMs` | `double` | `2000` | Time in milliseconds. |

Return: `Status` — Check Status; ok reports success.

### `DelayEffect::setParameters` {#symbol-delayeffect-setparameters}

```cpp
Status setParameters(const DelayEffectParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const DelayEffectParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `DelayEffect::setTaps` {#symbol-delayeffect-settaps}

```cpp
Status setTaps(std::span<const EchoTap>) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const EchoTap>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `DelayEffect::process` {#symbol-delayeffect-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `DelayEffect::reset` {#symbol-delayeffect-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

## `AmplitudeModulationParameters` {#symbol-amplitudemodulationparameters}

```cpp
struct AmplitudeModulationParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `rateHz` | `double` | `4` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `depth` | `double` | `.5` | Value/configuration in the declared type; apply the linked module contract. |
| `autopan` | `bool` | `false` | Explicit true/false option or predicate. |

## `AmplitudeModulator` {#symbol-amplitudemodulator}

```cpp
template <Sample T> class AmplitudeModulator;
```

### `AmplitudeModulator::prepare` {#symbol-amplitudemodulator-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |

Return: `Status` — Check Status; ok reports success.

### `AmplitudeModulator::setParameters` {#symbol-amplitudemodulator-setparameters}

```cpp
Status setParameters(const AmplitudeModulationParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const AmplitudeModulationParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `AmplitudeModulator::process` {#symbol-amplitudemodulator-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `AmplitudeModulator::reset` {#symbol-amplitudemodulator-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

## `PlateParameters` {#symbol-plateparameters}

```cpp
struct PlateParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `decay` | `double` | `.7` | Value/configuration in the declared type; apply the linked module contract. |
| `diffusion` | `double` | `.7` | Value/configuration in the declared type; apply the linked module contract. |
| `dampingHz` | `double` | `6000` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `wet` | `double` | `.5` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `dry` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `shimmer` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `freeze` | `bool` | `false` | Explicit true/false option or predicate. |

## `PlateReverb` {#symbol-platereverb}

```cpp
template <Sample T> class PlateReverb;
```

### `PlateReverb::PlateReverb<T>` {#symbol-platereverb-platereverb-t}

```cpp
PlateReverb();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `PlateReverb::~PlateReverb<T>` {#symbol-platereverb-platereverb-t-2}

```cpp
~PlateReverb();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `PlateReverb::PlateReverb<T>` {#symbol-platereverb-platereverb-t-3}

```cpp
PlateReverb(PlateReverb&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `PlateReverb<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `PlateReverb::operator=` {#symbol-platereverb-operator}

```cpp
PlateReverb& operator=(PlateReverb&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `PlateReverb<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `PlateReverb<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `PlateReverb::prepare` {#symbol-platereverb-prepare}

```cpp
Status prepare(const ProcessSpec&, double size = 1) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `size` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `PlateReverb::setParameters` {#symbol-platereverb-setparameters}

```cpp
Status setParameters(const PlateParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const PlateParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `PlateReverb::process` {#symbol-platereverb-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `PlateReverb::reset` {#symbol-platereverb-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

## `SpringParameters` {#symbol-springparameters}

```cpp
struct SpringParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `decaySeconds` | `double` | `2` | Duration or position in seconds. |
| `dispersion` | `double` | `.6` | Value/configuration in the declared type; apply the linked module contract. |
| `dampingHz` | `double` | `5000` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `wet` | `double` | `.5` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `dry` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |

## `SpringReverb` {#symbol-springreverb}

```cpp
template <Sample T> class SpringReverb;
```

### `SpringReverb::SpringReverb<T>` {#symbol-springreverb-springreverb-t}

```cpp
SpringReverb();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `SpringReverb::~SpringReverb<T>` {#symbol-springreverb-springreverb-t-2}

```cpp
~SpringReverb();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `SpringReverb::SpringReverb<T>` {#symbol-springreverb-springreverb-t-3}

```cpp
SpringReverb(SpringReverb&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `SpringReverb<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `SpringReverb::operator=` {#symbol-springreverb-operator}

```cpp
SpringReverb& operator=(SpringReverb&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `SpringReverb<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `SpringReverb<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SpringReverb::prepare` {#symbol-springreverb-prepare}

```cpp
Status prepare(const ProcessSpec&, double lengthSeconds = .035) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `lengthSeconds` | `double` | `.035` | Duration or position in seconds. |

Return: `Status` — Check Status; ok reports success.

### `SpringReverb::setParameters` {#symbol-springreverb-setparameters}

```cpp
Status setParameters(const SpringParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const SpringParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `SpringReverb::process` {#symbol-springreverb-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `SpringReverb::reset` {#symbol-springreverb-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.
