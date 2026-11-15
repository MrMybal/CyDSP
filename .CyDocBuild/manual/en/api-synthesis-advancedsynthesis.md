---
id: api-synthesis-advancedsynthesis
title: AdvancedSynthesis
description: FM/PM, hard sync, unison, multisegment envelopes, sample-and-hold LFO and procedural sources.
section: API reference
order: 1180
revision: 1
parent: api-synthesis
symbol: cydsp::EnvelopeSegment
---

FM/PM, hard sync, unison, multisegment envelopes, sample-and-hold LFO and procedural sources.

## Include {#include}

```cpp
#include <cydsp/synthesis/AdvancedSynthesis.h>
```

[Usage and contracts](doc:synthesis) · [Common processing rules](doc:contracts)

## `EnvelopeSegment` {#symbol-envelopesegment}

```cpp
struct EnvelopeSegment;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `seconds` | `double` | `.1` | Duration or position in seconds. |
| `target` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `curve` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |

## `MultiSegmentEnvelope` {#symbol-multisegmentenvelope}

```cpp
class MultiSegmentEnvelope;
```

### `MultiSegmentEnvelope::MultiSegmentEnvelope` {#symbol-multisegmentenvelope-multisegmentenvelope}

```cpp
MultiSegmentEnvelope() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `MultiSegmentEnvelope::MultiSegmentEnvelope` {#symbol-multisegmentenvelope-multisegmentenvelope-2}

```cpp
MultiSegmentEnvelope(const MultiSegmentEnvelope&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const MultiSegmentEnvelope &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `MultiSegmentEnvelope::operator=` {#symbol-multisegmentenvelope-operator}

```cpp
MultiSegmentEnvelope& operator=(const MultiSegmentEnvelope&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const MultiSegmentEnvelope &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `MultiSegmentEnvelope &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MultiSegmentEnvelope::MultiSegmentEnvelope` {#symbol-multisegmentenvelope-multisegmentenvelope-3}

```cpp
MultiSegmentEnvelope(MultiSegmentEnvelope&& other) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `MultiSegmentEnvelope &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `MultiSegmentEnvelope::operator=` {#symbol-multisegmentenvelope-operator-2}

```cpp
MultiSegmentEnvelope& operator=(MultiSegmentEnvelope&& other) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `MultiSegmentEnvelope &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `MultiSegmentEnvelope &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MultiSegmentEnvelope::prepare` {#symbol-multisegmentenvelope-prepare}

```cpp
Status prepare(double sampleRate, std::size_t maximumSegments = 32) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `sampleRate` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `maximumSegments` | `std::size_t` | `32` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `MultiSegmentEnvelope::setSegments` {#symbol-multisegmentenvelope-setsegments}

```cpp
Status setSegments(std::span<const EnvelopeSegment>, std::size_t releaseSegment, std::size_t loopBegin = 0, std::size_t loopEnd = 0) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const EnvelopeSegment>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `releaseSegment` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `loopBegin` | `std::size_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `loopEnd` | `std::size_t` | `0` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `MultiSegmentEnvelope::trigger` {#symbol-multisegmentenvelope-trigger}

```cpp
void trigger() noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

### `MultiSegmentEnvelope::release` {#symbol-multisegmentenvelope-release}

```cpp
void release() noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

### `MultiSegmentEnvelope::reset` {#symbol-multisegmentenvelope-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `MultiSegmentEnvelope::next` {#symbol-multisegmentenvelope-next}

```cpp
double next() noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MultiSegmentEnvelope::active` {#symbol-multisegmentenvelope-active}

```cpp
bool active() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `LfoShape` {#symbol-lfoshape}

```cpp
enum class LfoShape { sine, triangle, sampleAndHold, smoothRandom };
```

## `ModulationLfo` {#symbol-modulationlfo}

```cpp
class ModulationLfo;
```

### `ModulationLfo::prepare` {#symbol-modulationlfo-prepare}

```cpp
Status prepare(double sampleRate) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `sampleRate` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |

Return: `Status` — Check Status; ok reports success.

### `ModulationLfo::setParameters` {#symbol-modulationlfo-setparameters}

```cpp
Status setParameters(double frequencyHz, LfoShape) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `frequencyHz` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `argument 2` | `LfoShape` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `ModulationLfo::reset` {#symbol-modulationlfo-reset}

```cpp
void reset(std::uint64_t seed = 1) noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `seed` | `std::uint64_t` | `1` | Per-instance deterministic random seed. |

### `ModulationLfo::next` {#symbol-modulationlfo-next}

```cpp
double next() noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `ModulatedOscillatorParameters` {#symbol-modulatedoscillatorparameters}

```cpp
struct ModulatedOscillatorParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `waveform` | `Waveform` | `Waveform::sine` | Value/configuration in the declared type; apply the linked module contract. |
| `frequencyHz` | `double` | `440` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `fmDepth` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `pmDepthCycles` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `syncHz` | `double` | `0` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `morph` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `detuneCents` | `double` | `10` | Value/configuration in the declared type; apply the linked module contract. |
| `spread` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `gain` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `unison` | `std::size_t` | `1` | Value/configuration in the declared type; apply the linked module contract. |

## `ModulatedOscillator` {#symbol-modulatedoscillator}

```cpp
template <Sample T> class ModulatedOscillator;
```

### `ModulatedOscillator::ModulatedOscillator<T>` {#symbol-modulatedoscillator-modulatedoscillator-t}

```cpp
ModulatedOscillator();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `ModulatedOscillator::~ModulatedOscillator<T>` {#symbol-modulatedoscillator-modulatedoscillator-t-2}

```cpp
~ModulatedOscillator();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `ModulatedOscillator::ModulatedOscillator<T>` {#symbol-modulatedoscillator-modulatedoscillator-t-3}

```cpp
ModulatedOscillator(ModulatedOscillator&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `ModulatedOscillator<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `ModulatedOscillator::operator=` {#symbol-modulatedoscillator-operator}

```cpp
ModulatedOscillator& operator=(ModulatedOscillator&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `ModulatedOscillator<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `ModulatedOscillator<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `ModulatedOscillator::prepare` {#symbol-modulatedoscillator-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const Wavetable<T>* const> tables = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `tables` | `std::span<const Wavetable<T> *const>` | `{}` | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `ModulatedOscillator::setParameters` {#symbol-modulatedoscillator-setparameters}

```cpp
Status setParameters(const ModulatedOscillatorParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ModulatedOscillatorParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `ModulatedOscillator::process` {#symbol-modulatedoscillator-process}

```cpp
Status process(AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `ModulatedOscillator::process` {#symbol-modulatedoscillator-process-2}

```cpp
Status process(AudioBlock<const T> fm, AudioBlock<const T> pm, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `fm` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `pm` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 3` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `ModulatedOscillator::reset` {#symbol-modulatedoscillator-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `ModulatedOscillator::latencyFrames` {#symbol-modulatedoscillator-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `ProceduralKind` {#symbol-proceduralkind}

```cpp
enum class ProceduralKind { wind, engine, impact };
```

## `ProceduralParameters` {#symbol-proceduralparameters}

```cpp
struct ProceduralParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `kind` | `ProceduralKind` | `ProceduralKind::wind` | Value/configuration in the declared type; apply the linked module contract. |
| `frequencyHz` | `double` | `80` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `intensity` | `double` | `.5` | Value/configuration in the declared type; apply the linked module contract. |
| `roughness` | `double` | `.5` | Value/configuration in the declared type; apply the linked module contract. |
| `decaySeconds` | `double` | `.2` | Duration or position in seconds. |

## `ProceduralSource` {#symbol-proceduralsource}

```cpp
template <Sample T> class ProceduralSource;
```

### `ProceduralSource::prepare` {#symbol-proceduralsource-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |

Return: `Status` — Check Status; ok reports success.

### `ProceduralSource::setParameters` {#symbol-proceduralsource-setparameters}

```cpp
Status setParameters(const ProceduralParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProceduralParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `ProceduralSource::trigger` {#symbol-proceduralsource-trigger}

```cpp
void trigger(double strength = 1) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `strength` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |

### `ProceduralSource::process` {#symbol-proceduralsource-process}

```cpp
Status process(AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `ProceduralSource::reset` {#symbol-proceduralsource-reset}

```cpp
void reset(std::uint64_t seed = 1) noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `seed` | `std::uint64_t` | `1` | Per-instance deterministic random seed. |
