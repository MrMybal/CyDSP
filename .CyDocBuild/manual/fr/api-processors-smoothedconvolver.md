---
id: api-processors-smoothedconvolver
title: SmoothedConvolver
description: Préparation d’une nouvelle IR hors thread audio, préchauffage et fondu de plans conservés.
section: Référence API
order: 1165
sourceRevision: 1
parent: api-processors
symbol: cydsp::SmoothedConvolver
---

Préparation d’une nouvelle IR hors thread audio, préchauffage et fondu de plans conservés.

## Inclusion {#include}

```cpp
#include <cydsp/processors/SmoothedConvolver.h>
```

[Utilisation et contrats](doc:convolution) · [Règles communes de traitement](doc:contracts)

## `SmoothedConvolver` {#symbol-smoothedconvolver}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class SmoothedConvolver;
```

### `SmoothedConvolver::SmoothedConvolver<T>` {#symbol-smoothedconvolver-smoothedconvolver-t}

```cpp
SmoothedConvolver() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `SmoothedConvolver::SmoothedConvolver<T>` {#symbol-smoothedconvolver-smoothedconvolver-t-2}

```cpp
SmoothedConvolver(const SmoothedConvolver&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const SmoothedConvolver<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `SmoothedConvolver::operator=` {#symbol-smoothedconvolver-operator}

```cpp
SmoothedConvolver& operator=(const SmoothedConvolver&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const SmoothedConvolver<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `SmoothedConvolver<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedConvolver::SmoothedConvolver<T>` {#symbol-smoothedconvolver-smoothedconvolver-t-3}

```cpp
SmoothedConvolver(SmoothedConvolver&& other) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `SmoothedConvolver<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `SmoothedConvolver::operator=` {#symbol-smoothedconvolver-operator-2}

```cpp
SmoothedConvolver& operator=(SmoothedConvolver&& other) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `SmoothedConvolver<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `SmoothedConvolver<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedConvolver::prepare` {#symbol-smoothedconvolver-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const T> impulse, std::size_t partitionFrames = 128, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `impulse` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `partitionFrames` | `std::size_t` | `128` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SmoothedConvolver::stageImpulse` {#symbol-smoothedconvolver-stageimpulse}

```cpp
Status stageImpulse(std::span<const T>) noexcept;
```

Préparer le stage hors traitement temps réel ; respecter la durée de vie des données/contextes de transition.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SmoothedConvolver::startTransition` {#symbol-smoothedconvolver-starttransition}

```cpp
Status startTransition(std::size_t frames = 256, bool prewarm = true) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `frames` | `std::size_t` | `256` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `prewarm` | `bool` | `true` | Option ou prédicat vrai/faux explicite. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SmoothedConvolver::discardStagedImpulse` {#symbol-smoothedconvolver-discardstagedimpulse}

```cpp
void discardStagedImpulse() noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

### `SmoothedConvolver::prepare` {#symbol-smoothedconvolver-prepare-2}

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

### `SmoothedConvolver::configuration` {#symbol-smoothedconvolver-configuration}

```cpp
ConvolutionConfig configuration() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `ConvolutionConfig` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedConvolver::reset` {#symbol-smoothedconvolver-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `SmoothedConvolver::process` {#symbol-smoothedconvolver-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SmoothedConvolver::hasStagedImpulse` {#symbol-smoothedconvolver-hasstagedimpulse}

```cpp
bool hasStagedImpulse() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedConvolver::isTransitioning` {#symbol-smoothedconvolver-istransitioning}

```cpp
bool isTransitioning() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedConvolver::warmupRemainingFrames` {#symbol-smoothedconvolver-warmupremainingframes}

```cpp
std::size_t warmupRemainingFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedConvolver::transitionRemainingFrames` {#symbol-smoothedconvolver-transitionremainingframes}

```cpp
std::size_t transitionRemainingFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedConvolver::latencyFrames` {#symbol-smoothedconvolver-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedConvolver::tailFrames` {#symbol-smoothedconvolver-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedConvolver::partitionFrames` {#symbol-smoothedconvolver-partitionframes}

```cpp
std::size_t partitionFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedConvolver::backend` {#symbol-smoothedconvolver-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
