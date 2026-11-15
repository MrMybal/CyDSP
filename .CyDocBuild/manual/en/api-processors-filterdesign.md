---
id: api-processors-filterdesign
title: FilterDesign
description: Butterworth, Chebyshev I/II, elliptic and Bessel low/high-pass designs.
section: API reference
order: 1142
revision: 1
parent: api-processors
symbol: cydsp::IirFamily
---

Butterworth, Chebyshev I/II, elliptic and Bessel low/high-pass designs.

## Include {#include}

```cpp
#include <cydsp/processors/FilterDesign.h>
```

[Usage and contracts](doc:filter-design) · [Common processing rules](doc:contracts)

## `IirFamily` {#symbol-iirfamily}

```cpp
enum class IirFamily { butterworth, chebyshevI, chebyshevII, elliptic, bessel };
```

## `IirDesign` {#symbol-iirdesign}

```cpp
struct IirDesign;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `family` | `IirFamily` | `IirFamily::butterworth` | Value/configuration in the declared type; apply the linked module contract. |
| `response` | `BiquadType` | `BiquadType::lowPass` | Value/configuration in the declared type; apply the linked module contract. |
| `order` | `unsigned int` | `4` | Value/configuration in the declared type; apply the linked module contract. |
| `sampleRate` | `double` | `48000` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `cutoff` | `double` | `1000` | Value/configuration in the declared type; apply the linked module contract. |
| `rippleDb` | `double` | `1` | Logarithmic level in the indicated dB reference. |
| `stopBandDb` | `double` | `80` | Logarithmic level in the indicated dB reference. |
| `ellipticRolloff` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |

## `designIir` {#symbol-designiir}

```cpp
Status designIir(const IirDesign&, std::span<BiquadCoefficients>, std::size_t& count) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const IirDesign &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `std::span<BiquadCoefficients>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `count` | `std::size_t &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.
