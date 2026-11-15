---
id: api-processors-adaptivefilter
title: AdaptiveFilter
description: Adaptation LMS/NLMS préparée des coefficients à partir du signal et de la référence souhaitée.
section: Référence API
order: 1123
sourceRevision: 1
parent: api-processors
symbol: cydsp::AdaptiveFilter
---

Adaptation LMS/NLMS préparée des coefficients à partir du signal et de la référence souhaitée.

## Inclusion {#include}

```cpp
#include <cydsp/processors/AdaptiveFilter.h>
```

[Utilisation et contrats](doc:restoration) · [Règles communes de traitement](doc:contracts)

## `AdaptiveAlgorithm` {#symbol-adaptivealgorithm}

```cpp
enum class AdaptiveAlgorithm { lms, nlms };
```

## `AdaptiveParameters` {#symbol-adaptiveparameters}

```cpp
struct AdaptiveParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `step` | `double` | `.1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `leak` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `regularization` | `double` | `1e-6` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `coefficientLimit` | `double` | `100` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `adapt` | `bool` | `true` | Option ou prédicat vrai/faux explicite. |

## `AdaptiveFilter` {#symbol-adaptivefilter}

```cpp
template <Sample T> class AdaptiveFilter;
```

### `AdaptiveFilter::AdaptiveFilter<T>` {#symbol-adaptivefilter-adaptivefilter-t}

```cpp
AdaptiveFilter();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `AdaptiveFilter::~AdaptiveFilter<T>` {#symbol-adaptivefilter-adaptivefilter-t-2}

```cpp
~AdaptiveFilter();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `AdaptiveFilter::AdaptiveFilter<T>` {#symbol-adaptivefilter-adaptivefilter-t-3}

```cpp
AdaptiveFilter(AdaptiveFilter&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AdaptiveFilter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `AdaptiveFilter::operator=` {#symbol-adaptivefilter-operator}

```cpp
AdaptiveFilter& operator=(AdaptiveFilter&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AdaptiveFilter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `AdaptiveFilter<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AdaptiveFilter::prepare` {#symbol-adaptivefilter-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t taps = 128, AdaptiveAlgorithm = AdaptiveAlgorithm::nlms, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `taps` | `std::size_t` | `128` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `AdaptiveAlgorithm` | `AdaptiveAlgorithm::nlms` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AdaptiveFilter::setParameters` {#symbol-adaptivefilter-setparameters}

```cpp
Status setParameters(const AdaptiveParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const AdaptiveParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AdaptiveFilter::process` {#symbol-adaptivefilter-process}

```cpp
Status process(AudioBlock<const T> reference, AudioBlock<const T> desired, AudioBlock<T> predicted, AudioBlock<T> residual) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `reference` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `desired` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `predicted` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |
| `residual` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AdaptiveFilter::coefficients` {#symbol-adaptivefilter-coefficients}

```cpp
std::span<const double> coefficients(std::size_t channel) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Indice/compte de canaux dans la disposition planaire déclarée. |

Retour : `std::span<const double>` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AdaptiveFilter::clippedUpdates` {#symbol-adaptivefilter-clippedupdates}

```cpp
std::uint64_t clippedUpdates() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::uint64_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AdaptiveFilter::reset` {#symbol-adaptivefilter-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.
