---
id: api-synthesis-livegranulator
title: LiveGranulator
description: Historique circulaire live préparé, freeze et grains avant/arrière routés indépendamment.
section: Référence API
order: 1184
sourceRevision: 1
parent: api-synthesis
symbol: cydsp::LiveGranulator
---

Historique circulaire live préparé, freeze et grains avant/arrière routés indépendamment.

## Inclusion {#include}

```cpp
#include <cydsp/synthesis/LiveGranulator.h>
```

[Utilisation et contrats](doc:granular) · [Règles communes de traitement](doc:contracts)

## `LiveGrainParameters` {#symbol-livegrainparameters}

```cpp
struct LiveGrainParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `ageSeconds` | `double` | `.1` | Durée ou position en secondes. |
| `durationMs` | `double` | `80` | Durée en millisecondes. |
| `pitchRatio` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `gain` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `inputChannel` | `std::size_t` | `0` | Indice/compte de canaux dans la disposition planaire déclarée. |
| `channelGains` | `std::array<double, 8>` | `{1, 0, 0, 0, 0, 0, 0, 0}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `LiveCloudParameters` {#symbol-livecloudparameters}

```cpp
struct LiveCloudParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `grain` | `LiveGrainParameters` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `grainsPerSecond` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `ageSpreadSeconds` | `double` | `0` | Durée ou position en secondes. |
| `pitchSpreadSemitones` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `LiveGranulator` {#symbol-livegranulator}

```cpp
template <Sample T> class LiveGranulator;
```

### `LiveGranulator::LiveGranulator<T>` {#symbol-livegranulator-livegranulator-t}

```cpp
LiveGranulator();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `LiveGranulator::~LiveGranulator<T>` {#symbol-livegranulator-livegranulator-t-2}

```cpp
~LiveGranulator();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `LiveGranulator::LiveGranulator<T>` {#symbol-livegranulator-livegranulator-t-3}

```cpp
LiveGranulator(LiveGranulator&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `LiveGranulator<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `LiveGranulator::operator=` {#symbol-livegranulator-operator}

```cpp
LiveGranulator& operator=(LiveGranulator&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `LiveGranulator<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `LiveGranulator<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `LiveGranulator::prepare` {#symbol-livegranulator-prepare}

```cpp
Status prepare(const ProcessSpec& input, std::size_t outputChannels = 2, double historySeconds = 4, std::size_t maximumGrains = 128, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `input` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `outputChannels` | `std::size_t` | `2` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `historySeconds` | `double` | `4` | Durée ou position en secondes. |
| `maximumGrains` | `std::size_t` | `128` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `LiveGranulator::trigger` {#symbol-livegranulator-trigger}

```cpp
Status trigger(const LiveGrainParameters&, std::size_t frameOffset = 0) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const LiveGrainParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `frameOffset` | `std::size_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `LiveGranulator::setCloud` {#symbol-livegranulator-setcloud}

```cpp
Status setCloud(const LiveCloudParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const LiveCloudParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `LiveGranulator::setFrozen` {#symbol-livegranulator-setfrozen}

```cpp
void setFrozen(bool) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `bool` | — | Option ou prédicat vrai/faux explicite. |

### `LiveGranulator::process` {#symbol-livegranulator-process}

```cpp
Status process(AudioBlock<const T> capture, AudioBlock<T> output) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `capture` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `output` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `LiveGranulator::reset` {#symbol-livegranulator-reset}

```cpp
void reset(std::uint64_t seed = 1) noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `seed` | `std::uint64_t` | `1` | Graine aléatoire déterministe par instance. |

### `LiveGranulator::activeGrains` {#symbol-livegranulator-activegrains}

```cpp
std::size_t activeGrains() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `LiveGranulator::droppedGrains` {#symbol-livegranulator-droppedgrains}

```cpp
std::uint64_t droppedGrains() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::uint64_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
