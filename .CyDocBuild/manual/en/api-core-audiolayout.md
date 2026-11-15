---
id: api-core-audiolayout
title: AudioLayout
description: Convert between planar blocks and frame-major interleaved spans without allocation.
section: API reference
order: 1112
revision: 1
parent: api-core
symbol: cydsp::interleave
---

Convert between planar blocks and frame-major interleaved spans without allocation.

## Include {#include}

```cpp
#include <cydsp/core/AudioLayout.h>
```

[Usage and contracts](doc:buffers) · [Common processing rules](doc:contracts)

## `interleave` {#symbol-interleave}

```cpp
Status interleave(AudioBlock<const float>, std::span<float>) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const float>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `std::span<float>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

## `interleave` {#symbol-interleave-2}

```cpp
Status interleave(AudioBlock<const double>, std::span<double>) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const double>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `std::span<double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

## `deinterleave` {#symbol-deinterleave}

```cpp
Status deinterleave(std::span<const float>, AudioBlock<float>) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const float>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `AudioBlock<float>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

## `deinterleave` {#symbol-deinterleave-2}

```cpp
Status deinterleave(std::span<const double>, AudioBlock<double>) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `AudioBlock<double>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.
