---
id: api-synthesis-sourcebatch
title: SourceBatch
description: Fixed source slots, shared sample/wavetable banks, SIMD sine voices and multichannel gains.
section: API reference
order: 1187
revision: 1
parent: api-synthesis
symbol: cydsp::SourceBatch
---

Fixed source slots, shared sample/wavetable banks, SIMD sine voices and multichannel gains.

## Include {#include}

```cpp
#include <cydsp/synthesis/SourceBatch.h>
```

[Usage and contracts](doc:sources) · [Common processing rules](doc:contracts)

## `BatchSourceKind` {#symbol-batchsourcekind}

```cpp
enum class BatchSourceKind { sine, wavetable, grain };
```

## `BatchSource` {#symbol-batchsource}

```cpp
struct BatchSource;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `kind` | `BatchSourceKind` | `BatchSourceKind::sine` | Value/configuration in the declared type; apply the linked module contract. |
| `asset` | `std::size_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `frequencyHz` | `double` | `440` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `gain` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `phase` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `samplePosition` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `playbackRatio` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `durationFrames` | `std::size_t` | `4800` | Frame count or offset; one frame contains one sample per channel. |
| `startOffsetFrames` | `std::size_t` | `0` | Frame count or offset; one frame contains one sample per channel. |
| `channelGains` | `std::array<double, 64>` | `{1}` | Value/configuration in the declared type; apply the linked module contract. |

## `SourceBatch` {#symbol-sourcebatch}

```cpp
template <Sample T> class SourceBatch;
```

### `SourceBatch::SourceBatch<T>` {#symbol-sourcebatch-sourcebatch-t}

```cpp
SourceBatch();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `SourceBatch::~SourceBatch<T>` {#symbol-sourcebatch-sourcebatch-t-2}

```cpp
~SourceBatch();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `SourceBatch::SourceBatch<T>` {#symbol-sourcebatch-sourcebatch-t-3}

```cpp
SourceBatch(SourceBatch&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `SourceBatch<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `SourceBatch::operator=` {#symbol-sourcebatch-operator}

```cpp
SourceBatch& operator=(SourceBatch&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `SourceBatch<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `SourceBatch<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SourceBatch::prepare` {#symbol-sourcebatch-prepare}

```cpp
Status prepare(const ProcessSpec& output, std::size_t maximumVoices, std::span<const Wavetable<T>* const> tables = {}, std::span<const SampleData<T>* const> samples = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `output` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `maximumVoices` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `tables` | `std::span<const Wavetable<T> *const>` | `{}` | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `samples` | `std::span<const SampleData<T> *const>` | `{}` | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `SourceBatch::start` {#symbol-sourcebatch-start}

```cpp
Status start(std::size_t slot, const BatchSource&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `slot` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `const BatchSource &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `SourceBatch::stop` {#symbol-sourcebatch-stop}

```cpp
Status stop(std::size_t slot) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `slot` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `SourceBatch::setPosition` {#symbol-sourcebatch-setposition}

```cpp
Status setPosition(std::size_t slot, Vec3, int ambisonicOrder = -1) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `slot` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |
| `ambisonicOrder` | `int` | `-1` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `SourceBatch::process` {#symbol-sourcebatch-process}

```cpp
Status process(AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `SourceBatch::reset` {#symbol-sourcebatch-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `SourceBatch::activeVoices` {#symbol-sourcebatch-activevoices}

```cpp
std::size_t activeVoices() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SourceBatch::backend` {#symbol-sourcebatch-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
