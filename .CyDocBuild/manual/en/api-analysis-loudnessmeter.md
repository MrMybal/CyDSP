---
id: api-analysis-loudnessmeter
title: LoudnessMeter
description: K-weighted momentary, short-term and integrated loudness with exact bounded history.
section: API reference
order: 1104
revision: 1
parent: api-analysis
symbol: cydsp::LoudnessMeter
---

K-weighted momentary, short-term and integrated loudness with exact bounded history.

## Include {#include}

```cpp
#include <cydsp/analysis/LoudnessMeter.h>
```

[Usage and contracts](doc:metering) · [Common processing rules](doc:contracts)

## `LoudnessConfig` {#symbol-loudnessconfig}

```cpp
struct LoudnessConfig;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `maximumGatingBlocks` | `std::size_t` | `288000` | Value/configuration in the declared type; apply the linked module contract. |
| `channelWeights` | `std::span<const double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

## `LoudnessLevels` {#symbol-loudnesslevels}

```cpp
struct LoudnessLevels;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `momentaryLufs` | `double` | `-std::numeric_limits<double>::infinity()` | Value/configuration in the declared type; apply the linked module contract. |
| `shortTermLufs` | `double` | `-std::numeric_limits<double>::infinity()` | Value/configuration in the declared type; apply the linked module contract. |
| `momentaryReady` | `bool` | `false` | Explicit true/false option or predicate. |
| `shortTermReady` | `bool` | `false` | Explicit true/false option or predicate. |

## `LoudnessMeter` {#symbol-loudnessmeter}

```cpp
template <Sample T> class LoudnessMeter;
```

### `LoudnessMeter::LoudnessMeter<T>` {#symbol-loudnessmeter-loudnessmeter-t}

```cpp
LoudnessMeter();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `LoudnessMeter::~LoudnessMeter<T>` {#symbol-loudnessmeter-loudnessmeter-t-2}

```cpp
~LoudnessMeter();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `LoudnessMeter::LoudnessMeter<T>` {#symbol-loudnessmeter-loudnessmeter-t-3}

```cpp
LoudnessMeter(LoudnessMeter&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `LoudnessMeter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `LoudnessMeter::operator=` {#symbol-loudnessmeter-operator}

```cpp
LoudnessMeter& operator=(LoudnessMeter&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `LoudnessMeter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `LoudnessMeter<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `LoudnessMeter::prepare` {#symbol-loudnessmeter-prepare}

```cpp
Status prepare(const ProcessSpec&, const LoudnessConfig& = {}) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `const LoudnessConfig &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `LoudnessMeter::process` {#symbol-loudnessmeter-process}

```cpp
Status process(AudioBlock<const T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |

Return: `Status` — Check Status; ok reports success.

### `LoudnessMeter::reset` {#symbol-loudnessmeter-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `LoudnessMeter::levels` {#symbol-loudnessmeter-levels}

```cpp
LoudnessLevels levels() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `LoudnessLevels` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `LoudnessMeter::integratedLufs` {#symbol-loudnessmeter-integratedlufs}

```cpp
double integratedLufs() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `LoudnessMeter::loudnessRangeLu` {#symbol-loudnessmeter-loudnessrangelu}

```cpp
double loudnessRangeLu() noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `LoudnessMeter::gatingBlocks` {#symbol-loudnessmeter-gatingblocks}

```cpp
std::size_t gatingBlocks() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `LoudnessMeter::capacity` {#symbol-loudnessmeter-capacity}

```cpp
std::size_t capacity() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
