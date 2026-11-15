---
id: api-math-vectorops
title: VectorOps
description: Prepared scalar/SIMD copy, gain, mixing, dot products and complex arithmetic.
section: API reference
order: 1121
revision: 1
parent: api-math
symbol: cydsp::VectorOps
---

Prepared scalar/SIMD copy, gain, mixing, dot products and complex arithmetic.

## Include {#include}

```cpp
#include <cydsp/math/VectorOps.h>
```

[Usage and contracts](doc:simd) · [Common processing rules](doc:contracts)

## `VectorOps` {#symbol-vectorops}

```cpp
class VectorOps;
```

### `VectorOps::VectorOps` {#symbol-vectorops-vectorops}

```cpp
VectorOps() noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `VectorOps::dot` {#symbol-vectorops-dot}

```cpp
Status dot(std::span<const float>, std::span<const float>, float& result) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const float>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<const float>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `result` | `float &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

### `VectorOps::dot` {#symbol-vectorops-dot-2}

```cpp
Status dot(std::span<const double>, std::span<const double>, double& result) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<const double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `result` | `double &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

### `VectorOps::dot` {#symbol-vectorops-dot-3}

```cpp
Status dot(std::span<const float>, std::span<const double>, double& result) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const float>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<const double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `result` | `double &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

### `VectorOps::multiply` {#symbol-vectorops-multiply}

```cpp
Status multiply(std::span<const float>, std::span<const float>, std::span<float>) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const float>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<const float>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 3` | `std::span<float>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `VectorOps::multiply` {#symbol-vectorops-multiply-2}

```cpp
Status multiply(std::span<const double>, std::span<const double>, std::span<double>) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<const double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 3` | `std::span<double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `VectorOps::complexMultiplyAccumulate` {#symbol-vectorops-complexmultiplyaccumulate}

```cpp
Status complexMultiplyAccumulate(std::span<const std::complex<float>>, std::span<const std::complex<float>>, std::span<std::complex<float>>) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const std::complex<float>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<const std::complex<float>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 3` | `std::span<std::complex<float>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `VectorOps::complexMultiplyAccumulate` {#symbol-vectorops-complexmultiplyaccumulate-2}

```cpp
Status complexMultiplyAccumulate(std::span<const std::complex<double>>, std::span<const std::complex<double>>, std::span<std::complex<double>>) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const std::complex<double>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<const std::complex<double>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 3` | `std::span<std::complex<double>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `VectorOps::complexButterfly` {#symbol-vectorops-complexbutterfly}

```cpp
Status complexButterfly(std::span<std::complex<float>>, std::span<std::complex<float>>, std::span<const std::complex<float>>, bool conjugateTwiddles = false) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<std::complex<float>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<std::complex<float>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 3` | `std::span<const std::complex<float>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `conjugateTwiddles` | `bool` | `false` | Explicit true/false option or predicate. |

Return: `Status` — Check Status; ok reports success.

### `VectorOps::complexButterfly` {#symbol-vectorops-complexbutterfly-2}

```cpp
Status complexButterfly(std::span<std::complex<double>>, std::span<std::complex<double>>, std::span<const std::complex<double>>, bool conjugateTwiddles = false) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<std::complex<double>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<std::complex<double>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 3` | `std::span<const std::complex<double>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `conjugateTwiddles` | `bool` | `false` | Explicit true/false option or predicate. |

Return: `Status` — Check Status; ok reports success.

### `VectorOps::create` {#symbol-vectorops-create}

```cpp
static Status create(CpuBackend requested, VectorOps& result) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `requested` | `CpuBackend` | — | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |
| `result` | `VectorOps &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

### `VectorOps::backend` {#symbol-vectorops-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `VectorOps::copy` {#symbol-vectorops-copy}

```cpp
Status copy(std::span<const float>, std::span<float>) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const float>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<float>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `VectorOps::copy` {#symbol-vectorops-copy-2}

```cpp
Status copy(std::span<const double>, std::span<double>) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `VectorOps::scale` {#symbol-vectorops-scale}

```cpp
Status scale(std::span<const float>, std::span<float>, float) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const float>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<float>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 3` | `float` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `VectorOps::scale` {#symbol-vectorops-scale-2}

```cpp
Status scale(std::span<const double>, std::span<double>, double) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 3` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `VectorOps::addScaled` {#symbol-vectorops-addscaled}

```cpp
Status addScaled(std::span<const float>, std::span<float>, float) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const float>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<float>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 3` | `float` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `VectorOps::addScaled` {#symbol-vectorops-addscaled-2}

```cpp
Status addScaled(std::span<const double>, std::span<double>, double) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 3` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `VectorOps::fill` {#symbol-vectorops-fill}

```cpp
void fill(std::span<float>, float) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<float>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `float` | — | Value/configuration in the declared type; apply the linked module contract. |

### `VectorOps::fill` {#symbol-vectorops-fill-2}

```cpp
void fill(std::span<double>, double) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
