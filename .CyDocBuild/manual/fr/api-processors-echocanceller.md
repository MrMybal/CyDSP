---
id: api-processors-echocanceller
title: EchoCanceller
description: Chemins NLMS indépendants par microphone, référence mono alignée et gel lié en double parole.
section: Référence API
order: 1139
sourceRevision: 1
parent: api-processors
symbol: cydsp::EchoCanceller
---

Chemins NLMS indépendants par microphone, référence mono alignée et gel lié en double parole.

## Inclusion {#include}

```cpp
#include <cydsp/processors/EchoCanceller.h>
```

[Utilisation et contrats](doc:echo) · [Règles communes de traitement](doc:contracts)

## `EchoCancellationConfig` {#symbol-echocancellationconfig}

```cpp
struct EchoCancellationConfig;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `taps` | `std::size_t` | `512` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `referenceDelayFrames` | `std::size_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `initialLearningMs` | `double` | `250` | Durée en millisecondes. |

## `EchoCancellationParameters` {#symbol-echocancellationparameters}

```cpp
struct EchoCancellationParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `step` | `double` | `.25` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `doubleTalkRatio` | `double` | `2.5` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `hangoverMs` | `double` | `100` | Durée en millisecondes. |
| `minimumReferencePower` | `double` | `1e-8` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `adaptationEnabled` | `bool` | `true` | Option ou prédicat vrai/faux explicite. |
| `forceAdaptation` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

## `EchoCancellationMetrics` {#symbol-echocancellationmetrics}

```cpp
struct EchoCancellationMetrics;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `referenceActive` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `doubleTalk` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `adapting` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `echoReturnLossEnhancementDb` | `double` | `0` | Niveau logarithmique selon la référence dB indiquée. |
| `residualPower` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `adaptedFrames` | `std::uint64_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `frozenFrames` | `std::uint64_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

## `EchoCanceller` {#symbol-echocanceller}

```cpp
template <Sample T> class EchoCanceller;
```

### `EchoCanceller::EchoCanceller<T>` {#symbol-echocanceller-echocanceller-t}

```cpp
EchoCanceller();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `EchoCanceller::~EchoCanceller<T>` {#symbol-echocanceller-echocanceller-t-2}

```cpp
~EchoCanceller();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `EchoCanceller::EchoCanceller<T>` {#symbol-echocanceller-echocanceller-t-3}

```cpp
EchoCanceller(EchoCanceller&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `EchoCanceller<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `EchoCanceller::operator=` {#symbol-echocanceller-operator}

```cpp
EchoCanceller& operator=(EchoCanceller&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `EchoCanceller<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `EchoCanceller<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `EchoCanceller::prepare` {#symbol-echocanceller-prepare}

```cpp
Status prepare(const ProcessSpec& microphones, const EchoCancellationConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `microphones` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `const EchoCancellationConfig &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `EchoCanceller::setParameters` {#symbol-echocanceller-setparameters}

```cpp
Status setParameters(const EchoCancellationParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const EchoCancellationParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `EchoCanceller::process` {#symbol-echocanceller-process}

```cpp
Status process(AudioBlock<const T> farEnd, AudioBlock<const T> microphones, AudioBlock<T> residual) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `farEnd` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `microphones` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `residual` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `EchoCanceller::metrics` {#symbol-echocanceller-metrics}

```cpp
EchoCancellationMetrics metrics() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `EchoCancellationMetrics` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `EchoCanceller::coefficients` {#symbol-echocanceller-coefficients}

```cpp
std::span<const double> coefficients(std::size_t channel) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Indice/compte de canaux dans la disposition planaire déclarée. |

Retour : `std::span<const double>` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `EchoCanceller::reset` {#symbol-echocanceller-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.
