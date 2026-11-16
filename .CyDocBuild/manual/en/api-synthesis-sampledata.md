---
id: api-synthesis-sampledata
title: SampleData
description: Copied in-memory samples, prepared antialiasing mips, interpolation and loop crossfades.
section: API reference
order: 1186
revision: 1
parent: api-synthesis
symbol: cydsp::SampleData
---

Copied in-memory samples, prepared antialiasing mips, interpolation and loop crossfades.

## Include {#include}

```cpp
#include <cydsp/synthesis/SampleData.h>
```

[Usage and contracts](doc:sampling) · [Common processing rules](doc:contracts)

## `SampleInterpolation` {#symbol-sampleinterpolation}

```cpp
enum class SampleInterpolation { linear, bandLimited };
```

## `SampleData` {#symbol-sampledata}

```cpp
template <Sample T> class SampleData;
```

### `SampleData::prepare` {#symbol-sampledata-prepare}

```cpp
Status prepare(AudioBlock<const T>, double sampleRate, unsigned rootNote = 60, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `sampleRate` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `rootNote` | `unsigned int` | `60` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `SampleData::setLoop` {#symbol-sampledata-setloop}

```cpp
Status setLoop(std::size_t first, std::size_t end, std::size_t crossfadeFrames = 0) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `first` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `end` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `crossfadeFrames` | `std::size_t` | `0` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `SampleData::samples` {#symbol-sampledata-samples}

```cpp
AudioBlock<const T> samples() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `AudioBlock<const T>` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SampleData::sampleRate` {#symbol-sampledata-samplerate}

```cpp
double sampleRate() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SampleData::rootNote` {#symbol-sampledata-rootnote}

```cpp
unsigned rootNote() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `unsigned int` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SampleData::loopStart` {#symbol-sampledata-loopstart}

```cpp
std::size_t loopStart() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SampleData::loopEnd` {#symbol-sampledata-loopend}

```cpp
std::size_t loopEnd() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SampleData::loopCrossfade` {#symbol-sampledata-loopcrossfade}

```cpp
std::size_t loopCrossfade() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SampleData::mipLevels` {#symbol-sampledata-miplevels}

```cpp
std::size_t mipLevels() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SampleData::value` {#symbol-sampledata-value}

```cpp
T value(std::size_t channel, double position, double ratio = 1, SampleInterpolation = SampleInterpolation::bandLimited) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Channel index/count in the declared planar layout. |
| `position` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `ratio` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 4` | `SampleInterpolation` | `SampleInterpolation::bandLimited` | Value/configuration in the declared type; apply the linked module contract. |

Return: `T` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SampleData::render` {#symbol-sampledata-render}

```cpp
Status render(double& position, double ratio, SampleInterpolation, AudioBlock<T> output, std::size_t& inside) const noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `position` | `double &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |
| `ratio` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `SampleInterpolation` | — | Value/configuration in the declared type; apply the linked module contract. |
| `output` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |
| `inside` | `std::size_t &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.
