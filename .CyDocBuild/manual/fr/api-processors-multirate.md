---
id: api-processors-multirate
title: Multirate
description: Conversion de fréquence FIR entière et bancs dyadiques d’analyse/synthèse.
section: Référence API
order: 1151
sourceRevision: 1
parent: api-processors
symbol: cydsp::RateConversion
---

Conversion de fréquence FIR entière et bancs dyadiques d’analyse/synthèse.

## Inclusion {#include}

```cpp
#include <cydsp/processors/Multirate.h>
```

[Utilisation et contrats](doc:resampling) · [Règles communes de traitement](doc:contracts)

## `RateConversion` {#symbol-rateconversion}

```cpp
enum class RateConversion { decimate, interpolate };
```

## `FirRateConverter` {#symbol-firrateconverter}

```cpp
template <Sample T> class FirRateConverter;
```

### `FirRateConverter::FirRateConverter<T>` {#symbol-firrateconverter-firrateconverter-t}

```cpp
FirRateConverter();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `FirRateConverter::~FirRateConverter<T>` {#symbol-firrateconverter-firrateconverter-t-2}

```cpp
~FirRateConverter();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `FirRateConverter::FirRateConverter<T>` {#symbol-firrateconverter-firrateconverter-t-3}

```cpp
FirRateConverter(FirRateConverter&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `FirRateConverter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `FirRateConverter::operator=` {#symbol-firrateconverter-operator}

```cpp
FirRateConverter& operator=(FirRateConverter&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `FirRateConverter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `FirRateConverter<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `FirRateConverter::prepare` {#symbol-firrateconverter-prepare}

```cpp
Status prepare(const ProcessSpec& input, unsigned factor, RateConversion, std::size_t tapsPerPhase = 32, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `input` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `factor` | `unsigned int` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `RateConversion` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `tapsPerPhase` | `std::size_t` | `32` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `FirRateConverter::outputFrames` {#symbol-firrateconverter-outputframes}

```cpp
std::size_t outputFrames(std::size_t inputFrames) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `inputFrames` | `std::size_t` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `FirRateConverter::process` {#symbol-firrateconverter-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `FirRateConverter::reset` {#symbol-firrateconverter-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `FirRateConverter::outputSampleRate` {#symbol-firrateconverter-outputsamplerate}

```cpp
double outputSampleRate() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `DyadicFilterBank` {#symbol-dyadicfilterbank}

```cpp
template <Sample T> class DyadicFilterBank;
```

### `DyadicFilterBank::DyadicFilterBank<T>` {#symbol-dyadicfilterbank-dyadicfilterbank-t}

```cpp
DyadicFilterBank() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `DyadicFilterBank::DyadicFilterBank<T>` {#symbol-dyadicfilterbank-dyadicfilterbank-t-2}

```cpp
DyadicFilterBank(const DyadicFilterBank&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const DyadicFilterBank<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `DyadicFilterBank::operator=` {#symbol-dyadicfilterbank-operator}

```cpp
DyadicFilterBank& operator=(const DyadicFilterBank&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const DyadicFilterBank<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `DyadicFilterBank<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `DyadicFilterBank::DyadicFilterBank<T>` {#symbol-dyadicfilterbank-dyadicfilterbank-t-3}

```cpp
DyadicFilterBank(DyadicFilterBank&& o) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `o` | `DyadicFilterBank<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `DyadicFilterBank::operator=` {#symbol-dyadicfilterbank-operator-2}

```cpp
DyadicFilterBank& operator=(DyadicFilterBank&& o) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `o` | `DyadicFilterBank<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `DyadicFilterBank<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `DyadicFilterBank::prepare` {#symbol-dyadicfilterbank-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t bands = 8) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `bands` | `std::size_t` | `8` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DyadicFilterBank::analysisFrames` {#symbol-dyadicfilterbank-analysisframes}

```cpp
std::size_t analysisFrames(std::size_t inputFrames) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `inputFrames` | `std::size_t` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `DyadicFilterBank::analyze` {#symbol-dyadicfilterbank-analyze}

```cpp
Status analyze(AudioBlock<const T>, AudioBlock<T> bands) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `bands` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DyadicFilterBank::synthesize` {#symbol-dyadicfilterbank-synthesize}

```cpp
Status synthesize(AudioBlock<const T> bands, AudioBlock<T>) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `bands` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DyadicFilterBank::reset` {#symbol-dyadicfilterbank-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.
