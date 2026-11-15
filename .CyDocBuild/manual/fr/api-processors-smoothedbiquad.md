---
id: api-processors-smoothedbiquad
title: SmoothedBiquad
description: Fondu entre états de filtres stables, sans interpolation de coefficients potentiellement instables.
section: Référence API
order: 1163
sourceRevision: 1
parent: api-processors
symbol: cydsp::SmoothedBiquad
---

Fondu entre états de filtres stables, sans interpolation de coefficients potentiellement instables.

## Inclusion {#include}

```cpp
#include <cydsp/processors/SmoothedBiquad.h>
```

[Utilisation et contrats](doc:filters) · [Règles communes de traitement](doc:contracts)

## `SmoothedBiquad` {#symbol-smoothedbiquad}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class SmoothedBiquad;
```

### `SmoothedBiquad::SmoothedBiquad<T>` {#symbol-smoothedbiquad-smoothedbiquad-t}

```cpp
SmoothedBiquad() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `SmoothedBiquad::SmoothedBiquad<T>` {#symbol-smoothedbiquad-smoothedbiquad-t-2}

```cpp
SmoothedBiquad(const SmoothedBiquad&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const SmoothedBiquad<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `SmoothedBiquad::operator=` {#symbol-smoothedbiquad-operator}

```cpp
SmoothedBiquad& operator=(const SmoothedBiquad&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const SmoothedBiquad<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `SmoothedBiquad<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedBiquad::SmoothedBiquad<T>` {#symbol-smoothedbiquad-smoothedbiquad-t-3}

```cpp
SmoothedBiquad(SmoothedBiquad&& other) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `SmoothedBiquad<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `SmoothedBiquad::operator=` {#symbol-smoothedbiquad-operator-2}

```cpp
SmoothedBiquad& operator=(SmoothedBiquad&& other) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `SmoothedBiquad<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `SmoothedBiquad<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedBiquad::prepare` {#symbol-smoothedbiquad-prepare}

```cpp
Status prepare(const ProcessSpec&, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SmoothedBiquad::setCoefficients` {#symbol-smoothedbiquad-setcoefficients}

```cpp
Status setCoefficients(const BiquadCoefficients&, std::size_t transitionFrames = 64) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const BiquadCoefficients &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `transitionFrames` | `std::size_t` | `64` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SmoothedBiquad::reset` {#symbol-smoothedbiquad-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `SmoothedBiquad::process` {#symbol-smoothedbiquad-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SmoothedBiquad::isTransitioning` {#symbol-smoothedbiquad-istransitioning}

```cpp
bool isTransitioning() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedBiquad::hasQueuedTransition` {#symbol-smoothedbiquad-hasqueuedtransition}

```cpp
bool hasQueuedTransition() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedBiquad::transitionRemainingFrames` {#symbol-smoothedbiquad-transitionremainingframes}

```cpp
std::size_t transitionRemainingFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedBiquad::targetCoefficients` {#symbol-smoothedbiquad-targetcoefficients}

```cpp
const BiquadCoefficients& targetCoefficients() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `const BiquadCoefficients &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedBiquad::backend` {#symbol-smoothedbiquad-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
