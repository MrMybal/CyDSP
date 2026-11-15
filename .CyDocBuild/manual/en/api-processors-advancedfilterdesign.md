---
id: api-processors-advancedfilterdesign
title: AdvancedFilterDesign
description: High-order band designs, FIR equalization and minimum-phase FIR conversion.
section: API reference
order: 1126
revision: 1
parent: api-processors
symbol: cydsp::IirBandDesign
---

High-order band designs, FIR equalization and minimum-phase FIR conversion.

## Include {#include}

```cpp
#include <cydsp/processors/AdvancedFilterDesign.h>
```

[Usage and contracts](doc:filter-design) · [Common processing rules](doc:contracts)

## `IirBandDesign` {#symbol-iirbanddesign}

```cpp
struct IirBandDesign;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `prototype` | `IirDesign` | — | Value/configuration in the declared type; apply the linked module contract. |
| `lowHz` | `double` | `500` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `highHz` | `double` | `2000` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `bandStop` | `bool` | `false` | Explicit true/false option or predicate. |

## `designBandIir` {#symbol-designbandiir}

```cpp
Status designBandIir(const IirBandDesign&, std::span<BiquadCoefficients>, std::size_t& count) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const IirBandDesign &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `std::span<BiquadCoefficients>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `count` | `std::size_t &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `FirEqPoint` {#symbol-fireqpoint}

```cpp
struct FirEqPoint;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `frequencyHz` | `double` | `0` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `gainDb` | `double` | `0` | Logarithmic level in the indicated dB reference. |

## `FirPhase` {#symbol-firphase}

```cpp
enum class FirPhase { linear, minimum };
```

## `designFirEqualizer` {#symbol-designfirequalizer}

```cpp
Status designFirEqualizer(double sampleRate, std::span<const FirEqPoint>, std::span<double> taps, FirPhase = FirPhase::linear, CpuBackend = CpuBackend::automatic) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `sampleRate` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `argument 2` | `std::span<const FirEqPoint>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `taps` | `std::span<double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 4` | `FirPhase` | `FirPhase::linear` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

## `minimumPhaseFir` {#symbol-minimumphasefir}

```cpp
Status minimumPhaseFir(std::span<const double> input, std::span<double> output, double floorDb = -180, CpuBackend = CpuBackend::automatic) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `input` | `std::span<const double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `output` | `std::span<double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `floorDb` | `double` | `-180` | Logarithmic level in the indicated dB reference. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.
