---
id: api-analysis-diagnostics
title: Diagnostics
description: Frequency/phase response, transfer/coherence, distortion and measurement sweeps.
section: API reference
order: 1101
revision: 1
parent: api-analysis
symbol: cydsp::FilterResponse
---

Frequency/phase response, transfer/coherence, distortion and measurement sweeps.

## Include {#include}

```cpp
#include <cydsp/analysis/Diagnostics.h>
```

[Usage and contracts](doc:diagnostics) · [Common processing rules](doc:contracts)

## `FilterResponse` {#symbol-filterresponse}

```cpp
struct FilterResponse;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `gain` | `std::complex<double>` | `{1, 0}` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `phaseRadians` | `double` | `0` | Angle in radians. |
| `groupDelayFrames` | `double` | `0` | Frame count or offset; one frame contains one sample per channel. |

## `firResponse` {#symbol-firresponse}

```cpp
Status firResponse(std::span<const double>, double sampleRate, double frequencyHz, FilterResponse&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `sampleRate` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `frequencyHz` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `argument 4` | `FilterResponse &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `iirResponse` {#symbol-iirresponse}

```cpp
Status iirResponse(std::span<const BiquadCoefficients>, double sampleRate, double frequencyHz, FilterResponse&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const BiquadCoefficients>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `sampleRate` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `frequencyHz` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `argument 4` | `FilterResponse &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `TransferBin` {#symbol-transferbin}

```cpp
struct TransferBin;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `gain` | `std::complex<double>` | `{}` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `phaseRadians` | `double` | `0` | Angle in radians. |
| `groupDelayFrames` | `double` | `0` | Frame count or offset; one frame contains one sample per channel. |
| `coherence` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |

## `TransferAnalyzer` {#symbol-transferanalyzer}

```cpp
template <Sample T> class TransferAnalyzer;
```

### `TransferAnalyzer::TransferAnalyzer<T>` {#symbol-transferanalyzer-transferanalyzer-t}

```cpp
TransferAnalyzer();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `TransferAnalyzer::~TransferAnalyzer<T>` {#symbol-transferanalyzer-transferanalyzer-t-2}

```cpp
~TransferAnalyzer();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `TransferAnalyzer::TransferAnalyzer<T>` {#symbol-transferanalyzer-transferanalyzer-t-3}

```cpp
TransferAnalyzer(TransferAnalyzer&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `TransferAnalyzer<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `TransferAnalyzer::operator=` {#symbol-transferanalyzer-operator}

```cpp
TransferAnalyzer& operator=(TransferAnalyzer&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `TransferAnalyzer<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `TransferAnalyzer<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `TransferAnalyzer::prepare` {#symbol-transferanalyzer-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t fftFrames = 2048, std::size_t hopFrames = 512, std::size_t averages = 32, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `fftFrames` | `std::size_t` | `2048` | Frame count or offset; one frame contains one sample per channel. |
| `hopFrames` | `std::size_t` | `512` | Frame count or offset; one frame contains one sample per channel. |
| `averages` | `std::size_t` | `32` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `TransferAnalyzer::process` {#symbol-transferanalyzer-process}

```cpp
Status process(AudioBlock<const T> reference, AudioBlock<const T> measured) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `reference` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `measured` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |

Return: `Status` — Check Status; ok reports success.

### `TransferAnalyzer::bin` {#symbol-transferanalyzer-bin}

```cpp
TransferBin bin(std::size_t channel, std::size_t index) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Channel index/count in the declared planar layout. |
| `index` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `TransferBin` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `TransferAnalyzer::bins` {#symbol-transferanalyzer-bins}

```cpp
std::size_t bins() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `TransferAnalyzer::analyzedFrames` {#symbol-transferanalyzer-analyzedframes}

```cpp
std::uint64_t analyzedFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::uint64_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `TransferAnalyzer::reset` {#symbol-transferanalyzer-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

## `DistortionReport` {#symbol-distortionreport}

```cpp
struct DistortionReport;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `fundamentalRms` | `double` | `0` | Time in milliseconds. |
| `harmonicsRms` | `double` | `0` | Time in milliseconds. |
| `noiseRms` | `double` | `0` | Time in milliseconds. |
| `dc` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `thd` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `thdPlusNoise` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `measuredHarmonics` | `unsigned int` | `0` | Value/configuration in the declared type; apply the linked module contract. |

## `measureDistortion` {#symbol-measuredistortion}

```cpp
template <Sample T> [[nodiscard]] Status measureDistortion(std::span<const T>, double sampleRate, double fundamentalHz, DistortionReport&, unsigned harmonics = 10) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `sampleRate` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `fundamentalHz` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `argument 4` | `DistortionReport &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |
| `harmonics` | `unsigned int` | `10` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

## `MeasurementSignal` {#symbol-measurementsignal}

```cpp
enum class MeasurementSignal { sine, whiteNoise, impulse, linearSweep, logSweep };
```

## `MeasurementSignalParameters` {#symbol-measurementsignalparameters}

```cpp
struct MeasurementSignalParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `kind` | `MeasurementSignal` | `MeasurementSignal::logSweep` | Value/configuration in the declared type; apply the linked module contract. |
| `startHz` | `double` | `20` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `endHz` | `double` | `20000` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `durationSeconds` | `double` | `5` | Duration or position in seconds. |
| `amplitude` | `double` | `.5` | Value/configuration in the declared type; apply the linked module contract. |
| `fadeSeconds` | `double` | `.01` | Duration or position in seconds. |

## `MeasurementGenerator` {#symbol-measurementgenerator}

```cpp
template <Sample T> class MeasurementGenerator;
```

### `MeasurementGenerator::prepare` {#symbol-measurementgenerator-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |

Return: `Status` — Check Status; ok reports success.

### `MeasurementGenerator::setParameters` {#symbol-measurementgenerator-setparameters}

```cpp
Status setParameters(const MeasurementSignalParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const MeasurementSignalParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `MeasurementGenerator::process` {#symbol-measurementgenerator-process}

```cpp
Status process(AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `MeasurementGenerator::reset` {#symbol-measurementgenerator-reset}

```cpp
void reset(std::uint64_t seed = 1) noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `seed` | `std::uint64_t` | `1` | Per-instance deterministic random seed. |

## `inverseLogSweep` {#symbol-inverselogsweep}

```cpp
template <Sample T> [[nodiscard]] Status inverseLogSweep(std::span<const T> sweep, double startHz, double endHz, std::span<T> inverse) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `sweep` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `startHz` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `endHz` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `inverse` | `std::span<T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

## `deconvolveSweep` {#symbol-deconvolvesweep}

```cpp
template <Sample T> [[nodiscard]] Status deconvolveSweep(std::span<const T> recording, std::span<const T> excitation, std::span<T> impulse, double regularizationDb = -120, CpuBackend = CpuBackend::automatic) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `recording` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `excitation` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `impulse` | `std::span<T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `regularizationDb` | `double` | `-120` | Logarithmic level in the indicated dB reference. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.
