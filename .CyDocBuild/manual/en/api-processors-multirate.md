---
id: api-processors-multirate
title: Multirate
description: Integer FIR rate conversion and dyadic analysis/synthesis filter banks.
section: API reference
order: 1151
revision: 1
parent: api-processors
symbol: cydsp::RateConversion
---

Integer FIR rate conversion and dyadic analysis/synthesis filter banks.

## Include {#include}

```cpp
#include <cydsp/processors/Multirate.h>
```

[Usage and contracts](doc:resampling) · [Common processing rules](doc:contracts)

## `RateConversion` {#symbol-rateconversion}

```cpp
enum class RateConversion { decimate, interpolate };
```

## `FirRateConverter` {#symbol-firrateconverter}

```cpp
template <Sample T> class FirRateConverter;
```

### `FirRateConverter::FirRateConverter<T>` {#symbol-firrateconverter-firrateconverter-t}

```cpp
FirRateConverter();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `FirRateConverter::~FirRateConverter<T>` {#symbol-firrateconverter-firrateconverter-t-2}

```cpp
~FirRateConverter();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `FirRateConverter::FirRateConverter<T>` {#symbol-firrateconverter-firrateconverter-t-3}

```cpp
FirRateConverter(FirRateConverter&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `FirRateConverter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `FirRateConverter::operator=` {#symbol-firrateconverter-operator}

```cpp
FirRateConverter& operator=(FirRateConverter&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `FirRateConverter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `FirRateConverter<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `FirRateConverter::prepare` {#symbol-firrateconverter-prepare}

```cpp
Status prepare(const ProcessSpec& input, unsigned factor, RateConversion, std::size_t tapsPerPhase = 32, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `input` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `factor` | `unsigned int` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `RateConversion` | — | Value/configuration in the declared type; apply the linked module contract. |
| `tapsPerPhase` | `std::size_t` | `32` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `FirRateConverter::outputFrames` {#symbol-firrateconverter-outputframes}

```cpp
std::size_t outputFrames(std::size_t inputFrames) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `inputFrames` | `std::size_t` | — | Frame count or offset; one frame contains one sample per channel. |

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `FirRateConverter::process` {#symbol-firrateconverter-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `FirRateConverter::reset` {#symbol-firrateconverter-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `FirRateConverter::outputSampleRate` {#symbol-firrateconverter-outputsamplerate}

```cpp
double outputSampleRate() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `DyadicFilterBank` {#symbol-dyadicfilterbank}

```cpp
template <Sample T> class DyadicFilterBank;
```

### `DyadicFilterBank::DyadicFilterBank<T>` {#symbol-dyadicfilterbank-dyadicfilterbank-t}

```cpp
DyadicFilterBank() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `DyadicFilterBank::DyadicFilterBank<T>` {#symbol-dyadicfilterbank-dyadicfilterbank-t-2}

```cpp
DyadicFilterBank(const DyadicFilterBank&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const DyadicFilterBank<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `DyadicFilterBank::operator=` {#symbol-dyadicfilterbank-operator}

```cpp
DyadicFilterBank& operator=(const DyadicFilterBank&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const DyadicFilterBank<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `DyadicFilterBank<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `DyadicFilterBank::DyadicFilterBank<T>` {#symbol-dyadicfilterbank-dyadicfilterbank-t-3}

```cpp
DyadicFilterBank(DyadicFilterBank&& o) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `o` | `DyadicFilterBank<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `DyadicFilterBank::operator=` {#symbol-dyadicfilterbank-operator-2}

```cpp
DyadicFilterBank& operator=(DyadicFilterBank&& o) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `o` | `DyadicFilterBank<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `DyadicFilterBank<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `DyadicFilterBank::prepare` {#symbol-dyadicfilterbank-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t bands = 8) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `bands` | `std::size_t` | `8` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `DyadicFilterBank::analysisFrames` {#symbol-dyadicfilterbank-analysisframes}

```cpp
std::size_t analysisFrames(std::size_t inputFrames) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `inputFrames` | `std::size_t` | — | Frame count or offset; one frame contains one sample per channel. |

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `DyadicFilterBank::analyze` {#symbol-dyadicfilterbank-analyze}

```cpp
Status analyze(AudioBlock<const T>, AudioBlock<T> bands) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `bands` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `DyadicFilterBank::synthesize` {#symbol-dyadicfilterbank-synthesize}

```cpp
Status synthesize(AudioBlock<const T> bands, AudioBlock<T>) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `bands` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `DyadicFilterBank::reset` {#symbol-dyadicfilterbank-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.
