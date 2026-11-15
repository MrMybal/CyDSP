---
id: api-spatial-spatialrendering
title: SpatialRendering
description: Analytic/measured binaural, shared HRTF preparation, VBAP and order-7 ACN/SN3D Ambisonics.
section: API reference
order: 1175
revision: 1
parent: api-spatial
symbol: cydsp::BinauralEmitter
---

Analytic/measured binaural, shared HRTF preparation, VBAP and order-7 ACN/SN3D Ambisonics.

## Include {#include}

```cpp
#include <cydsp/spatial/SpatialRendering.h>
```

[Usage and contracts](doc:binaural) · [Common processing rules](doc:contracts)

## `BinauralEmitter` {#symbol-binauralemitter}

```cpp
struct BinauralEmitter;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `position` | `Vec3` | `{1, 0, 0}` | Value/configuration in the declared type; apply the linked module contract. |
| `gain` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `radiusMetres` | `double` | `0` | Length in metres. |

## `AnalyticBinaural` {#symbol-analyticbinaural}

```cpp
template <Sample T> class AnalyticBinaural;
```

### `AnalyticBinaural::AnalyticBinaural<T>` {#symbol-analyticbinaural-analyticbinaural-t}

```cpp
AnalyticBinaural();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `AnalyticBinaural::~AnalyticBinaural<T>` {#symbol-analyticbinaural-analyticbinaural-t-2}

```cpp
~AnalyticBinaural();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `AnalyticBinaural::AnalyticBinaural<T>` {#symbol-analyticbinaural-analyticbinaural-t-3}

```cpp
AnalyticBinaural(AnalyticBinaural&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AnalyticBinaural<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `AnalyticBinaural::operator=` {#symbol-analyticbinaural-operator}

```cpp
AnalyticBinaural& operator=(AnalyticBinaural&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AnalyticBinaural<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `AnalyticBinaural<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AnalyticBinaural::prepare` {#symbol-analyticbinaural-prepare}

```cpp
Status prepare(const ProcessSpec&, double headRadiusMetres = .0875, double soundSpeed = 343) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `headRadiusMetres` | `double` | `.0875` | Length in metres. |
| `soundSpeed` | `double` | `343` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `AnalyticBinaural::setEmitters` {#symbol-analyticbinaural-setemitters}

```cpp
Status setEmitters(std::span<const BinauralEmitter>, std::size_t rampFrames = 256) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const BinauralEmitter>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `rampFrames` | `std::size_t` | `256` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `AnalyticBinaural::process` {#symbol-analyticbinaural-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T> stereo) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `stereo` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `AnalyticBinaural::reset` {#symbol-analyticbinaural-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

## `HrirPoint` {#symbol-hrirpoint}

```cpp
template <Sample T> struct HrirPoint;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `direction` | `Vec3` | `{1, 0, 0}` | Value/configuration in the declared type; apply the linked module contract. |
| `left` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `right` | `std::span<const T>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `leftDelayFrames` | `double` | `0` | Frame count or offset; one frame contains one sample per channel. |
| `rightDelayFrames` | `double` | `0` | Frame count or offset; one frame contains one sample per channel. |

## `HrtfPreparation` {#symbol-hrtfpreparation}

```cpp
struct HrtfPreparation;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `sourceSampleRate` | `double` | `48000` | Value/configuration in the declared type; apply the linked module contract. |
| `targetSampleRate` | `double` | `48000` | Value/configuration in the declared type; apply the linked module contract. |
| `detectOnsets` | `bool` | `true` | Explicit true/false option or predicate. |
| `onsetThresholdDb` | `double` | `-40` | Logarithmic level in the indicated dB reference. |
| `angularWidthRadians` | `double` | `.7` | Angle in radians. |
| `maximumTaps` | `std::size_t` | `2048` | Value/configuration in the declared type; apply the linked module contract. |

## `HrtfSet` {#symbol-hrtfset}

```cpp
template <Sample T> class HrtfSet;
```

### `HrtfSet::prepare` {#symbol-hrtfset-prepare}

```cpp
Status prepare(std::span<const HrirPoint<T>>, const HrtfPreparation& = {}) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const HrirPoint<T>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `const HrtfPreparation &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `HrtfSet::points` {#symbol-hrtfset-points}

```cpp
std::size_t points() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `HrtfSet::taps` {#symbol-hrtfset-taps}

```cpp
std::size_t taps() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `HrtfSet::sampleRate` {#symbol-hrtfset-samplerate}

```cpp
double sampleRate() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `HrtfSet::latencyFrames` {#symbol-hrtfset-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `HrtfSet::truncated` {#symbol-hrtfset-truncated}

```cpp
bool truncated() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `HrirBinaural` {#symbol-hrirbinaural}

```cpp
template <Sample T> class HrirBinaural;
```

### `HrirBinaural::HrirBinaural<T>` {#symbol-hrirbinaural-hrirbinaural-t}

```cpp
HrirBinaural();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `HrirBinaural::~HrirBinaural<T>` {#symbol-hrirbinaural-hrirbinaural-t-2}

```cpp
~HrirBinaural();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `HrirBinaural::HrirBinaural<T>` {#symbol-hrirbinaural-hrirbinaural-t-3}

```cpp
HrirBinaural(HrirBinaural&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `HrirBinaural<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `HrirBinaural::operator=` {#symbol-hrirbinaural-operator}

```cpp
HrirBinaural& operator=(HrirBinaural&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `HrirBinaural<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `HrirBinaural<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `HrirBinaural::prepare` {#symbol-hrirbinaural-prepare}

```cpp
Status prepare(const ProcessSpec& mono, std::span<const HrirPoint<T>>, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `mono` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `std::span<const HrirPoint<T>>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `HrirBinaural::prepare` {#symbol-hrirbinaural-prepare-2}

```cpp
Status prepare(const ProcessSpec& mono, const HrtfSet<T>&, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `mono` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `const HrtfSet<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `HrirBinaural::setDirection` {#symbol-hrirbinaural-setdirection}

```cpp
Status setDirection(Vec3, std::size_t transitionFrames = 256) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |
| `transitionFrames` | `std::size_t` | `256` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `HrirBinaural::process` {#symbol-hrirbinaural-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T> stereo) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `stereo` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `HrirBinaural::reset` {#symbol-hrirbinaural-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `HrirBinaural::tailFrames` {#symbol-hrirbinaural-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `HrirBinaural::latencyFrames` {#symbol-hrirbinaural-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `VbapLayout` {#symbol-vbaplayout}

```cpp
class VbapLayout;
```

### `VbapLayout::VbapLayout` {#symbol-vbaplayout-vbaplayout}

```cpp
VbapLayout();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `VbapLayout::~VbapLayout` {#symbol-vbaplayout-vbaplayout-2}

```cpp
~VbapLayout();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `VbapLayout::VbapLayout` {#symbol-vbaplayout-vbaplayout-3}

```cpp
VbapLayout(VbapLayout&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `VbapLayout &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `VbapLayout::operator=` {#symbol-vbaplayout-operator}

```cpp
VbapLayout& operator=(VbapLayout&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `VbapLayout &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `VbapLayout &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `VbapLayout::prepare` {#symbol-vbaplayout-prepare}

```cpp
Status prepare(std::span<const Vec3> speakers, bool threeDimensional, bool nearestFallback = false) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `speakers` | `std::span<const Vec3>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `threeDimensional` | `bool` | — | Explicit true/false option or predicate. |
| `nearestFallback` | `bool` | `false` | Explicit true/false option or predicate. |

Return: `Status` — Check Status; ok reports success.

### `VbapLayout::gains` {#symbol-vbaplayout-gains}

```cpp
Status gains(Vec3 direction, std::span<double>) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `direction` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `std::span<double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `VbapLayout::speakers` {#symbol-vbaplayout-speakers}

```cpp
std::size_t speakers() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `ambisonicCoefficients` {#symbol-ambisoniccoefficients}

```cpp
Status ambisonicCoefficients(unsigned order, Vec3 direction, std::span<double>) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `order` | `unsigned int` | — | Value/configuration in the declared type; apply the linked module contract. |
| `direction` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `std::span<double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

## `ambisonicRotation` {#symbol-ambisonicrotation}

```cpp
Status ambisonicRotation(unsigned order, Quaternion, std::span<double>) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `order` | `unsigned int` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `Quaternion` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `std::span<double>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

## `AmbisonicRenderer` {#symbol-ambisonicrenderer}

```cpp
template <Sample T> class AmbisonicRenderer;
```

### `AmbisonicRenderer::AmbisonicRenderer<T>` {#symbol-ambisonicrenderer-ambisonicrenderer-t}

```cpp
AmbisonicRenderer();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `AmbisonicRenderer::~AmbisonicRenderer<T>` {#symbol-ambisonicrenderer-ambisonicrenderer-t-2}

```cpp
~AmbisonicRenderer();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `AmbisonicRenderer::AmbisonicRenderer<T>` {#symbol-ambisonicrenderer-ambisonicrenderer-t-3}

```cpp
AmbisonicRenderer(AmbisonicRenderer&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AmbisonicRenderer<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `AmbisonicRenderer::operator=` {#symbol-ambisonicrenderer-operator}

```cpp
AmbisonicRenderer& operator=(AmbisonicRenderer&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AmbisonicRenderer<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `AmbisonicRenderer<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AmbisonicRenderer::prepare` {#symbol-ambisonicrenderer-prepare}

```cpp
Status prepare(const ProcessSpec& mono, unsigned order, std::span<const Vec3> speakers, double regularization = 1e-6, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `mono` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `order` | `unsigned int` | — | Value/configuration in the declared type; apply the linked module contract. |
| `speakers` | `std::span<const Vec3>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `regularization` | `double` | `1e-6` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `AmbisonicRenderer::setDirection` {#symbol-ambisonicrenderer-setdirection}

```cpp
Status setDirection(Vec3) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `AmbisonicRenderer::setRotation` {#symbol-ambisonicrenderer-setrotation}

```cpp
Status setRotation(Quaternion) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Quaternion` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `AmbisonicRenderer::encode` {#symbol-ambisonicrenderer-encode}

```cpp
Status encode(AudioBlock<const T> mono, AudioBlock<T> ambisonic) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `mono` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `ambisonic` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `AmbisonicRenderer::rotateField` {#symbol-ambisonicrenderer-rotatefield}

```cpp
Status rotateField(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `AmbisonicRenderer::decode` {#symbol-ambisonicrenderer-decode}

```cpp
Status decode(AudioBlock<const T>, AudioBlock<T> speakers) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `speakers` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `AmbisonicRenderer::reset` {#symbol-ambisonicrenderer-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `AmbisonicRenderer::channels` {#symbol-ambisonicrenderer-channels}

```cpp
std::size_t channels() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `HoaBinaural` {#symbol-hoabinaural}

```cpp
template <Sample T> class HoaBinaural;
```

### `HoaBinaural::HoaBinaural<T>` {#symbol-hoabinaural-hoabinaural-t}

```cpp
HoaBinaural();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `HoaBinaural::~HoaBinaural<T>` {#symbol-hoabinaural-hoabinaural-t-2}

```cpp
~HoaBinaural();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `HoaBinaural::HoaBinaural<T>` {#symbol-hoabinaural-hoabinaural-t-3}

```cpp
HoaBinaural(HoaBinaural&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `HoaBinaural<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `HoaBinaural::operator=` {#symbol-hoabinaural-operator}

```cpp
HoaBinaural& operator=(HoaBinaural&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `HoaBinaural<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `HoaBinaural<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `HoaBinaural::prepare` {#symbol-hoabinaural-prepare}

```cpp
Status prepare(const ProcessSpec& ambisonic, unsigned order, const HrtfSet<T>&, double regularization = 1e-6, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `ambisonic` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `order` | `unsigned int` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `const HrtfSet<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `regularization` | `double` | `1e-6` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `HoaBinaural::setRotation` {#symbol-hoabinaural-setrotation}

```cpp
Status setRotation(Quaternion) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Quaternion` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `HoaBinaural::process` {#symbol-hoabinaural-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T> stereo) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `stereo` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `HoaBinaural::reset` {#symbol-hoabinaural-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `HoaBinaural::tailFrames` {#symbol-hoabinaural-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `HoaBinaural::latencyFrames` {#symbol-hoabinaural-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
