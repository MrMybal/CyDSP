---
id: api-processors-pitcheffects
title: PitchEffects
description: Correction vers une hauteur cible et harmonisation multivoix par moteurs à délais ou spectraux.
section: Référence API
order: 1156
sourceRevision: 1
parent: api-processors
symbol: cydsp::PitchEffects
---

Correction vers une hauteur cible et harmonisation multivoix par moteurs à délais ou spectraux.

## Inclusion {#include}

```cpp
#include <cydsp/processors/PitchEffects.h>
```

[Utilisation et contrats](doc:pitch) · [Règles communes de traitement](doc:contracts)

## `HarmonyVoice` {#symbol-harmonyvoice}

```cpp
struct HarmonyVoice;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `semitones` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `gain` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `pan` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `delayMs` | `double` | `0` | Durée en millisecondes. |

## `PitchCorrectionParameters` {#symbol-pitchcorrectionparameters}

```cpp
struct PitchCorrectionParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `enabled` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `targetHz` | `double` | `0` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `referenceA4Hz` | `double` | `440` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `retuneMs` | `double` | `20` | Durée en millisecondes. |
| `scalePitchClasses` | `unsigned int` | `0xfff` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `PitchEffects` {#symbol-pitcheffects}

```cpp
template <Sample T> class PitchEffects;
```

### `PitchEffects::PitchEffects<T>` {#symbol-pitcheffects-pitcheffects-t}

```cpp
PitchEffects();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `PitchEffects::~PitchEffects<T>` {#symbol-pitcheffects-pitcheffects-t-2}

```cpp
~PitchEffects();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `PitchEffects::PitchEffects<T>` {#symbol-pitcheffects-pitcheffects-t-3}

```cpp
PitchEffects(PitchEffects&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `PitchEffects<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `PitchEffects::operator=` {#symbol-pitcheffects-operator}

```cpp
PitchEffects& operator=(PitchEffects&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `PitchEffects<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `PitchEffects<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `PitchEffects::prepare` {#symbol-pitcheffects-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t maximumVoices = 8, const MusicalAnalysisConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `maximumVoices` | `std::size_t` | `8` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `const MusicalAnalysisConfig &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `PitchEffects::prepareSpectral` {#symbol-pitcheffects-preparespectral}

```cpp
Status prepareSpectral(const ProcessSpec&, std::size_t maximumVoices = 8, const StftConfig& = {}, const MusicalAnalysisConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `maximumVoices` | `std::size_t` | `8` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `const StftConfig &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 4` | `const MusicalAnalysisConfig &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `PitchEffects::latencyFrames` {#symbol-pitcheffects-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `PitchEffects::setVoices` {#symbol-pitcheffects-setvoices}

```cpp
Status setVoices(std::span<const HarmonyVoice>) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const HarmonyVoice>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `PitchEffects::setCorrection` {#symbol-pitcheffects-setcorrection}

```cpp
Status setCorrection(const PitchCorrectionParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const PitchCorrectionParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `PitchEffects::process` {#symbol-pitcheffects-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `PitchEffects::reset` {#symbol-pitcheffects-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `PitchEffects::detectedPitchHz` {#symbol-pitcheffects-detectedpitchhz}

```cpp
double detectedPitchHz() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
