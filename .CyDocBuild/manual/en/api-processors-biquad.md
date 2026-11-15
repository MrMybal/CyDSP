---
id: api-processors-biquad
title: Biquad
description: Normalized stable coefficients, peaking/shelf designs and a prepared multichannel biquad.
section: API reference
order: 1130
revision: 1
parent: api-processors
symbol: cydsp::Biquad
---

Normalized stable coefficients, peaking/shelf designs and a prepared multichannel biquad.

## Include {#include}

```cpp
#include <cydsp/processors/Biquad.h>
```

[Usage and contracts](doc:filters) · [Common processing rules](doc:contracts)

## `BiquadType` {#symbol-biquadtype}

```cpp
enum class BiquadType { lowPass, highPass, bandPass, notch, allPass };
```

## `BiquadCoefficients` {#symbol-biquadcoefficients}

```cpp
struct BiquadCoefficients;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `b0` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `b1` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `b2` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `a1` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `a2` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |

### `BiquadCoefficients::isStable` {#symbol-biquadcoefficients-isstable}

```cpp
constexpr bool isStable() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `designBiquad` {#symbol-designbiquad}

```cpp
Status designBiquad(BiquadType, double sampleRate, double frequency, double q, BiquadCoefficients& result) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `BiquadType` | — | Value/configuration in the declared type; apply the linked module contract. |
| `sampleRate` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `frequency` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `q` | `double` | — | Positive finite filter quality factor. |
| `result` | `BiquadCoefficients &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `designPeakingBiquad` {#symbol-designpeakingbiquad}

```cpp
Status designPeakingBiquad(double sampleRate, double frequency, double q, double gainDb, BiquadCoefficients&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `sampleRate` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `frequency` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `q` | `double` | — | Positive finite filter quality factor. |
| `gainDb` | `double` | — | Logarithmic level in the indicated dB reference. |
| `argument 5` | `BiquadCoefficients &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `designShelfBiquad` {#symbol-designshelfbiquad}

```cpp
Status designShelfBiquad(bool highShelf, double sampleRate, double frequency, double slope, double gainDb, BiquadCoefficients&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `highShelf` | `bool` | — | Explicit true/false option or predicate. |
| `sampleRate` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `frequency` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `slope` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `gainDb` | `double` | — | Logarithmic level in the indicated dB reference. |
| `argument 6` | `BiquadCoefficients &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `PeakingBiquadDesigner` {#symbol-peakingbiquaddesigner}

```cpp
class PeakingBiquadDesigner;
```

### `PeakingBiquadDesigner::prepare` {#symbol-peakingbiquaddesigner-prepare}

```cpp
Status prepare(double sampleRate, double frequencyHz, double q) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `sampleRate` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `frequencyHz` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `q` | `double` | — | Positive finite filter quality factor. |

Return: `Status` — Check Status; ok reports success.

### `PeakingBiquadDesigner::design` {#symbol-peakingbiquaddesigner-design}

```cpp
Status design(double gainDb, BiquadCoefficients&) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `gainDb` | `double` | — | Logarithmic level in the indicated dB reference. |
| `argument 2` | `BiquadCoefficients &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

### `PeakingBiquadDesigner::designFor` {#symbol-peakingbiquaddesigner-designfor}

```cpp
template <Sample T> [[nodiscard]] Status designFor(double gainDb, BiquadCoefficients& output) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `gainDb` | `double` | — | Logarithmic level in the indicated dB reference. |
| `output` | `BiquadCoefficients &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `Biquad` {#symbol-biquad}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class Biquad;
```

### `Biquad::Biquad<T>` {#symbol-biquad-biquad-t}

```cpp
Biquad() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `Biquad::Biquad<T>` {#symbol-biquad-biquad-t-2}

```cpp
Biquad(const Biquad&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const Biquad<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `Biquad::operator=` {#symbol-biquad-operator}

```cpp
Biquad& operator=(const Biquad&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const Biquad<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Biquad<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Biquad::Biquad<T>` {#symbol-biquad-biquad-t-3}

```cpp
Biquad(Biquad&& other) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `Biquad<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `Biquad::operator=` {#symbol-biquad-operator-2}

```cpp
Biquad& operator=(Biquad&& other) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `Biquad<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Biquad<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Biquad::prepare` {#symbol-biquad-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |

Return: `Status` — Check Status; ok reports success.

### `Biquad::reset` {#symbol-biquad-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `Biquad::setCoefficients` {#symbol-biquad-setcoefficients}

```cpp
Status setCoefficients(const BiquadCoefficients&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const BiquadCoefficients &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `Biquad::process` {#symbol-biquad-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.
