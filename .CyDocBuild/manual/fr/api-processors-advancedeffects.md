---
id: api-processors-advancedeffects
title: AdvancedEffects
description: Échos multitap filtrés/modulés, modulation d’amplitude, réverbérations à plaque et ressort.
section: Référence API
order: 1125
sourceRevision: 1
parent: api-processors
symbol: cydsp::DelayEffectMode
---

Échos multitap filtrés/modulés, modulation d’amplitude, réverbérations à plaque et ressort.

## Inclusion {#include}

```cpp
#include <cydsp/processors/AdvancedEffects.h>
```

[Utilisation et contrats](doc:effects) · [Règles communes de traitement](doc:contracts)

## `DelayEffectMode` {#symbol-delayeffectmode}

```cpp
enum class DelayEffectMode { echo, flanger, vibrato, wowFlutter };
```

## `EchoTap` {#symbol-echotap}

```cpp
struct EchoTap;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `delayMs` | `double` | `250` | Durée en millisecondes. |
| `gain` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |

## `DelayEffectParameters` {#symbol-delayeffectparameters}

```cpp
struct DelayEffectParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `mode` | `DelayEffectMode` | `DelayEffectMode::echo` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `delayMs` | `double` | `250` | Durée en millisecondes. |
| `depthMs` | `double` | `0` | Durée en millisecondes. |
| `rateHz` | `double` | `.5` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `feedback` | `double` | `.3` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `lowPassHz` | `double` | `10000` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `feedbackDrive` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `duckDb` | `double` | `0` | Niveau logarithmique selon la référence dB indiquée. |
| `wet` | `double` | `.5` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `dry` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `pingPong` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

## `DelayEffect` {#symbol-delayeffect}

```cpp
template <Sample T> class DelayEffect;
```

### `DelayEffect::DelayEffect<T>` {#symbol-delayeffect-delayeffect-t}

```cpp
DelayEffect();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `DelayEffect::~DelayEffect<T>` {#symbol-delayeffect-delayeffect-t-2}

```cpp
~DelayEffect();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `DelayEffect::DelayEffect<T>` {#symbol-delayeffect-delayeffect-t-3}

```cpp
DelayEffect(DelayEffect&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `DelayEffect<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `DelayEffect::operator=` {#symbol-delayeffect-operator}

```cpp
DelayEffect& operator=(DelayEffect&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `DelayEffect<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `DelayEffect<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `DelayEffect::prepare` {#symbol-delayeffect-prepare}

```cpp
Status prepare(const ProcessSpec&, double maximumDelayMs = 2000) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `maximumDelayMs` | `double` | `2000` | Durée en millisecondes. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DelayEffect::setParameters` {#symbol-delayeffect-setparameters}

```cpp
Status setParameters(const DelayEffectParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const DelayEffectParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DelayEffect::setTaps` {#symbol-delayeffect-settaps}

```cpp
Status setTaps(std::span<const EchoTap>) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const EchoTap>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DelayEffect::process` {#symbol-delayeffect-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DelayEffect::reset` {#symbol-delayeffect-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

## `AmplitudeModulationParameters` {#symbol-amplitudemodulationparameters}

```cpp
struct AmplitudeModulationParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `rateHz` | `double` | `4` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `depth` | `double` | `.5` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `autopan` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

## `AmplitudeModulator` {#symbol-amplitudemodulator}

```cpp
template <Sample T> class AmplitudeModulator;
```

### `AmplitudeModulator::prepare` {#symbol-amplitudemodulator-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AmplitudeModulator::setParameters` {#symbol-amplitudemodulator-setparameters}

```cpp
Status setParameters(const AmplitudeModulationParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const AmplitudeModulationParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AmplitudeModulator::process` {#symbol-amplitudemodulator-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AmplitudeModulator::reset` {#symbol-amplitudemodulator-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

## `PlateParameters` {#symbol-plateparameters}

```cpp
struct PlateParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `decay` | `double` | `.7` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `diffusion` | `double` | `.7` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `dampingHz` | `double` | `6000` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `wet` | `double` | `.5` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `dry` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `shimmer` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `freeze` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

## `PlateReverb` {#symbol-platereverb}

```cpp
template <Sample T> class PlateReverb;
```

### `PlateReverb::PlateReverb<T>` {#symbol-platereverb-platereverb-t}

```cpp
PlateReverb();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `PlateReverb::~PlateReverb<T>` {#symbol-platereverb-platereverb-t-2}

```cpp
~PlateReverb();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `PlateReverb::PlateReverb<T>` {#symbol-platereverb-platereverb-t-3}

```cpp
PlateReverb(PlateReverb&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `PlateReverb<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `PlateReverb::operator=` {#symbol-platereverb-operator}

```cpp
PlateReverb& operator=(PlateReverb&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `PlateReverb<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `PlateReverb<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `PlateReverb::prepare` {#symbol-platereverb-prepare}

```cpp
Status prepare(const ProcessSpec&, double size = 1) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `size` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `PlateReverb::setParameters` {#symbol-platereverb-setparameters}

```cpp
Status setParameters(const PlateParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const PlateParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `PlateReverb::process` {#symbol-platereverb-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `PlateReverb::reset` {#symbol-platereverb-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

## `SpringParameters` {#symbol-springparameters}

```cpp
struct SpringParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `decaySeconds` | `double` | `2` | Durée ou position en secondes. |
| `dispersion` | `double` | `.6` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `dampingHz` | `double` | `5000` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `wet` | `double` | `.5` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `dry` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |

## `SpringReverb` {#symbol-springreverb}

```cpp
template <Sample T> class SpringReverb;
```

### `SpringReverb::SpringReverb<T>` {#symbol-springreverb-springreverb-t}

```cpp
SpringReverb();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `SpringReverb::~SpringReverb<T>` {#symbol-springreverb-springreverb-t-2}

```cpp
~SpringReverb();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `SpringReverb::SpringReverb<T>` {#symbol-springreverb-springreverb-t-3}

```cpp
SpringReverb(SpringReverb&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `SpringReverb<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `SpringReverb::operator=` {#symbol-springreverb-operator}

```cpp
SpringReverb& operator=(SpringReverb&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `SpringReverb<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `SpringReverb<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SpringReverb::prepare` {#symbol-springreverb-prepare}

```cpp
Status prepare(const ProcessSpec&, double lengthSeconds = .035) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `lengthSeconds` | `double` | `.035` | Durée ou position en secondes. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SpringReverb::setParameters` {#symbol-springreverb-setparameters}

```cpp
Status setParameters(const SpringParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const SpringParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SpringReverb::process` {#symbol-springreverb-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SpringReverb::reset` {#symbol-springreverb-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.
