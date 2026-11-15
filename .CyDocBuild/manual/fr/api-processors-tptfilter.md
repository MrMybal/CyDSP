---
id: api-processors-tptfilter
title: TptFilter
description: Filtres TPT à variables d’état et ladder, séparation Linkwitz–Riley et conceptions complémentaires.
section: Référence API
order: 1167
sourceRevision: 1
parent: api-processors
symbol: cydsp::TptFilter
---

Filtres TPT à variables d’état et ladder, séparation Linkwitz–Riley et conceptions complémentaires.

## Inclusion {#include}

```cpp
#include <cydsp/processors/TptFilter.h>
```

[Utilisation et contrats](doc:filters) · [Règles communes de traitement](doc:contracts)

## `TptFilterMode` {#symbol-tptfiltermode}

```cpp
enum class TptFilterMode { lowPass, bandPass, highPass, notch, allPass, firstOrderLowPass, firstOrderHighPass, firstOrderAllPass };
```

## `TptFilter` {#symbol-tptfilter}

```cpp
template <Sample T> class TptFilter;
```

### `TptFilter::TptFilter<T>` {#symbol-tptfilter-tptfilter-t}

```cpp
TptFilter();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `TptFilter::~TptFilter<T>` {#symbol-tptfilter-tptfilter-t-2}

```cpp
~TptFilter();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `TptFilter::TptFilter<T>` {#symbol-tptfilter-tptfilter-t-3}

```cpp
TptFilter(TptFilter&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `TptFilter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `TptFilter::operator=` {#symbol-tptfilter-operator}

```cpp
TptFilter& operator=(TptFilter&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `TptFilter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `TptFilter<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `TptFilter::prepare` {#symbol-tptfilter-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `TptFilter::setParameters` {#symbol-tptfilter-setparameters}

```cpp
Status setParameters(double frequency, double q, TptFilterMode) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `frequency` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `q` | `double` | — | Facteur de qualité du filtre positif et fini. |
| `argument 3` | `TptFilterMode` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `TptFilter::process` {#symbol-tptfilter-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `TptFilter::reset` {#symbol-tptfilter-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

## `LinkwitzRiley` {#symbol-linkwitzriley}

```cpp
template <Sample T> class LinkwitzRiley;
```

### `LinkwitzRiley::LinkwitzRiley<T>` {#symbol-linkwitzriley-linkwitzriley-t}

```cpp
LinkwitzRiley();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `LinkwitzRiley::~LinkwitzRiley<T>` {#symbol-linkwitzriley-linkwitzriley-t-2}

```cpp
~LinkwitzRiley();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `LinkwitzRiley::LinkwitzRiley<T>` {#symbol-linkwitzriley-linkwitzriley-t-3}

```cpp
LinkwitzRiley(LinkwitzRiley&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `LinkwitzRiley<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `LinkwitzRiley::operator=` {#symbol-linkwitzriley-operator}

```cpp
LinkwitzRiley& operator=(LinkwitzRiley&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `LinkwitzRiley<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `LinkwitzRiley<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `LinkwitzRiley::prepare` {#symbol-linkwitzriley-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `LinkwitzRiley::setFrequency` {#symbol-linkwitzriley-setfrequency}

```cpp
Status setFrequency(double frequency) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `frequency` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `LinkwitzRiley::process` {#symbol-linkwitzriley-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T> low, AudioBlock<T> high) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `low` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |
| `high` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `LinkwitzRiley::reset` {#symbol-linkwitzriley-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

## `LadderFilter` {#symbol-ladderfilter}

```cpp
template <Sample T> class LadderFilter;
```

### `LadderFilter::LadderFilter<T>` {#symbol-ladderfilter-ladderfilter-t}

```cpp
LadderFilter();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `LadderFilter::~LadderFilter<T>` {#symbol-ladderfilter-ladderfilter-t-2}

```cpp
~LadderFilter();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `LadderFilter::LadderFilter<T>` {#symbol-ladderfilter-ladderfilter-t-3}

```cpp
LadderFilter(LadderFilter&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `LadderFilter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `LadderFilter::operator=` {#symbol-ladderfilter-operator}

```cpp
LadderFilter& operator=(LadderFilter&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `LadderFilter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `LadderFilter<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `LadderFilter::prepare` {#symbol-ladderfilter-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `LadderFilter::setParameters` {#symbol-ladderfilter-setparameters}

```cpp
Status setParameters(double frequency, double resonance, double drive = 1) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `frequency` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `resonance` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `drive` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `LadderFilter::process` {#symbol-ladderfilter-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `LadderFilter::reset` {#symbol-ladderfilter-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

## `designButterworth` {#symbol-designbutterworth}

```cpp
Status designButterworth(BiquadType, unsigned order, double sampleRate, double cutoff, std::span<BiquadCoefficients> sections, std::size_t& count) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `BiquadType` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `order` | `unsigned int` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `sampleRate` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `cutoff` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `sections` | `std::span<BiquadCoefficients>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `count` | `std::size_t &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `designWindowedFir` {#symbol-designwindowedfir}

```cpp
Status designWindowedFir(std::span<double> taps, double normalizedCutoff, bool highPass = false, double attenuationDb = 100) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `taps` | `std::span<double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `normalizedCutoff` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `highPass` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `attenuationDb` | `double` | `100` | Niveau logarithmique selon la référence dB indiquée. |

Retour : `Status` — Vérifier Status ; ok indique le succès.
