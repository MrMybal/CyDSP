---
id: api-spatial-acousticpaths
title: AcousticPaths
description: Stable acoustic-path slots with shared input history, delays and three-band gains.
section: API reference
order: 1168
revision: 1
parent: api-spatial
symbol: cydsp::AcousticPath
---

Stable acoustic-path slots with shared input history, delays and three-band gains.

## Include {#include}

```cpp
#include <cydsp/spatial/AcousticPaths.h>
```

[Usage and contracts](doc:acoustics) · [Common processing rules](doc:contracts)

## `AcousticPath` {#symbol-acousticpath}

```cpp
struct AcousticPath;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `id` | `std::uint64_t` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `arrivalDirection` | `Vec3` | `{1, 0, 0}` | Value/configuration in the declared type; apply the linked module contract. |
| `delaySeconds` | `double` | `0` | Duration or position in seconds. |
| `bandGains` | `std::array<double, 3>` | `{1, 1, 1}` | Value/configuration in the declared type; apply the linked module contract. |

## `AcousticPathRenderer` {#symbol-acousticpathrenderer}

```cpp
template <Sample T> class AcousticPathRenderer;
```

### `AcousticPathRenderer::AcousticPathRenderer<T>` {#symbol-acousticpathrenderer-acousticpathrenderer-t}

```cpp
AcousticPathRenderer();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `AcousticPathRenderer::~AcousticPathRenderer<T>` {#symbol-acousticpathrenderer-acousticpathrenderer-t-2}

```cpp
~AcousticPathRenderer();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `AcousticPathRenderer::AcousticPathRenderer<T>` {#symbol-acousticpathrenderer-acousticpathrenderer-t-3}

```cpp
AcousticPathRenderer(AcousticPathRenderer&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AcousticPathRenderer<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `AcousticPathRenderer::operator=` {#symbol-acousticpathrenderer-operator}

```cpp
AcousticPathRenderer& operator=(AcousticPathRenderer&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AcousticPathRenderer<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `AcousticPathRenderer<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AcousticPathRenderer::prepare` {#symbol-acousticpathrenderer-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t maximumPaths = 16, double maximumDelaySeconds = 2) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `maximumPaths` | `std::size_t` | `16` | Value/configuration in the declared type; apply the linked module contract. |
| `maximumDelaySeconds` | `double` | `2` | Duration or position in seconds. |

Return: `Status` — Check Status; ok reports success.

### `AcousticPathRenderer::setPaths` {#symbol-acousticpathrenderer-setpaths}

```cpp
Status setPaths(std::span<const AcousticPath>, std::size_t transitionFrames = 128) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const AcousticPath>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `transitionFrames` | `std::size_t` | `128` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `AcousticPathRenderer::process` {#symbol-acousticpathrenderer-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T> arrivals) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `arrivals` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `AcousticPathRenderer::reset` {#symbol-acousticpathrenderer-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `AcousticPathRenderer::maximumPaths` {#symbol-acousticpathrenderer-maximumpaths}

```cpp
std::size_t maximumPaths() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AcousticPathRenderer::activePaths` {#symbol-acousticpathrenderer-activepaths}

```cpp
std::size_t activePaths() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AcousticPathRenderer::path` {#symbol-acousticpathrenderer-path}

```cpp
AcousticPath path(std::size_t slot) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `slot` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `AcousticPath` — Returned value/view; respect borrowed lifetime and the units named by this API.
