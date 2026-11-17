---
id: api-spatial-spatialmath
title: SpatialMath
description: Metre/radian source/listener geometry, media, distance, Doppler, directivity and material response.
section: API reference
order: 1176
revision: 1
parent: api-spatial
symbol: cydsp::Vec3
---

Metre/radian source/listener geometry, media, distance, Doppler, directivity and material response.

## Include {#include}

```cpp
#include <cydsp/spatial/SpatialMath.h>
```

[Usage and contracts](doc:spatial) · [Common processing rules](doc:contracts)

## `Vec3` {#symbol-vec3}

```cpp
struct Vec3;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `x` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `y` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `z` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |

## `Quaternion` {#symbol-quaternion}

```cpp
struct Quaternion;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `w` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `x` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `y` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `z` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |

## `Pose3` {#symbol-pose3}

```cpp
struct Pose3;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `position` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |
| `orientation` | `Quaternion` | — | Value/configuration in the declared type; apply the linked module contract. |

## `SpatialSource` {#symbol-spatialsource}

```cpp
struct SpatialSource;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `pose` | `Pose3` | — | Value/configuration in the declared type; apply the linked module contract. |
| `velocity` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |
| `radiusMetres` | `double` | `0` | Length in metres. |

## `SpatialListener` {#symbol-spatiallistener}

```cpp
struct SpatialListener;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `pose` | `Pose3` | — | Value/configuration in the declared type; apply the linked module contract. |
| `velocity` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |
| `headRadiusMetres` | `double` | `.0875` | Length in metres. |

## `RelativeSource` {#symbol-relativesource}

```cpp
struct RelativeSource;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `direction` | `Vec3` | `{1, 0, 0}` | Value/configuration in the declared type; apply the linked module contract. |
| `localDirection` | `Vec3` | `{1, 0, 0}` | Value/configuration in the declared type; apply the linked module contract. |
| `distanceMetres` | `double` | `0` | Length in metres. |
| `azimuthRadians` | `double` | `0` | Angle in radians. |
| `elevationRadians` | `double` | `0` | Angle in radians. |
| `sourceRadialVelocity` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `listenerRadialVelocity` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |

## `operator+` {#symbol-operator}

```cpp
Vec3 operator+(Vec3, Vec3) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Vec3` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `operator-` {#symbol-operator-2}

```cpp
Vec3 operator-(Vec3, Vec3) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Vec3` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `operator*` {#symbol-operator-3}

```cpp
Vec3 operator*(Vec3, double) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Vec3` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `dot` {#symbol-dot}

```cpp
double dot(Vec3, Vec3) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `cross` {#symbol-cross}

```cpp
Vec3 cross(Vec3, Vec3) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Vec3` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `length` {#symbol-length}

```cpp
double length(Vec3) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `double` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `normalize` {#symbol-normalize}

```cpp
Status normalize(Quaternion, Quaternion&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `Quaternion` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `Quaternion &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `rotate` {#symbol-rotate}

```cpp
Vec3 rotate(Quaternion unitQuaternion, Vec3) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `unitQuaternion` | `Quaternion` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Vec3` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `interpolatePose` {#symbol-interpolatepose}

```cpp
Status interpolatePose(const Pose3&, const Pose3&, double fraction, Pose3&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const Pose3 &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `const Pose3 &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `fraction` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 4` | `Pose3 &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `relativeSource` {#symbol-relativesource-2}

```cpp
Status relativeSource(const SpatialSource&, const SpatialListener&, RelativeSource&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const SpatialSource &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `const SpatialListener &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `RelativeSource &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `relativeSources` {#symbol-relativesources}

```cpp
Status relativeSources(std::span<const SpatialSource>, std::span<const SpatialListener>, std::span<RelativeSource>) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const SpatialSource>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<const SpatialListener>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 3` | `std::span<RelativeSource>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

## `AcousticMedium` {#symbol-acousticmedium}

```cpp
struct AcousticMedium;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `speedMetresPerSecond` | `double` | `343` | Velocity in metres per second. |
| `densityKgPerCubicMetre` | `double` | `1.2` | Value/configuration in the declared type; apply the linked module contract. |
| `absorptionDbPerMetre` | `std::array<double, 3>` | `{0, 0, 0}` | Value/configuration in the declared type; apply the linked module contract. |

## `validMedium` {#symbol-validmedium}

```cpp
bool validMedium(const AcousticMedium&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const AcousticMedium &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `atmosphericAbsorption` {#symbol-atmosphericabsorption}

```cpp
Status atmosphericAbsorption(double frequencyHz, double temperatureCelsius, double relativeHumidityPercent, double& dbPerMetre, double pressurePascals = 101325) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `frequencyHz` | `double` | — | Frequency in Hz; use the supported sample-rate/Nyquist range. |
| `temperatureCelsius` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `relativeHumidityPercent` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `dbPerMetre` | `double &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |
| `pressurePascals` | `double` | `101325` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

## `airMedium` {#symbol-airmedium}

```cpp
Status airMedium(double temperatureCelsius, double relativeHumidityPercent, AcousticMedium&, double pressurePascals = 101325) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `temperatureCelsius` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `relativeHumidityPercent` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `AcousticMedium &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |
| `pressurePascals` | `double` | `101325` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

## `waterMedium` {#symbol-watermedium}

```cpp
AcousticMedium waterMedium() noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `AcousticMedium` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `MediumSegment` {#symbol-mediumsegment}

```cpp
struct MediumSegment;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `lengthMetres` | `double` | `0` | Length in metres. |
| `medium` | `AcousticMedium` | — | Value/configuration in the declared type; apply the linked module contract. |

## `AcousticTravel` {#symbol-acoustictravel}

```cpp
struct AcousticTravel;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `seconds` | `double` | `0` | Duration or position in seconds. |
| `bandGains` | `std::array<double, 3>` | `{1, 1, 1}` | Value/configuration in the declared type; apply the linked module contract. |

## `acousticTravel` {#symbol-acoustictravel-2}

```cpp
Status acousticTravel(std::span<const MediumSegment>, AcousticTravel&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const MediumSegment>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `AcousticTravel &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `DistanceModel` {#symbol-distancemodel}

```cpp
enum class DistanceModel { inverse, exponential, linear };
```

## `DistanceAttenuation` {#symbol-distanceattenuation}

```cpp
struct DistanceAttenuation;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `model` | `DistanceModel` | `DistanceModel::inverse` | Value/configuration in the declared type; apply the linked module contract. |
| `referenceMetres` | `double` | `1` | Length in metres. |
| `maximumMetres` | `double` | `100` | Length in metres. |
| `rolloff` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `clampDistance` | `bool` | `true` | Explicit true/false option or predicate. |

## `distanceGain` {#symbol-distancegain}

```cpp
Status distanceGain(double distanceMetres, const DistanceAttenuation&, double& gain) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `distanceMetres` | `double` | — | Length in metres. |
| `argument 2` | `const DistanceAttenuation &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `gain` | `double &` | — | Linear amplitude multiplier; follow the processor bounds and mixing law. |

Return: `Status` — Check Status; ok reports success.

## `dopplerRatio` {#symbol-dopplerratio}

```cpp
Status dopplerRatio(const RelativeSource&, const AcousticMedium&, double& ratio, double intensity = 1, double maximumSemitones = 24) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const RelativeSource &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `const AcousticMedium &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `ratio` | `double &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |
| `intensity` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `maximumSemitones` | `double` | `24` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

## `DopplerPlan` {#symbol-dopplerplan}

```cpp
class DopplerPlan;
```

### `DopplerPlan::prepare` {#symbol-dopplerplan-prepare}

```cpp
Status prepare(double speedMetresPerSecond = 343, double intensity = 1, double maximumSemitones = 24) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `speedMetresPerSecond` | `double` | `343` | Velocity in metres per second. |
| `intensity` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `maximumSemitones` | `double` | `24` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `DopplerPlan::ratio` {#symbol-dopplerplan-ratio}

```cpp
Status ratio(double sourceRadialVelocity, double listenerRadialVelocity, double& output) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `sourceRadialVelocity` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `listenerRadialVelocity` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `output` | `double &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `DirectivityPattern` {#symbol-directivitypattern}

```cpp
enum class DirectivityPattern { omni, subcardioid, cardioid, supercardioid, hypercardioid, figureEight, custom };
```

## `Directivity` {#symbol-directivity}

```cpp
struct Directivity;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `pattern` | `DirectivityPattern` | `DirectivityPattern::omni` | Value/configuration in the declared type; apply the linked module contract. |
| `customOmniWeight` | `double` | `.5` | Value/configuration in the declared type; apply the linked module contract. |
| `amount` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `minimumGainDb` | `double` | `-96` | Logarithmic level in the indicated dB reference. |
| `rearLossDb` | `std::array<double, 3>` | `{0, 0, 0}` | Logarithmic level in the indicated dB reference. |
| `preservePolarity` | `bool` | `false` | Explicit true/false option or predicate. |

## `directivityGains` {#symbol-directivitygains}

```cpp
Status directivityGains(double forwardCosine, const Directivity&, std::array<double, 3>&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `forwardCosine` | `double` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `const Directivity &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `std::array<double, 3> &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `AcousticMaterial` {#symbol-acousticmaterial}

```cpp
struct AcousticMaterial;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `absorption` | `std::array<double, 3>` | `{.1, .2, .3}` | Value/configuration in the declared type; apply the linked module contract. |
| `transmissionLossDb` | `std::array<double, 3>` | `{20, 30, 40}` | Logarithmic level in the indicated dB reference. |
| `referenceThicknessMetres` | `double` | `.2` | Length in metres. |
| `scattering` | `double` | `.2` | Value/configuration in the declared type; apply the linked module contract. |

## `MaterialResponse` {#symbol-materialresponse}

```cpp
struct MaterialResponse;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `transmission` | `std::array<double, 3>` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `reflection` | `std::array<double, 3>` | `{}` | Value/configuration in the declared type; apply the linked module contract. |

## `materialResponse` {#symbol-materialresponse-2}

```cpp
Status materialResponse(const AcousticMaterial&, double thicknessMetres, MaterialResponse&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const AcousticMaterial &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `thicknessMetres` | `double` | — | Length in metres. |
| `argument 3` | `MaterialResponse &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `AcousticMaterialBand` {#symbol-acousticmaterialband}

```cpp
struct AcousticMaterialBand;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `absorption` | `double` | `.2` | Value/configuration in the declared type; apply the linked module contract. |
| `transmissionLossDb` | `double` | `30` | Logarithmic level in the indicated dB reference. |
| `referenceThicknessMetres` | `double` | `.2` | Length in metres. |
| `scattering` | `double` | `.2` | Value/configuration in the declared type; apply the linked module contract. |

## `MaterialBandResponse` {#symbol-materialbandresponse}

```cpp
struct MaterialBandResponse;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `transmission` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |
| `reflection` | `double` | `0` | Value/configuration in the declared type; apply the linked module contract. |

## `materialBandResponse` {#symbol-materialbandresponse-2}

```cpp
Status materialBandResponse(const AcousticMaterialBand&, double thicknessMetres, MaterialBandResponse&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const AcousticMaterialBand &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `thicknessMetres` | `double` | — | Length in metres. |
| `argument 3` | `MaterialBandResponse &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `PreparedMaterialBand` {#symbol-preparedmaterialband}

```cpp
class PreparedMaterialBand;
```

### `PreparedMaterialBand::prepare` {#symbol-preparedmaterialband-prepare}

```cpp
Status prepare(const AcousticMaterialBand&) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const AcousticMaterialBand &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `PreparedMaterialBand::response` {#symbol-preparedmaterialband-response}

```cpp
Status response(double thicknessMetres, MaterialBandResponse&) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `thicknessMetres` | `double` | — | Length in metres. |
| `argument 2` | `MaterialBandResponse &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `PreparedAcousticMaterial` {#symbol-preparedacousticmaterial}

```cpp
class PreparedAcousticMaterial;
```

### `PreparedAcousticMaterial::prepare` {#symbol-preparedacousticmaterial-prepare}

```cpp
Status prepare(const AcousticMaterial&) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const AcousticMaterial &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `PreparedAcousticMaterial::response` {#symbol-preparedacousticmaterial-response}

```cpp
Status response(double thicknessMetres, MaterialResponse&) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `thicknessMetres` | `double` | — | Length in metres. |
| `argument 2` | `MaterialResponse &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `PropagationResult` {#symbol-propagationresult}

```cpp
struct PropagationResult;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `relative` | `RelativeSource` | — | Value/configuration in the declared type; apply the linked module contract. |
| `travel` | `AcousticTravel` | — | Value/configuration in the declared type; apply the linked module contract. |
| `doppler` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |

## `evaluatePropagation` {#symbol-evaluatepropagation}

```cpp
Status evaluatePropagation(const SpatialSource&, const SpatialListener&, const AcousticMedium&, const DistanceAttenuation&, const Directivity&, PropagationResult&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const SpatialSource &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `const SpatialListener &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `const AcousticMedium &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 4` | `const DistanceAttenuation &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 5` | `const Directivity &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 6` | `PropagationResult &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.
