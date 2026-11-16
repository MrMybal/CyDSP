---
id: api-synthesis-livegranulator
title: LiveGranulator
description: Prepared circular live-audio history, freeze and independently routed forward/reverse grains.
section: API reference
order: 1184
revision: 1
parent: api-synthesis
symbol: cydsp::LiveGranulator
---

Prepared circular live-audio history, freeze and independently routed forward/reverse grains.

## Include {#include}

```cpp
#include <cydsp/synthesis/LiveGranulator.h>
```

[Usage and contracts](doc:granular) · [Common processing rules](doc:contracts)

## `LiveGrainParameters` {#symbol-livegrainparameters}

```cpp
struct LiveGrainParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `ageSeconds` | `double` | `.1` | Duration or position in seconds. |
| `durationMs` | `double` | `80` | Time in milliseconds. |
| `pitchRatio` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `gain` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `inputChannel` | `std::size_t` | `0` | Channel index/count in the declared planar layout. |
| `channelGains` | `std::array<double, 8>` | `{1, 0, 0, 0, 0, 0, 0, 0}` | Value/configuration in the declared type; apply the linked module contract. |

## `LiveCloudParameters` {#symbol-livecloudparameters}

```cpp
struct LiveCloudParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `grain` | `LiveGrainParameters` | — | Value/configuration in the declared type; apply the linked module contract. |
| `grainsPerSecond` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `ageSpreadSeconds` | `double` | `0` | Duration or position in seconds. |
| `pitchSpreadSemitones` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |

## `LiveGranulator` {#symbol-livegranulator}

```cpp
template <Sample T> class LiveGranulator;
```

### `LiveGranulator::LiveGranulator<T>` {#symbol-livegranulator-livegranulator-t}

```cpp
LiveGranulator();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `LiveGranulator::~LiveGranulator<T>` {#symbol-livegranulator-livegranulator-t-2}

```cpp
~LiveGranulator();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `LiveGranulator::LiveGranulator<T>` {#symbol-livegranulator-livegranulator-t-3}

```cpp
LiveGranulator(LiveGranulator&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `LiveGranulator<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `LiveGranulator::operator=` {#symbol-livegranulator-operator}

```cpp
LiveGranulator& operator=(LiveGranulator&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `LiveGranulator<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `LiveGranulator<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `LiveGranulator::prepare` {#symbol-livegranulator-prepare}

```cpp
Status prepare(const ProcessSpec& input, std::size_t outputChannels = 2, double historySeconds = 4, std::size_t maximumGrains = 128, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `input` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `outputChannels` | `std::size_t` | `2` | Value/configuration in the declared type; apply the linked module contract. |
| `historySeconds` | `double` | `4` | Duration or position in seconds. |
| `maximumGrains` | `std::size_t` | `128` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `LiveGranulator::trigger` {#symbol-livegranulator-trigger}

```cpp
Status trigger(const LiveGrainParameters&, std::size_t frameOffset = 0) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const LiveGrainParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `frameOffset` | `std::size_t` | `0` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `LiveGranulator::setCloud` {#symbol-livegranulator-setcloud}

```cpp
Status setCloud(const LiveCloudParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const LiveCloudParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `LiveGranulator::setFrozen` {#symbol-livegranulator-setfrozen}

```cpp
void setFrozen(bool) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `bool` | — | Explicit true/false option or predicate. |

### `LiveGranulator::process` {#symbol-livegranulator-process}

```cpp
Status process(AudioBlock<const T> capture, AudioBlock<T> output) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `capture` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `output` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `LiveGranulator::reset` {#symbol-livegranulator-reset}

```cpp
void reset(std::uint64_t seed = 1) noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `seed` | `std::uint64_t` | `1` | Per-instance deterministic random seed. |

### `LiveGranulator::activeGrains` {#symbol-livegranulator-activegrains}

```cpp
std::size_t activeGrains() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `LiveGranulator::droppedGrains` {#symbol-livegranulator-droppedgrains}

```cpp
std::uint64_t droppedGrains() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::uint64_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
