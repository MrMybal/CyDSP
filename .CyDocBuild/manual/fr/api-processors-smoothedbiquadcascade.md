---
id: api-processors-smoothedbiquadcascade
title: SmoothedBiquadCascade
description: Transitions de cascades préparées avec regroupement de la cible suivante.
section: Référence API
order: 1164
sourceRevision: 1
parent: api-processors
symbol: cydsp::SmoothedBiquadCascade
---

Transitions de cascades préparées avec regroupement de la cible suivante.

## Inclusion {#include}

```cpp
#include <cydsp/processors/SmoothedBiquadCascade.h>
```

[Utilisation et contrats](doc:filters) · [Règles communes de traitement](doc:contracts)

## `SmoothedBiquadCascade` {#symbol-smoothedbiquadcascade}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class SmoothedBiquadCascade;
```

### `SmoothedBiquadCascade::SmoothedBiquadCascade<T>` {#symbol-smoothedbiquadcascade-smoothedbiquadcascade-t}

```cpp
SmoothedBiquadCascade() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `SmoothedBiquadCascade::SmoothedBiquadCascade<T>` {#symbol-smoothedbiquadcascade-smoothedbiquadcascade-t-2}

```cpp
SmoothedBiquadCascade(const SmoothedBiquadCascade&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const SmoothedBiquadCascade<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `SmoothedBiquadCascade::operator=` {#symbol-smoothedbiquadcascade-operator}

```cpp
SmoothedBiquadCascade& operator=(const SmoothedBiquadCascade&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const SmoothedBiquadCascade<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `SmoothedBiquadCascade<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedBiquadCascade::SmoothedBiquadCascade<T>` {#symbol-smoothedbiquadcascade-smoothedbiquadcascade-t-3}

```cpp
SmoothedBiquadCascade(SmoothedBiquadCascade&& other) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `SmoothedBiquadCascade<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `SmoothedBiquadCascade::operator=` {#symbol-smoothedbiquadcascade-operator-2}

```cpp
SmoothedBiquadCascade& operator=(SmoothedBiquadCascade&& other) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `SmoothedBiquadCascade<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `SmoothedBiquadCascade<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedBiquadCascade::prepare` {#symbol-smoothedbiquadcascade-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t maxSections = 16, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `maxSections` | `std::size_t` | `16` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SmoothedBiquadCascade::setCoefficients` {#symbol-smoothedbiquadcascade-setcoefficients}

```cpp
Status setCoefficients(std::span<const BiquadCoefficients>, std::size_t transitionFrames = 128) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const BiquadCoefficients>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `transitionFrames` | `std::size_t` | `128` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SmoothedBiquadCascade::reset` {#symbol-smoothedbiquadcascade-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `SmoothedBiquadCascade::process` {#symbol-smoothedbiquadcascade-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SmoothedBiquadCascade::maxSections` {#symbol-smoothedbiquadcascade-maxsections}

```cpp
std::size_t maxSections() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedBiquadCascade::targetSections` {#symbol-smoothedbiquadcascade-targetsections}

```cpp
std::size_t targetSections() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedBiquadCascade::isTransitioning` {#symbol-smoothedbiquadcascade-istransitioning}

```cpp
bool isTransitioning() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedBiquadCascade::transitionRemainingFrames` {#symbol-smoothedbiquadcascade-transitionremainingframes}

```cpp
std::size_t transitionRemainingFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedBiquadCascade::backend` {#symbol-smoothedbiquadcascade-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
