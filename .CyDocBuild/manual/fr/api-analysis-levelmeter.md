---
id: api-analysis-levelmeter
title: LevelMeter
description: Peak échantillon, RMS, DC, facteur de crête et corrélation stéréo sur fenêtre glissante.
section: Référence API
order: 1103
sourceRevision: 1
parent: api-analysis
symbol: cydsp::LevelMeter
---

Peak échantillon, RMS, DC, facteur de crête et corrélation stéréo sur fenêtre glissante.

## Inclusion {#include}

```cpp
#include <cydsp/analysis/LevelMeter.h>
```

[Utilisation et contrats](doc:metering) · [Règles communes de traitement](doc:contracts)

## `ChannelLevels` {#symbol-channellevels}

```cpp
struct ChannelLevels;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `peak` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `rms` | `double` | `0` | Durée en millisecondes. |
| `mean` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `heldPeak` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `ChannelLevels::peakDbfs` {#symbol-channellevels-peakdbfs}

```cpp
double peakDbfs() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ChannelLevels::rmsDbfs` {#symbol-channellevels-rmsdbfs}

```cpp
double rmsDbfs() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ChannelLevels::crestFactorDb` {#symbol-channellevels-crestfactordb}

```cpp
double crestFactorDb() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `LevelMeter` {#symbol-levelmeter}

```cpp
template <Sample T> class LevelMeter;
```

### `LevelMeter::LevelMeter<T>` {#symbol-levelmeter-levelmeter-t}

```cpp
LevelMeter();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `LevelMeter::~LevelMeter<T>` {#symbol-levelmeter-levelmeter-t-2}

```cpp
~LevelMeter();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `LevelMeter::LevelMeter<T>` {#symbol-levelmeter-levelmeter-t-3}

```cpp
LevelMeter(LevelMeter&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `LevelMeter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `LevelMeter::operator=` {#symbol-levelmeter-operator}

```cpp
LevelMeter& operator=(LevelMeter&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `LevelMeter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `LevelMeter<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `LevelMeter::prepare` {#symbol-levelmeter-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t windowFrames = 2048) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `windowFrames` | `std::size_t` | `2048` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `LevelMeter::process` {#symbol-levelmeter-process}

```cpp
Status process(AudioBlock<const T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `LevelMeter::reset` {#symbol-levelmeter-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `LevelMeter::levels` {#symbol-levelmeter-levels}

```cpp
ChannelLevels levels(std::size_t channel) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Indice/compte de canaux dans la disposition planaire déclarée. |

Retour : `ChannelLevels` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `LevelMeter::stereoCorrelation` {#symbol-levelmeter-stereocorrelation}

```cpp
double stereoCorrelation() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `LevelMeter::windowFrames` {#symbol-levelmeter-windowframes}

```cpp
std::size_t windowFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
