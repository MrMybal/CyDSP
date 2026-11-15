---
id: api-analysis-levelmeter
title: LevelMeter
description: Sliding sample peak, RMS, DC, crest factor and stereo correlation.
section: API reference
order: 1103
revision: 1
parent: api-analysis
symbol: cydsp::LevelMeter
---

Sliding sample peak, RMS, DC, crest factor and stereo correlation.

## Include {#include}

```cpp
#include <cydsp/analysis/LevelMeter.h>
```

[Usage and contracts](doc:metering) · [Common processing rules](doc:contracts)

## `ChannelLevels` {#symbol-channellevels}

```cpp
struct ChannelLevels;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `peak` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `rms` | `double` | `0` | Time in milliseconds. |
| `mean` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `heldPeak` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |

### `ChannelLevels::peakDbfs` {#symbol-channellevels-peakdbfs}

```cpp
double peakDbfs() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ChannelLevels::rmsDbfs` {#symbol-channellevels-rmsdbfs}

```cpp
double rmsDbfs() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ChannelLevels::crestFactorDb` {#symbol-channellevels-crestfactordb}

```cpp
double crestFactorDb() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `LevelMeter` {#symbol-levelmeter}

```cpp
template <Sample T> class LevelMeter;
```

### `LevelMeter::LevelMeter<T>` {#symbol-levelmeter-levelmeter-t}

```cpp
LevelMeter();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `LevelMeter::~LevelMeter<T>` {#symbol-levelmeter-levelmeter-t-2}

```cpp
~LevelMeter();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `LevelMeter::LevelMeter<T>` {#symbol-levelmeter-levelmeter-t-3}

```cpp
LevelMeter(LevelMeter&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `LevelMeter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `LevelMeter::operator=` {#symbol-levelmeter-operator}

```cpp
LevelMeter& operator=(LevelMeter&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `LevelMeter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `LevelMeter<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `LevelMeter::prepare` {#symbol-levelmeter-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t windowFrames = 2048) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `windowFrames` | `std::size_t` | `2048` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `LevelMeter::process` {#symbol-levelmeter-process}

```cpp
Status process(AudioBlock<const T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |

Return: `Status` — Check Status; ok reports success.

### `LevelMeter::reset` {#symbol-levelmeter-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `LevelMeter::levels` {#symbol-levelmeter-levels}

```cpp
ChannelLevels levels(std::size_t channel) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Channel index/count in the declared planar layout. |

Return: `ChannelLevels` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `LevelMeter::stereoCorrelation` {#symbol-levelmeter-stereocorrelation}

```cpp
double stereoCorrelation() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `LevelMeter::windowFrames` {#symbol-levelmeter-windowframes}

```cpp
std::size_t windowFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
