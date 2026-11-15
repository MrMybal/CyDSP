---
id: api-analysis-spectrumanalyzer
title: SpectrumAnalyzer
description: Real-FFT amplitude and PSD spectra, spectral features and Mel/logarithmic bands.
section: API reference
order: 1106
revision: 1
parent: api-analysis
symbol: cydsp::SpectrumAnalyzer
---

Real-FFT amplitude and PSD spectra, spectral features and Mel/logarithmic bands.

## Include {#include}

```cpp
#include <cydsp/analysis/SpectrumAnalyzer.h>
```

[Usage and contracts](doc:analysis) · [Common processing rules](doc:contracts)

## `SpectrumConfig` {#symbol-spectrumconfig}

```cpp
struct SpectrumConfig;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `fftFrames` | `std::size_t` | `2048` | Frame count or offset; one frame contains one sample per channel. |
| `hopFrames` | `std::size_t` | `512` | Frame count or offset; one frame contains one sample per channel. |
| `window` | `WindowType` | `WindowType::hann` | Value/configuration in the declared type; apply the linked module contract. |

## `SpectralFeatures` {#symbol-spectralfeatures}

```cpp
struct SpectralFeatures;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `rms` | `double` | `0` | Time in milliseconds. |
| `centroidHz` | `double` | `0` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `rolloffHz` | `double` | `0` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `flatness` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `positiveFlux` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `dominantHz` | `double` | `0` | Frequency in Hz; use the supported sample-rate/Nyquist range. |

## `SpectrumAnalyzer` {#symbol-spectrumanalyzer}

```cpp
template <Sample T> class SpectrumAnalyzer;
```

### `SpectrumAnalyzer::SpectrumAnalyzer<T>` {#symbol-spectrumanalyzer-spectrumanalyzer-t}

```cpp
SpectrumAnalyzer();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `SpectrumAnalyzer::~SpectrumAnalyzer<T>` {#symbol-spectrumanalyzer-spectrumanalyzer-t-2}

```cpp
~SpectrumAnalyzer();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `SpectrumAnalyzer::SpectrumAnalyzer<T>` {#symbol-spectrumanalyzer-spectrumanalyzer-t-3}

```cpp
SpectrumAnalyzer(SpectrumAnalyzer&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `SpectrumAnalyzer<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `SpectrumAnalyzer::operator=` {#symbol-spectrumanalyzer-operator}

```cpp
SpectrumAnalyzer& operator=(SpectrumAnalyzer&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `SpectrumAnalyzer<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `SpectrumAnalyzer<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SpectrumAnalyzer::prepare` {#symbol-spectrumanalyzer-prepare}

```cpp
Status prepare(const ProcessSpec&, const SpectrumConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `const SpectrumConfig &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `SpectrumAnalyzer::process` {#symbol-spectrumanalyzer-process}

```cpp
Status process(AudioBlock<const T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |

Return: `Status` — Check Status; ok reports success.

### `SpectrumAnalyzer::reset` {#symbol-spectrumanalyzer-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `SpectrumAnalyzer::hasFrame` {#symbol-spectrumanalyzer-hasframe}

```cpp
bool hasFrame() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SpectrumAnalyzer::frameCount` {#symbol-spectrumanalyzer-framecount}

```cpp
std::uint64_t frameCount() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::uint64_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SpectrumAnalyzer::bins` {#symbol-spectrumanalyzer-bins}

```cpp
std::size_t bins() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SpectrumAnalyzer::binFrequency` {#symbol-spectrumanalyzer-binfrequency}

```cpp
double binFrequency(std::size_t bin) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `bin` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SpectrumAnalyzer::magnitudes` {#symbol-spectrumanalyzer-magnitudes}

```cpp
std::span<const double> magnitudes(std::size_t channel) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Channel index/count in the declared planar layout. |

Return: `std::span<const double>` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SpectrumAnalyzer::powerDensity` {#symbol-spectrumanalyzer-powerdensity}

```cpp
std::span<const double> powerDensity(std::size_t channel) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Channel index/count in the declared planar layout. |

Return: `std::span<const double>` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SpectrumAnalyzer::complexSpectrum` {#symbol-spectrumanalyzer-complexspectrum}

```cpp
std::span<const std::complex<double>> complexSpectrum(std::size_t channel) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Channel index/count in the declared planar layout. |

Return: `std::span<const std::complex<double>>` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SpectrumAnalyzer::features` {#symbol-spectrumanalyzer-features}

```cpp
SpectralFeatures features(std::size_t channel) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Channel index/count in the declared planar layout. |

Return: `SpectralFeatures` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SpectrumAnalyzer::backend` {#symbol-spectrumanalyzer-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `SpectralBandScale` {#symbol-spectralbandscale}

```cpp
enum class SpectralBandScale { mel, logarithmic };
```

## `SpectralBandConfig` {#symbol-spectralbandconfig}

```cpp
struct SpectralBandConfig;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `bands` | `std::size_t` | `40` | Value/configuration in the declared type; apply the linked module contract. |
| `minimumHz` | `double` | `20` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `maximumHz` | `double` | `0` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `scale` | `SpectralBandScale` | `SpectralBandScale::mel` | Value/configuration in the declared type; apply the linked module contract. |

## `SpectralBands` {#symbol-spectralbands}

```cpp
class SpectralBands;
```

### `SpectralBands::SpectralBands` {#symbol-spectralbands-spectralbands}

```cpp
SpectralBands();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `SpectralBands::~SpectralBands` {#symbol-spectralbands-spectralbands-2}

```cpp
~SpectralBands();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `SpectralBands::SpectralBands` {#symbol-spectralbands-spectralbands-3}

```cpp
SpectralBands(SpectralBands&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `SpectralBands &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `SpectralBands::operator=` {#symbol-spectralbands-operator}

```cpp
SpectralBands& operator=(SpectralBands&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `SpectralBands &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `SpectralBands &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SpectralBands::prepare` {#symbol-spectralbands-prepare}

```cpp
Status prepare(double sampleRate, std::size_t fftFrames, const SpectralBandConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `sampleRate` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `fftFrames` | `std::size_t` | — | Frame count or offset; one frame contains one sample per channel. |
| `argument 3` | `const SpectralBandConfig &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `SpectralBands::process` {#symbol-spectralbands-process}

```cpp
Status process(std::span<const double> bins, std::span<double> bands) const noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `bins` | `std::span<const double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `bands` | `std::span<double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `SpectralBands::centerFrequencies` {#symbol-spectralbands-centerfrequencies}

```cpp
std::span<const double> centerFrequencies() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::span<const double>` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SpectralBands::backend` {#symbol-spectralbands-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
