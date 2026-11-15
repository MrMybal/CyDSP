---
id: api-processors-tptfilter
title: TptFilter
description: TPT state-variable and ladder filters, Linkwitz–Riley splitting and additional filter designs.
section: API reference
order: 1167
revision: 1
parent: api-processors
symbol: cydsp::TptFilter
---

TPT state-variable and ladder filters, Linkwitz–Riley splitting and additional filter designs.

## Include {#include}

```cpp
#include <cydsp/processors/TptFilter.h>
```

[Usage and contracts](doc:filters) · [Common processing rules](doc:contracts)

## `TptFilterMode` {#symbol-tptfiltermode}

```cpp
enum class TptFilterMode { lowPass, bandPass, highPass, notch, allPass, firstOrderLowPass, firstOrderHighPass, firstOrderAllPass };
```

## `TptFilter` {#symbol-tptfilter}

```cpp
template <Sample T> class TptFilter;
```

### `TptFilter::TptFilter<T>` {#symbol-tptfilter-tptfilter-t}

```cpp
TptFilter();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `TptFilter::~TptFilter<T>` {#symbol-tptfilter-tptfilter-t-2}

```cpp
~TptFilter();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `TptFilter::TptFilter<T>` {#symbol-tptfilter-tptfilter-t-3}

```cpp
TptFilter(TptFilter&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `TptFilter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `TptFilter::operator=` {#symbol-tptfilter-operator}

```cpp
TptFilter& operator=(TptFilter&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `TptFilter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `TptFilter<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `TptFilter::prepare` {#symbol-tptfilter-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |

Return: `Status` — Check Status; ok reports success.

### `TptFilter::setParameters` {#symbol-tptfilter-setparameters}

```cpp
Status setParameters(double frequency, double q, TptFilterMode) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `frequency` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `q` | `double` | — | Positive finite filter quality factor. |
| `argument 3` | `TptFilterMode` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `TptFilter::process` {#symbol-tptfilter-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `TptFilter::reset` {#symbol-tptfilter-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

## `LinkwitzRiley` {#symbol-linkwitzriley}

```cpp
template <Sample T> class LinkwitzRiley;
```

### `LinkwitzRiley::LinkwitzRiley<T>` {#symbol-linkwitzriley-linkwitzriley-t}

```cpp
LinkwitzRiley();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `LinkwitzRiley::~LinkwitzRiley<T>` {#symbol-linkwitzriley-linkwitzriley-t-2}

```cpp
~LinkwitzRiley();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `LinkwitzRiley::LinkwitzRiley<T>` {#symbol-linkwitzriley-linkwitzriley-t-3}

```cpp
LinkwitzRiley(LinkwitzRiley&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `LinkwitzRiley<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `LinkwitzRiley::operator=` {#symbol-linkwitzriley-operator}

```cpp
LinkwitzRiley& operator=(LinkwitzRiley&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `LinkwitzRiley<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `LinkwitzRiley<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `LinkwitzRiley::prepare` {#symbol-linkwitzriley-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |

Return: `Status` — Check Status; ok reports success.

### `LinkwitzRiley::setFrequency` {#symbol-linkwitzriley-setfrequency}

```cpp
Status setFrequency(double frequency) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `frequency` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |

Return: `Status` — Check Status; ok reports success.

### `LinkwitzRiley::process` {#symbol-linkwitzriley-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T> low, AudioBlock<T> high) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `low` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |
| `high` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `LinkwitzRiley::reset` {#symbol-linkwitzriley-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

## `LadderFilter` {#symbol-ladderfilter}

```cpp
template <Sample T> class LadderFilter;
```

### `LadderFilter::LadderFilter<T>` {#symbol-ladderfilter-ladderfilter-t}

```cpp
LadderFilter();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `LadderFilter::~LadderFilter<T>` {#symbol-ladderfilter-ladderfilter-t-2}

```cpp
~LadderFilter();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `LadderFilter::LadderFilter<T>` {#symbol-ladderfilter-ladderfilter-t-3}

```cpp
LadderFilter(LadderFilter&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `LadderFilter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `LadderFilter::operator=` {#symbol-ladderfilter-operator}

```cpp
LadderFilter& operator=(LadderFilter&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `LadderFilter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `LadderFilter<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `LadderFilter::prepare` {#symbol-ladderfilter-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |

Return: `Status` — Check Status; ok reports success.

### `LadderFilter::setParameters` {#symbol-ladderfilter-setparameters}

```cpp
Status setParameters(double frequency, double resonance, double drive = 1) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `frequency` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `resonance` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `drive` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `LadderFilter::process` {#symbol-ladderfilter-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `LadderFilter::reset` {#symbol-ladderfilter-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

## `designButterworth` {#symbol-designbutterworth}

```cpp
Status designButterworth(BiquadType, unsigned order, double sampleRate, double cutoff, std::span<BiquadCoefficients> sections, std::size_t& count) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `BiquadType` | — | Value/configuration in the declared type; apply the linked module contract. |
| `order` | `unsigned int` | — | Value/configuration in the declared type; apply the linked module contract. |
| `sampleRate` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `cutoff` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `sections` | `std::span<BiquadCoefficients>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `count` | `std::size_t &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `designWindowedFir` {#symbol-designwindowedfir}

```cpp
Status designWindowedFir(std::span<double> taps, double normalizedCutoff, bool highPass = false, double attenuationDb = 100) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `taps` | `std::span<double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `normalizedCutoff` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `highPass` | `bool` | `false` | Explicit true/false option or predicate. |
| `attenuationDb` | `double` | `100` | Logarithmic level in the indicated dB reference. |

Return: `Status` — Check Status; ok reports success.
