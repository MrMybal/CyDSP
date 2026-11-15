---
id: api-processors-asyncconvolver
title: AsyncConvolver
description: Un producteur prépare les IR pendant qu’un consommateur audio utilise les plans conservés sans attente.
section: Référence API
order: 1127
sourceRevision: 1
parent: api-processors
symbol: cydsp::AsyncConvolver
---

Un producteur prépare les IR pendant qu’un consommateur audio utilise les plans conservés sans attente.

## Inclusion {#include}

```cpp
#include <cydsp/processors/AsyncConvolver.h>
```

[Utilisation et contrats](doc:threading) · [Règles communes de traitement](doc:contracts)

## `AsyncConvolver` {#symbol-asyncconvolver}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class AsyncConvolver;
```

### `AsyncConvolver::AsyncConvolver<T>` {#symbol-asyncconvolver-asyncconvolver-t}

```cpp
AsyncConvolver() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `AsyncConvolver::AsyncConvolver<T>` {#symbol-asyncconvolver-asyncconvolver-t-2}

```cpp
AsyncConvolver(const AsyncConvolver&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const AsyncConvolver<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `AsyncConvolver::operator=` {#symbol-asyncconvolver-operator}

```cpp
AsyncConvolver& operator=(const AsyncConvolver&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const AsyncConvolver<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `AsyncConvolver<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AsyncConvolver::AsyncConvolver<T>` {#symbol-asyncconvolver-asyncconvolver-t-3}

```cpp
AsyncConvolver(AsyncConvolver&&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AsyncConvolver<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `AsyncConvolver::operator=` {#symbol-asyncconvolver-operator-2}

```cpp
AsyncConvolver& operator=(AsyncConvolver&&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AsyncConvolver<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `AsyncConvolver<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AsyncConvolver::prepare` {#symbol-asyncconvolver-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const T>, std::size_t partition = 128, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `partition` | `std::size_t` | `128` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AsyncConvolver::stageImpulse` {#symbol-asyncconvolver-stageimpulse}

```cpp
Status stageImpulse(std::span<const T>, std::size_t transitionFrames = 256, bool prewarm = true) noexcept;
```

Préparer le stage hors traitement temps réel ; respecter la durée de vie des données/contextes de transition.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `transitionFrames` | `std::size_t` | `256` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `prewarm` | `bool` | `true` | Option ou prédicat vrai/faux explicite. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AsyncConvolver::prepare` {#symbol-asyncconvolver-prepare-2}

```cpp
Status prepare(const ProcessSpec&, std::span<const T>, const ConvolutionConfig&, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 3` | `const ConvolutionConfig &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AsyncConvolver::configuration` {#symbol-asyncconvolver-configuration}

```cpp
ConvolutionConfig configuration() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `ConvolutionConfig` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AsyncConvolver::reset` {#symbol-asyncconvolver-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `AsyncConvolver::process` {#symbol-asyncconvolver-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AsyncConvolver::hasPendingImpulse` {#symbol-asyncconvolver-haspendingimpulse}

```cpp
bool hasPendingImpulse() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AsyncConvolver::isTransitioning` {#symbol-asyncconvolver-istransitioning}

```cpp
bool isTransitioning() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AsyncConvolver::warmupRemainingFrames` {#symbol-asyncconvolver-warmupremainingframes}

```cpp
std::size_t warmupRemainingFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AsyncConvolver::tailFrames` {#symbol-asyncconvolver-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AsyncConvolver::latencyFrames` {#symbol-asyncconvolver-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
