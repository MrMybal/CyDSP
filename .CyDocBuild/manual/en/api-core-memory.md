---
id: api-core-memory
title: Memory
description: Byte-range overlap detection for borrowed spans.
section: API reference
order: 1115
revision: 1
parent: api-core
symbol: cydsp::memory::overlaps
---

Byte-range overlap detection for borrowed spans.

## Include {#include}

```cpp
#include <cydsp/core/Memory.h>
```

[Usage and contracts](doc:buffers) · [Common processing rules](doc:contracts)

## `memory::overlaps` {#symbol-memory-overlaps}

```cpp
inline bool overlaps(const void* a, std::size_t bytesA, const void* b, std::size_t bytesB) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `a` | `const void *` | — | Borrowed callback context; the application owns lifetime and synchronization. |
| `bytesA` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `b` | `const void *` | — | Borrowed callback context; the application owns lifetime and synchronization. |
| `bytesB` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.
