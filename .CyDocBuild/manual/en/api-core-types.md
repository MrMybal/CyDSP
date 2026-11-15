---
id: api-core-types
title: Types
description: Sample types, processing dimensions, status values and arithmetic policies.
section: API reference
order: 1117
revision: 1
parent: api-core
symbol: cydsp::Sample
---

Sample types, processing dimensions, status values and arithmetic policies.

## Include {#include}

```cpp
#include <cydsp/core/Types.h>
```

[Usage and contracts](doc:contracts) · [Common processing rules](doc:contracts)

## `Sample` {#symbol-sample}

```cpp
template <class T> concept Sample = std::same_as<std::remove_const_t<T>, float> || std::same_as<std::remove_const_t<T>, double>;
```

## `isFinite` {#symbol-isfinite}

```cpp
template <Sample T> constexpr bool isFinite(T value) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `value` | `T` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `MathAccuracy` {#symbol-mathaccuracy}

```cpp
enum class MathAccuracy { precise, fast };
```

## `Status` {#symbol-status}

```cpp
enum class Status { ok, invalid_spec, invalid_argument, not_prepared, channel_mismatch, block_too_large, unsupported_backend, partial_overlap, allocation_failed, transition_in_progress, end_of_stream, update_queue_full, io_error, unsupported_format, device_unavailable, would_block, timed_out, ambiguous, incompatible_version, disconnected };
```

## `ProcessSpec` {#symbol-processspec}

```cpp
struct ProcessSpec;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `sampleRate` | `double` | `48000.0` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `maxBlockFrames` | `std::size_t` | `512` | Frame count or offset; one frame contains one sample per channel. |
| `channels` | `std::size_t` | `2` | Channel index/count in the declared planar layout. |

### `ProcessSpec::isValid` {#symbol-processspec-isvalid}

```cpp
constexpr bool isValid() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `isPowerOfTwo` {#symbol-ispoweroftwo}

```cpp
constexpr bool isPowerOfTwo(std::size_t value) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `value` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.
