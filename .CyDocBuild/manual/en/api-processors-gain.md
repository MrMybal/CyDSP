---
id: api-processors-gain
title: Gain
description: Linear gain with a ramp advancing once per frame across all channels.
section: API reference
order: 1144
revision: 1
parent: api-processors
symbol: cydsp::Gain
---

Linear gain with a ramp advancing once per frame across all channels.

## Include {#include}

```cpp
#include <cydsp/processors/Gain.h>
```

[Usage and contracts](doc:mixing) · [Common processing rules](doc:contracts)

## `Gain` {#symbol-gain}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class Gain;
```

### `Gain::prepare` {#symbol-gain-prepare}

```cpp
Status prepare(const ProcessSpec&, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `Gain::reset` {#symbol-gain-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `Gain::setGain` {#symbol-gain-setgain}

```cpp
Status setGain(T linearGain, std::size_t rampFrames = 0) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `linearGain` | `T` | — | Value/configuration in the declared type; apply the linked module contract. |
| `rampFrames` | `std::size_t` | `0` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `Gain::process` {#symbol-gain-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `Gain::backend` {#symbol-gain-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Gain::currentGain` {#symbol-gain-currentgain}

```cpp
T currentGain() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `T` — Returned value/view; respect borrowed lifetime and the units named by this API.
