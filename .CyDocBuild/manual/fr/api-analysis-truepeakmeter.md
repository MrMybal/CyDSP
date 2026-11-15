---
id: api-analysis-truepeakmeter
title: TruePeakMeter
description: Estimation suréchantillonnée des crêtes interéchantillons avec retard de reconstruction explicite.
section: Référence API
order: 1107
sourceRevision: 1
parent: api-analysis
symbol: cydsp::TruePeakMeter
---

Estimation suréchantillonnée des crêtes interéchantillons avec retard de reconstruction explicite.

## Inclusion {#include}

```cpp
#include <cydsp/analysis/TruePeakMeter.h>
```

[Utilisation et contrats](doc:metering) · [Règles communes de traitement](doc:contracts)

## `TruePeakMeter` {#symbol-truepeakmeter}

```cpp
template <Sample T> class TruePeakMeter;
```

### `TruePeakMeter::TruePeakMeter<T>` {#symbol-truepeakmeter-truepeakmeter-t}

```cpp
TruePeakMeter();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `TruePeakMeter::~TruePeakMeter<T>` {#symbol-truepeakmeter-truepeakmeter-t-2}

```cpp
~TruePeakMeter();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `TruePeakMeter::TruePeakMeter<T>` {#symbol-truepeakmeter-truepeakmeter-t-3}

```cpp
TruePeakMeter(TruePeakMeter&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `TruePeakMeter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `TruePeakMeter::operator=` {#symbol-truepeakmeter-operator}

```cpp
TruePeakMeter& operator=(TruePeakMeter&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `TruePeakMeter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `TruePeakMeter<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `TruePeakMeter::prepare` {#symbol-truepeakmeter-prepare}

```cpp
Status prepare(const ProcessSpec&, unsigned oversampling = 4, std::size_t tapsPerPhase = 64, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `oversampling` | `unsigned int` | `4` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `tapsPerPhase` | `std::size_t` | `64` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `TruePeakMeter::process` {#symbol-truepeakmeter-process}

```cpp
Status process(AudioBlock<const T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `TruePeakMeter::flush` {#symbol-truepeakmeter-flush}

```cpp
void flush() noexcept;
```

Vider la queue documentée et vérifier la fin ; réinitialiser avant un nouveau flux.

### `TruePeakMeter::reset` {#symbol-truepeakmeter-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `TruePeakMeter::samplePeak` {#symbol-truepeakmeter-samplepeak}

```cpp
double samplePeak(std::size_t channel) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Indice/compte de canaux dans la disposition planaire déclarée. |

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `TruePeakMeter::truePeak` {#symbol-truepeakmeter-truepeak}

```cpp
double truePeak(std::size_t channel) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Indice/compte de canaux dans la disposition planaire déclarée. |

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `TruePeakMeter::truePeakDbtp` {#symbol-truepeakmeter-truepeakdbtp}

```cpp
double truePeakDbtp(std::size_t channel) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Indice/compte de canaux dans la disposition planaire déclarée. |

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `TruePeakMeter::latencyFrames` {#symbol-truepeakmeter-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `TruePeakMeter::backend` {#symbol-truepeakmeter-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
