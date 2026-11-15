---
id: api-processors-advanceddynamics
title: AdvancedDynamics
description: ADAA saturation, dynamic EQ, de-essing, transient shaping, multiband dynamics and true-peak limiting.
section: API reference
order: 1124
revision: 1
parent: api-processors
symbol: cydsp::AdaaShape
---

ADAA saturation, dynamic EQ, de-essing, transient shaping, multiband dynamics and true-peak limiting.

## Include {#include}

```cpp
#include <cydsp/processors/AdvancedDynamics.h>
```

[Usage and contracts](doc:advanced-dynamics) · [Common processing rules](doc:contracts)

## `AdaaShape` {#symbol-adaashape}

```cpp
enum class AdaaShape { tanh, atan, cubic, hardClip, wavefolder, hysteresis };
```

## `SaturationParameters` {#symbol-saturationparameters}

```cpp
struct SaturationParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `shape` | `AdaaShape` | `AdaaShape::tanh` | Value/configuration in the declared type; apply the linked module contract. |
| `drive` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `bias` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `outputGain` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `hysteresisWidth` | `double` | `.1` | Value/configuration in the declared type; apply the linked module contract. |
| `hysteresisAmount` | `double` | `.5` | Value/configuration in the declared type; apply the linked module contract. |
| `antialias` | `bool` | `true` | Explicit true/false option or predicate. |
| `math` | `MathAccuracy` | `MathAccuracy::precise` | Value/configuration in the declared type; apply the linked module contract. |

## `SaturationInputStage` {#symbol-saturationinputstage}

```cpp
struct SaturationInputStage;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `gain` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `bias` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `scale` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |

## `AdaaSaturator` {#symbol-adaasaturator}

```cpp
template <Sample T> class AdaaSaturator;
```

### `AdaaSaturator::AdaaSaturator<T>` {#symbol-adaasaturator-adaasaturator-t}

```cpp
AdaaSaturator();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `AdaaSaturator::~AdaaSaturator<T>` {#symbol-adaasaturator-adaasaturator-t-2}

```cpp
~AdaaSaturator();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `AdaaSaturator::AdaaSaturator<T>` {#symbol-adaasaturator-adaasaturator-t-3}

```cpp
AdaaSaturator(AdaaSaturator&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AdaaSaturator<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `AdaaSaturator::operator=` {#symbol-adaasaturator-operator}

```cpp
AdaaSaturator& operator=(AdaaSaturator&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AdaaSaturator<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `AdaaSaturator<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AdaaSaturator::prepare` {#symbol-adaasaturator-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |

Return: `Status` — Check Status; ok reports success.

### `AdaaSaturator::setParameters` {#symbol-adaasaturator-setparameters}

```cpp
Status setParameters(const SaturationParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const SaturationParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `AdaaSaturator::process` {#symbol-adaasaturator-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `AdaaSaturator::processMixed` {#symbol-adaasaturator-processmixed}

```cpp
Status processMixed(AudioBlock<const T>, AudioBlock<T>, const SaturationInputStage&, double dryGain = 0, double wetGain = 1) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |
| `argument 3` | `const SaturationInputStage &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `dryGain` | `double` | `0` | Linear amplitude multiplier; follow the processor bounds and mixing law. |
| `wetGain` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |

Return: `Status` — Check Status; ok reports success.

### `AdaaSaturator::reset` {#symbol-adaasaturator-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

## `DynamicEqBand` {#symbol-dynamiceqband}

```cpp
struct DynamicEqBand;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `frequencyHz` | `double` | `1000` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `q` | `double` | `1` | Positive finite filter quality factor. |
| `thresholdDb` | `double` | `-24` | Logarithmic level in the indicated dB reference. |
| `rangeDb` | `double` | `-12` | Logarithmic level in the indicated dB reference. |
| `attackMs` | `double` | `10` | Time in milliseconds. |
| `releaseMs` | `double` | `100` | Time in milliseconds. |
| `staticGainDb` | `double` | `0` | Logarithmic level in the indicated dB reference. |

## `DynamicEqualizer` {#symbol-dynamicequalizer}

```cpp
template <Sample T> class DynamicEqualizer;
```

### `DynamicEqualizer::DynamicEqualizer<T>` {#symbol-dynamicequalizer-dynamicequalizer-t}

```cpp
DynamicEqualizer();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `DynamicEqualizer::~DynamicEqualizer<T>` {#symbol-dynamicequalizer-dynamicequalizer-t-2}

```cpp
~DynamicEqualizer();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `DynamicEqualizer::DynamicEqualizer<T>` {#symbol-dynamicequalizer-dynamicequalizer-t-3}

```cpp
DynamicEqualizer(DynamicEqualizer&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `DynamicEqualizer<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `DynamicEqualizer::operator=` {#symbol-dynamicequalizer-operator}

```cpp
DynamicEqualizer& operator=(DynamicEqualizer&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `DynamicEqualizer<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `DynamicEqualizer<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `DynamicEqualizer::prepare` {#symbol-dynamicequalizer-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t maximumBands = 8) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `maximumBands` | `std::size_t` | `8` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `DynamicEqualizer::setBands` {#symbol-dynamicequalizer-setbands}

```cpp
Status setBands(std::span<const DynamicEqBand>) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const DynamicEqBand>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `DynamicEqualizer::process` {#symbol-dynamicequalizer-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `DynamicEqualizer::process` {#symbol-dynamicequalizer-process-2}

```cpp
Status process(AudioBlock<const T>, AudioBlock<const T> sidechain, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `sidechain` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 3` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `DynamicEqualizer::reset` {#symbol-dynamicequalizer-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

## `DeEsser` {#symbol-deesser}

```cpp
template <Sample T> class DeEsser;
```

### `DeEsser::prepare` {#symbol-deesser-prepare}

```cpp
Status prepare(const ProcessSpec& spec) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `spec` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |

Return: `Status` — Check Status; ok reports success.

### `DeEsser::setParameters` {#symbol-deesser-setparameters}

```cpp
Status setParameters(double frequencyHz = 6500, double q = .7, double thresholdDb = -24, double maximumCutDb = 12) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `frequencyHz` | `double` | `6500` | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `q` | `double` | `.7` | Positive finite filter quality factor. |
| `thresholdDb` | `double` | `-24` | Logarithmic level in the indicated dB reference. |
| `maximumCutDb` | `double` | `12` | Logarithmic level in the indicated dB reference. |

Return: `Status` — Check Status; ok reports success.

### `DeEsser::process` {#symbol-deesser-process}

```cpp
Status process(AudioBlock<const T> in, AudioBlock<T> out) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `in` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `out` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `DeEsser::reset` {#symbol-deesser-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

## `TransientParameters` {#symbol-transientparameters}

```cpp
struct TransientParameters;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `attack` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `sustain` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `fastMs` | `double` | `1` | Time in milliseconds. |
| `slowMs` | `double` | `30` | Time in milliseconds. |
| `releaseMs` | `double` | `150` | Time in milliseconds. |
| `maximumGainDb` | `double` | `24` | Logarithmic level in the indicated dB reference. |
| `math` | `MathAccuracy` | `MathAccuracy::precise` | Value/configuration in the declared type; apply the linked module contract. |

## `TransientShaper` {#symbol-transientshaper}

```cpp
template <Sample T> class TransientShaper;
```

### `TransientShaper::TransientShaper<T>` {#symbol-transientshaper-transientshaper-t}

```cpp
TransientShaper();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `TransientShaper::~TransientShaper<T>` {#symbol-transientshaper-transientshaper-t-2}

```cpp
~TransientShaper();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `TransientShaper::TransientShaper<T>` {#symbol-transientshaper-transientshaper-t-3}

```cpp
TransientShaper(TransientShaper&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `TransientShaper<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `TransientShaper::operator=` {#symbol-transientshaper-operator}

```cpp
TransientShaper& operator=(TransientShaper&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `TransientShaper<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `TransientShaper<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `TransientShaper::prepare` {#symbol-transientshaper-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |

Return: `Status` — Check Status; ok reports success.

### `TransientShaper::setParameters` {#symbol-transientshaper-setparameters}

```cpp
Status setParameters(const TransientParameters&) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const TransientParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `TransientShaper::process` {#symbol-transientshaper-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `TransientShaper::processGain` {#symbol-transientshaper-processgain}

```cpp
Status processGain(AudioBlock<const T> detector, AudioBlock<T> gains) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `detector` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `gains` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `TransientShaper::reset` {#symbol-transientshaper-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

## `MultibandMode` {#symbol-multibandmode}

```cpp
enum class MultibandMode { bypass, compressor, saturator };
```

## `MultibandDynamics` {#symbol-multibanddynamics}

```cpp
template <Sample T> class MultibandDynamics;
```

### `MultibandDynamics::MultibandDynamics<T>` {#symbol-multibanddynamics-multibanddynamics-t}

```cpp
MultibandDynamics();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `MultibandDynamics::~MultibandDynamics<T>` {#symbol-multibanddynamics-multibanddynamics-t-2}

```cpp
~MultibandDynamics();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `MultibandDynamics::MultibandDynamics<T>` {#symbol-multibanddynamics-multibanddynamics-t-3}

```cpp
MultibandDynamics(MultibandDynamics&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `MultibandDynamics<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `MultibandDynamics::operator=` {#symbol-multibanddynamics-operator}

```cpp
MultibandDynamics& operator=(MultibandDynamics&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `MultibandDynamics<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `MultibandDynamics<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `MultibandDynamics::prepare` {#symbol-multibanddynamics-prepare}

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

### `MultibandDynamics::setBand` {#symbol-multibanddynamics-setband}

```cpp
Status setBand(std::size_t, MultibandMode, const DynamicsParameters& = {}, const SaturationParameters& = {}) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `MultibandMode` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `const DynamicsParameters &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 4` | `const SaturationParameters &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `MultibandDynamics::process` {#symbol-multibanddynamics-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `MultibandDynamics::reset` {#symbol-multibanddynamics-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `MultibandDynamics::latencyFrames` {#symbol-multibanddynamics-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `TruePeakLimiter` {#symbol-truepeaklimiter}

```cpp
template <Sample T> class TruePeakLimiter;
```

### `TruePeakLimiter::TruePeakLimiter<T>` {#symbol-truepeaklimiter-truepeaklimiter-t}

```cpp
TruePeakLimiter();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `TruePeakLimiter::~TruePeakLimiter<T>` {#symbol-truepeaklimiter-truepeaklimiter-t-2}

```cpp
~TruePeakLimiter();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `TruePeakLimiter::TruePeakLimiter<T>` {#symbol-truepeaklimiter-truepeaklimiter-t-3}

```cpp
TruePeakLimiter(TruePeakLimiter&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `TruePeakLimiter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `TruePeakLimiter::operator=` {#symbol-truepeaklimiter-operator}

```cpp
TruePeakLimiter& operator=(TruePeakLimiter&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `TruePeakLimiter<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `TruePeakLimiter<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `TruePeakLimiter::prepare` {#symbol-truepeaklimiter-prepare}

```cpp
Status prepare(const ProcessSpec&, unsigned oversampling = 4, double lookAheadMs = 5, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `oversampling` | `unsigned int` | `4` | Value/configuration in the declared type; apply the linked module contract. |
| `lookAheadMs` | `double` | `5` | Time in milliseconds. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `TruePeakLimiter::setParameters` {#symbol-truepeaklimiter-setparameters}

```cpp
Status setParameters(double ceilingDbtp = -1, double releaseMs = 100, double marginDb = .5) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `ceilingDbtp` | `double` | `-1` | Logarithmic level in the indicated dB reference. |
| `releaseMs` | `double` | `100` | Time in milliseconds. |
| `marginDb` | `double` | `.5` | Logarithmic level in the indicated dB reference. |

Return: `Status` — Check Status; ok reports success.

### `TruePeakLimiter::process` {#symbol-truepeaklimiter-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `TruePeakLimiter::reset` {#symbol-truepeaklimiter-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.

### `TruePeakLimiter::latencyFrames` {#symbol-truepeaklimiter-latencyframes}

```cpp
double latencyFrames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.
