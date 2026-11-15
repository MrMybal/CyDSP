---
id: api-synthesis-physicalmodels
title: PhysicalModels
description: Karplus–Strong/waveguide strings and tubes, modal banks and membrane/plate modes.
section: API reference
order: 1184
revision: 1
parent: api-synthesis
symbol: cydsp::ResonantMode
---

Karplus–Strong/waveguide strings and tubes, modal banks and membrane/plate modes.

## Include {#include}

```cpp
#include <cydsp/synthesis/PhysicalModels.h>
```

[Usage and contracts](doc:physical) · [Common processing rules](doc:contracts)

## `ResonantMode` {#symbol-resonantmode}

```cpp
struct ResonantMode;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `frequencyHz` | `double` | `440` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `decaySeconds` | `double` | `2` | Duration or position in seconds. |
| `gain` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `pan` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |

## `ModalSurface` {#symbol-modalsurface}

```cpp
enum class ModalSurface { rectangularMembrane, rectangularPlate };
```

## `ModalResonator` {#symbol-modalresonator}

```cpp
template <Sample T> class ModalResonator;
```

### `ModalResonator::ModalResonator<T>` {#symbol-modalresonator-modalresonator-t}

```cpp
ModalResonator();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `ModalResonator::~ModalResonator<T>` {#symbol-modalresonator-modalresonator-t-2}

```cpp
~ModalResonator();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `ModalResonator::ModalResonator<T>` {#symbol-modalresonator-modalresonator-t-3}

```cpp
ModalResonator(ModalResonator&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `ModalResonator<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `ModalResonator::operator=` {#symbol-modalresonator-operator}

```cpp
ModalResonator& operator=(ModalResonator&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `ModalResonator<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `ModalResonator<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ModalResonator::prepare` {#symbol-modalresonator-prepare}

```cpp
Status prepare(const ProcessSpec& mono, std::size_t outputChannels = 2, std::size_t maximumModes = 128) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `mono` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `outputChannels` | `std::size_t` | `2` | Value/configuration in the declared type; apply the linked module contract. |
| `maximumModes` | `std::size_t` | `128` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `ModalResonator::setModes` {#symbol-modalresonator-setmodes}

```cpp
Status setModes(std::span<const ResonantMode>) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const ResonantMode>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `ModalResonator::setSurface` {#symbol-modalresonator-setsurface}

```cpp
Status setSurface(ModalSurface, double fundamentalHz, double aspect = 1.3, double decaySeconds = 2, double strikeX = .37, double strikeY = .43) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `ModalSurface` | — | Value/configuration in the declared type; apply the linked module contract. |
| `fundamentalHz` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `aspect` | `double` | `1.3` | Value/configuration in the declared type; apply the linked module contract. |
| `decaySeconds` | `double` | `2` | Duration or position in seconds. |
| `strikeX` | `double` | `.37` | Value/configuration in the declared type; apply the linked module contract. |
| `strikeY` | `double` | `.43` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `ModalResonator::process` {#symbol-modalresonator-process}

```cpp
Status process(AudioBlock<const T> excitation, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `excitation` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `ModalResonator::reset` {#symbol-modalresonator-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

## `WaveguideModel` {#symbol-waveguidemodel}

```cpp
enum class WaveguideModel { karplusStrong, string, tube };
```

## `WaveguideParameters` {#symbol-waveguideparameters}

```cpp
struct WaveguideParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `model` | `WaveguideModel` | `WaveguideModel::karplusStrong` | Value/configuration in the declared type; apply the linked module contract. |
| `frequencyHz` | `double` | `220` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `decaySeconds` | `double` | `2` | Duration or position in seconds. |
| `brightness` | `double` | `.5` | Value/configuration in the declared type; apply the linked module contract. |
| `dispersion` | `double` | `.2` | Value/configuration in the declared type; apply the linked module contract. |
| `closedTube` | `bool` | `false` | Explicit true/false option or predicate. |

## `Waveguide` {#symbol-waveguide}

```cpp
template <Sample T> class Waveguide;
```

### `Waveguide::Waveguide<T>` {#symbol-waveguide-waveguide-t}

```cpp
Waveguide();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `Waveguide::~Waveguide<T>` {#symbol-waveguide-waveguide-t-2}

```cpp
~Waveguide();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `Waveguide::Waveguide<T>` {#symbol-waveguide-waveguide-t-3}

```cpp
Waveguide(Waveguide&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Waveguide<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `Waveguide::operator=` {#symbol-waveguide-operator}

```cpp
Waveguide& operator=(Waveguide&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Waveguide<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Waveguide<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `Waveguide::prepare` {#symbol-waveguide-prepare}

```cpp
Status prepare(const ProcessSpec&, double minimumFrequencyHz = 20) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `minimumFrequencyHz` | `double` | `20` | Frequency in Hz; use the supported sample-rate/Nyquist range. |

Return: `Status` — Check Status; ok reports success.

### `Waveguide::setParameters` {#symbol-waveguide-setparameters}

```cpp
Status setParameters(const WaveguideParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const WaveguideParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `Waveguide::pluck` {#symbol-waveguide-pluck}

```cpp
void pluck(double gain = 1, std::uint64_t seed = 1) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `gain` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `seed` | `std::uint64_t` | `1` | Per-instance deterministic random seed. |

### `Waveguide::process` {#symbol-waveguide-process}

```cpp
Status process(AudioBlock<const T> excitation, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `excitation` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `Waveguide::render` {#symbol-waveguide-render}

```cpp
Status render(AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `Waveguide::reset` {#symbol-waveguide-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.
