---
id: api-spatial-hybridreverb
title: HybridReverb
description: Prepared convolution early response combined with an independently controlled FDN tail.
section: API reference
order: 1173
revision: 1
parent: api-spatial
symbol: cydsp::HybridReverb
---

Prepared convolution early response combined with an independently controlled FDN tail.

## Include {#include}

```cpp
#include <cydsp/spatial/HybridReverb.h>
```

[Usage and contracts](doc:reverb) · [Common processing rules](doc:contracts)

## `HybridReverb` {#symbol-hybridreverb}

```cpp
template <Sample T> class HybridReverb;
```

### `HybridReverb::HybridReverb<T>` {#symbol-hybridreverb-hybridreverb-t}

```cpp
HybridReverb();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `HybridReverb::~HybridReverb<T>` {#symbol-hybridreverb-hybridreverb-t-2}

```cpp
~HybridReverb();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `HybridReverb::HybridReverb<T>` {#symbol-hybridreverb-hybridreverb-t-3}

```cpp
HybridReverb(HybridReverb&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `HybridReverb<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `HybridReverb::operator=` {#symbol-hybridreverb-operator}

```cpp
HybridReverb& operator=(HybridReverb&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `HybridReverb<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `HybridReverb<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `HybridReverb::prepare` {#symbol-hybridreverb-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const T> impulse, const FdnConfig& = {}, const ConvolutionConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `impulse` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 3` | `const FdnConfig &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 4` | `const ConvolutionConfig &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `HybridReverb::setMix` {#symbol-hybridreverb-setmix}

```cpp
Status setMix(double impulseGain, double lateGain, std::size_t rampFrames = 256) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `impulseGain` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `lateGain` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `rampFrames` | `std::size_t` | `256` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `HybridReverb::setLateParameters` {#symbol-hybridreverb-setlateparameters}

```cpp
Status setLateParameters(const FdnParameters&, std::size_t rampFrames = 256) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const FdnParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `rampFrames` | `std::size_t` | `256` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `HybridReverb::stageImpulse` {#symbol-hybridreverb-stageimpulse}

```cpp
Status stageImpulse(std::span<const T>) noexcept;
```

Stage outside realtime processing; retained assets/contexts must satisfy the transition lifetime.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `HybridReverb::startImpulseTransition` {#symbol-hybridreverb-startimpulsetransition}

```cpp
Status startImpulseTransition(std::size_t frames = 256) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `frames` | `std::size_t` | `256` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `HybridReverb::process` {#symbol-hybridreverb-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `HybridReverb::reset` {#symbol-hybridreverb-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `HybridReverb::nominalTailFrames` {#symbol-hybridreverb-nominaltailframes}

```cpp
std::size_t nominalTailFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
