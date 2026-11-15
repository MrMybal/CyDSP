---
id: api-processors-generators
title: Generators
description: Oscillators and ADSR envelopes with sample and block processing.
section: API reference
order: 1145
revision: 1
parent: api-processors
symbol: cydsp::AdsrParameters
---

Oscillators and ADSR envelopes with sample and block processing.

## Include {#include}

```cpp
#include <cydsp/processors/Generators.h>
```

[Usage and contracts](doc:synthesis) · [Common processing rules](doc:contracts)

## `AdsrParameters` {#symbol-adsrparameters}

```cpp
struct AdsrParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `attackSeconds` | `double` | `0.01` | Duration or position in seconds. |
| `decaySeconds` | `double` | `0.1` | Duration or position in seconds. |
| `sustain` | `double` | `0.7` | Value/configuration in the declared type; apply the linked module contract. |
| `releaseSeconds` | `double` | `0.2` | Duration or position in seconds. |

## `Adsr` {#symbol-adsr}

```cpp
class Adsr;
```

### `Adsr::prepare` {#symbol-adsr-prepare}

```cpp
Status prepare(double sampleRate) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `sampleRate` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |

Return: `Status` — Check Status; ok reports success.

### `Adsr::setParameters` {#symbol-adsr-setparameters}

```cpp
Status setParameters(const AdsrParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const AdsrParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `Adsr::reset` {#symbol-adsr-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `Adsr::noteOn` {#symbol-adsr-noteon}

```cpp
void noteOn() noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

### `Adsr::noteOff` {#symbol-adsr-noteoff}

```cpp
void noteOff() noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

### `Adsr::next` {#symbol-adsr-next}

```cpp
double next() noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Adsr::process` {#symbol-adsr-process}

```cpp
void process(std::span<float> output) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `output` | `std::span<float>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

### `Adsr::process` {#symbol-adsr-process-2}

```cpp
void process(std::span<double> output) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `output` | `std::span<double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

### `Adsr::active` {#symbol-adsr-active}

```cpp
bool active() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Adsr::value` {#symbol-adsr-value}

```cpp
double value() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `Waveform` {#symbol-waveform}

```cpp
enum class Waveform { sine, triangle, saw, square, whiteNoise };
```

## `Oscillator` {#symbol-oscillator}

```cpp
template <Sample T> class Oscillator;
```

### `Oscillator::prepare` {#symbol-oscillator-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |

Return: `Status` — Check Status; ok reports success.

### `Oscillator::setFrequency` {#symbol-oscillator-setfrequency}

```cpp
Status setFrequency(double hz) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `hz` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |

Return: `Status` — Check Status; ok reports success.

### `Oscillator::setWaveform` {#symbol-oscillator-setwaveform}

```cpp
void setWaveform(Waveform value) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `value` | `Waveform` | — | Value/configuration in the declared type; apply the linked module contract. |

### `Oscillator::reset` {#symbol-oscillator-reset}

```cpp
void reset(double phase = 0) noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `phase` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |

### `Oscillator::next` {#symbol-oscillator-next}

```cpp
T next() noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

Return: `T` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Oscillator::process` {#symbol-oscillator-process}

```cpp
Status process(AudioBlock<T> output, bool add = false) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `output` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |
| `add` | `bool` | `false` | Explicit true/false option or predicate. |

Return: `Status` — Check Status; ok reports success.
