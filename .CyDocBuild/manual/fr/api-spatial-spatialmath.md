---
id: api-spatial-spatialmath
title: SpatialMath
description: Géométrie source/auditeur en mètres/radians, milieux, distance, Doppler, directivité et matériaux.
section: Référence API
order: 1174
sourceRevision: 1
parent: api-spatial
symbol: cydsp::Vec3
---

Géométrie source/auditeur en mètres/radians, milieux, distance, Doppler, directivité et matériaux.

## Inclusion {#include}

```cpp
#include <cydsp/spatial/SpatialMath.h>
```

[Utilisation et contrats](doc:spatial) · [Règles communes de traitement](doc:contracts)

## `Vec3` {#symbol-vec3}

```cpp
struct Vec3;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `x` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `y` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `z` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `Quaternion` {#symbol-quaternion}

```cpp
struct Quaternion;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `w` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `x` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `y` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `z` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `Pose3` {#symbol-pose3}

```cpp
struct Pose3;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `position` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `orientation` | `Quaternion` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `SpatialSource` {#symbol-spatialsource}

```cpp
struct SpatialSource;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `pose` | `Pose3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `velocity` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `radiusMetres` | `double` | `0` | Longueur en mètres. |

## `SpatialListener` {#symbol-spatiallistener}

```cpp
struct SpatialListener;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `pose` | `Pose3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `velocity` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `headRadiusMetres` | `double` | `.0875` | Longueur en mètres. |

## `RelativeSource` {#symbol-relativesource}

```cpp
struct RelativeSource;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `direction` | `Vec3` | `{1, 0, 0}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `localDirection` | `Vec3` | `{1, 0, 0}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `distanceMetres` | `double` | `0` | Longueur en mètres. |
| `azimuthRadians` | `double` | `0` | Angle en radians. |
| `elevationRadians` | `double` | `0` | Angle en radians. |
| `sourceRadialVelocity` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `listenerRadialVelocity` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `operator+` {#symbol-operator}

```cpp
Vec3 operator+(Vec3, Vec3) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Vec3` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `operator-` {#symbol-operator-2}

```cpp
Vec3 operator-(Vec3, Vec3) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Vec3` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `operator*` {#symbol-operator-3}

```cpp
Vec3 operator*(Vec3, double) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Vec3` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `dot` {#symbol-dot}

```cpp
double dot(Vec3, Vec3) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `cross` {#symbol-cross}

```cpp
Vec3 cross(Vec3, Vec3) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Vec3` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `length` {#symbol-length}

```cpp
double length(Vec3) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `normalize` {#symbol-normalize}

```cpp
Status normalize(Quaternion, Quaternion&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Quaternion` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `Quaternion &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `rotate` {#symbol-rotate}

```cpp
Vec3 rotate(Quaternion unitQuaternion, Vec3) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `unitQuaternion` | `Quaternion` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Vec3` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `interpolatePose` {#symbol-interpolatepose}

```cpp
Status interpolatePose(const Pose3&, const Pose3&, double fraction, Pose3&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const Pose3 &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `const Pose3 &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `fraction` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 4` | `Pose3 &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `relativeSource` {#symbol-relativesource-2}

```cpp
Status relativeSource(const SpatialSource&, const SpatialListener&, RelativeSource&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const SpatialSource &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `const SpatialListener &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `RelativeSource &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `relativeSources` {#symbol-relativesources}

```cpp
Status relativeSources(std::span<const SpatialSource>, std::span<const SpatialListener>, std::span<RelativeSource>) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const SpatialSource>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<const SpatialListener>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 3` | `std::span<RelativeSource>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `AcousticMedium` {#symbol-acousticmedium}

```cpp
struct AcousticMedium;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `speedMetresPerSecond` | `double` | `343` | Vitesse en mètres par seconde. |
| `densityKgPerCubicMetre` | `double` | `1.2` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `absorptionDbPerMetre` | `std::array<double, 3>` | `{0, 0, 0}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `validMedium` {#symbol-validmedium}

```cpp
bool validMedium(const AcousticMedium&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const AcousticMedium &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `atmosphericAbsorption` {#symbol-atmosphericabsorption}

```cpp
Status atmosphericAbsorption(double frequencyHz, double temperatureCelsius, double relativeHumidityPercent, double& dbPerMetre, double pressurePascals = 101325) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `frequencyHz` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `temperatureCelsius` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `relativeHumidityPercent` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `dbPerMetre` | `double &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |
| `pressurePascals` | `double` | `101325` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `airMedium` {#symbol-airmedium}

```cpp
Status airMedium(double temperatureCelsius, double relativeHumidityPercent, AcousticMedium&, double pressurePascals = 101325) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `temperatureCelsius` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `relativeHumidityPercent` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `AcousticMedium &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |
| `pressurePascals` | `double` | `101325` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `waterMedium` {#symbol-watermedium}

```cpp
AcousticMedium waterMedium() noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `AcousticMedium` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `MediumSegment` {#symbol-mediumsegment}

```cpp
struct MediumSegment;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `lengthMetres` | `double` | `0` | Longueur en mètres. |
| `medium` | `AcousticMedium` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `AcousticTravel` {#symbol-acoustictravel}

```cpp
struct AcousticTravel;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `seconds` | `double` | `0` | Durée ou position en secondes. |
| `bandGains` | `std::array<double, 3>` | `{1, 1, 1}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `acousticTravel` {#symbol-acoustictravel-2}

```cpp
Status acousticTravel(std::span<const MediumSegment>, AcousticTravel&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const MediumSegment>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `AcousticTravel &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `DistanceModel` {#symbol-distancemodel}

```cpp
enum class DistanceModel { inverse, exponential, linear };
```

## `DistanceAttenuation` {#symbol-distanceattenuation}

```cpp
struct DistanceAttenuation;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `model` | `DistanceModel` | `DistanceModel::inverse` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `referenceMetres` | `double` | `1` | Longueur en mètres. |
| `maximumMetres` | `double` | `100` | Longueur en mètres. |
| `rolloff` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `clampDistance` | `bool` | `true` | Option ou prédicat vrai/faux explicite. |

## `distanceGain` {#symbol-distancegain}

```cpp
Status distanceGain(double distanceMetres, const DistanceAttenuation&, double& gain) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `distanceMetres` | `double` | — | Longueur en mètres. |
| `argument 2` | `const DistanceAttenuation &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `gain` | `double &` | — | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `dopplerRatio` {#symbol-dopplerratio}

```cpp
Status dopplerRatio(const RelativeSource&, const AcousticMedium&, double& ratio, double intensity = 1, double maximumSemitones = 24) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const RelativeSource &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `const AcousticMedium &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `ratio` | `double &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |
| `intensity` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `maximumSemitones` | `double` | `24` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `DopplerPlan` {#symbol-dopplerplan}

```cpp
class DopplerPlan;
```

### `DopplerPlan::prepare` {#symbol-dopplerplan-prepare}

```cpp
Status prepare(double speedMetresPerSecond = 343, double intensity = 1, double maximumSemitones = 24) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `speedMetresPerSecond` | `double` | `343` | Vitesse en mètres par seconde. |
| `intensity` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `maximumSemitones` | `double` | `24` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DopplerPlan::ratio` {#symbol-dopplerplan-ratio}

```cpp
Status ratio(double sourceRadialVelocity, double listenerRadialVelocity, double& output) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `sourceRadialVelocity` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `listenerRadialVelocity` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `output` | `double &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `DirectivityPattern` {#symbol-directivitypattern}

```cpp
enum class DirectivityPattern { omni, subcardioid, cardioid, supercardioid, hypercardioid, figureEight, custom };
```

## `Directivity` {#symbol-directivity}

```cpp
struct Directivity;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `pattern` | `DirectivityPattern` | `DirectivityPattern::omni` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `customOmniWeight` | `double` | `.5` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `amount` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `minimumGainDb` | `double` | `-96` | Niveau logarithmique selon la référence dB indiquée. |
| `rearLossDb` | `std::array<double, 3>` | `{0, 0, 0}` | Niveau logarithmique selon la référence dB indiquée. |
| `preservePolarity` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

## `directivityGains` {#symbol-directivitygains}

```cpp
Status directivityGains(double forwardCosine, const Directivity&, std::array<double, 3>&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `forwardCosine` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `const Directivity &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `std::array<double, 3> &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `AcousticMaterial` {#symbol-acousticmaterial}

```cpp
struct AcousticMaterial;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `absorption` | `std::array<double, 3>` | `{.1, .2, .3}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `transmissionLossDb` | `std::array<double, 3>` | `{20, 30, 40}` | Niveau logarithmique selon la référence dB indiquée. |
| `referenceThicknessMetres` | `double` | `.2` | Longueur en mètres. |
| `scattering` | `double` | `.2` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `MaterialResponse` {#symbol-materialresponse}

```cpp
struct MaterialResponse;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `transmission` | `std::array<double, 3>` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `reflection` | `std::array<double, 3>` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `materialResponse` {#symbol-materialresponse-2}

```cpp
Status materialResponse(const AcousticMaterial&, double thicknessMetres, MaterialResponse&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const AcousticMaterial &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `thicknessMetres` | `double` | — | Longueur en mètres. |
| `argument 3` | `MaterialResponse &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `AcousticMaterialBand` {#symbol-acousticmaterialband}

```cpp
struct AcousticMaterialBand;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `absorption` | `double` | `.2` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `transmissionLossDb` | `double` | `30` | Niveau logarithmique selon la référence dB indiquée. |
| `referenceThicknessMetres` | `double` | `.2` | Longueur en mètres. |
| `scattering` | `double` | `.2` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `MaterialBandResponse` {#symbol-materialbandresponse}

```cpp
struct MaterialBandResponse;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `transmission` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `reflection` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `materialBandResponse` {#symbol-materialbandresponse-2}

```cpp
Status materialBandResponse(const AcousticMaterialBand&, double thicknessMetres, MaterialBandResponse&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const AcousticMaterialBand &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `thicknessMetres` | `double` | — | Longueur en mètres. |
| `argument 3` | `MaterialBandResponse &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `PreparedMaterialBand` {#symbol-preparedmaterialband}

```cpp
class PreparedMaterialBand;
```

### `PreparedMaterialBand::prepare` {#symbol-preparedmaterialband-prepare}

```cpp
Status prepare(const AcousticMaterialBand&) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const AcousticMaterialBand &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `PreparedMaterialBand::response` {#symbol-preparedmaterialband-response}

```cpp
Status response(double thicknessMetres, MaterialBandResponse&) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `thicknessMetres` | `double` | — | Longueur en mètres. |
| `argument 2` | `MaterialBandResponse &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `PreparedAcousticMaterial` {#symbol-preparedacousticmaterial}

```cpp
class PreparedAcousticMaterial;
```

### `PreparedAcousticMaterial::prepare` {#symbol-preparedacousticmaterial-prepare}

```cpp
Status prepare(const AcousticMaterial&) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const AcousticMaterial &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `PreparedAcousticMaterial::response` {#symbol-preparedacousticmaterial-response}

```cpp
Status response(double thicknessMetres, MaterialResponse&) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `thicknessMetres` | `double` | — | Longueur en mètres. |
| `argument 2` | `MaterialResponse &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `PropagationResult` {#symbol-propagationresult}

```cpp
struct PropagationResult;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `relative` | `RelativeSource` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `travel` | `AcousticTravel` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `doppler` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `evaluatePropagation` {#symbol-evaluatepropagation}

```cpp
Status evaluatePropagation(const SpatialSource&, const SpatialListener&, const AcousticMedium&, const DistanceAttenuation&, const Directivity&, PropagationResult&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const SpatialSource &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `const SpatialListener &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `const AcousticMedium &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 4` | `const DistanceAttenuation &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 5` | `const Directivity &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 6` | `PropagationResult &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.
