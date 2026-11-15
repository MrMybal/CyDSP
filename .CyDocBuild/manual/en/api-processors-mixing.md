---
id: api-processors-mixing
title: Mixing
description: Panning, dry/wet laws, elementary waveshaping and DC bias.
section: API reference
order: 1149
revision: 1
parent: api-processors
symbol: cydsp::PanLaw
---

Panning, dry/wet laws, elementary waveshaping and DC bias.

## Include {#include}

```cpp
#include <cydsp/processors/Mixing.h>
```

[Usage and contracts](doc:mixing) · [Common processing rules](doc:contracts)

## `PanLaw` {#symbol-panlaw}

```cpp
enum class PanLaw { balanced, linear, sine3dB, sine4_5dB, sine6dB, squareRoot3dB, squareRoot4_5dB };
```

## `MixLaw` {#symbol-mixlaw}

```cpp
enum class MixLaw { linear, sine3dB, sine4_5dB, sine6dB, squareRoot3dB, squareRoot4_5dB };
```

## `Panner` {#symbol-panner}

```cpp
template <Sample T> class Panner;
```

### `Panner::prepare` {#symbol-panner-prepare}

```cpp
Status prepare(const ProcessSpec&, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `Panner::setPan` {#symbol-panner-setpan}

```cpp
Status setPan(double pan, PanLaw = PanLaw::sine3dB, std::size_t rampFrames = 0) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `pan` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `PanLaw` | `PanLaw::sine3dB` | Value/configuration in the declared type; apply the linked module contract. |
| `rampFrames` | `std::size_t` | `0` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `Panner::process` {#symbol-panner-process}

```cpp
Status process(AudioBlock<const T> input, AudioBlock<T> stereoOutput) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `input` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `stereoOutput` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `Panner::reset` {#symbol-panner-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

## `DryWetMixer` {#symbol-drywetmixer}

```cpp
template <Sample T> class DryWetMixer;
```

### `DryWetMixer::DryWetMixer<T>` {#symbol-drywetmixer-drywetmixer-t}

```cpp
DryWetMixer();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `DryWetMixer::~DryWetMixer<T>` {#symbol-drywetmixer-drywetmixer-t-2}

```cpp
~DryWetMixer();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `DryWetMixer::DryWetMixer<T>` {#symbol-drywetmixer-drywetmixer-t-3}

```cpp
DryWetMixer(DryWetMixer&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `DryWetMixer<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `DryWetMixer::operator=` {#symbol-drywetmixer-operator}

```cpp
DryWetMixer& operator=(DryWetMixer&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `DryWetMixer<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `DryWetMixer<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `DryWetMixer::prepare` {#symbol-drywetmixer-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t maximumWetLatency = 0) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `maximumWetLatency` | `std::size_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `DryWetMixer::setMix` {#symbol-drywetmixer-setmix}

```cpp
Status setMix(double wet, MixLaw = MixLaw::linear) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `wet` | `double` | — | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `argument 2` | `MixLaw` | `MixLaw::linear` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `DryWetMixer::setWetLatency` {#symbol-drywetmixer-setwetlatency}

```cpp
Status setWetLatency(std::size_t frames) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `frames` | `std::size_t` | — | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `DryWetMixer::pushDrySamples` {#symbol-drywetmixer-pushdrysamples}

```cpp
Status pushDrySamples(AudioBlock<const T>) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |

Return: `Status` — Check Status; ok reports success.

### `DryWetMixer::mixWetSamples` {#symbol-drywetmixer-mixwetsamples}

```cpp
Status mixWetSamples(AudioBlock<T>) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `DryWetMixer::reset` {#symbol-drywetmixer-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

## `Shaper` {#symbol-shaper}

```cpp
enum class Shaper { tanh, atan, hardClip, cubic, fullWaveRectifier };
```

## `WaveShaper` {#symbol-waveshaper}

```cpp
template <Sample T> class WaveShaper;
```

### `WaveShaper::Function` {#symbol-waveshaper-function}

```cpp
using Function = T (*)(T, void*) noexcept;
```

### `WaveShaper::prepare` {#symbol-waveshaper-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |

Return: `Status` — Check Status; ok reports success.

### `WaveShaper::setDrive` {#symbol-waveshaper-setdrive}

```cpp
Status setDrive(double linearGain) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `linearGain` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `WaveShaper::setShape` {#symbol-waveshaper-setshape}

```cpp
void setShape(Shaper shape) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `shape` | `Shaper` | — | Value/configuration in the declared type; apply the linked module contract. |

### `WaveShaper::setFunction` {#symbol-waveshaper-setfunction}

```cpp
void setFunction(Function function, void* context = nullptr) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `function` | `Function` | — | Value/configuration in the declared type; apply the linked module contract. |
| `context` | `void *` | `nullptr` | Borrowed callback context; the application owns lifetime and synchronization. |

### `WaveShaper::reset` {#symbol-waveshaper-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `WaveShaper::process` {#symbol-waveshaper-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

## `Bias` {#symbol-bias}

```cpp
template <Sample T> class Bias;
```

### `Bias::prepare` {#symbol-bias-prepare}

```cpp
Status prepare(const ProcessSpec& spec) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `spec` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |

Return: `Status` — Check Status; ok reports success.

### `Bias::setBias` {#symbol-bias-setbias}

```cpp
Status setBias(T value) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `value` | `T` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `Bias::reset` {#symbol-bias-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `Bias::process` {#symbol-bias-process}

```cpp
Status process(AudioBlock<const T> in, AudioBlock<T> out) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `in` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `out` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.
