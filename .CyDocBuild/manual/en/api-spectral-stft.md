---
id: api-spectral-stft
title: Stft
description: Causal weighted overlap-add analysis/synthesis with an allocation-free spectral callback.
section: API reference
order: 1180
revision: 1
parent: api-spectral
symbol: cydsp::Stft
---

Causal weighted overlap-add analysis/synthesis with an allocation-free spectral callback.

## Include {#include}

```cpp
#include <cydsp/spectral/Stft.h>
```

[Usage and contracts](doc:spectral) · [Common processing rules](doc:contracts)

## `StftConfig` {#symbol-stftconfig}

```cpp
struct StftConfig;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `fftFrames` | `std::size_t` | `1024` | Frame count or offset; one frame contains one sample per channel. |
| `hopFrames` | `std::size_t` | `256` | Frame count or offset; one frame contains one sample per channel. |
| `window` | `WindowType` | `WindowType::hann` | Value/configuration in the declared type; apply the linked module contract. |

## `Stft` {#symbol-stft}

```cpp
template <Sample T> class Stft;
```

### `Stft::Processor` {#symbol-stft-processor}

```cpp
using Processor = void (*)(void*, std::size_t channel, std::span<std::complex<T>> bins) noexcept;
```

### `Stft::Stft<T>` {#symbol-stft-stft-t}

```cpp
Stft();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `Stft::~Stft<T>` {#symbol-stft-stft-t-2}

```cpp
~Stft();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `Stft::Stft<T>` {#symbol-stft-stft-t-3}

```cpp
Stft(Stft&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Stft<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `Stft::operator=` {#symbol-stft-operator}

```cpp
Stft& operator=(Stft&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Stft<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Stft<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Stft::prepare` {#symbol-stft-prepare}

```cpp
Status prepare(const ProcessSpec&, const StftConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `const StftConfig &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `Stft::setProcessor` {#symbol-stft-setprocessor}

```cpp
void setProcessor(Processor, void* context = nullptr) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Processor` | — | Value/configuration in the declared type; apply the linked module contract. |
| `context` | `void *` | `nullptr` | Borrowed callback context; the application owns lifetime and synchronization. |

### `Stft::process` {#symbol-stft-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `Stft::flush` {#symbol-stft-flush}

```cpp
Status flush(AudioBlock<T>) noexcept;
```

Drain the documented tail and inspect completion; reset before starting a new stream.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `Stft::reset` {#symbol-stft-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `Stft::latencyFrames` {#symbol-stft-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Stft::tailFrames` {#symbol-stft-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Stft::bins` {#symbol-stft-bins}

```cpp
std::size_t bins() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Stft::frameCount` {#symbol-stft-framecount}

```cpp
std::uint64_t frameCount() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::uint64_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Stft::backend` {#symbol-stft-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
