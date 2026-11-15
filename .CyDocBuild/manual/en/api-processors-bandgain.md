---
id: api-processors-bandgain
title: BandGain
description: Prepared three-band propagation gains using the existing filter foundations.
section: API reference
order: 1129
revision: 1
parent: api-processors
symbol: cydsp::BandGain
---

Prepared three-band propagation gains using the existing filter foundations.

## Include {#include}

```cpp
#include <cydsp/processors/BandGain.h>
```

[Usage and contracts](doc:spatial) · [Common processing rules](doc:contracts)

## `BandGain` {#symbol-bandgain}

```cpp
template <Sample T> class BandGain;
```

### `BandGain::BandGain<T>` {#symbol-bandgain-bandgain-t}

```cpp
BandGain();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `BandGain::~BandGain<T>` {#symbol-bandgain-bandgain-t-2}

```cpp
~BandGain();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `BandGain::BandGain<T>` {#symbol-bandgain-bandgain-t-3}

```cpp
BandGain(BandGain&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `BandGain<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `BandGain::operator=` {#symbol-bandgain-operator}

```cpp
BandGain& operator=(BandGain&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `BandGain<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `BandGain<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `BandGain::prepare` {#symbol-bandgain-prepare}

```cpp
Status prepare(const ProcessSpec&, double lowCrossoverHz = 400, double highCrossoverHz = 4000) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `lowCrossoverHz` | `double` | `400` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `highCrossoverHz` | `double` | `4000` | Frequency in Hz; use the supported sample-rate/Nyquist range. |

Return: `Status` — Check Status; ok reports success.

### `BandGain::setGains` {#symbol-bandgain-setgains}

```cpp
Status setGains(const std::array<double, 3>&, std::size_t rampFrames = 0) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const std::array<double, 3> &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `rampFrames` | `std::size_t` | `0` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `BandGain::process` {#symbol-bandgain-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `BandGain::reset` {#symbol-bandgain-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.
