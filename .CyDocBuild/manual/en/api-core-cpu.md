---
id: api-core-cpu
title: Cpu
description: CPU/OS capability detection, compiled backends and explicit backend names.
section: API reference
order: 1114
revision: 1
parent: api-core
symbol: cydsp::Architecture
---

CPU/OS capability detection, compiled backends and explicit backend names.

## Include {#include}

```cpp
#include <cydsp/core/Cpu.h>
```

[Usage and contracts](doc:simd) · [Common processing rules](doc:contracts)

## `Architecture` {#symbol-architecture}

```cpp
enum class Architecture { unknown, x86, x64, arm32, arm64 };
```

## `CpuBackend` {#symbol-cpubackend}

```cpp
enum class CpuBackend { automatic, scalar, sse2, avx2, neon, avx512, sve };
```

## `CpuFeatures` {#symbol-cpufeatures}

```cpp
struct CpuFeatures;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `architecture` | `Architecture` | `Architecture::unknown` | Value/configuration in the declared type; apply the linked module contract. |
| `sse2` | `bool` | `false` | Explicit true/false option or predicate. |
| `avx2` | `bool` | `false` | Explicit true/false option or predicate. |
| `neon` | `bool` | `false` | Explicit true/false option or predicate. |
| `avx512` | `bool` | `false` | Explicit true/false option or predicate. |
| `sve` | `bool` | `false` | Explicit true/false option or predicate. |

## `detectCpuFeatures` {#symbol-detectcpufeatures}

```cpp
CpuFeatures detectCpuFeatures() noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuFeatures` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `backendName` {#symbol-backendname}

```cpp
const char* backendName(CpuBackend) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `CpuBackend` | — | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `const char *` — Returned value/view; respect borrowed lifetime and the units named by this API.
