---
id: api-core-spscqueue
title: SpscQueue
description: Communication à capacité fixe entre un producteur et un consommateur, avec stockage conservé.
section: Référence API
order: 1116
sourceRevision: 1
parent: api-core
symbol: cydsp::SpscQueue
---

Communication à capacité fixe entre un producteur et un consommateur, avec stockage conservé.

## Inclusion {#include}

```cpp
#include <cydsp/core/SpscQueue.h>
```

[Utilisation et contrats](doc:threading) · [Règles communes de traitement](doc:contracts)

## `SpscQueue` {#symbol-spscqueue}

```cpp
template <class T> requires(std::is_trivially_copyable_v<T> && std::is_default_constructible_v<T>) class SpscQueue;
```

### `SpscQueue::prepare` {#symbol-spscqueue-prepare}

```cpp
Status prepare(std::size_t capacity) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `capacity` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SpscQueue::push` {#symbol-spscqueue-push}

```cpp
bool push(const T& value) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `value` | `const T &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SpscQueue::peek` {#symbol-spscqueue-peek}

```cpp
const T* peek() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `const T *` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SpscQueue::discard` {#symbol-spscqueue-discard}

```cpp
bool discard() noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SpscQueue::pop` {#symbol-spscqueue-pop}

```cpp
bool pop(T& value) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `value` | `T &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SpscQueue::capacity` {#symbol-spscqueue-capacity}

```cpp
std::size_t capacity() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
