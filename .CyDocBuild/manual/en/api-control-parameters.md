---
id: api-control-parameters
title: Parameters
description: Stable parameter IDs, normalized mappings and sample-offset events rendered as control blocks.
section: API reference
order: 1108
revision: 1
parent: api-control
symbol: cydsp::ParameterUnit
---

Stable parameter IDs, normalized mappings and sample-offset events rendered as control blocks.

## Include {#include}

```cpp
#include <cydsp/control/Parameters.h>
```

[Usage and contracts](doc:parameters) · [Common processing rules](doc:contracts)

## `ParameterUnit` {#symbol-parameterunit}

```cpp
enum class ParameterUnit { none, hertz, seconds, milliseconds, decibels, linearGain, semitones, metres, degrees };
```

## `ParameterScale` {#symbol-parameterscale}

```cpp
enum class ParameterScale { linear, logarithmic };
```

## `ParameterDescriptor` {#symbol-parameterdescriptor}

```cpp
struct ParameterDescriptor;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `id` | `std::uint32_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `name` | `std::string_view` | — | Value/configuration in the declared type; apply the linked module contract. |
| `unit` | `ParameterUnit` | `ParameterUnit::none` | Value/configuration in the declared type; apply the linked module contract. |
| `minimum` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `maximum` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `initial` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `scale` | `ParameterScale` | `ParameterScale::linear` | Value/configuration in the declared type; apply the linked module contract. |
| `modulatable` | `bool` | `true` | Explicit true/false option or predicate. |

## `ParameterEvent` {#symbol-parameterevent}

```cpp
struct ParameterEvent;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `id` | `std::uint32_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `frameOffset` | `std::size_t` | `0` | Frame count or offset; one frame contains one sample per channel. |
| `value` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `rampFrames` | `std::size_t` | `0` | Frame count or offset; one frame contains one sample per channel. |

## `parameterToNormalized` {#symbol-parametertonormalized}

```cpp
double parameterToNormalized(const ParameterDescriptor&, double value) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ParameterDescriptor &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `value` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `parameterFromNormalized` {#symbol-parameterfromnormalized}

```cpp
double parameterFromNormalized(const ParameterDescriptor&, double normalized) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ParameterDescriptor &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `normalized` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `DspTiming` {#symbol-dsptiming}

```cpp
struct DspTiming;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `latencyFrames` | `std::size_t` | `0` | Frame count or offset; one frame contains one sample per channel. |
| `tailFrames` | `std::size_t` | `0` | Frame count or offset; one frame contains one sample per channel. |

## `ParameterBank` {#symbol-parameterbank}

```cpp
class ParameterBank;
```

### `ParameterBank::ParameterBank` {#symbol-parameterbank-parameterbank}

```cpp
ParameterBank();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `ParameterBank::~ParameterBank` {#symbol-parameterbank-parameterbank-2}

```cpp
~ParameterBank();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `ParameterBank::ParameterBank` {#symbol-parameterbank-parameterbank-3}

```cpp
ParameterBank(ParameterBank&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `ParameterBank &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `ParameterBank::operator=` {#symbol-parameterbank-operator}

```cpp
ParameterBank& operator=(ParameterBank&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `ParameterBank &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `ParameterBank &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ParameterBank::prepare` {#symbol-parameterbank-prepare}

```cpp
Status prepare(std::span<const ParameterDescriptor>, std::size_t maximumBlockFrames) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const ParameterDescriptor>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `maximumBlockFrames` | `std::size_t` | — | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `ParameterBank::render` {#symbol-parameterbank-render}

```cpp
Status render(std::span<const ParameterEvent>, AudioBlock<double>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const ParameterEvent>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `AudioBlock<double>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `ParameterBank::reset` {#symbol-parameterbank-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `ParameterBank::value` {#symbol-parameterbank-value}

```cpp
double value(std::uint32_t id) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `id` | `std::uint32_t` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ParameterBank::size` {#symbol-parameterbank-size}

```cpp
std::size_t size() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
