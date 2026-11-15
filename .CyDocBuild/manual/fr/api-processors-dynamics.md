---
id: api-processors-dynamics
title: Dynamics
description: Compresseur, expandeur et gate avec détection peak/RMS, sidechain et anticipation préparée.
section: Référence API
order: 1138
sourceRevision: 1
parent: api-processors
symbol: cydsp::Dynamics
---

Compresseur, expandeur et gate avec détection peak/RMS, sidechain et anticipation préparée.

## Inclusion {#include}

```cpp
#include <cydsp/processors/Dynamics.h>
```

[Utilisation et contrats](doc:dynamics) · [Règles communes de traitement](doc:contracts)

## `DetectorMode` {#symbol-detectormode}

```cpp
enum class DetectorMode { peak, rms };
```

## `DynamicsMode` {#symbol-dynamicsmode}

```cpp
enum class DynamicsMode { compressor, expander, noiseGate, limiter };
```

## `DynamicsParameters` {#symbol-dynamicsparameters}

```cpp
struct DynamicsParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `thresholdDb` | `double` | `-18` | Niveau logarithmique selon la référence dB indiquée. |
| `ratio` | `double` | `4` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `kneeDb` | `double` | `6` | Niveau logarithmique selon la référence dB indiquée. |
| `attackMs` | `double` | `10` | Durée en millisecondes. |
| `releaseMs` | `double` | `100` | Durée en millisecondes. |
| `makeupDb` | `double` | `0` | Niveau logarithmique selon la référence dB indiquée. |
| `linkedChannels` | `bool` | `true` | Option ou prédicat vrai/faux explicite. |
| `detector` | `DetectorMode` | `DetectorMode::peak` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `holdMs` | `double` | `0` | Durée en millisecondes. |
| `hysteresisDb` | `double` | `0` | Niveau logarithmique selon la référence dB indiquée. |
| `lookAheadMs` | `double` | `0` | Durée en millisecondes. |

## `BallisticsFilter` {#symbol-ballisticsfilter}

```cpp
template <Sample T> class BallisticsFilter;
```

### `BallisticsFilter::BallisticsFilter<T>` {#symbol-ballisticsfilter-ballisticsfilter-t}

```cpp
BallisticsFilter();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `BallisticsFilter::~BallisticsFilter<T>` {#symbol-ballisticsfilter-ballisticsfilter-t-2}

```cpp
~BallisticsFilter();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `BallisticsFilter::BallisticsFilter<T>` {#symbol-ballisticsfilter-ballisticsfilter-t-3}

```cpp
BallisticsFilter(BallisticsFilter&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `BallisticsFilter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `BallisticsFilter::operator=` {#symbol-ballisticsfilter-operator}

```cpp
BallisticsFilter& operator=(BallisticsFilter&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `BallisticsFilter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `BallisticsFilter<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `BallisticsFilter::prepare` {#symbol-ballisticsfilter-prepare}

```cpp
Status prepare(const ProcessSpec&, DetectorMode = DetectorMode::peak) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `DetectorMode` | `DetectorMode::peak` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `BallisticsFilter::setTimes` {#symbol-ballisticsfilter-settimes}

```cpp
Status setTimes(double attackMs, double releaseMs) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `attackMs` | `double` | — | Durée en millisecondes. |
| `releaseMs` | `double` | — | Durée en millisecondes. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `BallisticsFilter::reset` {#symbol-ballisticsfilter-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `BallisticsFilter::process` {#symbol-ballisticsfilter-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `Dynamics` {#symbol-dynamics}

```cpp
template <Sample T> class Dynamics;
```

### `Dynamics::Dynamics<T>` {#symbol-dynamics-dynamics-t}

```cpp
Dynamics();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `Dynamics::~Dynamics<T>` {#symbol-dynamics-dynamics-t-2}

```cpp
~Dynamics();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `Dynamics::Dynamics<T>` {#symbol-dynamics-dynamics-t-3}

```cpp
Dynamics(Dynamics&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Dynamics<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `Dynamics::operator=` {#symbol-dynamics-operator}

```cpp
Dynamics& operator=(Dynamics&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Dynamics<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Dynamics<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Dynamics::prepare` {#symbol-dynamics-prepare}

```cpp
Status prepare(const ProcessSpec&, DynamicsMode = DynamicsMode::compressor, double maximumLookAheadMs = 20) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `DynamicsMode` | `DynamicsMode::compressor` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `maximumLookAheadMs` | `double` | `20` | Durée en millisecondes. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Dynamics::setParameters` {#symbol-dynamics-setparameters}

```cpp
Status setParameters(const DynamicsParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const DynamicsParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Dynamics::reset` {#symbol-dynamics-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `Dynamics::process` {#symbol-dynamics-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Dynamics::process` {#symbol-dynamics-process-2}

```cpp
Status process(AudioBlock<const T> program, AudioBlock<const T> sidechain, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `program` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `sidechain` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 3` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Dynamics::processMixed` {#symbol-dynamics-processmixed}

```cpp
Status processMixed(AudioBlock<const T>, AudioBlock<T>, double dryGain, double wetGain) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |
| `dryGain` | `double` | — | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `wetGain` | `double` | — | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Dynamics::processMixed` {#symbol-dynamics-processmixed-2}

```cpp
Status processMixed(AudioBlock<const T>, AudioBlock<const T> sidechain, AudioBlock<T>, double dryGain, double wetGain) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `sidechain` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 3` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |
| `dryGain` | `double` | — | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `wetGain` | `double` | — | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Dynamics::gainReductionDb` {#symbol-dynamics-gainreductiondb}

```cpp
double gainReductionDb() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Dynamics::minimumAppliedGain` {#symbol-dynamics-minimumappliedgain}

```cpp
double minimumAppliedGain() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Dynamics::latencyFrames` {#symbol-dynamics-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
