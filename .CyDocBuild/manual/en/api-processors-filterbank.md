---
id: api-processors-filterbank
title: FilterBank
description: Complementary FIR bands whose neutral sum reconstructs the delayed input.
section: API reference
order: 1141
revision: 1
parent: api-processors
symbol: cydsp::FirFilterBank
---

Complementary FIR bands whose neutral sum reconstructs the delayed input.

## Include {#include}

```cpp
#include <cydsp/processors/FilterBank.h>
```

[Usage and contracts](doc:filter-design) · [Common processing rules](doc:contracts)

## `FirFilterBank` {#symbol-firfilterbank}

```cpp
template <Sample T> class FirFilterBank;
```

### `FirFilterBank::FirFilterBank<T>` {#symbol-firfilterbank-firfilterbank-t}

```cpp
FirFilterBank();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `FirFilterBank::~FirFilterBank<T>` {#symbol-firfilterbank-firfilterbank-t-2}

```cpp
~FirFilterBank();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `FirFilterBank::FirFilterBank<T>` {#symbol-firfilterbank-firfilterbank-t-3}

```cpp
FirFilterBank(FirFilterBank&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `FirFilterBank<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `FirFilterBank::operator=` {#symbol-firfilterbank-operator}

```cpp
FirFilterBank& operator=(FirFilterBank&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `FirFilterBank<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `FirFilterBank<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `FirFilterBank::prepare` {#symbol-firfilterbank-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const double> crossoversHz, std::size_t taps = 129, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `crossoversHz` | `std::span<const double>` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `taps` | `std::size_t` | `129` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `FirFilterBank::process` {#symbol-firfilterbank-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T> bands) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `bands` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `FirFilterBank::sumBands` {#symbol-firfilterbank-sumbands}

```cpp
Status sumBands(AudioBlock<const T> bands, AudioBlock<T> output) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `bands` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `output` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `FirFilterBank::reset` {#symbol-firfilterbank-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `FirFilterBank::bands` {#symbol-firfilterbank-bands}

```cpp
std::size_t bands() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `FirFilterBank::latencyFrames` {#symbol-firfilterbank-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `FirFilterBank::backend` {#symbol-firfilterbank-backend}

```cpp
CpuBackend backend() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `CpuBackend` — Returned value/view; respect borrowed lifetime and the units named by this API.
