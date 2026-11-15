---
id: api-processors-matrixconvolver
title: MatrixConvolver
description: Routes d’impulsions MIMO avec transformations d’entrée et couverture des sorties partagées.
section: Référence API
order: 1148
sourceRevision: 1
parent: api-processors
symbol: cydsp::MatrixConvolver
---

Routes d’impulsions MIMO avec transformations d’entrée et couverture des sorties partagées.

## Inclusion {#include}

```cpp
#include <cydsp/processors/MatrixConvolver.h>
```

[Utilisation et contrats](doc:convolution-options) · [Règles communes de traitement](doc:contracts)

## `ConvolutionRoute` {#symbol-convolutionroute}

```cpp
template <Sample T> struct ConvolutionRoute;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `inputChannel` | `std::size_t` | `0` | Indice/compte de canaux dans la disposition planaire déclarée. |
| `outputChannel` | `std::size_t` | `0` | Indice/compte de canaux dans la disposition planaire déclarée. |
| `impulse` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

## `MatrixConvolver` {#symbol-matrixconvolver}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class MatrixConvolver;
```

### `MatrixConvolver::MatrixConvolver<T>` {#symbol-matrixconvolver-matrixconvolver-t}

```cpp
MatrixConvolver() noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `MatrixConvolver::~MatrixConvolver<T>` {#symbol-matrixconvolver-matrixconvolver-t-2}

```cpp
~MatrixConvolver();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `MatrixConvolver::MatrixConvolver<T>` {#symbol-matrixconvolver-matrixconvolver-t-3}

```cpp
MatrixConvolver(const MatrixConvolver&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const MatrixConvolver<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `MatrixConvolver::operator=` {#symbol-matrixconvolver-operator}

```cpp
MatrixConvolver& operator=(const MatrixConvolver&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const MatrixConvolver<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `MatrixConvolver<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MatrixConvolver::MatrixConvolver<T>` {#symbol-matrixconvolver-matrixconvolver-t-4}

```cpp
MatrixConvolver(MatrixConvolver&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `MatrixConvolver<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `MatrixConvolver::operator=` {#symbol-matrixconvolver-operator-2}

```cpp
MatrixConvolver& operator=(MatrixConvolver&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `MatrixConvolver<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `MatrixConvolver<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MatrixConvolver::prepare` {#symbol-matrixconvolver-prepare}

```cpp
Status prepare(const ProcessSpec& inputSpec, std::size_t outputChannels, std::span<const ConvolutionRoute<T>>, std::size_t earlyPartition = 128, std::size_t latePartition = 1024, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `inputSpec` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `outputChannels` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `std::span<const ConvolutionRoute<T>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `earlyPartition` | `std::size_t` | `128` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `latePartition` | `std::size_t` | `1024` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 6` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `MatrixConvolver::prepare` {#symbol-matrixconvolver-prepare-2}

```cpp
Status prepare(const ProcessSpec& inputSpec, std::size_t outputChannels, std::span<const ConvolutionRoute<T>>, const ConvolutionConfig&, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `inputSpec` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `outputChannels` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `std::span<const ConvolutionRoute<T>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 4` | `const ConvolutionConfig &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `MatrixConvolver::configuration` {#symbol-matrixconvolver-configuration}

```cpp
ConvolutionConfig configuration() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `ConvolutionConfig` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MatrixConvolver::schedulingActive` {#symbol-matrixconvolver-schedulingactive}

```cpp
bool schedulingActive() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MatrixConvolver::workUnitsPerPartition` {#symbol-matrixconvolver-workunitsperpartition}

```cpp
std::size_t workUnitsPerPartition() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MatrixConvolver::maximumWorkUnitsPerFrame` {#symbol-matrixconvolver-maximumworkunitsperframe}

```cpp
std::size_t maximumWorkUnitsPerFrame() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MatrixConvolver::reset` {#symbol-matrixconvolver-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `MatrixConvolver::process` {#symbol-matrixconvolver-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `MatrixConvolver::latencyFrames` {#symbol-matrixconvolver-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MatrixConvolver::tailFrames` {#symbol-matrixconvolver-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MatrixConvolver::backend` {#symbol-matrixconvolver-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
