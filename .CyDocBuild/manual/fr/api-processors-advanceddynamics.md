---
id: api-processors-advanceddynamics
title: AdvancedDynamics
description: Saturation ADAA, EQ dynamique, de-essing, transitoires, dynamique multibande et limitation true peak.
section: Référence API
order: 1124
sourceRevision: 1
parent: api-processors
symbol: cydsp::AdaaShape
---

Saturation ADAA, EQ dynamique, de-essing, transitoires, dynamique multibande et limitation true peak.

## Inclusion {#include}

```cpp
#include <cydsp/processors/AdvancedDynamics.h>
```

[Utilisation et contrats](doc:advanced-dynamics) · [Règles communes de traitement](doc:contracts)

## `AdaaShape` {#symbol-adaashape}

```cpp
enum class AdaaShape { tanh, atan, cubic, hardClip, wavefolder, hysteresis };
```

## `SaturationParameters` {#symbol-saturationparameters}

```cpp
struct SaturationParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `shape` | `AdaaShape` | `AdaaShape::tanh` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `drive` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `bias` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `outputGain` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `hysteresisWidth` | `double` | `.1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `hysteresisAmount` | `double` | `.5` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `antialias` | `bool` | `true` | Option ou prédicat vrai/faux explicite. |
| `math` | `MathAccuracy` | `MathAccuracy::precise` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `SaturationInputStage` {#symbol-saturationinputstage}

```cpp
struct SaturationInputStage;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `gain` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `bias` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `scale` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `AdaaSaturator` {#symbol-adaasaturator}

```cpp
template <Sample T> class AdaaSaturator;
```

### `AdaaSaturator::AdaaSaturator<T>` {#symbol-adaasaturator-adaasaturator-t}

```cpp
AdaaSaturator();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `AdaaSaturator::~AdaaSaturator<T>` {#symbol-adaasaturator-adaasaturator-t-2}

```cpp
~AdaaSaturator();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `AdaaSaturator::AdaaSaturator<T>` {#symbol-adaasaturator-adaasaturator-t-3}

```cpp
AdaaSaturator(AdaaSaturator&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AdaaSaturator<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `AdaaSaturator::operator=` {#symbol-adaasaturator-operator}

```cpp
AdaaSaturator& operator=(AdaaSaturator&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AdaaSaturator<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `AdaaSaturator<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AdaaSaturator::prepare` {#symbol-adaasaturator-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AdaaSaturator::setParameters` {#symbol-adaasaturator-setparameters}

```cpp
Status setParameters(const SaturationParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const SaturationParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AdaaSaturator::process` {#symbol-adaasaturator-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AdaaSaturator::processMixed` {#symbol-adaasaturator-processmixed}

```cpp
Status processMixed(AudioBlock<const T>, AudioBlock<T>, const SaturationInputStage&, double dryGain = 0, double wetGain = 1) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |
| `argument 3` | `const SaturationInputStage &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `dryGain` | `double` | `0` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `wetGain` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AdaaSaturator::reset` {#symbol-adaasaturator-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

## `DynamicEqBand` {#symbol-dynamiceqband}

```cpp
struct DynamicEqBand;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `frequencyHz` | `double` | `1000` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `q` | `double` | `1` | Facteur de qualité du filtre positif et fini. |
| `thresholdDb` | `double` | `-24` | Niveau logarithmique selon la référence dB indiquée. |
| `rangeDb` | `double` | `-12` | Niveau logarithmique selon la référence dB indiquée. |
| `attackMs` | `double` | `10` | Durée en millisecondes. |
| `releaseMs` | `double` | `100` | Durée en millisecondes. |
| `staticGainDb` | `double` | `0` | Niveau logarithmique selon la référence dB indiquée. |

## `DynamicEqualizer` {#symbol-dynamicequalizer}

```cpp
template <Sample T> class DynamicEqualizer;
```

### `DynamicEqualizer::DynamicEqualizer<T>` {#symbol-dynamicequalizer-dynamicequalizer-t}

```cpp
DynamicEqualizer();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `DynamicEqualizer::~DynamicEqualizer<T>` {#symbol-dynamicequalizer-dynamicequalizer-t-2}

```cpp
~DynamicEqualizer();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `DynamicEqualizer::DynamicEqualizer<T>` {#symbol-dynamicequalizer-dynamicequalizer-t-3}

```cpp
DynamicEqualizer(DynamicEqualizer&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `DynamicEqualizer<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `DynamicEqualizer::operator=` {#symbol-dynamicequalizer-operator}

```cpp
DynamicEqualizer& operator=(DynamicEqualizer&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `DynamicEqualizer<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `DynamicEqualizer<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `DynamicEqualizer::prepare` {#symbol-dynamicequalizer-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t maximumBands = 8) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `maximumBands` | `std::size_t` | `8` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DynamicEqualizer::setBands` {#symbol-dynamicequalizer-setbands}

```cpp
Status setBands(std::span<const DynamicEqBand>) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const DynamicEqBand>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DynamicEqualizer::process` {#symbol-dynamicequalizer-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DynamicEqualizer::process` {#symbol-dynamicequalizer-process-2}

```cpp
Status process(AudioBlock<const T>, AudioBlock<const T> sidechain, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `sidechain` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 3` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DynamicEqualizer::reset` {#symbol-dynamicequalizer-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

## `DeEsser` {#symbol-deesser}

```cpp
template <Sample T> class DeEsser;
```

### `DeEsser::prepare` {#symbol-deesser-prepare}

```cpp
Status prepare(const ProcessSpec& spec) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `spec` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DeEsser::setParameters` {#symbol-deesser-setparameters}

```cpp
Status setParameters(double frequencyHz = 6500, double q = .7, double thresholdDb = -24, double maximumCutDb = 12) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `frequencyHz` | `double` | `6500` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `q` | `double` | `.7` | Facteur de qualité du filtre positif et fini. |
| `thresholdDb` | `double` | `-24` | Niveau logarithmique selon la référence dB indiquée. |
| `maximumCutDb` | `double` | `12` | Niveau logarithmique selon la référence dB indiquée. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DeEsser::process` {#symbol-deesser-process}

```cpp
Status process(AudioBlock<const T> in, AudioBlock<T> out) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `in` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `out` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DeEsser::reset` {#symbol-deesser-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

## `TransientParameters` {#symbol-transientparameters}

```cpp
struct TransientParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `attack` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `sustain` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `fastMs` | `double` | `1` | Durée en millisecondes. |
| `slowMs` | `double` | `30` | Durée en millisecondes. |
| `releaseMs` | `double` | `150` | Durée en millisecondes. |
| `maximumGainDb` | `double` | `24` | Niveau logarithmique selon la référence dB indiquée. |
| `math` | `MathAccuracy` | `MathAccuracy::precise` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `TransientShaper` {#symbol-transientshaper}

```cpp
template <Sample T> class TransientShaper;
```

### `TransientShaper::TransientShaper<T>` {#symbol-transientshaper-transientshaper-t}

```cpp
TransientShaper();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `TransientShaper::~TransientShaper<T>` {#symbol-transientshaper-transientshaper-t-2}

```cpp
~TransientShaper();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `TransientShaper::TransientShaper<T>` {#symbol-transientshaper-transientshaper-t-3}

```cpp
TransientShaper(TransientShaper&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `TransientShaper<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `TransientShaper::operator=` {#symbol-transientshaper-operator}

```cpp
TransientShaper& operator=(TransientShaper&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `TransientShaper<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `TransientShaper<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `TransientShaper::prepare` {#symbol-transientshaper-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `TransientShaper::setParameters` {#symbol-transientshaper-setparameters}

```cpp
Status setParameters(const TransientParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const TransientParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `TransientShaper::process` {#symbol-transientshaper-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `TransientShaper::processGain` {#symbol-transientshaper-processgain}

```cpp
Status processGain(AudioBlock<const T> detector, AudioBlock<T> gains) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `detector` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `gains` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `TransientShaper::reset` {#symbol-transientshaper-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

## `MultibandMode` {#symbol-multibandmode}

```cpp
enum class MultibandMode { bypass, compressor, saturator };
```

## `MultibandDynamics` {#symbol-multibanddynamics}

```cpp
template <Sample T> class MultibandDynamics;
```

### `MultibandDynamics::MultibandDynamics<T>` {#symbol-multibanddynamics-multibanddynamics-t}

```cpp
MultibandDynamics();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `MultibandDynamics::~MultibandDynamics<T>` {#symbol-multibanddynamics-multibanddynamics-t-2}

```cpp
~MultibandDynamics();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `MultibandDynamics::MultibandDynamics<T>` {#symbol-multibanddynamics-multibanddynamics-t-3}

```cpp
MultibandDynamics(MultibandDynamics&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `MultibandDynamics<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `MultibandDynamics::operator=` {#symbol-multibanddynamics-operator}

```cpp
MultibandDynamics& operator=(MultibandDynamics&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `MultibandDynamics<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `MultibandDynamics<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MultibandDynamics::prepare` {#symbol-multibanddynamics-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const double> crossoversHz, std::size_t taps = 129, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `crossoversHz` | `std::span<const double>` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `taps` | `std::size_t` | `129` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `MultibandDynamics::setBand` {#symbol-multibanddynamics-setband}

```cpp
Status setBand(std::size_t, MultibandMode, const DynamicsParameters& = {}, const SaturationParameters& = {}) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `MultibandMode` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `const DynamicsParameters &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 4` | `const SaturationParameters &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `MultibandDynamics::process` {#symbol-multibanddynamics-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `MultibandDynamics::reset` {#symbol-multibanddynamics-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `MultibandDynamics::latencyFrames` {#symbol-multibanddynamics-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `TruePeakLimiter` {#symbol-truepeaklimiter}

```cpp
template <Sample T> class TruePeakLimiter;
```

### `TruePeakLimiter::TruePeakLimiter<T>` {#symbol-truepeaklimiter-truepeaklimiter-t}

```cpp
TruePeakLimiter();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `TruePeakLimiter::~TruePeakLimiter<T>` {#symbol-truepeaklimiter-truepeaklimiter-t-2}

```cpp
~TruePeakLimiter();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `TruePeakLimiter::TruePeakLimiter<T>` {#symbol-truepeaklimiter-truepeaklimiter-t-3}

```cpp
TruePeakLimiter(TruePeakLimiter&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `TruePeakLimiter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `TruePeakLimiter::operator=` {#symbol-truepeaklimiter-operator}

```cpp
TruePeakLimiter& operator=(TruePeakLimiter&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `TruePeakLimiter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `TruePeakLimiter<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `TruePeakLimiter::prepare` {#symbol-truepeaklimiter-prepare}

```cpp
Status prepare(const ProcessSpec&, unsigned oversampling = 4, double lookAheadMs = 5, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `oversampling` | `unsigned int` | `4` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `lookAheadMs` | `double` | `5` | Durée en millisecondes. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `TruePeakLimiter::setParameters` {#symbol-truepeaklimiter-setparameters}

```cpp
Status setParameters(double ceilingDbtp = -1, double releaseMs = 100, double marginDb = .5) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `ceilingDbtp` | `double` | `-1` | Niveau logarithmique selon la référence dB indiquée. |
| `releaseMs` | `double` | `100` | Durée en millisecondes. |
| `marginDb` | `double` | `.5` | Niveau logarithmique selon la référence dB indiquée. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `TruePeakLimiter::process` {#symbol-truepeaklimiter-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `TruePeakLimiter::reset` {#symbol-truepeaklimiter-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `TruePeakLimiter::latencyFrames` {#symbol-truepeaklimiter-latencyframes}

```cpp
double latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
