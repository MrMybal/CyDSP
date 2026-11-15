---
id: api-processors-channelmatrix
title: ChannelMatrix
description: Matrices de routage indexées par sortie, rampes de gain et alias exacts entre canaux.
section: Référence API
order: 1133
sourceRevision: 1
parent: api-processors
symbol: cydsp::ChannelMatrix
---

Matrices de routage indexées par sortie, rampes de gain et alias exacts entre canaux.

## Inclusion {#include}

```cpp
#include <cydsp/processors/ChannelMatrix.h>
```

[Utilisation et contrats](doc:mixing) · [Règles communes de traitement](doc:contracts)

## `defaultChannelMatrix` {#symbol-defaultchannelmatrix}

```cpp
Status defaultChannelMatrix(std::size_t inputChannels, std::size_t outputChannels, std::span<double> coefficients) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `inputChannels` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `outputChannels` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `coefficients` | `std::span<double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `ChannelMatrix` {#symbol-channelmatrix}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class ChannelMatrix;
```

### `ChannelMatrix::ChannelMatrix<T>` {#symbol-channelmatrix-channelmatrix-t}

```cpp
ChannelMatrix() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `ChannelMatrix::ChannelMatrix<T>` {#symbol-channelmatrix-channelmatrix-t-2}

```cpp
ChannelMatrix(const ChannelMatrix&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ChannelMatrix<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `ChannelMatrix::operator=` {#symbol-channelmatrix-operator}

```cpp
ChannelMatrix& operator=(const ChannelMatrix&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ChannelMatrix<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `ChannelMatrix<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ChannelMatrix::ChannelMatrix<T>` {#symbol-channelmatrix-channelmatrix-t-3}

```cpp
ChannelMatrix(ChannelMatrix&& other) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `ChannelMatrix<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `ChannelMatrix::operator=` {#symbol-channelmatrix-operator-2}

```cpp
ChannelMatrix& operator=(ChannelMatrix&& other) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `ChannelMatrix<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `ChannelMatrix<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ChannelMatrix::prepare` {#symbol-channelmatrix-prepare}

```cpp
Status prepare(const ProcessSpec& inputSpec, std::size_t outputChannels, std::span<const double> coefficients, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `inputSpec` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `outputChannels` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `coefficients` | `std::span<const double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ChannelMatrix::setMatrix` {#symbol-channelmatrix-setmatrix}

```cpp
Status setMatrix(std::span<const double>, std::size_t rampFrames = 0) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `rampFrames` | `std::size_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ChannelMatrix::reset` {#symbol-channelmatrix-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `ChannelMatrix::process` {#symbol-channelmatrix-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ChannelMatrix::inputChannels` {#symbol-channelmatrix-inputchannels}

```cpp
std::size_t inputChannels() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ChannelMatrix::outputChannels` {#symbol-channelmatrix-outputchannels}

```cpp
std::size_t outputChannels() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ChannelMatrix::rampRemainingFrames` {#symbol-channelmatrix-rampremainingframes}

```cpp
std::size_t rampRemainingFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ChannelMatrix::backend` {#symbol-channelmatrix-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
