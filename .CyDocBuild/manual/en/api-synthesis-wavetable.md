---
id: api-synthesis-wavetable
title: Wavetable
description: Prepared harmonic-band tables and block-selected antialiasing oscillator banks.
section: API reference
order: 1189
revision: 1
parent: api-synthesis
symbol: cydsp::Wavetable
---

Prepared harmonic-band tables and block-selected antialiasing oscillator banks.

## Include {#include}

```cpp
#include <cydsp/synthesis/Wavetable.h>
```

[Usage and contracts](doc:synthesis) · [Common processing rules](doc:contracts)

## `Wavetable` {#symbol-wavetable}

```cpp
template <Sample T> class Wavetable;
```

### `Wavetable::Wavetable<T>` {#symbol-wavetable-wavetable-t}

```cpp
Wavetable();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `Wavetable::~Wavetable<T>` {#symbol-wavetable-wavetable-t-2}

```cpp
~Wavetable();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `Wavetable::Wavetable<T>` {#symbol-wavetable-wavetable-t-3}

```cpp
Wavetable(Wavetable&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Wavetable<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `Wavetable::operator=` {#symbol-wavetable-operator}

```cpp
Wavetable& operator=(Wavetable&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Wavetable<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Wavetable<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Wavetable::prepare` {#symbol-wavetable-prepare}

```cpp
Status prepare(std::span<const T> cycle, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `cycle` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `Wavetable::value` {#symbol-wavetable-value}

```cpp
T value(double phase, double frequency, double sampleRate) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `phase` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `frequency` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `sampleRate` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |

Return: `T` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Wavetable::size` {#symbol-wavetable-size}

```cpp
std::size_t size() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Wavetable::levels` {#symbol-wavetable-levels}

```cpp
std::size_t levels() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `WavetableOscillator` {#symbol-wavetableoscillator}

```cpp
template <Sample T> class WavetableOscillator;
```

### `WavetableOscillator::prepare` {#symbol-wavetableoscillator-prepare}

```cpp
Status prepare(const ProcessSpec&, const Wavetable<T>&) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `const Wavetable<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `WavetableOscillator::setFrequency` {#symbol-wavetableoscillator-setfrequency}

```cpp
Status setFrequency(double) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `WavetableOscillator::reset` {#symbol-wavetableoscillator-reset}

```cpp
void reset(double phase = 0) noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `phase` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |

### `WavetableOscillator::next` {#symbol-wavetableoscillator-next}

```cpp
T next() noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

Return: `T` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `WavetableOscillator::process` {#symbol-wavetableoscillator-process}

```cpp
Status process(AudioBlock<T>, bool add = false) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |
| `add` | `bool` | `false` | Explicit true/false option or predicate. |

Return: `Status` — Check Status; ok reports success.
