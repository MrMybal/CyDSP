---
id: api-processors-scheduledconvolver
title: ScheduledConvolver
description: Répartition du travail FFT/MAC tardif sur les frames d’entrée avec attaque immédiate.
section: Référence API
order: 1161
sourceRevision: 1
parent: api-processors
symbol: cydsp::ScheduledConvolver
---

Répartition du travail FFT/MAC tardif sur les frames d’entrée avec attaque immédiate.

## Inclusion {#include}

```cpp
#include <cydsp/processors/ScheduledConvolver.h>
```

[Utilisation et contrats](doc:convolution-options) · [Règles communes de traitement](doc:contracts)

## `ScheduledConvolver` {#symbol-scheduledconvolver}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class ScheduledConvolver;
```

### `ScheduledConvolver::ScheduledConvolver<T>` {#symbol-scheduledconvolver-scheduledconvolver-t}

```cpp
ScheduledConvolver() noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `ScheduledConvolver::~ScheduledConvolver<T>` {#symbol-scheduledconvolver-scheduledconvolver-t-2}

```cpp
~ScheduledConvolver();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `ScheduledConvolver::ScheduledConvolver<T>` {#symbol-scheduledconvolver-scheduledconvolver-t-3}

```cpp
ScheduledConvolver(const ScheduledConvolver&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ScheduledConvolver<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `ScheduledConvolver::operator=` {#symbol-scheduledconvolver-operator}

```cpp
ScheduledConvolver& operator=(const ScheduledConvolver&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ScheduledConvolver<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `ScheduledConvolver<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ScheduledConvolver::ScheduledConvolver<T>` {#symbol-scheduledconvolver-scheduledconvolver-t-4}

```cpp
ScheduledConvolver(ScheduledConvolver&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `ScheduledConvolver<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `ScheduledConvolver::operator=` {#symbol-scheduledconvolver-operator-2}

```cpp
ScheduledConvolver& operator=(ScheduledConvolver&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `ScheduledConvolver<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `ScheduledConvolver<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ScheduledConvolver::prepare` {#symbol-scheduledconvolver-prepare}

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

### `ScheduledConvolver::reset` {#symbol-scheduledconvolver-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `ScheduledConvolver::process` {#symbol-scheduledconvolver-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ScheduledConvolver::latencyFrames` {#symbol-scheduledconvolver-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ScheduledConvolver::tailFrames` {#symbol-scheduledconvolver-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ScheduledConvolver::backend` {#symbol-scheduledconvolver-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ScheduledConvolver::schedulingActive` {#symbol-scheduledconvolver-schedulingactive}

```cpp
bool schedulingActive() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ScheduledConvolver::workUnitsPerPartition` {#symbol-scheduledconvolver-workunitsperpartition}

```cpp
std::size_t workUnitsPerPartition() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ScheduledConvolver::maximumWorkUnitsPerFrame` {#symbol-scheduledconvolver-maximumworkunitsperframe}

```cpp
std::size_t maximumWorkUnitsPerFrame() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
