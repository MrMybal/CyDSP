---
id: api-processors-echocanceller
title: EchoCanceller
description: Independent microphone NLMS paths with aligned mono reference and linked double-talk gating.
section: API reference
order: 1139
revision: 1
parent: api-processors
symbol: cydsp::EchoCanceller
---

Independent microphone NLMS paths with aligned mono reference and linked double-talk gating.

## Include {#include}

```cpp
#include <cydsp/processors/EchoCanceller.h>
```

[Usage and contracts](doc:echo) · [Common processing rules](doc:contracts)

## `EchoCancellationConfig` {#symbol-echocancellationconfig}

```cpp
struct EchoCancellationConfig;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `taps` | `std::size_t` | `512` | Value/configuration in the declared type; apply the linked module contract. |
| `referenceDelayFrames` | `std::size_t` | `0` | Frame count or offset; one frame contains one sample per channel. |
| `initialLearningMs` | `double` | `250` | Time in milliseconds. |

## `EchoCancellationParameters` {#symbol-echocancellationparameters}

```cpp
struct EchoCancellationParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `step` | `double` | `.25` | Value/configuration in the declared type; apply the linked module contract. |
| `doubleTalkRatio` | `double` | `2.5` | Value/configuration in the declared type; apply the linked module contract. |
| `hangoverMs` | `double` | `100` | Time in milliseconds. |
| `minimumReferencePower` | `double` | `1e-8` | Value/configuration in the declared type; apply the linked module contract. |
| `adaptationEnabled` | `bool` | `true` | Explicit true/false option or predicate. |
| `forceAdaptation` | `bool` | `false` | Explicit true/false option or predicate. |

## `EchoCancellationMetrics` {#symbol-echocancellationmetrics}

```cpp
struct EchoCancellationMetrics;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `referenceActive` | `bool` | `false` | Explicit true/false option or predicate. |
| `doubleTalk` | `bool` | `false` | Explicit true/false option or predicate. |
| `adapting` | `bool` | `false` | Explicit true/false option or predicate. |
| `echoReturnLossEnhancementDb` | `double` | `0` | Logarithmic level in the indicated dB reference. |
| `residualPower` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `adaptedFrames` | `std::uint64_t` | `0` | Frame count or offset; one frame contains one sample per channel. |
| `frozenFrames` | `std::uint64_t` | `0` | Frame count or offset; one frame contains one sample per channel. |

## `EchoCanceller` {#symbol-echocanceller}

```cpp
template <Sample T> class EchoCanceller;
```

### `EchoCanceller::EchoCanceller<T>` {#symbol-echocanceller-echocanceller-t}

```cpp
EchoCanceller();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `EchoCanceller::~EchoCanceller<T>` {#symbol-echocanceller-echocanceller-t-2}

```cpp
~EchoCanceller();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `EchoCanceller::EchoCanceller<T>` {#symbol-echocanceller-echocanceller-t-3}

```cpp
EchoCanceller(EchoCanceller&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `EchoCanceller<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `EchoCanceller::operator=` {#symbol-echocanceller-operator}

```cpp
EchoCanceller& operator=(EchoCanceller&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `EchoCanceller<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `EchoCanceller<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `EchoCanceller::prepare` {#symbol-echocanceller-prepare}

```cpp
Status prepare(const ProcessSpec& microphones, const EchoCancellationConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `microphones` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `const EchoCancellationConfig &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `EchoCanceller::setParameters` {#symbol-echocanceller-setparameters}

```cpp
Status setParameters(const EchoCancellationParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const EchoCancellationParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `EchoCanceller::process` {#symbol-echocanceller-process}

```cpp
Status process(AudioBlock<const T> farEnd, AudioBlock<const T> microphones, AudioBlock<T> residual) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `farEnd` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `microphones` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `residual` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `EchoCanceller::metrics` {#symbol-echocanceller-metrics}

```cpp
EchoCancellationMetrics metrics() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `EchoCancellationMetrics` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `EchoCanceller::coefficients` {#symbol-echocanceller-coefficients}

```cpp
std::span<const double> coefficients(std::size_t channel) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Channel index/count in the declared planar layout. |

Return: `std::span<const double>` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `EchoCanceller::reset` {#symbol-echocanceller-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.
