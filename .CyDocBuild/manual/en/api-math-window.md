---
id: api-math-window
title: Window
description: Generate float/double windows with explicit symmetry and normalization.
section: API reference
order: 1122
revision: 1
parent: api-math
symbol: cydsp::WindowType
---

Generate float/double windows with explicit symmetry and normalization.

## Include {#include}

```cpp
#include <cydsp/math/Window.h>
```

[Usage and contracts](doc:fft) · [Common processing rules](doc:contracts)

## `WindowType` {#symbol-windowtype}

```cpp
enum class WindowType { rectangular, hann, hamming, blackman, blackmanHarris, nuttall, flatTop, bartlett, triangular, cosine, kaiser };
```

## `WindowSymmetry` {#symbol-windowsymmetry}

```cpp
enum class WindowSymmetry { symmetric, periodic };
```

## `WindowNormalization` {#symbol-windownormalization}

```cpp
enum class WindowNormalization { none, peak, coherentGain, energy };
```

## `WindowParameters` {#symbol-windowparameters}

```cpp
struct WindowParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `type` | `WindowType` | `WindowType::hann` | Value/configuration in the declared type; apply the linked module contract. |
| `symmetry` | `WindowSymmetry` | `WindowSymmetry::symmetric` | Value/configuration in the declared type; apply the linked module contract. |
| `normalization` | `WindowNormalization` | `WindowNormalization::none` | Value/configuration in the declared type; apply the linked module contract. |
| `beta` | `double` | `8.6` | Value/configuration in the declared type; apply the linked module contract. |

## `makeWindow` {#symbol-makewindow}

```cpp
Status makeWindow(const WindowParameters&, std::span<float>) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const WindowParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `std::span<float>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

## `makeWindow` {#symbol-makewindow-2}

```cpp
Status makeWindow(const WindowParameters&, std::span<double>) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const WindowParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `std::span<double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

## `makeWindow` {#symbol-makewindow-3}

```cpp
Status makeWindow(WindowType, std::span<float>, WindowSymmetry = WindowSymmetry::symmetric) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `WindowType` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `std::span<float>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 3` | `WindowSymmetry` | `WindowSymmetry::symmetric` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

## `makeWindow` {#symbol-makewindow-4}

```cpp
Status makeWindow(WindowType, std::span<double>, WindowSymmetry = WindowSymmetry::symmetric) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `WindowType` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `std::span<double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 3` | `WindowSymmetry` | `WindowSymmetry::symmetric` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.
