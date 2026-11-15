---
id: api-processors-partitionedconvolver
title: PartitionedConvolver
description: Partitions FFT uniformes avec latence de bloc explicite et queue finie.
section: Référence API
order: 1155
sourceRevision: 1
parent: api-processors
symbol: cydsp::PartitionedConvolver
---

Partitions FFT uniformes avec latence de bloc explicite et queue finie.

## Inclusion {#include}

```cpp
#include <cydsp/processors/PartitionedConvolver.h>
```

[Utilisation et contrats](doc:convolution) · [Règles communes de traitement](doc:contracts)

## `PartitionedConvolver` {#symbol-partitionedconvolver}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class PartitionedConvolver;
```

### `PartitionedConvolver::PartitionedConvolver<T>` {#symbol-partitionedconvolver-partitionedconvolver-t}

```cpp
PartitionedConvolver() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `PartitionedConvolver::PartitionedConvolver<T>` {#symbol-partitionedconvolver-partitionedconvolver-t-2}

```cpp
PartitionedConvolver(const PartitionedConvolver&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const PartitionedConvolver<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `PartitionedConvolver::operator=` {#symbol-partitionedconvolver-operator}

```cpp
PartitionedConvolver& operator=(const PartitionedConvolver&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const PartitionedConvolver<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `PartitionedConvolver<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `PartitionedConvolver::PartitionedConvolver<T>` {#symbol-partitionedconvolver-partitionedconvolver-t-3}

```cpp
PartitionedConvolver(PartitionedConvolver&& other) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `PartitionedConvolver<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `PartitionedConvolver::operator=` {#symbol-partitionedconvolver-operator-2}

```cpp
PartitionedConvolver& operator=(PartitionedConvolver&& other) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `PartitionedConvolver<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `PartitionedConvolver<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `PartitionedConvolver::prepare` {#symbol-partitionedconvolver-prepare}

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

### `PartitionedConvolver::reset` {#symbol-partitionedconvolver-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `PartitionedConvolver::process` {#symbol-partitionedconvolver-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `PartitionedConvolver::latencyFrames` {#symbol-partitionedconvolver-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `PartitionedConvolver::tailFrames` {#symbol-partitionedconvolver-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `PartitionedConvolver::backend` {#symbol-partitionedconvolver-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
