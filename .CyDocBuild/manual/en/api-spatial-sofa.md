---
id: api-spatial-sofa
title: Sofa
description: Optional memory-only SOFA import, float64 preparation and portable HRTF cache.
section: API reference
order: 1174
revision: 1
parent: api-spatial
symbol: cydsp::SofaBackend
---

Optional memory-only SOFA import, float64 preparation and portable HRTF cache.

## Include {#include}

```cpp
#include <cydsp/spatial/Sofa.h>
```

[Usage and contracts](doc:sofa) · [Common processing rules](doc:contracts)

## `SofaBackend` {#symbol-sofabackend}

```cpp
enum class SofaBackend { automatic, compact, complete };
```

## `SofaError` {#symbol-sofaerror}

```cpp
enum class SofaError { none, invalid_file, unsupported_convention, unsupported_storage, invalid_dimensions, invalid_coordinates, invalid_sampling_rate, invalid_delay, invalid_samples, limit_exceeded, precision_loss, backend_unavailable, allocation_failed };
```

## `SofaLoadReport` {#symbol-sofaloadreport}

```cpp
struct SofaLoadReport;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `backend` | `SofaBackend` | `SofaBackend::automatic` | Value/configuration in the declared type; apply the linked module contract. |
| `error` | `SofaError` | `SofaError::none` | Value/configuration in the declared type; apply the linked module contract. |
| `sourcePrecisionBits` | `unsigned int` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `precisionReduced` | `bool` | `false` | Explicit true/false option or predicate. |
| `receiversSwapped` | `bool` | `false` | Explicit true/false option or predicate. |

## `SofaInflate` {#symbol-sofainflate}

```cpp
using SofaInflate = Status (*)(std::span<const std::byte> input, std::span<std::byte> output, void* context) noexcept;
```

## `SofaLoadOptions` {#symbol-sofaloadoptions}

```cpp
struct SofaLoadOptions;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `backend` | `SofaBackend` | `SofaBackend::automatic` | Value/configuration in the declared type; apply the linked module contract. |
| `maximumFileBytes` | `std::size_t` | `128u * 1024u * 1024u` | Value/configuration in the declared type; apply the linked module contract. |
| `maximumDecodedBytes` | `std::size_t` | `512u * 1024u * 1024u` | Value/configuration in the declared type; apply the linked module contract. |
| `maximumPoints` | `std::size_t` | `100000` | Value/configuration in the declared type; apply the linked module contract. |
| `maximumTaps` | `std::size_t` | `32768` | Value/configuration in the declared type; apply the linked module contract. |
| `maximumMetadataBytes` | `std::size_t` | `65536` | Value/configuration in the declared type; apply the linked module contract. |
| `allowGeneralFir` | `bool` | `false` | Explicit true/false option or predicate. |
| `allowPrecisionLoss` | `bool` | `false` | Explicit true/false option or predicate. |
| `inflate` | `SofaInflate` | `nullptr` | Value/configuration in the declared type; apply the linked module contract. |
| `inflateContext` | `void *` | `nullptr` | Borrowed callback context; the application owns lifetime and synchronization. |

## `BasicSofaHrtf` {#symbol-basicsofahrtf}

```cpp
template <Sample T> struct BasicSofaHrtf;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `sampleRate` | `double` | `0` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `directions` | `std::vector<Vec3>` | — | Value/configuration in the declared type; apply the linked module contract. |
| `distancesMetres` | `std::vector<double>` | — | Length in metres. |
| `left` | `std::vector<T>` | — | Value/configuration in the declared type; apply the linked module contract. |
| `right` | `std::vector<T>` | — | Value/configuration in the declared type; apply the linked module contract. |
| `leftDelay` | `std::vector<double>` | — | Value/configuration in the declared type; apply the linked module contract. |
| `rightDelay` | `std::vector<double>` | — | Value/configuration in the declared type; apply the linked module contract. |
| `points` | `std::size_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `taps` | `std::size_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `convention` | `std::string` | — | Value/configuration in the declared type; apply the linked module contract. |
| `title` | `std::string` | — | Value/configuration in the declared type; apply the linked module contract. |
| `database` | `std::string` | — | Value/configuration in the declared type; apply the linked module contract. |
| `listener` | `std::string` | — | Value/configuration in the declared type; apply the linked module contract. |
| `license` | `std::string` | — | Value/configuration in the declared type; apply the linked module contract. |

## `SofaHrtf` {#symbol-sofahrtf}

```cpp
using SofaHrtf = BasicSofaHrtf<float>;
```

## `SofaHrtf64` {#symbol-sofahrtf64}

```cpp
using SofaHrtf64 = BasicSofaHrtf<double>;
```

## `loadSofa` {#symbol-loadsofa}

```cpp
Status loadSofa(std::span<const std::byte> file, SofaHrtf& out, const SofaLoadOptions& options = {}, SofaLoadReport* report = nullptr) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `file` | `std::span<const std::byte>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `out` | `SofaHrtf &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |
| `options` | `const SofaLoadOptions &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `report` | `SofaLoadReport *` | `nullptr` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

## `loadSofa` {#symbol-loadsofa-2}

```cpp
Status loadSofa(std::span<const std::byte> file, SofaHrtf64& out, const SofaLoadOptions& options = {}, SofaLoadReport* report = nullptr) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `file` | `std::span<const std::byte>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `out` | `SofaHrtf64 &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |
| `options` | `const SofaLoadOptions &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `report` | `SofaLoadReport *` | `nullptr` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

## `sofaBackendAvailable` {#symbol-sofabackendavailable}

```cpp
bool sofaBackendAvailable(SofaBackend backend) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `backend` | `SofaBackend` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `HrtfLevelNormalization` {#symbol-hrtflevelnormalization}

```cpp
enum class HrtfLevelNormalization { none, frontalEnergy };
```

## `SofaPreparationReport` {#symbol-sofapreparationreport}

```cpp
struct SofaPreparationReport;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `selectedPoints` | `std::size_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `inputTaps` | `std::size_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `preparedTaps` | `std::size_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `distanceMetres` | `double` | `0` | Length in metres. |
| `appliedGain` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `truncated` | `bool` | `false` | Explicit true/false option or predicate. |
| `diffuseFieldEqualized` | `bool` | `false` | Explicit true/false option or predicate. |
| `uniformDiffuseWeights` | `bool` | `false` | Explicit true/false option or predicate. |

## `SofaPreparation` {#symbol-sofapreparation}

```cpp
struct SofaPreparation;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `hrtf` | `HrtfPreparation` | — | Value/configuration in the declared type; apply the linked module contract. |
| `distanceMetres` | `double` | `1` | Length in metres. |
| `distanceToleranceMetres` | `double` | `.001` | Length in metres. |
| `diffuseFieldEqualization` | `bool` | `false` | Explicit true/false option or predicate. |
| `diffuseFieldWeights` | `std::span<const double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `equalizationTaps` | `std::size_t` | `129` | Value/configuration in the declared type; apply the linked module contract. |
| `maximumEqualizationDb` | `double` | `12` | Logarithmic level in the indicated dB reference. |
| `normalization` | `HrtfLevelNormalization` | `HrtfLevelNormalization::none` | Value/configuration in the declared type; apply the linked module contract. |

## `prepareSofaHrtf` {#symbol-preparesofahrtf}

```cpp
Status prepareSofaHrtf(const SofaHrtf& asset, HrtfSet<float>& out, const SofaPreparation& options = {}, SofaPreparationReport* report = nullptr) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `asset` | `const SofaHrtf &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `out` | `HrtfSet<float> &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |
| `options` | `const SofaPreparation &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `report` | `SofaPreparationReport *` | `nullptr` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

## `prepareSofaHrtf` {#symbol-preparesofahrtf-2}

```cpp
Status prepareSofaHrtf(const SofaHrtf64& asset, HrtfSet<double>& out, const SofaPreparation& options = {}, SofaPreparationReport* report = nullptr) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `asset` | `const SofaHrtf64 &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `out` | `HrtfSet<double> &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |
| `options` | `const SofaPreparation &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `report` | `SofaPreparationReport *` | `nullptr` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

## `encodeHrtfAsset` {#symbol-encodehrtfasset}

```cpp
Status encodeHrtfAsset(const SofaHrtf& asset, std::vector<std::byte>& out) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `asset` | `const SofaHrtf &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `out` | `std::vector<std::byte> &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `encodeHrtfAsset` {#symbol-encodehrtfasset-2}

```cpp
Status encodeHrtfAsset(const SofaHrtf64& asset, std::vector<std::byte>& out) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `asset` | `const SofaHrtf64 &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `out` | `std::vector<std::byte> &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `decodeHrtfAsset` {#symbol-decodehrtfasset}

```cpp
Status decodeHrtfAsset(std::span<const std::byte> file, SofaHrtf& out, const SofaLoadOptions& options = {}) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `file` | `std::span<const std::byte>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `out` | `SofaHrtf &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |
| `options` | `const SofaLoadOptions &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

## `decodeHrtfAsset` {#symbol-decodehrtfasset-2}

```cpp
Status decodeHrtfAsset(std::span<const std::byte> file, SofaHrtf64& out, const SofaLoadOptions& options = {}) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `file` | `std::span<const std::byte>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `out` | `SofaHrtf64 &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |
| `options` | `const SofaLoadOptions &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.
