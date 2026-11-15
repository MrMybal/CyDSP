---
id: api-processors-nonuniformconvolver
title: NonuniformConvolver
description: Partitions précoces et tardives de tailles différentes pour réduire le coût des longues IR.
section: Référence API
order: 1153
sourceRevision: 1
parent: api-processors
symbol: cydsp::NonuniformConvolver
---

Partitions précoces et tardives de tailles différentes pour réduire le coût des longues IR.

## Inclusion {#include}

```cpp
#include <cydsp/processors/NonuniformConvolver.h>
```

[Utilisation et contrats](doc:convolution) · [Règles communes de traitement](doc:contracts)

## `NonuniformConvolver` {#symbol-nonuniformconvolver}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class NonuniformConvolver;
```

### `NonuniformConvolver::NonuniformConvolver<T>` {#symbol-nonuniformconvolver-nonuniformconvolver-t}

```cpp
NonuniformConvolver() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `NonuniformConvolver::NonuniformConvolver<T>` {#symbol-nonuniformconvolver-nonuniformconvolver-t-2}

```cpp
NonuniformConvolver(const NonuniformConvolver&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const NonuniformConvolver<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `NonuniformConvolver::operator=` {#symbol-nonuniformconvolver-operator}

```cpp
NonuniformConvolver& operator=(const NonuniformConvolver&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const NonuniformConvolver<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `NonuniformConvolver<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `NonuniformConvolver::NonuniformConvolver<T>` {#symbol-nonuniformconvolver-nonuniformconvolver-t-3}

```cpp
NonuniformConvolver(NonuniformConvolver&& other) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `NonuniformConvolver<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `NonuniformConvolver::operator=` {#symbol-nonuniformconvolver-operator-2}

```cpp
NonuniformConvolver& operator=(NonuniformConvolver&& other) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `NonuniformConvolver<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `NonuniformConvolver<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `NonuniformConvolver::prepare` {#symbol-nonuniformconvolver-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const T>, std::size_t earlyPartition = 128, std::size_t latePartition = 1024, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `earlyPartition` | `std::size_t` | `128` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `latePartition` | `std::size_t` | `1024` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `NonuniformConvolver::reset` {#symbol-nonuniformconvolver-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `NonuniformConvolver::process` {#symbol-nonuniformconvolver-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `NonuniformConvolver::latencyFrames` {#symbol-nonuniformconvolver-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `NonuniformConvolver::tailFrames` {#symbol-nonuniformconvolver-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `NonuniformConvolver::backend` {#symbol-nonuniformconvolver-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
