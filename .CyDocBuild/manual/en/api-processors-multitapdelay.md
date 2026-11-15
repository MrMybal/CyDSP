---
id: api-processors-multitapdelay
title: MultiTapDelay
description: Bounded input/output taps, crossfaded configurations and sequential Thiran fractional delay.
section: API reference
order: 1152
revision: 1
parent: api-processors
symbol: cydsp::MultiTapDelay
---

Bounded input/output taps, crossfaded configurations and sequential Thiran fractional delay.

## Include {#include}

```cpp
#include <cydsp/processors/MultiTapDelay.h>
```

[Usage and contracts](doc:delay) · [Common processing rules](doc:contracts)

## `DelayTap` {#symbol-delaytap}

```cpp
struct DelayTap;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `inputChannel` | `std::size_t` | `0` | Channel index/count in the declared planar layout. |
| `outputChannel` | `std::size_t` | `0` | Channel index/count in the declared planar layout. |
| `delayFrames` | `double` | `0` | Frame count or offset; one frame contains one sample per channel. |
| `gain` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `interpolation` | `HistoryInterpolation` | `HistoryInterpolation::linear` | Value/configuration in the declared type; apply the linked module contract. |

## `MultiTapDelay` {#symbol-multitapdelay}

```cpp
template <Sample T> class MultiTapDelay;
```

### `MultiTapDelay::MultiTapDelay<T>` {#symbol-multitapdelay-multitapdelay-t}

```cpp
MultiTapDelay();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `MultiTapDelay::~MultiTapDelay<T>` {#symbol-multitapdelay-multitapdelay-t-2}

```cpp
~MultiTapDelay();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `MultiTapDelay::MultiTapDelay<T>` {#symbol-multitapdelay-multitapdelay-t-3}

```cpp
MultiTapDelay(MultiTapDelay&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `MultiTapDelay<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `MultiTapDelay::operator=` {#symbol-multitapdelay-operator}

```cpp
MultiTapDelay& operator=(MultiTapDelay&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `MultiTapDelay<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `MultiTapDelay<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MultiTapDelay::prepare` {#symbol-multitapdelay-prepare}

```cpp
Status prepare(const ProcessSpec& inputSpec, std::size_t outputChannels, std::size_t maximumDelayFrames, std::size_t maximumTaps = 32, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `inputSpec` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `outputChannels` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `maximumDelayFrames` | `std::size_t` | — | Frame count or offset; one frame contains one sample per channel. |
| `maximumTaps` | `std::size_t` | `32` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `MultiTapDelay::setTaps` {#symbol-multitapdelay-settaps}

```cpp
Status setTaps(std::span<const DelayTap>, std::size_t transitionFrames = 0) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const DelayTap>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `transitionFrames` | `std::size_t` | `0` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `MultiTapDelay::process` {#symbol-multitapdelay-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `MultiTapDelay::reset` {#symbol-multitapdelay-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `MultiTapDelay::transitionRemainingFrames` {#symbol-multitapdelay-transitionremainingframes}

```cpp
std::size_t transitionRemainingFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MultiTapDelay::tailFrames` {#symbol-multitapdelay-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `ThiranDelay` {#symbol-thirandelay}

```cpp
template <Sample T> class ThiranDelay;
```

### `ThiranDelay::setDelay` {#symbol-thirandelay-setdelay}

```cpp
Status setDelay(double fractionalFrames) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `fractionalFrames` | `double` | — | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `ThiranDelay::processSample` {#symbol-thirandelay-processsample}

```cpp
T processSample(T input) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `input` | `T` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `T` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ThiranDelay::reset` {#symbol-thirandelay-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.
