---
id: api-processors-resampler
title: Resampler
description: Fixed/variable sinc sample-rate conversion with mandatory consumption/production counts.
section: API reference
order: 1159
revision: 1
parent: api-processors
symbol: cydsp::Resampler
---

Fixed/variable sinc sample-rate conversion with mandatory consumption/production counts.

## Include {#include}

```cpp
#include <cydsp/processors/Resampler.h>
```

[Usage and contracts](doc:resampling) · [Common processing rules](doc:contracts)

## `ResampleSpec` {#symbol-resamplespec}

```cpp
struct ResampleSpec;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `inputRate` | `std::uint32_t` | `48000` | Value/configuration in the declared type; apply the linked module contract. |
| `outputRate` | `std::uint32_t` | `44100` | Value/configuration in the declared type; apply the linked module contract. |
| `channels` | `std::size_t` | `2` | Channel index/count in the declared planar layout. |
| `maxInputFrames` | `std::size_t` | `512` | Frame count or offset; one frame contains one sample per channel. |
| `maxOutputFrames` | `std::size_t` | `1024` | Frame count or offset; one frame contains one sample per channel. |
| `taps` | `std::size_t` | `128` | Value/configuration in the declared type; apply the linked module contract. |
| `phases` | `std::size_t` | `1024` | Value/configuration in the declared type; apply the linked module contract. |
| `minimumRatio` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `maximumRatio` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |

### `ResampleSpec::isValid` {#symbol-resamplespec-isvalid}

```cpp
constexpr bool isValid() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `ResampleQuality` {#symbol-resamplequality}

```cpp
enum class ResampleQuality { economy, balanced, high };
```

## `resampleSpecForQuality` {#symbol-resamplespecforquality}

```cpp
constexpr ResampleSpec resampleSpecForQuality(ResampleSpec spec, ResampleQuality quality) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `spec` | `ResampleSpec` | — | Value/configuration in the declared type; apply the linked module contract. |
| `quality` | `ResampleQuality` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `ResampleSpec` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `ResampleResult` {#symbol-resampleresult}

```cpp
struct ResampleResult;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `status` | `Status` | `Status::ok` | Value/configuration in the declared type; apply the linked module contract. |
| `inputConsumed` | `std::size_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `outputProduced` | `std::size_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |

## `Resampler` {#symbol-resampler}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class Resampler;
```

### `Resampler::Resampler<T>` {#symbol-resampler-resampler-t}

```cpp
Resampler() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `Resampler::Resampler<T>` {#symbol-resampler-resampler-t-2}

```cpp
Resampler(const Resampler&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const Resampler<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `Resampler::operator=` {#symbol-resampler-operator}

```cpp
Resampler& operator=(const Resampler&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const Resampler<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Resampler<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Resampler::Resampler<T>` {#symbol-resampler-resampler-t-3}

```cpp
Resampler(Resampler&& other) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `Resampler<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `Resampler::operator=` {#symbol-resampler-operator-2}

```cpp
Resampler& operator=(Resampler&& other) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `Resampler<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Resampler<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Resampler::prepare` {#symbol-resampler-prepare}

```cpp
Status prepare(const ResampleSpec&, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ResampleSpec &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `Resampler::setRates` {#symbol-resampler-setrates}

```cpp
Status setRates(std::uint32_t inputRate, std::uint32_t outputRate) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `inputRate` | `std::uint32_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `outputRate` | `std::uint32_t` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `Resampler::setRatio` {#symbol-resampler-setratio}

```cpp
Status setRatio(double inputPerOutput, std::size_t outputRampFrames = 0) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `inputPerOutput` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `outputRampFrames` | `std::size_t` | `0` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `Resampler::ratio` {#symbol-resampler-ratio}

```cpp
double ratio() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Resampler::ratioTransitionRemainingFrames` {#symbol-resampler-ratiotransitionremainingframes}

```cpp
std::size_t ratioTransitionRemainingFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Resampler::reset` {#symbol-resampler-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `Resampler::finish` {#symbol-resampler-finish}

```cpp
ResampleResult finish(AudioBlock<T>) noexcept;
```

Drain the documented tail and inspect completion; reset before starting a new stream.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `ResampleResult` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Resampler::isFinished` {#symbol-resampler-isfinished}

```cpp
bool isFinished() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Resampler::process` {#symbol-resampler-process}

```cpp
ResampleResult process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `ResampleResult` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Resampler::backend` {#symbol-resampler-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Resampler::latencyInputFrames` {#symbol-resampler-latencyinputframes}

```cpp
double latencyInputFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.
