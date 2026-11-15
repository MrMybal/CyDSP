---
id: api-analysis-loudnessmeter
title: LoudnessMeter
description: Sonie pondérée K momentanée, courte et intégrée, avec historique exact borné.
section: Référence API
order: 1104
sourceRevision: 1
parent: api-analysis
symbol: cydsp::LoudnessMeter
---

Sonie pondérée K momentanée, courte et intégrée, avec historique exact borné.

## Inclusion {#include}

```cpp
#include <cydsp/analysis/LoudnessMeter.h>
```

[Utilisation et contrats](doc:metering) · [Règles communes de traitement](doc:contracts)

## `LoudnessConfig` {#symbol-loudnessconfig}

```cpp
struct LoudnessConfig;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `maximumGatingBlocks` | `std::size_t` | `288000` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `channelWeights` | `std::span<const double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

## `LoudnessLevels` {#symbol-loudnesslevels}

```cpp
struct LoudnessLevels;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `momentaryLufs` | `double` | `-std::numeric_limits<double>::infinity()` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `shortTermLufs` | `double` | `-std::numeric_limits<double>::infinity()` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `momentaryReady` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `shortTermReady` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

## `LoudnessMeter` {#symbol-loudnessmeter}

```cpp
template <Sample T> class LoudnessMeter;
```

### `LoudnessMeter::LoudnessMeter<T>` {#symbol-loudnessmeter-loudnessmeter-t}

```cpp
LoudnessMeter();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `LoudnessMeter::~LoudnessMeter<T>` {#symbol-loudnessmeter-loudnessmeter-t-2}

```cpp
~LoudnessMeter();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `LoudnessMeter::LoudnessMeter<T>` {#symbol-loudnessmeter-loudnessmeter-t-3}

```cpp
LoudnessMeter(LoudnessMeter&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `LoudnessMeter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `LoudnessMeter::operator=` {#symbol-loudnessmeter-operator}

```cpp
LoudnessMeter& operator=(LoudnessMeter&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `LoudnessMeter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `LoudnessMeter<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `LoudnessMeter::prepare` {#symbol-loudnessmeter-prepare}

```cpp
Status prepare(const ProcessSpec&, const LoudnessConfig& = {}) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `const LoudnessConfig &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `LoudnessMeter::process` {#symbol-loudnessmeter-process}

```cpp
Status process(AudioBlock<const T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `LoudnessMeter::reset` {#symbol-loudnessmeter-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `LoudnessMeter::levels` {#symbol-loudnessmeter-levels}

```cpp
LoudnessLevels levels() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `LoudnessLevels` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `LoudnessMeter::integratedLufs` {#symbol-loudnessmeter-integratedlufs}

```cpp
double integratedLufs() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `LoudnessMeter::loudnessRangeLu` {#symbol-loudnessmeter-loudnessrangelu}

```cpp
double loudnessRangeLu() noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `LoudnessMeter::gatingBlocks` {#symbol-loudnessmeter-gatingblocks}

```cpp
std::size_t gatingBlocks() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `LoudnessMeter::capacity` {#symbol-loudnessmeter-capacity}

```cpp
std::size_t capacity() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
