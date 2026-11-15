---
id: api-processors-adaptivefilter
title: AdaptiveFilter
description: Prepared LMS/NLMS coefficient adaptation from program and desired signals.
section: API reference
order: 1123
revision: 1
parent: api-processors
symbol: cydsp::AdaptiveFilter
---

Prepared LMS/NLMS coefficient adaptation from program and desired signals.

## Include {#include}

```cpp
#include <cydsp/processors/AdaptiveFilter.h>
```

[Usage and contracts](doc:restoration) · [Common processing rules](doc:contracts)

## `AdaptiveAlgorithm` {#symbol-adaptivealgorithm}

```cpp
enum class AdaptiveAlgorithm { lms, nlms };
```

## `AdaptiveParameters` {#symbol-adaptiveparameters}

```cpp
struct AdaptiveParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `step` | `double` | `.1` | Value/configuration in the declared type; apply the linked module contract. |
| `leak` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `regularization` | `double` | `1e-6` | Value/configuration in the declared type; apply the linked module contract. |
| `coefficientLimit` | `double` | `100` | Value/configuration in the declared type; apply the linked module contract. |
| `adapt` | `bool` | `true` | Explicit true/false option or predicate. |

## `AdaptiveFilter` {#symbol-adaptivefilter}

```cpp
template <Sample T> class AdaptiveFilter;
```

### `AdaptiveFilter::AdaptiveFilter<T>` {#symbol-adaptivefilter-adaptivefilter-t}

```cpp
AdaptiveFilter();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `AdaptiveFilter::~AdaptiveFilter<T>` {#symbol-adaptivefilter-adaptivefilter-t-2}

```cpp
~AdaptiveFilter();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `AdaptiveFilter::AdaptiveFilter<T>` {#symbol-adaptivefilter-adaptivefilter-t-3}

```cpp
AdaptiveFilter(AdaptiveFilter&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AdaptiveFilter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `AdaptiveFilter::operator=` {#symbol-adaptivefilter-operator}

```cpp
AdaptiveFilter& operator=(AdaptiveFilter&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AdaptiveFilter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `AdaptiveFilter<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AdaptiveFilter::prepare` {#symbol-adaptivefilter-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t taps = 128, AdaptiveAlgorithm = AdaptiveAlgorithm::nlms, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `taps` | `std::size_t` | `128` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `AdaptiveAlgorithm` | `AdaptiveAlgorithm::nlms` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `AdaptiveFilter::setParameters` {#symbol-adaptivefilter-setparameters}

```cpp
Status setParameters(const AdaptiveParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const AdaptiveParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `AdaptiveFilter::process` {#symbol-adaptivefilter-process}

```cpp
Status process(AudioBlock<const T> reference, AudioBlock<const T> desired, AudioBlock<T> predicted, AudioBlock<T> residual) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `reference` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `desired` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `predicted` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |
| `residual` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `AdaptiveFilter::coefficients` {#symbol-adaptivefilter-coefficients}

```cpp
std::span<const double> coefficients(std::size_t channel) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Channel index/count in the declared planar layout. |

Return: `std::span<const double>` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AdaptiveFilter::clippedUpdates` {#symbol-adaptivefilter-clippedupdates}

```cpp
std::uint64_t clippedUpdates() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::uint64_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AdaptiveFilter::reset` {#symbol-adaptivefilter-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.
