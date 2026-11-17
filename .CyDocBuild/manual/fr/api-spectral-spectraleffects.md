---
id: api-spectral-spectraleffects
title: SpectralEffects
description: Freeze STFT, vocodeur, morphing/filtrage et décalage de fréquence fondé sur Hilbert.
section: Référence API
order: 1179
sourceRevision: 1
parent: api-spectral
symbol: cydsp::SpectralEffects
---

Freeze STFT, vocodeur, morphing/filtrage et décalage de fréquence fondé sur Hilbert.

## Inclusion {#include}

```cpp
#include <cydsp/spectral/SpectralEffects.h>
```

[Utilisation et contrats](doc:spectral) · [Règles communes de traitement](doc:contracts)

## `SpectralEffectMode` {#symbol-spectraleffectmode}

```cpp
enum class SpectralEffectMode { bypass, freeze, vocoder, morph, filter };
```

## `SpectralEffectParameters` {#symbol-spectraleffectparameters}

```cpp
struct SpectralEffectParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `mode` | `SpectralEffectMode` | `SpectralEffectMode::bypass` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `mix` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `envelopeRadiusBins` | `std::size_t` | `8` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `SpectralEffects` {#symbol-spectraleffects}

```cpp
template <Sample T> class SpectralEffects;
```

### `SpectralEffects::SpectralEffects<T>` {#symbol-spectraleffects-spectraleffects-t}

```cpp
SpectralEffects();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `SpectralEffects::~SpectralEffects<T>` {#symbol-spectraleffects-spectraleffects-t-2}

```cpp
~SpectralEffects();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `SpectralEffects::SpectralEffects<T>` {#symbol-spectraleffects-spectraleffects-t-3}

```cpp
SpectralEffects(SpectralEffects&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `SpectralEffects<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `SpectralEffects::operator=` {#symbol-spectraleffects-operator}

```cpp
SpectralEffects& operator=(SpectralEffects&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `SpectralEffects<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `SpectralEffects<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SpectralEffects::prepare` {#symbol-spectraleffects-prepare}

```cpp
Status prepare(const ProcessSpec&, const StftConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `const StftConfig &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SpectralEffects::setParameters` {#symbol-spectraleffects-setparameters}

```cpp
Status setParameters(const SpectralEffectParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const SpectralEffectParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SpectralEffects::setBinGains` {#symbol-spectraleffects-setbingains}

```cpp
Status setBinGains(std::span<const double>) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SpectralEffects::process` {#symbol-spectraleffects-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SpectralEffects::process` {#symbol-spectraleffects-process-2}

```cpp
Status process(AudioBlock<const T>, AudioBlock<const T> modulator, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `modulator` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 3` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SpectralEffects::reset` {#symbol-spectraleffects-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `SpectralEffects::latencyFrames` {#symbol-spectraleffects-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `FrequencyShifter` {#symbol-frequencyshifter}

```cpp
template <Sample T> class FrequencyShifter;
```

### `FrequencyShifter::FrequencyShifter<T>` {#symbol-frequencyshifter-frequencyshifter-t}

```cpp
FrequencyShifter();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `FrequencyShifter::~FrequencyShifter<T>` {#symbol-frequencyshifter-frequencyshifter-t-2}

```cpp
~FrequencyShifter();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `FrequencyShifter::FrequencyShifter<T>` {#symbol-frequencyshifter-frequencyshifter-t-3}

```cpp
FrequencyShifter(FrequencyShifter&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `FrequencyShifter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `FrequencyShifter::operator=` {#symbol-frequencyshifter-operator}

```cpp
FrequencyShifter& operator=(FrequencyShifter&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `FrequencyShifter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `FrequencyShifter<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `FrequencyShifter::prepare` {#symbol-frequencyshifter-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t hilbertTaps = 129, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `hilbertTaps` | `std::size_t` | `129` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `FrequencyShifter::setFrequency` {#symbol-frequencyshifter-setfrequency}

```cpp
Status setFrequency(double hz) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `hz` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `FrequencyShifter::process` {#symbol-frequencyshifter-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `FrequencyShifter::reset` {#symbol-frequencyshifter-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `FrequencyShifter::latencyFrames` {#symbol-frequencyshifter-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
