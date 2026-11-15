---
id: api-analysis-musicalanalyzer
title: MusicalAnalyzer
description: YIN pitch/confidence, onset, tempo/beat, chroma, MFCC and key estimates.
section: API reference
order: 1105
revision: 1
parent: api-analysis
symbol: cydsp::MusicalAnalyzer
---

YIN pitch/confidence, onset, tempo/beat, chroma, MFCC and key estimates.

## Include {#include}

```cpp
#include <cydsp/analysis/MusicalAnalyzer.h>
```

[Usage and contracts](doc:analysis) · [Common processing rules](doc:contracts)

## `MusicalAnalysisConfig` {#symbol-musicalanalysisconfig}

```cpp
struct MusicalAnalysisConfig;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `fftFrames` | `std::size_t` | `2048` | Frame count or offset; one frame contains one sample per channel. |
| `hopFrames` | `std::size_t` | `256` | Frame count or offset; one frame contains one sample per channel. |
| `melBands` | `std::size_t` | `40` | Value/configuration in the declared type; apply the linked module contract. |
| `mfccCount` | `std::size_t` | `13` | Value/configuration in the declared type; apply the linked module contract. |
| `minimumPitchHz` | `double` | `60` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `maximumPitchHz` | `double` | `2000` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `onsetThreshold` | `double` | `1.5` | Value/configuration in the declared type; apply the linked module contract. |
| `minimumBpm` | `double` | `50` | Value/configuration in the declared type; apply the linked module contract. |
| `maximumBpm` | `double` | `220` | Value/configuration in the declared type; apply the linked module contract. |

## `MusicalFeatures` {#symbol-musicalfeatures}

```cpp
struct MusicalFeatures;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `fundamentalHz` | `double` | `0` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `pitchConfidence` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `onsetStrength` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `tempoBpm` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `beatConfidence` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `beatPhase` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `keyConfidence` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `onset` | `bool` | `false` | Explicit true/false option or predicate. |
| `keyMinor` | `bool` | `false` | Explicit true/false option or predicate. |
| `keyPitchClass` | `unsigned int` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `chroma` | `std::array<double, 12>` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `mfcc` | `std::array<double, 32>` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `frameEnd` | `std::uint64_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |

## `MusicalAnalyzer` {#symbol-musicalanalyzer}

```cpp
template <Sample T> class MusicalAnalyzer;
```

### `MusicalAnalyzer::Callback` {#symbol-musicalanalyzer-callback}

```cpp
using Callback = void (*)(void*, const MusicalFeatures&) noexcept;
```

### `MusicalAnalyzer::MusicalAnalyzer<T>` {#symbol-musicalanalyzer-musicalanalyzer-t}

```cpp
MusicalAnalyzer();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `MusicalAnalyzer::~MusicalAnalyzer<T>` {#symbol-musicalanalyzer-musicalanalyzer-t-2}

```cpp
~MusicalAnalyzer();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `MusicalAnalyzer::MusicalAnalyzer<T>` {#symbol-musicalanalyzer-musicalanalyzer-t-3}

```cpp
MusicalAnalyzer(MusicalAnalyzer&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `MusicalAnalyzer<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `MusicalAnalyzer::operator=` {#symbol-musicalanalyzer-operator}

```cpp
MusicalAnalyzer& operator=(MusicalAnalyzer&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `MusicalAnalyzer<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `MusicalAnalyzer<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MusicalAnalyzer::prepare` {#symbol-musicalanalyzer-prepare}

```cpp
Status prepare(const ProcessSpec&, const MusicalAnalysisConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `const MusicalAnalysisConfig &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `MusicalAnalyzer::setCallback` {#symbol-musicalanalyzer-setcallback}

```cpp
void setCallback(Callback, void* context = nullptr) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Callback` | — | Value/configuration in the declared type; apply the linked module contract. |
| `context` | `void *` | `nullptr` | Borrowed callback context; the application owns lifetime and synchronization. |

### `MusicalAnalyzer::process` {#symbol-musicalanalyzer-process}

```cpp
Status process(AudioBlock<const T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |

Return: `Status` — Check Status; ok reports success.

### `MusicalAnalyzer::features` {#symbol-musicalanalyzer-features}

```cpp
MusicalFeatures features() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `MusicalFeatures` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MusicalAnalyzer::reset` {#symbol-musicalanalyzer-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.
