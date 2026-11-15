---
id: api-control-quality
title: Quality
description: Explicit economy, realtimeHigh and offline preparation policies and prepared graph transitions.
section: API reference
order: 1109
revision: 1
parent: api-control
symbol: cydsp::QualityProfile
---

Explicit economy, realtimeHigh and offline preparation policies and prepared graph transitions.

## Include {#include}

```cpp
#include <cydsp/control/Quality.h>
```

[Usage and contracts](doc:quality) · [Common processing rules](doc:contracts)

## `QualityProfile` {#symbol-qualityprofile}

```cpp
enum class QualityProfile { economy, realtimeHigh, offline };
```

## `QualitySettings` {#symbol-qualitysettings}

```cpp
struct QualitySettings;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `profile` | `QualityProfile` | `QualityProfile::realtimeHigh` | Value/configuration in the declared type; apply the linked module contract. |
| `spectral` | `StftConfig` | — | Value/configuration in the declared type; apply the linked module contract. |
| `separation` | `HpssConfig` | — | Value/configuration in the declared type; apply the linked module contract. |
| `hrtf` | `HrtfPreparation` | — | Value/configuration in the declared type; apply the linked module contract. |
| `resampling` | `ResampleQuality` | `ResampleQuality::balanced` | Value/configuration in the declared type; apply the linked module contract. |
| `sampleInterpolation` | `SampleInterpolation` | `SampleInterpolation::bandLimited` | Value/configuration in the declared type; apply the linked module contract. |
| `reverb` | `FdnConfig` | — | Value/configuration in the declared type; apply the linked module contract. |
| `oversampling` | `unsigned int` | `4` | Value/configuration in the declared type; apply the linked module contract. |
| `truePeakFactor` | `unsigned int` | `4` | Value/configuration in the declared type; apply the linked module contract. |
| `saturationAntialias` | `bool` | `true` | Explicit true/false option or predicate. |
| `saturationMath` | `MathAccuracy` | `MathAccuracy::precise` | Value/configuration in the declared type; apply the linked module contract. |
| `transientMath` | `MathAccuracy` | `MathAccuracy::precise` | Value/configuration in the declared type; apply the linked module contract. |
| `suggestedMaximumGrains` | `std::size_t` | `128` | Value/configuration in the declared type; apply the linked module contract. |
| `transitionFrames` | `std::size_t` | `256` | Frame count or offset; one frame contains one sample per channel. |

## `qualitySettings` {#symbol-qualitysettings-2}

```cpp
Status qualitySettings(QualityProfile, double sampleRate, QualitySettings&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `QualityProfile` | — | Value/configuration in the declared type; apply the linked module contract. |
| `sampleRate` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `argument 3` | `QualitySettings &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `QualityGraph` {#symbol-qualitygraph}

```cpp
template <Sample T> class QualityGraph;
```

### `QualityGraph::QualityGraph<T>` {#symbol-qualitygraph-qualitygraph-t}

```cpp
QualityGraph();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `QualityGraph::~QualityGraph<T>` {#symbol-qualitygraph-qualitygraph-t-2}

```cpp
~QualityGraph();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `QualityGraph::QualityGraph<T>` {#symbol-qualitygraph-qualitygraph-t-3}

```cpp
QualityGraph(QualityGraph&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `QualityGraph<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `QualityGraph::operator=` {#symbol-qualitygraph-operator}

```cpp
QualityGraph& operator=(QualityGraph&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `QualityGraph<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `QualityGraph<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `QualityGraph::prepare` {#symbol-qualitygraph-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const DspBus> inputs, std::span<const DspBus> outputs, std::size_t latencyBudgetFrames, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `inputs` | `std::span<const DspBus>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `outputs` | `std::span<const DspBus>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `latencyBudgetFrames` | `std::size_t` | — | Frame count or offset; one frame contains one sample per channel. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `QualityGraph::setInitial` {#symbol-qualitygraph-setinitial}

```cpp
Status setInitial(QualityProfile, MultiBusGraph<T>&&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `QualityProfile` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `MultiBusGraph<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

### `QualityGraph::stage` {#symbol-qualitygraph-stage}

```cpp
Status stage(QualityProfile, MultiBusGraph<T>&&) noexcept;
```

Stage outside realtime processing; retained assets/contexts must satisfy the transition lifetime.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `QualityProfile` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `MultiBusGraph<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

### `QualityGraph::startTransition` {#symbol-qualitygraph-starttransition}

```cpp
Status startTransition(std::size_t fadeFrames = 256, std::size_t extraWarmupFrames = 0) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `fadeFrames` | `std::size_t` | `256` | Frame count or offset; one frame contains one sample per channel. |
| `extraWarmupFrames` | `std::size_t` | `0` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `QualityGraph::process` {#symbol-qualitygraph-process}

```cpp
Status process(std::span<const AudioBlock<const T>>, std::span<AudioBlock<T>>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const AudioBlock<const T>>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `std::span<AudioBlock<T>>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `QualityGraph::reset` {#symbol-qualitygraph-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `QualityGraph::activeProfile` {#symbol-qualitygraph-activeprofile}

```cpp
QualityProfile activeProfile() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `QualityProfile` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `QualityGraph::transitioning` {#symbol-qualitygraph-transitioning}

```cpp
bool transitioning() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `QualityGraph::latencyFrames` {#symbol-qualitygraph-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
