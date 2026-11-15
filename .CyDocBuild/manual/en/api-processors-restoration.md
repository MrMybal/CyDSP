---
id: api-processors-restoration
title: Restoration
description: Spectral noise attenuation, hum removal and bounded click detection/repair.
section: API reference
order: 1160
revision: 1
parent: api-processors
symbol: cydsp::DenoiseParameters
---

Spectral noise attenuation, hum removal and bounded click detection/repair.

## Include {#include}

```cpp
#include <cydsp/processors/Restoration.h>
```

[Usage and contracts](doc:restoration) · [Common processing rules](doc:contracts)

## `DenoiseParameters` {#symbol-denoiseparameters}

```cpp
struct DenoiseParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `reductionDb` | `double` | `18` | Logarithmic level in the indicated dB reference. |
| `strength` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `smoothingMs` | `double` | `30` | Time in milliseconds. |
| `resonanceThresholdDb` | `double` | `18` | Logarithmic level in the indicated dB reference. |
| `resonanceReductionDb` | `double` | `0` | Logarithmic level in the indicated dB reference. |
| `persistenceMs` | `double` | `500` | Time in milliseconds. |
| `learnNoise` | `bool` | `false` | Explicit true/false option or predicate. |

## `SpectralDenoiser` {#symbol-spectraldenoiser}

```cpp
template <Sample T> class SpectralDenoiser;
```

### `SpectralDenoiser::SpectralDenoiser<T>` {#symbol-spectraldenoiser-spectraldenoiser-t}

```cpp
SpectralDenoiser();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `SpectralDenoiser::~SpectralDenoiser<T>` {#symbol-spectraldenoiser-spectraldenoiser-t-2}

```cpp
~SpectralDenoiser();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `SpectralDenoiser::SpectralDenoiser<T>` {#symbol-spectraldenoiser-spectraldenoiser-t-3}

```cpp
SpectralDenoiser(SpectralDenoiser&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `SpectralDenoiser<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `SpectralDenoiser::operator=` {#symbol-spectraldenoiser-operator}

```cpp
SpectralDenoiser& operator=(SpectralDenoiser&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `SpectralDenoiser<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `SpectralDenoiser<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SpectralDenoiser::prepare` {#symbol-spectraldenoiser-prepare}

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

### `SpectralDenoiser::setParameters` {#symbol-spectraldenoiser-setparameters}

```cpp
Status setParameters(const DenoiseParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const DenoiseParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `SpectralDenoiser::setNoiseProfile` {#symbol-spectraldenoiser-setnoiseprofile}

```cpp
Status setNoiseProfile(std::span<const double> power) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `power` | `std::span<const double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `SpectralDenoiser::noiseProfile` {#symbol-spectraldenoiser-noiseprofile}

```cpp
std::span<const double> noiseProfile() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::span<const double>` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SpectralDenoiser::process` {#symbol-spectraldenoiser-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `SpectralDenoiser::reset` {#symbol-spectraldenoiser-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `SpectralDenoiser::latencyFrames` {#symbol-spectraldenoiser-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `DehumParameters` {#symbol-dehumparameters}

```cpp
struct DehumParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `fundamentalHz` | `double` | `50` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `q` | `double` | `30` | Positive finite filter quality factor. |
| `reductionDb` | `double` | `60` | Logarithmic level in the indicated dB reference. |
| `harmonics` | `std::size_t` | `8` | Value/configuration in the declared type; apply the linked module contract. |

## `Dehum` {#symbol-dehum}

```cpp
template <Sample T> class Dehum;
```

### `Dehum::Dehum<T>` {#symbol-dehum-dehum-t}

```cpp
Dehum();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `Dehum::~Dehum<T>` {#symbol-dehum-dehum-t-2}

```cpp
~Dehum();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `Dehum::Dehum<T>` {#symbol-dehum-dehum-t-3}

```cpp
Dehum(Dehum&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Dehum<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `Dehum::operator=` {#symbol-dehum-operator}

```cpp
Dehum& operator=(Dehum&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Dehum<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Dehum<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Dehum::prepare` {#symbol-dehum-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t maximumHarmonics = 32) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `maximumHarmonics` | `std::size_t` | `32` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `Dehum::setParameters` {#symbol-dehum-setparameters}

```cpp
Status setParameters(const DehumParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const DehumParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `Dehum::process` {#symbol-dehum-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `Dehum::reset` {#symbol-dehum-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

## `DeclickParameters` {#symbol-declickparameters}

```cpp
struct DeclickParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `absoluteThreshold` | `double` | `.02` | Value/configuration in the declared type; apply the linked module contract. |
| `deviationRatio` | `double` | `6` | Value/configuration in the declared type; apply the linked module contract. |

## `Declicker` {#symbol-declicker}

```cpp
template <Sample T> class Declicker;
```

### `Declicker::Declicker<T>` {#symbol-declicker-declicker-t}

```cpp
Declicker();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `Declicker::~Declicker<T>` {#symbol-declicker-declicker-t-2}

```cpp
~Declicker();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `Declicker::Declicker<T>` {#symbol-declicker-declicker-t-3}

```cpp
Declicker(Declicker&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Declicker<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `Declicker::operator=` {#symbol-declicker-operator}

```cpp
Declicker& operator=(Declicker&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Declicker<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Declicker<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Declicker::prepare` {#symbol-declicker-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t contextFrames = 8) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `contextFrames` | `std::size_t` | `8` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `Declicker::setParameters` {#symbol-declicker-setparameters}

```cpp
Status setParameters(const DeclickParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const DeclickParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `Declicker::process` {#symbol-declicker-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `Declicker::reset` {#symbol-declicker-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `Declicker::latencyFrames` {#symbol-declicker-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Declicker::repairedSamples` {#symbol-declicker-repairedsamples}

```cpp
std::uint64_t repairedSamples() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::uint64_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
