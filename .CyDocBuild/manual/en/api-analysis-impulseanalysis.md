---
id: api-analysis-impulseanalysis
title: ImpulseAnalysis
description: Onset, early reflections, decay/RT60 and nominal FDN matching from an in-memory IR.
section: API reference
order: 1102
revision: 1
parent: api-analysis
symbol: cydsp::DecayFit
---

Onset, early reflections, decay/RT60 and nominal FDN matching from an in-memory IR.

## Include {#include}

```cpp
#include <cydsp/analysis/ImpulseAnalysis.h>
```

[Usage and contracts](doc:diagnostics) · [Common processing rules](doc:contracts)

## `DecayFit` {#symbol-decayfit}

```cpp
struct DecayFit;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `seconds` | `double` | `0` | Duration or position in seconds. |
| `rSquared` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `valid` | `bool` | `false` | Explicit true/false option or predicate. |

## `ImpulseBandAnalysis` {#symbol-impulsebandanalysis}

```cpp
struct ImpulseBandAnalysis;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `centerHz` | `double` | `0` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `energy` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `edt` | `DecayFit` | — | Value/configuration in the declared type; apply the linked module contract. |
| `t20` | `DecayFit` | — | Value/configuration in the declared type; apply the linked module contract. |
| `t30` | `DecayFit` | — | Value/configuration in the declared type; apply the linked module contract. |

## `EarlyReflection` {#symbol-earlyreflection}

```cpp
struct EarlyReflection;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `seconds` | `double` | `0` | Duration or position in seconds. |
| `amplitude` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |

## `ImpulseAnalysisConfig` {#symbol-impulseanalysisconfig}

```cpp
struct ImpulseAnalysisConfig;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `onsetThresholdDb` | `double` | `-40` | Logarithmic level in the indicated dB reference. |
| `directWindowMs` | `double` | `2.5` | Time in milliseconds. |
| `reflectionThresholdDb` | `double` | `-20` | Logarithmic level in the indicated dB reference. |
| `subtractTailNoise` | `bool` | `false` | Explicit true/false option or predicate. |

## `ImpulseReport` {#symbol-impulsereport}

```cpp
struct ImpulseReport;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `silent` | `bool` | `false` | Explicit true/false option or predicate. |
| `noiseCompensated` | `bool` | `false` | Explicit true/false option or predicate. |
| `onsetSeconds` | `double` | `0` | Duration or position in seconds. |
| `peakSeconds` | `double` | `0` | Duration or position in seconds. |
| `energy` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `centerTimeSeconds` | `double` | `0` | Duration or position in seconds. |
| `clarity50Db` | `double` | `0` | Logarithmic level in the indicated dB reference. |
| `clarity80Db` | `double` | `0` | Logarithmic level in the indicated dB reference. |
| `definition50` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `directToReverberantDb` | `double` | `0` | Logarithmic level in the indicated dB reference. |
| `broadband` | `ImpulseBandAnalysis` | — | Value/configuration in the declared type; apply the linked module contract. |
| `octaveBands` | `std::array<ImpulseBandAnalysis, 9>` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `reflections` | `std::array<EarlyReflection, 32>` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `reflectionCount` | `std::size_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |

## `analyzeImpulse` {#symbol-analyzeimpulse}

```cpp
Status analyzeImpulse(std::span<const float>, double sampleRate, ImpulseReport&, const ImpulseAnalysisConfig& = {}) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const float>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `sampleRate` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `argument 3` | `ImpulseReport &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |
| `argument 4` | `const ImpulseAnalysisConfig &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

## `analyzeImpulse` {#symbol-analyzeimpulse-2}

```cpp
Status analyzeImpulse(std::span<const double>, double sampleRate, ImpulseReport&, const ImpulseAnalysisConfig& = {}) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `sampleRate` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `argument 3` | `ImpulseReport &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |
| `argument 4` | `const ImpulseAnalysisConfig &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

## `matchFdnToImpulse` {#symbol-matchfdntoimpulse}

```cpp
Status matchFdnToImpulse(const ImpulseReport&, FdnParameters&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ImpulseReport &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `FdnParameters &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.
