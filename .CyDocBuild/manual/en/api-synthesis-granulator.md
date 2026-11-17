---
id: api-synthesis-granulator
title: Granulator
description: Bounded offline-sample grain voices and deterministic scheduled clouds.
section: API reference
order: 1184
revision: 1
parent: api-synthesis
symbol: cydsp::Granulator
---

Bounded offline-sample grain voices and deterministic scheduled clouds.

## Include {#include}

```cpp
#include <cydsp/synthesis/Granulator.h>
```

[Usage and contracts](doc:granular) · [Common processing rules](doc:contracts)

## `GrainEnvelope` {#symbol-grainenvelope}

```cpp
enum class GrainEnvelope { hann, triangular, rectangular };
```

## `GrainParameters` {#symbol-grainparameters}

```cpp
struct GrainParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `positionSeconds` | `double` | `0` | Duration or position in seconds. |
| `durationMs` | `double` | `80` | Time in milliseconds. |
| `pitchRatio` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `gain` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `pan` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `wrap` | `bool` | `true` | Explicit true/false option or predicate. |
| `envelope` | `GrainEnvelope` | `GrainEnvelope::hann` | Value/configuration in the declared type; apply the linked module contract. |
| `interpolation` | `SampleInterpolation` | `SampleInterpolation::bandLimited` | Value/configuration in the declared type; apply the linked module contract. |

## `GrainCloudParameters` {#symbol-graincloudparameters}

```cpp
struct GrainCloudParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `grain` | `GrainParameters` | — | Value/configuration in the declared type; apply the linked module contract. |
| `grainsPerSecond` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `positionSpreadSeconds` | `double` | `0` | Duration or position in seconds. |
| `pitchSpreadSemitones` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `panSpread` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |

## `Granulator` {#symbol-granulator}

```cpp
template <Sample T> class Granulator;
```

### `Granulator::Granulator<T>` {#symbol-granulator-granulator-t}

```cpp
Granulator();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `Granulator::~Granulator<T>` {#symbol-granulator-granulator-t-2}

```cpp
~Granulator();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `Granulator::Granulator<T>` {#symbol-granulator-granulator-t-3}

```cpp
Granulator(Granulator&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Granulator<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `Granulator::operator=` {#symbol-granulator-operator}

```cpp
Granulator& operator=(Granulator&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Granulator<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Granulator<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Granulator::prepare` {#symbol-granulator-prepare}

```cpp
Status prepare(const ProcessSpec&, const SampleData<T>&, std::size_t maximumGrains = 128, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `const SampleData<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `maximumGrains` | `std::size_t` | `128` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `Granulator::trigger` {#symbol-granulator-trigger}

```cpp
Status trigger(const GrainParameters&, std::size_t frameOffset = 0) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const GrainParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `frameOffset` | `std::size_t` | `0` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `Granulator::setCloud` {#symbol-granulator-setcloud}

```cpp
Status setCloud(const GrainCloudParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const GrainCloudParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `Granulator::reset` {#symbol-granulator-reset}

```cpp
void reset(std::uint64_t seed = 1) noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `seed` | `std::uint64_t` | `1` | Per-instance deterministic random seed. |

### `Granulator::process` {#symbol-granulator-process}

```cpp
Status process(AudioBlock<T>, bool add = false) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |
| `add` | `bool` | `false` | Explicit true/false option or predicate. |

Return: `Status` — Check Status; ok reports success.

### `Granulator::activeGrains` {#symbol-granulator-activegrains}

```cpp
std::size_t activeGrains() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Granulator::droppedGrains` {#symbol-granulator-droppedgrains}

```cpp
std::uint64_t droppedGrains() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::uint64_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Granulator::backend` {#symbol-granulator-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
