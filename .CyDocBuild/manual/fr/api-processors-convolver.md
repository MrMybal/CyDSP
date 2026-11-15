---
id: api-processors-convolver
title: Convolver
description: Sélection d’une stratégie de convolution préparée et de sa configuration de partitions.
section: Référence API
order: 1135
sourceRevision: 1
parent: api-processors
symbol: cydsp::Convolver
---

Sélection d’une stratégie de convolution préparée et de sa configuration de partitions.

## Inclusion {#include}

```cpp
#include <cydsp/processors/Convolver.h>
```

[Utilisation et contrats](doc:convolution) · [Règles communes de traitement](doc:contracts)

## `ConvolutionMode` {#symbol-convolutionmode}

```cpp
enum class ConvolutionMode { hybrid, nonuniform, scheduled };
```

## `ConvolutionConfig` {#symbol-convolutionconfig}

```cpp
struct ConvolutionConfig;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `mode` | `ConvolutionMode` | `ConvolutionMode::hybrid` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `earlyPartition` | `std::size_t` | `128` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `latePartition` | `std::size_t` | `1024` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `spectralHead` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

### `ConvolutionConfig::operator==` {#symbol-convolutionconfig-operator}

```cpp
bool operator==(const ConvolutionConfig&) const = default;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ConvolutionConfig &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `Convolver` {#symbol-convolver}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class Convolver;
```

### `Convolver::Convolver<T>` {#symbol-convolver-convolver-t}

```cpp
Convolver() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `Convolver::Convolver<T>` {#symbol-convolver-convolver-t-2}

```cpp
Convolver(const Convolver&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const Convolver<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `Convolver::operator=` {#symbol-convolver-operator}

```cpp
Convolver& operator=(const Convolver&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const Convolver<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Convolver<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Convolver::Convolver<T>` {#symbol-convolver-convolver-t-3}

```cpp
Convolver(Convolver&& other) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `Convolver<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `Convolver::operator=` {#symbol-convolver-operator-2}

```cpp
Convolver& operator=(Convolver&& other) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `Convolver<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Convolver<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Convolver::prepare` {#symbol-convolver-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const T>, const ConvolutionConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 3` | `const ConvolutionConfig &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Convolver::reset` {#symbol-convolver-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `Convolver::process` {#symbol-convolver-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Convolver::configuration` {#symbol-convolver-configuration}

```cpp
ConvolutionConfig configuration() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `ConvolutionConfig` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Convolver::latencyFrames` {#symbol-convolver-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Convolver::tailFrames` {#symbol-convolver-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Convolver::backend` {#symbol-convolver-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Convolver::partitionFrames` {#symbol-convolver-partitionframes}

```cpp
std::size_t partitionFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
