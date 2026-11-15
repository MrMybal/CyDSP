---
id: api-core-memory
title: Memory
description: Détection du recouvrement des plages mémoire empruntées.
section: Référence API
order: 1115
sourceRevision: 1
parent: api-core
symbol: cydsp::memory::overlaps
---

Détection du recouvrement des plages mémoire empruntées.

## Inclusion {#include}

```cpp
#include <cydsp/core/Memory.h>
```

[Utilisation et contrats](doc:buffers) · [Règles communes de traitement](doc:contracts)

## `memory::overlaps` {#symbol-memory-overlaps}

```cpp
inline bool overlaps(const void* a, std::size_t bytesA, const void* b, std::size_t bytesB) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `a` | `const void *` | — | Contexte de callback emprunté ; l’application possède durée de vie et synchronisation. |
| `bytesA` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `b` | `const void *` | — | Contexte de callback emprunté ; l’application possède durée de vie et synchronisation. |
| `bytesB` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
