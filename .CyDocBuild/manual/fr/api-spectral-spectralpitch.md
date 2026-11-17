---
id: api-spectral-spectralpitch
title: SpectralPitch
description: Transposition par vocodeur de phase avec phases liées entre canaux, compensation de formants et protection des transitoires.
section: Référence API
order: 1180
sourceRevision: 1
parent: api-spectral
symbol: cydsp::SpectralPitchParameters
---

Transposition par vocodeur de phase avec phases liées entre canaux, compensation de formants et protection des transitoires.

## Inclusion {#include}

```cpp
#include <cydsp/spectral/SpectralPitch.h>
```

[Utilisation et contrats](doc:pitch) · [Règles communes de traitement](doc:contracts)

## `SpectralPitchParameters` {#symbol-spectralpitchparameters}

```cpp
struct SpectralPitchParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `ratio` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `preserveFormants` | `bool` | `true` | Option ou prédicat vrai/faux explicite. |
| `protectTransients` | `bool` | `true` | Option ou prédicat vrai/faux explicite. |
| `lockSpectralPeaks` | `bool` | `true` | Option ou prédicat vrai/faux explicite. |
| `envelopeWidthHz` | `double` | `150` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `transientFluxThreshold` | `double` | `.35` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `SpectralPitchShifter` {#symbol-spectralpitchshifter}

```cpp
template <Sample T> class SpectralPitchShifter;
```

### `SpectralPitchShifter::SpectralPitchShifter<T>` {#symbol-spectralpitchshifter-spectralpitchshifter-t}

```cpp
SpectralPitchShifter();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `SpectralPitchShifter::~SpectralPitchShifter<T>` {#symbol-spectralpitchshifter-spectralpitchshifter-t-2}

```cpp
~SpectralPitchShifter();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `SpectralPitchShifter::SpectralPitchShifter<T>` {#symbol-spectralpitchshifter-spectralpitchshifter-t-3}

```cpp
SpectralPitchShifter(SpectralPitchShifter&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `SpectralPitchShifter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `SpectralPitchShifter::operator=` {#symbol-spectralpitchshifter-operator}

```cpp
SpectralPitchShifter& operator=(SpectralPitchShifter&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `SpectralPitchShifter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `SpectralPitchShifter<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SpectralPitchShifter::prepare` {#symbol-spectralpitchshifter-prepare}

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

### `SpectralPitchShifter::setParameters` {#symbol-spectralpitchshifter-setparameters}

```cpp
Status setParameters(const SpectralPitchParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const SpectralPitchParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SpectralPitchShifter::process` {#symbol-spectralpitchshifter-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SpectralPitchShifter::reset` {#symbol-spectralpitchshifter-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `SpectralPitchShifter::latencyFrames` {#symbol-spectralpitchshifter-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
