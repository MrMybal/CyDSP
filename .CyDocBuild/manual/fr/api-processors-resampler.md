---
id: api-processors-resampler
title: Resampler
description: Conversion sinc à taux fixe/variable avec comptes de consommation/production obligatoires.
section: Référence API
order: 1159
sourceRevision: 1
parent: api-processors
symbol: cydsp::Resampler
---

Conversion sinc à taux fixe/variable avec comptes de consommation/production obligatoires.

## Inclusion {#include}

```cpp
#include <cydsp/processors/Resampler.h>
```

[Utilisation et contrats](doc:resampling) · [Règles communes de traitement](doc:contracts)

## `ResampleSpec` {#symbol-resamplespec}

```cpp
struct ResampleSpec;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `inputRate` | `std::uint32_t` | `48000` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `outputRate` | `std::uint32_t` | `44100` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `channels` | `std::size_t` | `2` | Indice/compte de canaux dans la disposition planaire déclarée. |
| `maxInputFrames` | `std::size_t` | `512` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `maxOutputFrames` | `std::size_t` | `1024` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `taps` | `std::size_t` | `128` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `phases` | `std::size_t` | `1024` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `minimumRatio` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `maximumRatio` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `ResampleSpec::isValid` {#symbol-resamplespec-isvalid}

```cpp
constexpr bool isValid() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `ResampleQuality` {#symbol-resamplequality}

```cpp
enum class ResampleQuality { economy, balanced, high };
```

## `resampleSpecForQuality` {#symbol-resamplespecforquality}

```cpp
constexpr ResampleSpec resampleSpecForQuality(ResampleSpec spec, ResampleQuality quality) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `spec` | `ResampleSpec` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `quality` | `ResampleQuality` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `ResampleSpec` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `ResampleResult` {#symbol-resampleresult}

```cpp
struct ResampleResult;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `status` | `Status` | `Status::ok` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `inputConsumed` | `std::size_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `outputProduced` | `std::size_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `Resampler` {#symbol-resampler}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class Resampler;
```

### `Resampler::Resampler<T>` {#symbol-resampler-resampler-t}

```cpp
Resampler() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `Resampler::Resampler<T>` {#symbol-resampler-resampler-t-2}

```cpp
Resampler(const Resampler&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const Resampler<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `Resampler::operator=` {#symbol-resampler-operator}

```cpp
Resampler& operator=(const Resampler&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const Resampler<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Resampler<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Resampler::Resampler<T>` {#symbol-resampler-resampler-t-3}

```cpp
Resampler(Resampler&& other) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `Resampler<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `Resampler::operator=` {#symbol-resampler-operator-2}

```cpp
Resampler& operator=(Resampler&& other) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `Resampler<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Resampler<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Resampler::prepare` {#symbol-resampler-prepare}

```cpp
Status prepare(const ResampleSpec&, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ResampleSpec &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Resampler::setRates` {#symbol-resampler-setrates}

```cpp
Status setRates(std::uint32_t inputRate, std::uint32_t outputRate) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `inputRate` | `std::uint32_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `outputRate` | `std::uint32_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Resampler::setRatio` {#symbol-resampler-setratio}

```cpp
Status setRatio(double inputPerOutput, std::size_t outputRampFrames = 0) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `inputPerOutput` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `outputRampFrames` | `std::size_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Resampler::ratio` {#symbol-resampler-ratio}

```cpp
double ratio() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Resampler::ratioTransitionRemainingFrames` {#symbol-resampler-ratiotransitionremainingframes}

```cpp
std::size_t ratioTransitionRemainingFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Resampler::reset` {#symbol-resampler-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `Resampler::finish` {#symbol-resampler-finish}

```cpp
ResampleResult finish(AudioBlock<T>) noexcept;
```

Vider la queue documentée et vérifier la fin ; réinitialiser avant un nouveau flux.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `ResampleResult` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Resampler::isFinished` {#symbol-resampler-isfinished}

```cpp
bool isFinished() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Resampler::process` {#symbol-resampler-process}

```cpp
ResampleResult process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `ResampleResult` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Resampler::backend` {#symbol-resampler-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Resampler::latencyInputFrames` {#symbol-resampler-latencyinputframes}

```cpp
double latencyInputFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
