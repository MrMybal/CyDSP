---
id: api-spatial-doppler
title: Doppler
description: Doppler par rééchantillonnage variable préparé et comptes explicites de flux.
section: Référence API
order: 1171
sourceRevision: 1
parent: api-spatial
symbol: cydsp::DopplerResampler
---

Doppler par rééchantillonnage variable préparé et comptes explicites de flux.

## Inclusion {#include}

```cpp
#include <cydsp/spatial/Doppler.h>
```

[Utilisation et contrats](doc:spatial) · [Règles communes de traitement](doc:contracts)

## `DopplerResampler` {#symbol-dopplerresampler}

```cpp
template <Sample T> class DopplerResampler;
```

### `DopplerResampler::prepare` {#symbol-dopplerresampler-prepare}

```cpp
Status prepare(const ResampleSpec& spec, double maximumSemitones = 24, CpuBackend backend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `spec` | `const ResampleSpec &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `maximumSemitones` | `double` | `24` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `backend` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DopplerResampler::setMotion` {#symbol-dopplerresampler-setmotion}

```cpp
Status setMotion(const RelativeSource& source, const AcousticMedium& medium, std::size_t rampFrames = 0, double intensity = 1) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `source` | `const RelativeSource &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `medium` | `const AcousticMedium &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `rampFrames` | `std::size_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `intensity` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DopplerResampler::process` {#symbol-dopplerresampler-process}

```cpp
ResampleResult process(AudioBlock<const T> input, AudioBlock<T> output) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `input` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `output` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `ResampleResult` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `DopplerResampler::finish` {#symbol-dopplerresampler-finish}

```cpp
ResampleResult finish(AudioBlock<T> output) noexcept;
```

Vider la queue documentée et vérifier la fin ; réinitialiser avant un nouveau flux.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `output` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `ResampleResult` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `DopplerResampler::reset` {#symbol-dopplerresampler-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `DopplerResampler::ratio` {#symbol-dopplerresampler-ratio}

```cpp
double ratio() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `DopplerResampler::latencyInputFrames` {#symbol-dopplerresampler-latencyinputframes}

```cpp
double latencyInputFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
