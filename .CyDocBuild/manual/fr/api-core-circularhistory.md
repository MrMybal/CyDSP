---
id: api-core-circularhistory
title: CircularHistory
description: Un historique préparé partagé par des lecteurs entiers et fractionnaires indépendants.
section: Référence API
order: 1113
sourceRevision: 1
parent: api-core
symbol: cydsp::CircularHistory
---

Un historique préparé partagé par des lecteurs entiers et fractionnaires indépendants.

## Inclusion {#include}

```cpp
#include <cydsp/core/CircularHistory.h>
```

[Utilisation et contrats](doc:delay) · [Règles communes de traitement](doc:contracts)

## `HistoryInterpolation` {#symbol-historyinterpolation}

```cpp
enum class HistoryInterpolation { nearest, linear, lagrange3, catmullRom };
```

## `CircularHistory` {#symbol-circularhistory}

```cpp
template <Sample T> class CircularHistory;
```

### `CircularHistory::CircularHistory<T>` {#symbol-circularhistory-circularhistory-t}

```cpp
CircularHistory() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `CircularHistory::CircularHistory<T>` {#symbol-circularhistory-circularhistory-t-2}

```cpp
CircularHistory(const CircularHistory&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const CircularHistory<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `CircularHistory::operator=` {#symbol-circularhistory-operator}

```cpp
CircularHistory& operator=(const CircularHistory&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const CircularHistory<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `CircularHistory<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `CircularHistory::CircularHistory<T>` {#symbol-circularhistory-circularhistory-t-3}

```cpp
CircularHistory(CircularHistory&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `CircularHistory<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `CircularHistory::operator=` {#symbol-circularhistory-operator-2}

```cpp
CircularHistory& operator=(CircularHistory&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `CircularHistory<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `CircularHistory<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `CircularHistory::prepare` {#symbol-circularhistory-prepare}

```cpp
Status prepare(std::size_t channels, std::size_t historyFrames) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `channels` | `std::size_t` | — | Indice/compte de canaux dans la disposition planaire déclarée. |
| `historyFrames` | `std::size_t` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `CircularHistory::pushFrame` {#symbol-circularhistory-pushframe}

```cpp
Status pushFrame(std::span<const T>) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `CircularHistory::push` {#symbol-circularhistory-push}

```cpp
Status push(AudioBlock<const T>) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `CircularHistory::read` {#symbol-circularhistory-read}

```cpp
T read(std::size_t channel, double ageFrames, HistoryInterpolation = HistoryInterpolation::linear) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Indice/compte de canaux dans la disposition planaire déclarée. |
| `ageFrames` | `double` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `argument 3` | `HistoryInterpolation` | `HistoryInterpolation::linear` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `T` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `CircularHistory::readWindow` {#symbol-circularhistory-readwindow}

```cpp
Status readWindow(std::size_t channel, std::ptrdiff_t firstAge, std::span<double> output) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Indice/compte de canaux dans la disposition planaire déclarée. |
| `firstAge` | `std::ptrdiff_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `output` | `std::span<double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `CircularHistory::reset` {#symbol-circularhistory-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `CircularHistory::channels` {#symbol-circularhistory-channels}

```cpp
std::size_t channels() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `CircularHistory::capacity` {#symbol-circularhistory-capacity}

```cpp
std::size_t capacity() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `CircularHistory::availableFrames` {#symbol-circularhistory-availableframes}

```cpp
std::size_t availableFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
