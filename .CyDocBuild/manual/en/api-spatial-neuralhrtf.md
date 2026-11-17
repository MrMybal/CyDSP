---
id: api-spatial-neuralhrtf
title: NeuralHrtf
description: Experimental native directional HRTF field and prepared binaural renderer.
section: API reference
order: 1174
revision: 1
parent: api-spatial
symbol: cydsp::neuralHrtfMaxSections
---

Experimental native directional HRTF field and prepared binaural renderer.

## Include {#include}

```cpp
#include <cydsp/spatial/NeuralHrtf.h>
```

[Usage and contracts](doc:neural-hrtf) · [Common processing rules](doc:contracts)

## `neuralHrtfMaxSections` {#symbol-neuralhrtfmaxsections}

```cpp
inline constexpr std::size_t neuralHrtfMaxSections = 32;
```

## `NeuralHrtfSection` {#symbol-neuralhrtfsection}

```cpp
struct NeuralHrtfSection;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `frequencyHz` | `double` | `1000` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `q` | `double` | `1` | Positive finite filter quality factor. |
| `gainDb` | `double` | `0` | Logarithmic level in the indicated dB reference. |

## `NeuralHrtfParameters` {#symbol-neuralhrtfparameters}

```cpp
struct NeuralHrtfParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `sections` | `std::array<std::array<NeuralHrtfSection, neuralHrtfMaxSections>, 2>` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `levelDb` | `std::array<double, 2>` | `{}` | Logarithmic level in the indicated dB reference. |
| `sectionCount` | `std::size_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `sampleRate` | `double` | `0` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `itdMicroseconds` | `double` | `0` | Duration or position in seconds. |

## `NeuralHrtfInfo` {#symbol-neuralhrtfinfo}

```cpp
struct NeuralHrtfInfo;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `width` | `std::size_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `featureBands` | `std::size_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `sectionsPerEar` | `std::size_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `parameters` | `std::size_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `sampleRate` | `double` | `0` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `magnitudeOffsetDb` | `double` | `0` | Logarithmic level in the indicated dB reference. |
| `backend` | `CpuBackend` | `CpuBackend::scalar` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

## `NeuralHrtfModel` {#symbol-neuralhrtfmodel}

```cpp
class NeuralHrtfModel;
```

### `NeuralHrtfModel::load` {#symbol-neuralhrtfmodel-load}

```cpp
Status load(std::span<const std::byte>, CpuBackend = CpuBackend::automatic) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const std::byte>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `NeuralHrtfModel::info` {#symbol-neuralhrtfmodel-info}

```cpp
NeuralHrtfInfo info() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `NeuralHrtfInfo` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `NeuralHrtfModel::predict` {#symbol-neuralhrtfmodel-predict}

```cpp
Status predict(Vec3 listenerDirection, NeuralHrtfParameters&) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `listenerDirection` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `NeuralHrtfParameters &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

### `NeuralHrtfModel::predict` {#symbol-neuralhrtfmodel-predict-2}

```cpp
Status predict(std::span<const Vec3>, std::span<NeuralHrtfParameters>) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const Vec3>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<NeuralHrtfParameters>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

## `NeuralHrtfDesign` {#symbol-neuralhrtfdesign}

```cpp
struct NeuralHrtfDesign;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `sections` | `std::array<std::array<BiquadCoefficients, neuralHrtfMaxSections>, 2>` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `gain` | `std::array<double, 2>` | `{1, 1}` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `delayFrames` | `std::array<double, 2>` | `{}` | Frame count or offset; one frame contains one sample per channel. |
| `sectionCount` | `std::size_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `sampleRate` | `double` | `0` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `desiredItdMicroseconds` | `double` | `0` | Duration or position in seconds. |
| `intrinsicItdMicroseconds` | `double` | `0` | Duration or position in seconds. |

## `designNeuralHrtf` {#symbol-designneuralhrtf}

```cpp
Status designNeuralHrtf(const NeuralHrtfParameters&, NeuralHrtfDesign&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const NeuralHrtfParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `NeuralHrtfDesign &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `NeuralBinauralOptions` {#symbol-neuralbinauraloptions}

```cpp
struct NeuralBinauralOptions;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `maximumDelaySeconds` | `double` | `.005` | Duration or position in seconds. |
| `transitionHistoryFrames` | `std::size_t` | `512` | Frame count or offset; one frame contains one sample per channel. |

## `NeuralBinaural` {#symbol-neuralbinaural}

```cpp
template <Sample T> class NeuralBinaural;
```

### `NeuralBinaural::NeuralBinaural<T>` {#symbol-neuralbinaural-neuralbinaural-t}

```cpp
NeuralBinaural();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `NeuralBinaural::~NeuralBinaural<T>` {#symbol-neuralbinaural-neuralbinaural-t-2}

```cpp
~NeuralBinaural();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `NeuralBinaural::NeuralBinaural<T>` {#symbol-neuralbinaural-neuralbinaural-t-3}

```cpp
NeuralBinaural(NeuralBinaural&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `NeuralBinaural<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `NeuralBinaural::operator=` {#symbol-neuralbinaural-operator}

```cpp
NeuralBinaural& operator=(NeuralBinaural&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `NeuralBinaural<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `NeuralBinaural<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `NeuralBinaural::prepare` {#symbol-neuralbinaural-prepare}

```cpp
Status prepare(const ProcessSpec& monoSpec, const NeuralHrtfModel&, const NeuralBinauralOptions& = {}) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `monoSpec` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `const NeuralHrtfModel &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `const NeuralBinauralOptions &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `NeuralBinaural::setDesign` {#symbol-neuralbinaural-setdesign}

```cpp
Status setDesign(const NeuralHrtfDesign&, std::size_t transitionFrames = 256) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const NeuralHrtfDesign &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `transitionFrames` | `std::size_t` | `256` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `NeuralBinaural::setDirection` {#symbol-neuralbinaural-setdirection}

```cpp
Status setDirection(Vec3, std::size_t transitionFrames = 256) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |
| `transitionFrames` | `std::size_t` | `256` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `NeuralBinaural::process` {#symbol-neuralbinaural-process}

```cpp
Status process(AudioBlock<const T> mono, AudioBlock<T> stereo) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `mono` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `stereo` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `NeuralBinaural::processBatch` {#symbol-neuralbinaural-processbatch}

```cpp
static Status processBatch(std::span<NeuralBinaural> voices, AudioBlock<const T> sources, AudioBlock<T> stereoPairs) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `voices` | `std::span<NeuralBinaural<T>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `sources` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `stereoPairs` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `NeuralBinaural::setDesigns` {#symbol-neuralbinaural-setdesigns}

```cpp
static Status setDesigns(std::span<NeuralBinaural> voices, std::span<const NeuralHrtfDesign> designs, std::size_t transitionFrames = 256) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `voices` | `std::span<NeuralBinaural<T>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `designs` | `std::span<const NeuralHrtfDesign>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `transitionFrames` | `std::size_t` | `256` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `NeuralBinaural::reset` {#symbol-neuralbinaural-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `NeuralBinaural::transitionRemainingFrames` {#symbol-neuralbinaural-transitionremainingframes}

```cpp
std::size_t transitionRemainingFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `NeuralBinaural::latencyFrames` {#symbol-neuralbinaural-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
