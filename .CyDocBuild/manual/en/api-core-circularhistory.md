---
id: api-core-circularhistory
title: CircularHistory
description: One prepared signal history shared by independent integer and fractional readers.
section: API reference
order: 1113
revision: 1
parent: api-core
symbol: cydsp::CircularHistory
---

One prepared signal history shared by independent integer and fractional readers.

## Include {#include}

```cpp
#include <cydsp/core/CircularHistory.h>
```

[Usage and contracts](doc:delay) · [Common processing rules](doc:contracts)

## `HistoryInterpolation` {#symbol-historyinterpolation}

```cpp
enum class HistoryInterpolation { nearest, linear, lagrange3, catmullRom };
```

## `CircularHistory` {#symbol-circularhistory}

```cpp
template <Sample T> class CircularHistory;
```

### `CircularHistory::CircularHistory<T>` {#symbol-circularhistory-circularhistory-t}

```cpp
CircularHistory() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `CircularHistory::CircularHistory<T>` {#symbol-circularhistory-circularhistory-t-2}

```cpp
CircularHistory(const CircularHistory&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const CircularHistory<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `CircularHistory::operator=` {#symbol-circularhistory-operator}

```cpp
CircularHistory& operator=(const CircularHistory&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const CircularHistory<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `CircularHistory<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `CircularHistory::CircularHistory<T>` {#symbol-circularhistory-circularhistory-t-3}

```cpp
CircularHistory(CircularHistory&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `CircularHistory<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `CircularHistory::operator=` {#symbol-circularhistory-operator-2}

```cpp
CircularHistory& operator=(CircularHistory&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `CircularHistory<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `CircularHistory<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `CircularHistory::prepare` {#symbol-circularhistory-prepare}

```cpp
Status prepare(std::size_t channels, std::size_t historyFrames) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `channels` | `std::size_t` | — | Channel index/count in the declared planar layout. |
| `historyFrames` | `std::size_t` | — | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `CircularHistory::pushFrame` {#symbol-circularhistory-pushframe}

```cpp
Status pushFrame(std::span<const T>) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `CircularHistory::push` {#symbol-circularhistory-push}

```cpp
Status push(AudioBlock<const T>) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |

Return: `Status` — Check Status; ok reports success.

### `CircularHistory::read` {#symbol-circularhistory-read}

```cpp
T read(std::size_t channel, double ageFrames, HistoryInterpolation = HistoryInterpolation::linear) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Channel index/count in the declared planar layout. |
| `ageFrames` | `double` | — | Frame count or offset; one frame contains one sample per channel. |
| `argument 3` | `HistoryInterpolation` | `HistoryInterpolation::linear` | Value/configuration in the declared type; apply the linked module contract. |

Return: `T` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `CircularHistory::readWindow` {#symbol-circularhistory-readwindow}

```cpp
Status readWindow(std::size_t channel, std::ptrdiff_t firstAge, std::span<double> output) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Channel index/count in the declared planar layout. |
| `firstAge` | `std::ptrdiff_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `output` | `std::span<double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `CircularHistory::reset` {#symbol-circularhistory-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `CircularHistory::channels` {#symbol-circularhistory-channels}

```cpp
std::size_t channels() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `CircularHistory::capacity` {#symbol-circularhistory-capacity}

```cpp
std::size_t capacity() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `CircularHistory::availableFrames` {#symbol-circularhistory-availableframes}

```cpp
std::size_t availableFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
