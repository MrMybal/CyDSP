---
id: api-processors-sincinterpolator
title: SincInterpolator
description: Interpolation sinc fenêtrée préparée pour les positions fractionnaires.
section: Référence API
order: 1162
sourceRevision: 1
parent: api-processors
symbol: cydsp::SincInterpolator
---

Interpolation sinc fenêtrée préparée pour les positions fractionnaires.

## Inclusion {#include}

```cpp
#include <cydsp/processors/SincInterpolator.h>
```

[Utilisation et contrats](doc:delay) · [Règles communes de traitement](doc:contracts)

## `SincInterpolator` {#symbol-sincinterpolator}

```cpp
template <Sample T> class SincInterpolator;
```

### `SincInterpolator::SincInterpolator<T>` {#symbol-sincinterpolator-sincinterpolator-t}

```cpp
SincInterpolator();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `SincInterpolator::~SincInterpolator<T>` {#symbol-sincinterpolator-sincinterpolator-t-2}

```cpp
~SincInterpolator();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `SincInterpolator::SincInterpolator<T>` {#symbol-sincinterpolator-sincinterpolator-t-3}

```cpp
SincInterpolator(SincInterpolator&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `SincInterpolator<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `SincInterpolator::operator=` {#symbol-sincinterpolator-operator}

```cpp
SincInterpolator& operator=(SincInterpolator&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `SincInterpolator<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `SincInterpolator<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SincInterpolator::prepare` {#symbol-sincinterpolator-prepare}

```cpp
Status prepare(const ProcessSpec& output, double inputRate, std::size_t taps = 64, std::size_t phases = 1024, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `output` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `inputRate` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `taps` | `std::size_t` | `64` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `phases` | `std::size_t` | `1024` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SincInterpolator::process` {#symbol-sincinterpolator-process}

```cpp
Status process(AudioBlock<const T> source, double firstSourceFrame, AudioBlock<T> output) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `source` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `firstSourceFrame` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `output` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SincInterpolator::processPositions` {#symbol-sincinterpolator-processpositions}

```cpp
Status processPositions(AudioBlock<const T> source, std::span<const double> positions, AudioBlock<T> output) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `source` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `positions` | `std::span<const double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `output` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SincInterpolator::backend` {#symbol-sincinterpolator-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
