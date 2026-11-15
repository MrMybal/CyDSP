---
id: api-math-dspmath
title: DspMath
description: Decibel conversion, polynomial evaluation, lookup tables and smoothed control values.
section: API reference
order: 1120
revision: 1
parent: api-math
symbol: cydsp::decibelsToGain
---

Decibel conversion, polynomial evaluation, lookup tables and smoothed control values.

## Include {#include}

```cpp
#include <cydsp/math/DspMath.h>
```

[Usage and contracts](doc:parameters) · [Common processing rules](doc:contracts)

## `decibelsToGain` {#symbol-decibelstogain}

```cpp
template <Sample T> [[nodiscard]] T decibelsToGain(T db, T floorDb = T{-160}) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `db` | `T` | — | Logarithmic level in the indicated dB reference. |
| `floorDb` | `T` | `T{-160}` | Logarithmic level in the indicated dB reference. |

Return: `T` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `gainToDecibels` {#symbol-gaintodecibels}

```cpp
template <Sample T> [[nodiscard]] T gainToDecibels(T gain, T floorDb = T{-160}) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `gain` | `T` | — | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `floorDb` | `T` | `T{-160}` | Logarithmic level in the indicated dB reference. |

Return: `T` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `evaluatePolynomial` {#symbol-evaluatepolynomial}

```cpp
template <Sample T> [[nodiscard]] T evaluatePolynomial(std::span<const T> coefficients, T x) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `coefficients` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `x` | `T` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `T` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `SmoothingCurve` {#symbol-smoothingcurve}

```cpp
enum class SmoothingCurve { linear, multiplicative };
```

## `SmoothedValue` {#symbol-smoothedvalue}

```cpp
class SmoothedValue;
```

### `SmoothedValue::reset` {#symbol-smoothedvalue-reset}

```cpp
Status reset(double initial, SmoothingCurve curve = SmoothingCurve::linear) noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `initial` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `curve` | `SmoothingCurve` | `SmoothingCurve::linear` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `SmoothedValue::setTarget` {#symbol-smoothedvalue-settarget}

```cpp
Status setTarget(double target, std::size_t frames) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `target` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `frames` | `std::size_t` | — | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `SmoothedValue::next` {#symbol-smoothedvalue-next}

```cpp
double next() noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedValue::current` {#symbol-smoothedvalue-current}

```cpp
double current() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedValue::target` {#symbol-smoothedvalue-target}

```cpp
double target() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SmoothedValue::smoothing` {#symbol-smoothedvalue-smoothing}

```cpp
bool smoothing() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `LookupTable` {#symbol-lookuptable}

```cpp
template <Sample T> class LookupTable;
```

### `LookupTable::Function` {#symbol-lookuptable-function}

```cpp
using Function = T (*)(T, void*) noexcept;
```

### `LookupTable::prepare` {#symbol-lookuptable-prepare}

```cpp
Status prepare(T minimum, T maximum, std::size_t points, Function function, void* context = nullptr) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `minimum` | `T` | — | Value/configuration in the declared type; apply the linked module contract. |
| `maximum` | `T` | — | Value/configuration in the declared type; apply the linked module contract. |
| `points` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `function` | `Function` | — | Value/configuration in the declared type; apply the linked module contract. |
| `context` | `void *` | `nullptr` | Borrowed callback context; the application owns lifetime and synchronization. |

Return: `Status` — Check Status; ok reports success.

### `LookupTable::lookup` {#symbol-lookuptable-lookup}

```cpp
T lookup(T x) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `x` | `T` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `T` — Returned value/view; respect borrowed lifetime and the units named by this API.
