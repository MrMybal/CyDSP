---
id: api-processors-oversampler
title: Oversampler
description: Voies montantes/descendantes FIR, FIR demi-bande ou IIR préparées, avec leur latence.
section: Référence API
order: 1154
sourceRevision: 1
parent: api-processors
symbol: cydsp::Oversampler
---

Voies montantes/descendantes FIR, FIR demi-bande ou IIR préparées, avec leur latence.

## Inclusion {#include}

```cpp
#include <cydsp/processors/Oversampler.h>
```

[Utilisation et contrats](doc:resampling) · [Règles communes de traitement](doc:contracts)

## `OversamplingMode` {#symbol-oversamplingmode}

```cpp
enum class OversamplingMode { fir, halfBandFir, iir };
```

## `Oversampler` {#symbol-oversampler}

```cpp
template <Sample T> class Oversampler;
```

### `Oversampler::Oversampler<T>` {#symbol-oversampler-oversampler-t}

```cpp
Oversampler();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `Oversampler::~Oversampler<T>` {#symbol-oversampler-oversampler-t-2}

```cpp
~Oversampler();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `Oversampler::Oversampler<T>` {#symbol-oversampler-oversampler-t-3}

```cpp
Oversampler(Oversampler&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Oversampler<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `Oversampler::operator=` {#symbol-oversampler-operator}

```cpp
Oversampler& operator=(Oversampler&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Oversampler<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Oversampler<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Oversampler::prepare` {#symbol-oversampler-prepare}

```cpp
Status prepare(const ProcessSpec&, unsigned factor = 2, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `factor` | `unsigned int` | `2` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Oversampler::prepare` {#symbol-oversampler-prepare-2}

```cpp
Status prepare(const ProcessSpec&, unsigned factor, OversamplingMode, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `factor` | `unsigned int` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `OversamplingMode` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Oversampler::linearPhase` {#symbol-oversampler-linearphase}

```cpp
bool linearPhase() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Oversampler::latencyAtDcFrames` {#symbol-oversampler-latencyatdcframes}

```cpp
double latencyAtDcFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Oversampler::processUp` {#symbol-oversampler-processup}

```cpp
Status processUp(AudioBlock<const T>, AudioBlock<T>& highRateBlock) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `highRateBlock` | `AudioBlock<T> &` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Oversampler::processDown` {#symbol-oversampler-processdown}

```cpp
Status processDown(AudioBlock<T> output) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `output` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Oversampler::reset` {#symbol-oversampler-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `Oversampler::factor` {#symbol-oversampler-factor}

```cpp
unsigned factor() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `unsigned int` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Oversampler::latencyFrames` {#symbol-oversampler-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
