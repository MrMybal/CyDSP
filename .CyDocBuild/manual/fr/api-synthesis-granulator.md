---
id: api-synthesis-granulator
title: Granulator
description: Voix de grains bornées sur samples préparés et nuages planifiés reproductibles.
section: Référence API
order: 1183
sourceRevision: 1
parent: api-synthesis
symbol: cydsp::Granulator
---

Voix de grains bornées sur samples préparés et nuages planifiés reproductibles.

## Inclusion {#include}

```cpp
#include <cydsp/synthesis/Granulator.h>
```

[Utilisation et contrats](doc:granular) · [Règles communes de traitement](doc:contracts)

## `GrainEnvelope` {#symbol-grainenvelope}

```cpp
enum class GrainEnvelope { hann, triangular, rectangular };
```

## `GrainParameters` {#symbol-grainparameters}

```cpp
struct GrainParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `positionSeconds` | `double` | `0` | Durée ou position en secondes. |
| `durationMs` | `double` | `80` | Durée en millisecondes. |
| `pitchRatio` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `gain` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `pan` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `wrap` | `bool` | `true` | Option ou prédicat vrai/faux explicite. |
| `envelope` | `GrainEnvelope` | `GrainEnvelope::hann` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `interpolation` | `SampleInterpolation` | `SampleInterpolation::bandLimited` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `GrainCloudParameters` {#symbol-graincloudparameters}

```cpp
struct GrainCloudParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `grain` | `GrainParameters` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `grainsPerSecond` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `positionSpreadSeconds` | `double` | `0` | Durée ou position en secondes. |
| `pitchSpreadSemitones` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `panSpread` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `Granulator` {#symbol-granulator}

```cpp
template <Sample T> class Granulator;
```

### `Granulator::Granulator<T>` {#symbol-granulator-granulator-t}

```cpp
Granulator();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `Granulator::~Granulator<T>` {#symbol-granulator-granulator-t-2}

```cpp
~Granulator();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `Granulator::Granulator<T>` {#symbol-granulator-granulator-t-3}

```cpp
Granulator(Granulator&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Granulator<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `Granulator::operator=` {#symbol-granulator-operator}

```cpp
Granulator& operator=(Granulator&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Granulator<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Granulator<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Granulator::prepare` {#symbol-granulator-prepare}

```cpp
Status prepare(const ProcessSpec&, const SampleData<T>&, std::size_t maximumGrains = 128, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `const SampleData<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `maximumGrains` | `std::size_t` | `128` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Granulator::trigger` {#symbol-granulator-trigger}

```cpp
Status trigger(const GrainParameters&, std::size_t frameOffset = 0) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const GrainParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `frameOffset` | `std::size_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Granulator::setCloud` {#symbol-granulator-setcloud}

```cpp
Status setCloud(const GrainCloudParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const GrainCloudParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Granulator::reset` {#symbol-granulator-reset}

```cpp
void reset(std::uint64_t seed = 1) noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `seed` | `std::uint64_t` | `1` | Graine aléatoire déterministe par instance. |

### `Granulator::process` {#symbol-granulator-process}

```cpp
Status process(AudioBlock<T>, bool add = false) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |
| `add` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Granulator::activeGrains` {#symbol-granulator-activegrains}

```cpp
std::size_t activeGrains() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Granulator::droppedGrains` {#symbol-granulator-droppedgrains}

```cpp
std::uint64_t droppedGrains() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::uint64_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Granulator::backend` {#symbol-granulator-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
