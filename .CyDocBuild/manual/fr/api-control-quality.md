---
id: api-control-quality
title: Quality
description: Politiques explicites economy, realtimeHigh et offline, et transitions entre graphes préparés.
section: Référence API
order: 1109
sourceRevision: 1
parent: api-control
symbol: cydsp::QualityProfile
---

Politiques explicites economy, realtimeHigh et offline, et transitions entre graphes préparés.

## Inclusion {#include}

```cpp
#include <cydsp/control/Quality.h>
```

[Utilisation et contrats](doc:quality) · [Règles communes de traitement](doc:contracts)

## `QualityProfile` {#symbol-qualityprofile}

```cpp
enum class QualityProfile { economy, realtimeHigh, offline };
```

## `QualitySettings` {#symbol-qualitysettings}

```cpp
struct QualitySettings;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `profile` | `QualityProfile` | `QualityProfile::realtimeHigh` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `spectral` | `StftConfig` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `separation` | `HpssConfig` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `hrtf` | `HrtfPreparation` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `resampling` | `ResampleQuality` | `ResampleQuality::balanced` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `sampleInterpolation` | `SampleInterpolation` | `SampleInterpolation::bandLimited` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `reverb` | `FdnConfig` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `oversampling` | `unsigned int` | `4` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `truePeakFactor` | `unsigned int` | `4` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `saturationAntialias` | `bool` | `true` | Option ou prédicat vrai/faux explicite. |
| `saturationMath` | `MathAccuracy` | `MathAccuracy::precise` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `transientMath` | `MathAccuracy` | `MathAccuracy::precise` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `suggestedMaximumGrains` | `std::size_t` | `128` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `transitionFrames` | `std::size_t` | `256` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

## `qualitySettings` {#symbol-qualitysettings-2}

```cpp
Status qualitySettings(QualityProfile, double sampleRate, QualitySettings&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `QualityProfile` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `sampleRate` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `argument 3` | `QualitySettings &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `QualityGraph` {#symbol-qualitygraph}

```cpp
template <Sample T> class QualityGraph;
```

### `QualityGraph::QualityGraph<T>` {#symbol-qualitygraph-qualitygraph-t}

```cpp
QualityGraph();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `QualityGraph::~QualityGraph<T>` {#symbol-qualitygraph-qualitygraph-t-2}

```cpp
~QualityGraph();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `QualityGraph::QualityGraph<T>` {#symbol-qualitygraph-qualitygraph-t-3}

```cpp
QualityGraph(QualityGraph&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `QualityGraph<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `QualityGraph::operator=` {#symbol-qualitygraph-operator}

```cpp
QualityGraph& operator=(QualityGraph&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `QualityGraph<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `QualityGraph<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `QualityGraph::prepare` {#symbol-qualitygraph-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const DspBus> inputs, std::span<const DspBus> outputs, std::size_t latencyBudgetFrames, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `inputs` | `std::span<const DspBus>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `outputs` | `std::span<const DspBus>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `latencyBudgetFrames` | `std::size_t` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `QualityGraph::setInitial` {#symbol-qualitygraph-setinitial}

```cpp
Status setInitial(QualityProfile, MultiBusGraph<T>&&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `QualityProfile` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `MultiBusGraph<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `QualityGraph::stage` {#symbol-qualitygraph-stage}

```cpp
Status stage(QualityProfile, MultiBusGraph<T>&&) noexcept;
```

Préparer le stage hors traitement temps réel ; respecter la durée de vie des données/contextes de transition.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `QualityProfile` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `MultiBusGraph<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `QualityGraph::startTransition` {#symbol-qualitygraph-starttransition}

```cpp
Status startTransition(std::size_t fadeFrames = 256, std::size_t extraWarmupFrames = 0) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `fadeFrames` | `std::size_t` | `256` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `extraWarmupFrames` | `std::size_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `QualityGraph::process` {#symbol-qualitygraph-process}

```cpp
Status process(std::span<const AudioBlock<const T>>, std::span<AudioBlock<T>>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const AudioBlock<const T>>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `std::span<AudioBlock<T>>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `QualityGraph::reset` {#symbol-qualitygraph-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `QualityGraph::activeProfile` {#symbol-qualitygraph-activeprofile}

```cpp
QualityProfile activeProfile() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `QualityProfile` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `QualityGraph::transitioning` {#symbol-qualitygraph-transitioning}

```cpp
bool transitioning() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `QualityGraph::latencyFrames` {#symbol-qualitygraph-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
