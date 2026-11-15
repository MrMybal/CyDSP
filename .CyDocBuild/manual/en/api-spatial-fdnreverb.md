---
id: api-spatial-fdnreverb
title: FdnReverb
description: Orthogonal feedback-delay networks with band-dependent decay, diffusion and freeze.
section: API reference
order: 1172
revision: 1
parent: api-spatial
symbol: cydsp::FdnReverb
---

Orthogonal feedback-delay networks with band-dependent decay, diffusion and freeze.

## Include {#include}

```cpp
#include <cydsp/spatial/FdnReverb.h>
```

[Usage and contracts](doc:reverb) · [Common processing rules](doc:contracts)

## `FdnConfig` {#symbol-fdnconfig}

```cpp
struct FdnConfig;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `lines` | `std::size_t` | `16` | Value/configuration in the declared type; apply the linked module contract. |
| `roomScale` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `maximumPreDelayMs` | `double` | `250` | Time in milliseconds. |

## `FdnParameters` {#symbol-fdnparameters}

```cpp
struct FdnParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `decaySeconds` | `std::array<double, 3>` | `{2, 1.5, 1}` | Duration or position in seconds. |
| `preDelayMs` | `double` | `10` | Time in milliseconds. |
| `diffusion` | `double` | `.7` | Value/configuration in the declared type; apply the linked module contract. |
| `wet` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `dry` | `double` | `0` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `freeze` | `bool` | `false` | Explicit true/false option or predicate. |

## `FdnReverb` {#symbol-fdnreverb}

```cpp
template <Sample T> class FdnReverb;
```

### `FdnReverb::FdnReverb<T>` {#symbol-fdnreverb-fdnreverb-t}

```cpp
FdnReverb();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `FdnReverb::~FdnReverb<T>` {#symbol-fdnreverb-fdnreverb-t-2}

```cpp
~FdnReverb();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `FdnReverb::FdnReverb<T>` {#symbol-fdnreverb-fdnreverb-t-3}

```cpp
FdnReverb(FdnReverb&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `FdnReverb<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `FdnReverb::operator=` {#symbol-fdnreverb-operator}

```cpp
FdnReverb& operator=(FdnReverb&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `FdnReverb<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `FdnReverb<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `FdnReverb::prepare` {#symbol-fdnreverb-prepare}

```cpp
Status prepare(const ProcessSpec&, const FdnConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `const FdnConfig &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `FdnReverb::setParameters` {#symbol-fdnreverb-setparameters}

```cpp
Status setParameters(const FdnParameters&, std::size_t rampFrames = 256) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const FdnParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `rampFrames` | `std::size_t` | `256` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `FdnReverb::process` {#symbol-fdnreverb-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `FdnReverb::reset` {#symbol-fdnreverb-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `FdnReverb::nominalTailFrames` {#symbol-fdnreverb-nominaltailframes}

```cpp
std::size_t nominalTailFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `FdnReverb::backend` {#symbol-fdnreverb-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
