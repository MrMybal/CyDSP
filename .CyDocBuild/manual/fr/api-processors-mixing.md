---
id: api-processors-mixing
title: Mixing
description: Panoramique, lois dry/wet, mise en forme d’onde élémentaire et décalage continu.
section: Référence API
order: 1149
sourceRevision: 1
parent: api-processors
symbol: cydsp::PanLaw
---

Panoramique, lois dry/wet, mise en forme d’onde élémentaire et décalage continu.

## Inclusion {#include}

```cpp
#include <cydsp/processors/Mixing.h>
```

[Utilisation et contrats](doc:mixing) · [Règles communes de traitement](doc:contracts)

## `PanLaw` {#symbol-panlaw}

```cpp
enum class PanLaw { balanced, linear, sine3dB, sine4_5dB, sine6dB, squareRoot3dB, squareRoot4_5dB };
```

## `MixLaw` {#symbol-mixlaw}

```cpp
enum class MixLaw { linear, sine3dB, sine4_5dB, sine6dB, squareRoot3dB, squareRoot4_5dB };
```

## `Panner` {#symbol-panner}

```cpp
template <Sample T> class Panner;
```

### `Panner::prepare` {#symbol-panner-prepare}

```cpp
Status prepare(const ProcessSpec&, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Panner::setPan` {#symbol-panner-setpan}

```cpp
Status setPan(double pan, PanLaw = PanLaw::sine3dB, std::size_t rampFrames = 0) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `pan` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `PanLaw` | `PanLaw::sine3dB` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `rampFrames` | `std::size_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Panner::process` {#symbol-panner-process}

```cpp
Status process(AudioBlock<const T> input, AudioBlock<T> stereoOutput) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `input` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `stereoOutput` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Panner::reset` {#symbol-panner-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

## `DryWetMixer` {#symbol-drywetmixer}

```cpp
template <Sample T> class DryWetMixer;
```

### `DryWetMixer::DryWetMixer<T>` {#symbol-drywetmixer-drywetmixer-t}

```cpp
DryWetMixer();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `DryWetMixer::~DryWetMixer<T>` {#symbol-drywetmixer-drywetmixer-t-2}

```cpp
~DryWetMixer();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `DryWetMixer::DryWetMixer<T>` {#symbol-drywetmixer-drywetmixer-t-3}

```cpp
DryWetMixer(DryWetMixer&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `DryWetMixer<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `DryWetMixer::operator=` {#symbol-drywetmixer-operator}

```cpp
DryWetMixer& operator=(DryWetMixer&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `DryWetMixer<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `DryWetMixer<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `DryWetMixer::prepare` {#symbol-drywetmixer-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t maximumWetLatency = 0) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `maximumWetLatency` | `std::size_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DryWetMixer::setMix` {#symbol-drywetmixer-setmix}

```cpp
Status setMix(double wet, MixLaw = MixLaw::linear) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `wet` | `double` | — | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `argument 2` | `MixLaw` | `MixLaw::linear` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DryWetMixer::setWetLatency` {#symbol-drywetmixer-setwetlatency}

```cpp
Status setWetLatency(std::size_t frames) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `frames` | `std::size_t` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DryWetMixer::pushDrySamples` {#symbol-drywetmixer-pushdrysamples}

```cpp
Status pushDrySamples(AudioBlock<const T>) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DryWetMixer::mixWetSamples` {#symbol-drywetmixer-mixwetsamples}

```cpp
Status mixWetSamples(AudioBlock<T>) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DryWetMixer::reset` {#symbol-drywetmixer-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

## `Shaper` {#symbol-shaper}

```cpp
enum class Shaper { tanh, atan, hardClip, cubic, fullWaveRectifier };
```

## `WaveShaper` {#symbol-waveshaper}

```cpp
template <Sample T> class WaveShaper;
```

### `WaveShaper::Function` {#symbol-waveshaper-function}

```cpp
using Function = T (*)(T, void*) noexcept;
```

### `WaveShaper::prepare` {#symbol-waveshaper-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `WaveShaper::setDrive` {#symbol-waveshaper-setdrive}

```cpp
Status setDrive(double linearGain) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `linearGain` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `WaveShaper::setShape` {#symbol-waveshaper-setshape}

```cpp
void setShape(Shaper shape) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `shape` | `Shaper` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `WaveShaper::setFunction` {#symbol-waveshaper-setfunction}

```cpp
void setFunction(Function function, void* context = nullptr) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `function` | `Function` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `context` | `void *` | `nullptr` | Contexte de callback emprunté ; l’application possède durée de vie et synchronisation. |

### `WaveShaper::reset` {#symbol-waveshaper-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `WaveShaper::process` {#symbol-waveshaper-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `Bias` {#symbol-bias}

```cpp
template <Sample T> class Bias;
```

### `Bias::prepare` {#symbol-bias-prepare}

```cpp
Status prepare(const ProcessSpec& spec) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `spec` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Bias::setBias` {#symbol-bias-setbias}

```cpp
Status setBias(T value) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `value` | `T` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Bias::reset` {#symbol-bias-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `Bias::process` {#symbol-bias-process}

```cpp
Status process(AudioBlock<const T> in, AudioBlock<T> out) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `in` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `out` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.
