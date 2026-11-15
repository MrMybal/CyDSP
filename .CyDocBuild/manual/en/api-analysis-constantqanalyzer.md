---
id: api-analysis-constantqanalyzer
title: ConstantQAnalyzer
description: Direct variable-length complex projections with explicit kernel-size budgets.
section: API reference
order: 1100
revision: 1
parent: api-analysis
symbol: cydsp::ConstantQAnalyzer
---

Direct variable-length complex projections with explicit kernel-size budgets.

## Include {#include}

```cpp
#include <cydsp/analysis/ConstantQAnalyzer.h>
```

[Usage and contracts](doc:analysis) · [Common processing rules](doc:contracts)

## `ConstantQConfig` {#symbol-constantqconfig}

```cpp
struct ConstantQConfig;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `minimumHz` | `double` | `55` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `binsPerOctave` | `unsigned int` | `12` | Value/configuration in the declared type; apply the linked module contract. |
| `bins` | `std::size_t` | `96` | Value/configuration in the declared type; apply the linked module contract. |
| `hopFrames` | `std::size_t` | `512` | Frame count or offset; one frame contains one sample per channel. |
| `maximumKernelFrames` | `std::size_t` | `131072` | Frame count or offset; one frame contains one sample per channel. |

## `ConstantQAnalyzer` {#symbol-constantqanalyzer}

```cpp
template <Sample T> class ConstantQAnalyzer;
```

### `ConstantQAnalyzer::ConstantQAnalyzer<T>` {#symbol-constantqanalyzer-constantqanalyzer-t}

```cpp
ConstantQAnalyzer();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `ConstantQAnalyzer::~ConstantQAnalyzer<T>` {#symbol-constantqanalyzer-constantqanalyzer-t-2}

```cpp
~ConstantQAnalyzer();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `ConstantQAnalyzer::ConstantQAnalyzer<T>` {#symbol-constantqanalyzer-constantqanalyzer-t-3}

```cpp
ConstantQAnalyzer(ConstantQAnalyzer&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `ConstantQAnalyzer<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `ConstantQAnalyzer::operator=` {#symbol-constantqanalyzer-operator}

```cpp
ConstantQAnalyzer& operator=(ConstantQAnalyzer&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `ConstantQAnalyzer<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `ConstantQAnalyzer<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ConstantQAnalyzer::prepare` {#symbol-constantqanalyzer-prepare}

```cpp
Status prepare(const ProcessSpec&, const ConstantQConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `const ConstantQConfig &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `ConstantQAnalyzer::process` {#symbol-constantqanalyzer-process}

```cpp
Status process(AudioBlock<const T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |

Return: `Status` — Check Status; ok reports success.

### `ConstantQAnalyzer::reset` {#symbol-constantqanalyzer-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `ConstantQAnalyzer::hasFrame` {#symbol-constantqanalyzer-hasframe}

```cpp
bool hasFrame() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ConstantQAnalyzer::frameCount` {#symbol-constantqanalyzer-framecount}

```cpp
std::uint64_t frameCount() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::uint64_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ConstantQAnalyzer::centerFrequencies` {#symbol-constantqanalyzer-centerfrequencies}

```cpp
std::span<const double> centerFrequencies() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::span<const double>` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ConstantQAnalyzer::magnitudes` {#symbol-constantqanalyzer-magnitudes}

```cpp
std::span<const double> magnitudes(std::size_t channel) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Channel index/count in the declared planar layout. |

Return: `std::span<const double>` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ConstantQAnalyzer::windowFrames` {#symbol-constantqanalyzer-windowframes}

```cpp
std::size_t windowFrames(std::size_t bin) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `bin` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ConstantQAnalyzer::backend` {#symbol-constantqanalyzer-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
