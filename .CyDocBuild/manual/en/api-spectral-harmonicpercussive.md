---
id: api-spectral-harmonicpercussive
title: HarmonicPercussive
description: Causal time/frequency median separation with complementary harmonic/percussive outputs.
section: API reference
order: 1178
revision: 1
parent: api-spectral
symbol: cydsp::HarmonicPercussive
---

Causal time/frequency median separation with complementary harmonic/percussive outputs.

## Include {#include}

```cpp
#include <cydsp/spectral/HarmonicPercussive.h>
```

[Usage and contracts](doc:spectral) · [Common processing rules](doc:contracts)

## `HpssConfig` {#symbol-hpssconfig}

```cpp
struct HpssConfig;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `stft` | `StftConfig` | `{1024, 256, WindowType::hann}` | Value/configuration in the declared type; apply the linked module contract. |
| `temporalFrames` | `std::size_t` | `17` | Frame count or offset; one frame contains one sample per channel. |
| `frequencyBins` | `std::size_t` | `17` | Value/configuration in the declared type; apply the linked module contract. |

## `HarmonicPercussive` {#symbol-harmonicpercussive}

```cpp
template <Sample T> class HarmonicPercussive;
```

### `HarmonicPercussive::HarmonicPercussive<T>` {#symbol-harmonicpercussive-harmonicpercussive-t}

```cpp
HarmonicPercussive();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `HarmonicPercussive::~HarmonicPercussive<T>` {#symbol-harmonicpercussive-harmonicpercussive-t-2}

```cpp
~HarmonicPercussive();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `HarmonicPercussive::HarmonicPercussive<T>` {#symbol-harmonicpercussive-harmonicpercussive-t-3}

```cpp
HarmonicPercussive(HarmonicPercussive&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `HarmonicPercussive<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `HarmonicPercussive::operator=` {#symbol-harmonicpercussive-operator}

```cpp
HarmonicPercussive& operator=(HarmonicPercussive&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `HarmonicPercussive<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `HarmonicPercussive<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `HarmonicPercussive::prepare` {#symbol-harmonicpercussive-prepare}

```cpp
Status prepare(const ProcessSpec&, const HpssConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `const HpssConfig &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `HarmonicPercussive::process` {#symbol-harmonicpercussive-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T> harmonic, AudioBlock<T> percussive) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `harmonic` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |
| `percussive` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `HarmonicPercussive::reset` {#symbol-harmonicpercussive-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `HarmonicPercussive::latencyFrames` {#symbol-harmonicpercussive-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
