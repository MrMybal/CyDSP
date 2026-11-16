---
id: api-synthesis-contactexciter
title: ContactExciter
description: Force/velocity/impulse contact excitation for impacts, sliding, rolling and bowing.
section: API reference
order: 1182
revision: 1
parent: api-synthesis
symbol: cydsp::ContactExciter
---

Force/velocity/impulse contact excitation for impacts, sliding, rolling and bowing.

## Include {#include}

```cpp
#include <cydsp/synthesis/ContactExciter.h>
```

[Usage and contracts](doc:physical) · [Common processing rules](doc:contracts)

## `ContactKind` {#symbol-contactkind}

```cpp
enum class ContactKind { impact, sliding, rolling, bowed };
```

## `ContactParameters` {#symbol-contactparameters}

```cpp
struct ContactParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `kind` | `ContactKind` | `ContactKind::sliding` | Value/configuration in the declared type; apply the linked module contract. |
| `normalForceNewtons` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `velocityMetresPerSecond` | `double` | `.1` | Velocity in metres per second. |
| `friction` | `double` | `.5` | Value/configuration in the declared type; apply the linked module contract. |
| `roughness` | `double` | `.2` | Value/configuration in the declared type; apply the linked module contract. |
| `roughnessSpacingMetres` | `double` | `.001` | Length in metres. |
| `massKg` | `double` | `.01` | Value/configuration in the declared type; apply the linked module contract. |
| `stiffnessNewtonsPerMetre` | `double` | `10000` | Value/configuration in the declared type; apply the linked module contract. |
| `gain` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |

## `ContactExciter` {#symbol-contactexciter}

```cpp
template <Sample T> class ContactExciter;
```

### `ContactExciter::ContactExciter<T>` {#symbol-contactexciter-contactexciter-t}

```cpp
ContactExciter();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `ContactExciter::~ContactExciter<T>` {#symbol-contactexciter-contactexciter-t-2}

```cpp
~ContactExciter();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `ContactExciter::ContactExciter<T>` {#symbol-contactexciter-contactexciter-t-3}

```cpp
ContactExciter(ContactExciter&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `ContactExciter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `ContactExciter::operator=` {#symbol-contactexciter-operator}

```cpp
ContactExciter& operator=(ContactExciter&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `ContactExciter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `ContactExciter<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ContactExciter::prepare` {#symbol-contactexciter-prepare}

```cpp
Status prepare(const ProcessSpec& mono) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `mono` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |

Return: `Status` — Check Status; ok reports success.

### `ContactExciter::setParameters` {#symbol-contactexciter-setparameters}

```cpp
Status setParameters(const ContactParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ContactParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `ContactExciter::strike` {#symbol-contactexciter-strike}

```cpp
Status strike(double impulseNewtonSeconds) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `impulseNewtonSeconds` | `double` | — | Duration or position in seconds. |

Return: `Status` — Check Status; ok reports success.

### `ContactExciter::process` {#symbol-contactexciter-process}

```cpp
Status process(AudioBlock<const T> surfaceVelocity, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `surfaceVelocity` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `ContactExciter::render` {#symbol-contactexciter-render}

```cpp
Status render(AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `ContactExciter::reset` {#symbol-contactexciter-reset}

```cpp
void reset(std::uint64_t seed = 0x4359445350ULL) noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `seed` | `std::uint64_t` | `0x4359445350ULL` | Per-instance deterministic random seed. |
