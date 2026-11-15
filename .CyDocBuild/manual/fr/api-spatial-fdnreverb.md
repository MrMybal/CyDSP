---
id: api-spatial-fdnreverb
title: FdnReverb
description: Réseaux de retards à rétroaction orthogonaux avec décroissance par bandes, diffusion et freeze.
section: Référence API
order: 1172
sourceRevision: 1
parent: api-spatial
symbol: cydsp::FdnReverb
---

Réseaux de retards à rétroaction orthogonaux avec décroissance par bandes, diffusion et freeze.

## Inclusion {#include}

```cpp
#include <cydsp/spatial/FdnReverb.h>
```

[Utilisation et contrats](doc:reverb) · [Règles communes de traitement](doc:contracts)

## `FdnConfig` {#symbol-fdnconfig}

```cpp
struct FdnConfig;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `lines` | `std::size_t` | `16` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `roomScale` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `maximumPreDelayMs` | `double` | `250` | Durée en millisecondes. |

## `FdnParameters` {#symbol-fdnparameters}

```cpp
struct FdnParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `decaySeconds` | `std::array<double, 3>` | `{2, 1.5, 1}` | Durée ou position en secondes. |
| `preDelayMs` | `double` | `10` | Durée en millisecondes. |
| `diffusion` | `double` | `.7` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `wet` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `dry` | `double` | `0` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `freeze` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

## `FdnReverb` {#symbol-fdnreverb}

```cpp
template <Sample T> class FdnReverb;
```

### `FdnReverb::FdnReverb<T>` {#symbol-fdnreverb-fdnreverb-t}

```cpp
FdnReverb();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `FdnReverb::~FdnReverb<T>` {#symbol-fdnreverb-fdnreverb-t-2}

```cpp
~FdnReverb();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `FdnReverb::FdnReverb<T>` {#symbol-fdnreverb-fdnreverb-t-3}

```cpp
FdnReverb(FdnReverb&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `FdnReverb<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `FdnReverb::operator=` {#symbol-fdnreverb-operator}

```cpp
FdnReverb& operator=(FdnReverb&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `FdnReverb<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `FdnReverb<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `FdnReverb::prepare` {#symbol-fdnreverb-prepare}

```cpp
Status prepare(const ProcessSpec&, const FdnConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `const FdnConfig &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `FdnReverb::setParameters` {#symbol-fdnreverb-setparameters}

```cpp
Status setParameters(const FdnParameters&, std::size_t rampFrames = 256) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const FdnParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `rampFrames` | `std::size_t` | `256` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `FdnReverb::process` {#symbol-fdnreverb-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `FdnReverb::reset` {#symbol-fdnreverb-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `FdnReverb::nominalTailFrames` {#symbol-fdnreverb-nominaltailframes}

```cpp
std::size_t nominalTailFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `FdnReverb::backend` {#symbol-fdnreverb-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
