---
id: api-spatial-neuralhrtf
title: NeuralHrtf
description: Champ directionnel HRTF neuronal natif expérimental et rendu binaural préparé.
section: Référence API
order: 1174
sourceRevision: 1
parent: api-spatial
symbol: cydsp::neuralHrtfMaxSections
---

Champ directionnel HRTF neuronal natif expérimental et rendu binaural préparé.

## Inclusion {#include}

```cpp
#include <cydsp/spatial/NeuralHrtf.h>
```

[Utilisation et contrats](doc:neural-hrtf) · [Règles communes de traitement](doc:contracts)

## `neuralHrtfMaxSections` {#symbol-neuralhrtfmaxsections}

```cpp
inline constexpr std::size_t neuralHrtfMaxSections = 32;
```

## `NeuralHrtfSection` {#symbol-neuralhrtfsection}

```cpp
struct NeuralHrtfSection;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `frequencyHz` | `double` | `1000` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `q` | `double` | `1` | Facteur de qualité du filtre positif et fini. |
| `gainDb` | `double` | `0` | Niveau logarithmique selon la référence dB indiquée. |

## `NeuralHrtfParameters` {#symbol-neuralhrtfparameters}

```cpp
struct NeuralHrtfParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `sections` | `std::array<std::array<NeuralHrtfSection, neuralHrtfMaxSections>, 2>` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `levelDb` | `std::array<double, 2>` | `{}` | Niveau logarithmique selon la référence dB indiquée. |
| `sectionCount` | `std::size_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `sampleRate` | `double` | `0` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `itdMicroseconds` | `double` | `0` | Durée ou position en secondes. |

## `NeuralHrtfInfo` {#symbol-neuralhrtfinfo}

```cpp
struct NeuralHrtfInfo;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `width` | `std::size_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `featureBands` | `std::size_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `sectionsPerEar` | `std::size_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `parameters` | `std::size_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `sampleRate` | `double` | `0` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `magnitudeOffsetDb` | `double` | `0` | Niveau logarithmique selon la référence dB indiquée. |
| `backend` | `CpuBackend` | `CpuBackend::scalar` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

## `NeuralHrtfModel` {#symbol-neuralhrtfmodel}

```cpp
class NeuralHrtfModel;
```

### `NeuralHrtfModel::load` {#symbol-neuralhrtfmodel-load}

```cpp
Status load(std::span<const std::byte>, CpuBackend = CpuBackend::automatic) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const std::byte>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `NeuralHrtfModel::info` {#symbol-neuralhrtfmodel-info}

```cpp
NeuralHrtfInfo info() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `NeuralHrtfInfo` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `NeuralHrtfModel::predict` {#symbol-neuralhrtfmodel-predict}

```cpp
Status predict(Vec3 listenerDirection, NeuralHrtfParameters&) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `listenerDirection` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `NeuralHrtfParameters &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `NeuralHrtfModel::predict` {#symbol-neuralhrtfmodel-predict-2}

```cpp
Status predict(std::span<const Vec3>, std::span<NeuralHrtfParameters>) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const Vec3>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<NeuralHrtfParameters>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `NeuralHrtfDesign` {#symbol-neuralhrtfdesign}

```cpp
struct NeuralHrtfDesign;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `sections` | `std::array<std::array<BiquadCoefficients, neuralHrtfMaxSections>, 2>` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `gain` | `std::array<double, 2>` | `{1, 1}` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `delayFrames` | `std::array<double, 2>` | `{}` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `sectionCount` | `std::size_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `sampleRate` | `double` | `0` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `desiredItdMicroseconds` | `double` | `0` | Durée ou position en secondes. |
| `intrinsicItdMicroseconds` | `double` | `0` | Durée ou position en secondes. |

## `designNeuralHrtf` {#symbol-designneuralhrtf}

```cpp
Status designNeuralHrtf(const NeuralHrtfParameters&, NeuralHrtfDesign&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const NeuralHrtfParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `NeuralHrtfDesign &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `NeuralBinauralOptions` {#symbol-neuralbinauraloptions}

```cpp
struct NeuralBinauralOptions;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `maximumDelaySeconds` | `double` | `.005` | Durée ou position en secondes. |
| `transitionHistoryFrames` | `std::size_t` | `512` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

## `NeuralBinaural` {#symbol-neuralbinaural}

```cpp
template <Sample T> class NeuralBinaural;
```

### `NeuralBinaural::NeuralBinaural<T>` {#symbol-neuralbinaural-neuralbinaural-t}

```cpp
NeuralBinaural();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `NeuralBinaural::~NeuralBinaural<T>` {#symbol-neuralbinaural-neuralbinaural-t-2}

```cpp
~NeuralBinaural();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `NeuralBinaural::NeuralBinaural<T>` {#symbol-neuralbinaural-neuralbinaural-t-3}

```cpp
NeuralBinaural(NeuralBinaural&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `NeuralBinaural<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `NeuralBinaural::operator=` {#symbol-neuralbinaural-operator}

```cpp
NeuralBinaural& operator=(NeuralBinaural&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `NeuralBinaural<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `NeuralBinaural<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `NeuralBinaural::prepare` {#symbol-neuralbinaural-prepare}

```cpp
Status prepare(const ProcessSpec& monoSpec, const NeuralHrtfModel&, const NeuralBinauralOptions& = {}) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `monoSpec` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `const NeuralHrtfModel &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `const NeuralBinauralOptions &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `NeuralBinaural::setDesign` {#symbol-neuralbinaural-setdesign}

```cpp
Status setDesign(const NeuralHrtfDesign&, std::size_t transitionFrames = 256) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const NeuralHrtfDesign &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `transitionFrames` | `std::size_t` | `256` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `NeuralBinaural::setDirection` {#symbol-neuralbinaural-setdirection}

```cpp
Status setDirection(Vec3, std::size_t transitionFrames = 256) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `transitionFrames` | `std::size_t` | `256` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `NeuralBinaural::process` {#symbol-neuralbinaural-process}

```cpp
Status process(AudioBlock<const T> mono, AudioBlock<T> stereo) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `mono` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `stereo` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `NeuralBinaural::processBatch` {#symbol-neuralbinaural-processbatch}

```cpp
static Status processBatch(std::span<NeuralBinaural> voices, AudioBlock<const T> sources, AudioBlock<T> stereoPairs) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `voices` | `std::span<NeuralBinaural<T>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `sources` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `stereoPairs` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `NeuralBinaural::setDesigns` {#symbol-neuralbinaural-setdesigns}

```cpp
static Status setDesigns(std::span<NeuralBinaural> voices, std::span<const NeuralHrtfDesign> designs, std::size_t transitionFrames = 256) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `voices` | `std::span<NeuralBinaural<T>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `designs` | `std::span<const NeuralHrtfDesign>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `transitionFrames` | `std::size_t` | `256` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `NeuralBinaural::reset` {#symbol-neuralbinaural-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `NeuralBinaural::transitionRemainingFrames` {#symbol-neuralbinaural-transitionremainingframes}

```cpp
std::size_t transitionRemainingFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `NeuralBinaural::latencyFrames` {#symbol-neuralbinaural-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
