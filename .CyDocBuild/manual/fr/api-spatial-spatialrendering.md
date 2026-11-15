---
id: api-spatial-spatialrendering
title: SpatialRendering
description: Binaural analytique/mesuré, préparation HRTF partagée, VBAP et Ambisonics ACN/SN3D d’ordre 7.
section: Référence API
order: 1175
sourceRevision: 1
parent: api-spatial
symbol: cydsp::BinauralEmitter
---

Binaural analytique/mesuré, préparation HRTF partagée, VBAP et Ambisonics ACN/SN3D d’ordre 7.

## Inclusion {#include}

```cpp
#include <cydsp/spatial/SpatialRendering.h>
```

[Utilisation et contrats](doc:binaural) · [Règles communes de traitement](doc:contracts)

## `BinauralEmitter` {#symbol-binauralemitter}

```cpp
struct BinauralEmitter;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `position` | `Vec3` | `{1, 0, 0}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `gain` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `radiusMetres` | `double` | `0` | Longueur en mètres. |

## `AnalyticBinaural` {#symbol-analyticbinaural}

```cpp
template <Sample T> class AnalyticBinaural;
```

### `AnalyticBinaural::AnalyticBinaural<T>` {#symbol-analyticbinaural-analyticbinaural-t}

```cpp
AnalyticBinaural();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `AnalyticBinaural::~AnalyticBinaural<T>` {#symbol-analyticbinaural-analyticbinaural-t-2}

```cpp
~AnalyticBinaural();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `AnalyticBinaural::AnalyticBinaural<T>` {#symbol-analyticbinaural-analyticbinaural-t-3}

```cpp
AnalyticBinaural(AnalyticBinaural&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AnalyticBinaural<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `AnalyticBinaural::operator=` {#symbol-analyticbinaural-operator}

```cpp
AnalyticBinaural& operator=(AnalyticBinaural&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AnalyticBinaural<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `AnalyticBinaural<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AnalyticBinaural::prepare` {#symbol-analyticbinaural-prepare}

```cpp
Status prepare(const ProcessSpec&, double headRadiusMetres = .0875, double soundSpeed = 343) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `headRadiusMetres` | `double` | `.0875` | Longueur en mètres. |
| `soundSpeed` | `double` | `343` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AnalyticBinaural::setEmitters` {#symbol-analyticbinaural-setemitters}

```cpp
Status setEmitters(std::span<const BinauralEmitter>, std::size_t rampFrames = 256) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const BinauralEmitter>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `rampFrames` | `std::size_t` | `256` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AnalyticBinaural::process` {#symbol-analyticbinaural-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T> stereo) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `stereo` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AnalyticBinaural::reset` {#symbol-analyticbinaural-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

## `HrirPoint` {#symbol-hrirpoint}

```cpp
template <Sample T> struct HrirPoint;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `direction` | `Vec3` | `{1, 0, 0}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `left` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `right` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `leftDelayFrames` | `double` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `rightDelayFrames` | `double` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

## `HrtfPreparation` {#symbol-hrtfpreparation}

```cpp
struct HrtfPreparation;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `sourceSampleRate` | `double` | `48000` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `targetSampleRate` | `double` | `48000` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `detectOnsets` | `bool` | `true` | Option ou prédicat vrai/faux explicite. |
| `onsetThresholdDb` | `double` | `-40` | Niveau logarithmique selon la référence dB indiquée. |
| `angularWidthRadians` | `double` | `.7` | Angle en radians. |
| `maximumTaps` | `std::size_t` | `2048` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `HrtfSet` {#symbol-hrtfset}

```cpp
template <Sample T> class HrtfSet;
```

### `HrtfSet::prepare` {#symbol-hrtfset-prepare}

```cpp
Status prepare(std::span<const HrirPoint<T>>, const HrtfPreparation& = {}) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const HrirPoint<T>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `const HrtfPreparation &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `HrtfSet::points` {#symbol-hrtfset-points}

```cpp
std::size_t points() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `HrtfSet::taps` {#symbol-hrtfset-taps}

```cpp
std::size_t taps() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `HrtfSet::sampleRate` {#symbol-hrtfset-samplerate}

```cpp
double sampleRate() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `HrtfSet::latencyFrames` {#symbol-hrtfset-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `HrtfSet::truncated` {#symbol-hrtfset-truncated}

```cpp
bool truncated() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `HrirBinaural` {#symbol-hrirbinaural}

```cpp
template <Sample T> class HrirBinaural;
```

### `HrirBinaural::HrirBinaural<T>` {#symbol-hrirbinaural-hrirbinaural-t}

```cpp
HrirBinaural();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `HrirBinaural::~HrirBinaural<T>` {#symbol-hrirbinaural-hrirbinaural-t-2}

```cpp
~HrirBinaural();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `HrirBinaural::HrirBinaural<T>` {#symbol-hrirbinaural-hrirbinaural-t-3}

```cpp
HrirBinaural(HrirBinaural&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `HrirBinaural<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `HrirBinaural::operator=` {#symbol-hrirbinaural-operator}

```cpp
HrirBinaural& operator=(HrirBinaural&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `HrirBinaural<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `HrirBinaural<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `HrirBinaural::prepare` {#symbol-hrirbinaural-prepare}

```cpp
Status prepare(const ProcessSpec& mono, std::span<const HrirPoint<T>>, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `mono` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `std::span<const HrirPoint<T>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `HrirBinaural::prepare` {#symbol-hrirbinaural-prepare-2}

```cpp
Status prepare(const ProcessSpec& mono, const HrtfSet<T>&, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `mono` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `const HrtfSet<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `HrirBinaural::setDirection` {#symbol-hrirbinaural-setdirection}

```cpp
Status setDirection(Vec3, std::size_t transitionFrames = 256) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `transitionFrames` | `std::size_t` | `256` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `HrirBinaural::process` {#symbol-hrirbinaural-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T> stereo) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `stereo` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `HrirBinaural::reset` {#symbol-hrirbinaural-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `HrirBinaural::tailFrames` {#symbol-hrirbinaural-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `HrirBinaural::latencyFrames` {#symbol-hrirbinaural-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `VbapLayout` {#symbol-vbaplayout}

```cpp
class VbapLayout;
```

### `VbapLayout::VbapLayout` {#symbol-vbaplayout-vbaplayout}

```cpp
VbapLayout();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `VbapLayout::~VbapLayout` {#symbol-vbaplayout-vbaplayout-2}

```cpp
~VbapLayout();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `VbapLayout::VbapLayout` {#symbol-vbaplayout-vbaplayout-3}

```cpp
VbapLayout(VbapLayout&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `VbapLayout &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `VbapLayout::operator=` {#symbol-vbaplayout-operator}

```cpp
VbapLayout& operator=(VbapLayout&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `VbapLayout &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `VbapLayout &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `VbapLayout::prepare` {#symbol-vbaplayout-prepare}

```cpp
Status prepare(std::span<const Vec3> speakers, bool threeDimensional, bool nearestFallback = false) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `speakers` | `std::span<const Vec3>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `threeDimensional` | `bool` | — | Option ou prédicat vrai/faux explicite. |
| `nearestFallback` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `VbapLayout::gains` {#symbol-vbaplayout-gains}

```cpp
Status gains(Vec3 direction, std::span<double>) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `direction` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `std::span<double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `VbapLayout::speakers` {#symbol-vbaplayout-speakers}

```cpp
std::size_t speakers() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `ambisonicCoefficients` {#symbol-ambisoniccoefficients}

```cpp
Status ambisonicCoefficients(unsigned order, Vec3 direction, std::span<double>) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `order` | `unsigned int` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `direction` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `std::span<double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `ambisonicRotation` {#symbol-ambisonicrotation}

```cpp
Status ambisonicRotation(unsigned order, Quaternion, std::span<double>) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `order` | `unsigned int` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `Quaternion` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `std::span<double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `AmbisonicRenderer` {#symbol-ambisonicrenderer}

```cpp
template <Sample T> class AmbisonicRenderer;
```

### `AmbisonicRenderer::AmbisonicRenderer<T>` {#symbol-ambisonicrenderer-ambisonicrenderer-t}

```cpp
AmbisonicRenderer();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `AmbisonicRenderer::~AmbisonicRenderer<T>` {#symbol-ambisonicrenderer-ambisonicrenderer-t-2}

```cpp
~AmbisonicRenderer();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `AmbisonicRenderer::AmbisonicRenderer<T>` {#symbol-ambisonicrenderer-ambisonicrenderer-t-3}

```cpp
AmbisonicRenderer(AmbisonicRenderer&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AmbisonicRenderer<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `AmbisonicRenderer::operator=` {#symbol-ambisonicrenderer-operator}

```cpp
AmbisonicRenderer& operator=(AmbisonicRenderer&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AmbisonicRenderer<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `AmbisonicRenderer<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AmbisonicRenderer::prepare` {#symbol-ambisonicrenderer-prepare}

```cpp
Status prepare(const ProcessSpec& mono, unsigned order, std::span<const Vec3> speakers, double regularization = 1e-6, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `mono` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `order` | `unsigned int` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `speakers` | `std::span<const Vec3>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `regularization` | `double` | `1e-6` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AmbisonicRenderer::setDirection` {#symbol-ambisonicrenderer-setdirection}

```cpp
Status setDirection(Vec3) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AmbisonicRenderer::setRotation` {#symbol-ambisonicrenderer-setrotation}

```cpp
Status setRotation(Quaternion) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Quaternion` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AmbisonicRenderer::encode` {#symbol-ambisonicrenderer-encode}

```cpp
Status encode(AudioBlock<const T> mono, AudioBlock<T> ambisonic) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `mono` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `ambisonic` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AmbisonicRenderer::rotateField` {#symbol-ambisonicrenderer-rotatefield}

```cpp
Status rotateField(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AmbisonicRenderer::decode` {#symbol-ambisonicrenderer-decode}

```cpp
Status decode(AudioBlock<const T>, AudioBlock<T> speakers) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `speakers` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AmbisonicRenderer::reset` {#symbol-ambisonicrenderer-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `AmbisonicRenderer::channels` {#symbol-ambisonicrenderer-channels}

```cpp
std::size_t channels() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `HoaBinaural` {#symbol-hoabinaural}

```cpp
template <Sample T> class HoaBinaural;
```

### `HoaBinaural::HoaBinaural<T>` {#symbol-hoabinaural-hoabinaural-t}

```cpp
HoaBinaural();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `HoaBinaural::~HoaBinaural<T>` {#symbol-hoabinaural-hoabinaural-t-2}

```cpp
~HoaBinaural();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `HoaBinaural::HoaBinaural<T>` {#symbol-hoabinaural-hoabinaural-t-3}

```cpp
HoaBinaural(HoaBinaural&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `HoaBinaural<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `HoaBinaural::operator=` {#symbol-hoabinaural-operator}

```cpp
HoaBinaural& operator=(HoaBinaural&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `HoaBinaural<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `HoaBinaural<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `HoaBinaural::prepare` {#symbol-hoabinaural-prepare}

```cpp
Status prepare(const ProcessSpec& ambisonic, unsigned order, const HrtfSet<T>&, double regularization = 1e-6, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `ambisonic` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `order` | `unsigned int` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `const HrtfSet<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `regularization` | `double` | `1e-6` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `HoaBinaural::setRotation` {#symbol-hoabinaural-setrotation}

```cpp
Status setRotation(Quaternion) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Quaternion` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `HoaBinaural::process` {#symbol-hoabinaural-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T> stereo) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `stereo` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `HoaBinaural::reset` {#symbol-hoabinaural-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `HoaBinaural::tailFrames` {#symbol-hoabinaural-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `HoaBinaural::latencyFrames` {#symbol-hoabinaural-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
