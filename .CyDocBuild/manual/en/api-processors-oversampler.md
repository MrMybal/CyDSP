---
id: api-processors-oversampler
title: Oversampler
description: Prepared FIR, half-band FIR or IIR up/down paths and their latency.
section: API reference
order: 1154
revision: 1
parent: api-processors
symbol: cydsp::Oversampler
---

Prepared FIR, half-band FIR or IIR up/down paths and their latency.

## Include {#include}

```cpp
#include <cydsp/processors/Oversampler.h>
```

[Usage and contracts](doc:resampling) · [Common processing rules](doc:contracts)

## `OversamplingMode` {#symbol-oversamplingmode}

```cpp
enum class OversamplingMode { fir, halfBandFir, iir };
```

## `Oversampler` {#symbol-oversampler}

```cpp
template <Sample T> class Oversampler;
```

### `Oversampler::Oversampler<T>` {#symbol-oversampler-oversampler-t}

```cpp
Oversampler();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `Oversampler::~Oversampler<T>` {#symbol-oversampler-oversampler-t-2}

```cpp
~Oversampler();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `Oversampler::Oversampler<T>` {#symbol-oversampler-oversampler-t-3}

```cpp
Oversampler(Oversampler&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Oversampler<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `Oversampler::operator=` {#symbol-oversampler-operator}

```cpp
Oversampler& operator=(Oversampler&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Oversampler<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Oversampler<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Oversampler::prepare` {#symbol-oversampler-prepare}

```cpp
Status prepare(const ProcessSpec&, unsigned factor = 2, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `factor` | `unsigned int` | `2` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `Oversampler::prepare` {#symbol-oversampler-prepare-2}

```cpp
Status prepare(const ProcessSpec&, unsigned factor, OversamplingMode, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `factor` | `unsigned int` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `OversamplingMode` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `Oversampler::linearPhase` {#symbol-oversampler-linearphase}

```cpp
bool linearPhase() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Oversampler::latencyAtDcFrames` {#symbol-oversampler-latencyatdcframes}

```cpp
double latencyAtDcFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Oversampler::processUp` {#symbol-oversampler-processup}

```cpp
Status processUp(AudioBlock<const T>, AudioBlock<T>& highRateBlock) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `highRateBlock` | `AudioBlock<T> &` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `Oversampler::processDown` {#symbol-oversampler-processdown}

```cpp
Status processDown(AudioBlock<T> output) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `output` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `Oversampler::reset` {#symbol-oversampler-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `Oversampler::factor` {#symbol-oversampler-factor}

```cpp
unsigned factor() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `unsigned int` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Oversampler::latencyFrames` {#symbol-oversampler-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
