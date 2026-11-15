---
id: api-processors-delayline
title: DelayLine
description: Prepared fractional delay with controlled delay changes.
section: API reference
order: 1136
revision: 1
parent: api-processors
symbol: cydsp::DelayLine
---

Prepared fractional delay with controlled delay changes.

## Include {#include}

```cpp
#include <cydsp/processors/DelayLine.h>
```

[Usage and contracts](doc:delay) · [Common processing rules](doc:contracts)

## `DelayLine` {#symbol-delayline}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class DelayLine;
```

### `DelayLine::DelayLine<T>` {#symbol-delayline-delayline-t}

```cpp
DelayLine() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `DelayLine::DelayLine<T>` {#symbol-delayline-delayline-t-2}

```cpp
DelayLine(const DelayLine&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const DelayLine<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `DelayLine::operator=` {#symbol-delayline-operator}

```cpp
DelayLine& operator=(const DelayLine&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const DelayLine<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `DelayLine<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `DelayLine::DelayLine<T>` {#symbol-delayline-delayline-t-3}

```cpp
DelayLine(DelayLine&& other) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `DelayLine<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `DelayLine::operator=` {#symbol-delayline-operator-2}

```cpp
DelayLine& operator=(DelayLine&& other) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `DelayLine<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `DelayLine<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `DelayLine::prepare` {#symbol-delayline-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t maximumDelayFrames) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `maximumDelayFrames` | `std::size_t` | — | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `DelayLine::setDelay` {#symbol-delayline-setdelay}

```cpp
Status setDelay(double frames, std::size_t rampFrames = 0) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `frames` | `double` | — | Frame count or offset; one frame contains one sample per channel. |
| `rampFrames` | `std::size_t` | `0` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `DelayLine::reset` {#symbol-delayline-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `DelayLine::process` {#symbol-delayline-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `DelayLine::targetDelayFrames` {#symbol-delayline-targetdelayframes}

```cpp
double targetDelayFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `DelayLine::delayFrames` {#symbol-delayline-delayframes}

```cpp
double delayFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.
