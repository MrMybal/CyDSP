---
id: api-processors-hybridconvolver
title: HybridConvolver
description: Tête directe immédiate et queue FFT sans latence de bloc ajoutée.
section: Référence API
order: 1147
sourceRevision: 1
parent: api-processors
symbol: cydsp::HybridConvolver
---

Tête directe immédiate et queue FFT sans latence de bloc ajoutée.

## Inclusion {#include}

```cpp
#include <cydsp/processors/HybridConvolver.h>
```

[Utilisation et contrats](doc:convolution) · [Règles communes de traitement](doc:contracts)

## `HybridConvolver` {#symbol-hybridconvolver}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class HybridConvolver;
```

### `HybridConvolver::HybridConvolver<T>` {#symbol-hybridconvolver-hybridconvolver-t}

```cpp
HybridConvolver() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `HybridConvolver::HybridConvolver<T>` {#symbol-hybridconvolver-hybridconvolver-t-2}

```cpp
HybridConvolver(const HybridConvolver&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const HybridConvolver<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `HybridConvolver::operator=` {#symbol-hybridconvolver-operator}

```cpp
HybridConvolver& operator=(const HybridConvolver&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const HybridConvolver<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `HybridConvolver<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `HybridConvolver::HybridConvolver<T>` {#symbol-hybridconvolver-hybridconvolver-t-3}

```cpp
HybridConvolver(HybridConvolver&& other) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `HybridConvolver<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `HybridConvolver::operator=` {#symbol-hybridconvolver-operator-2}

```cpp
HybridConvolver& operator=(HybridConvolver&& other) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `HybridConvolver<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `HybridConvolver<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `HybridConvolver::prepare` {#symbol-hybridconvolver-prepare}

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

### `HybridConvolver::reset` {#symbol-hybridconvolver-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `HybridConvolver::process` {#symbol-hybridconvolver-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `HybridConvolver::latencyFrames` {#symbol-hybridconvolver-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `HybridConvolver::tailFrames` {#symbol-hybridconvolver-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `HybridConvolver::partitionFrames` {#symbol-hybridconvolver-partitionframes}

```cpp
std::size_t partitionFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `HybridConvolver::backend` {#symbol-hybridconvolver-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
