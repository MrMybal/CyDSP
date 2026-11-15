---
id: api-processors-effects
title: Effects
description: Chorus, phaser and Schroeder-style reverb using the common prepared processing API.
section: API reference
order: 1140
revision: 1
parent: api-processors
symbol: cydsp::ChorusParameters
---

Chorus, phaser and Schroeder-style reverb using the common prepared processing API.

## Include {#include}

```cpp
#include <cydsp/processors/Effects.h>
```

[Usage and contracts](doc:effects) · [Common processing rules](doc:contracts)

## `ChorusParameters` {#symbol-chorusparameters}

```cpp
struct ChorusParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `rateHz` | `double` | `.5` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `depth` | `double` | `.5` | Value/configuration in the declared type; apply the linked module contract. |
| `centreDelayMs` | `double` | `7` | Time in milliseconds. |
| `feedback` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `wet` | `double` | `.5` | Linear amplitude multiplier; follow the processor bounds and mixing law. |

## `PhaserParameters` {#symbol-phaserparameters}

```cpp
struct PhaserParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `rateHz` | `double` | `.5` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `depth` | `double` | `.5` | Value/configuration in the declared type; apply the linked module contract. |
| `centreHz` | `double` | `1000` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `feedback` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `wet` | `double` | `.5` | Linear amplitude multiplier; follow the processor bounds and mixing law. |

## `ReverbParameters` {#symbol-reverbparameters}

```cpp
struct ReverbParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `decaySeconds` | `double` | `1.5` | Duration or position in seconds. |
| `damping` | `double` | `.3` | Value/configuration in the declared type; apply the linked module contract. |
| `wet` | `double` | `.25` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `width` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |

## `Chorus` {#symbol-chorus}

```cpp
template <Sample T> class Chorus;
```

### `Chorus::Chorus<T>` {#symbol-chorus-chorus-t}

```cpp
Chorus();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `Chorus::~Chorus<T>` {#symbol-chorus-chorus-t-2}

```cpp
~Chorus();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `Chorus::Chorus<T>` {#symbol-chorus-chorus-t-3}

```cpp
Chorus(Chorus&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Chorus<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `Chorus::operator=` {#symbol-chorus-operator}

```cpp
Chorus& operator=(Chorus&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Chorus<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Chorus<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Chorus::prepare` {#symbol-chorus-prepare}

```cpp
Status prepare(const ProcessSpec&, double maximumDelayMs = 100) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `maximumDelayMs` | `double` | `100` | Time in milliseconds. |

Return: `Status` — Check Status; ok reports success.

### `Chorus::setParameters` {#symbol-chorus-setparameters}

```cpp
Status setParameters(const ChorusParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ChorusParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `Chorus::process` {#symbol-chorus-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `Chorus::reset` {#symbol-chorus-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

## `Phaser` {#symbol-phaser}

```cpp
template <Sample T> class Phaser;
```

### `Phaser::Phaser<T>` {#symbol-phaser-phaser-t}

```cpp
Phaser();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `Phaser::~Phaser<T>` {#symbol-phaser-phaser-t-2}

```cpp
~Phaser();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `Phaser::Phaser<T>` {#symbol-phaser-phaser-t-3}

```cpp
Phaser(Phaser&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Phaser<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `Phaser::operator=` {#symbol-phaser-operator}

```cpp
Phaser& operator=(Phaser&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Phaser<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Phaser<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Phaser::prepare` {#symbol-phaser-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |

Return: `Status` — Check Status; ok reports success.

### `Phaser::setParameters` {#symbol-phaser-setparameters}

```cpp
Status setParameters(const PhaserParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const PhaserParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `Phaser::process` {#symbol-phaser-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `Phaser::reset` {#symbol-phaser-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

## `Reverb` {#symbol-reverb}

```cpp
template <Sample T> class Reverb;
```

### `Reverb::Reverb<T>` {#symbol-reverb-reverb-t}

```cpp
Reverb();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `Reverb::~Reverb<T>` {#symbol-reverb-reverb-t-2}

```cpp
~Reverb();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `Reverb::Reverb<T>` {#symbol-reverb-reverb-t-3}

```cpp
Reverb(Reverb&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Reverb<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `Reverb::operator=` {#symbol-reverb-operator}

```cpp
Reverb& operator=(Reverb&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Reverb<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Reverb<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Reverb::prepare` {#symbol-reverb-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |

Return: `Status` — Check Status; ok reports success.

### `Reverb::setParameters` {#symbol-reverb-setparameters}

```cpp
Status setParameters(const ReverbParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ReverbParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `Reverb::process` {#symbol-reverb-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `Reverb::reset` {#symbol-reverb-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.
