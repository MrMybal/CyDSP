---
id: api-processors-generators
title: Generators
description: Oscillateurs et enveloppes ADSR avec traitement par échantillon et par bloc.
section: Référence API
order: 1145
sourceRevision: 1
parent: api-processors
symbol: cydsp::AdsrParameters
---

Oscillateurs et enveloppes ADSR avec traitement par échantillon et par bloc.

## Inclusion {#include}

```cpp
#include <cydsp/processors/Generators.h>
```

[Utilisation et contrats](doc:synthesis) · [Règles communes de traitement](doc:contracts)

## `AdsrParameters` {#symbol-adsrparameters}

```cpp
struct AdsrParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `attackSeconds` | `double` | `0.01` | Durée ou position en secondes. |
| `decaySeconds` | `double` | `0.1` | Durée ou position en secondes. |
| `sustain` | `double` | `0.7` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `releaseSeconds` | `double` | `0.2` | Durée ou position en secondes. |

## `Adsr` {#symbol-adsr}

```cpp
class Adsr;
```

### `Adsr::prepare` {#symbol-adsr-prepare}

```cpp
Status prepare(double sampleRate) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `sampleRate` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Adsr::setParameters` {#symbol-adsr-setparameters}

```cpp
Status setParameters(const AdsrParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const AdsrParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Adsr::reset` {#symbol-adsr-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `Adsr::noteOn` {#symbol-adsr-noteon}

```cpp
void noteOn() noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

### `Adsr::noteOff` {#symbol-adsr-noteoff}

```cpp
void noteOff() noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

### `Adsr::next` {#symbol-adsr-next}

```cpp
double next() noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Adsr::process` {#symbol-adsr-process}

```cpp
void process(std::span<float> output) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `output` | `std::span<float>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

### `Adsr::process` {#symbol-adsr-process-2}

```cpp
void process(std::span<double> output) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `output` | `std::span<double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

### `Adsr::active` {#symbol-adsr-active}

```cpp
bool active() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Adsr::value` {#symbol-adsr-value}

```cpp
double value() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `Waveform` {#symbol-waveform}

```cpp
enum class Waveform { sine, triangle, saw, square, whiteNoise };
```

## `Oscillator` {#symbol-oscillator}

```cpp
template <Sample T> class Oscillator;
```

### `Oscillator::prepare` {#symbol-oscillator-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Oscillator::setFrequency` {#symbol-oscillator-setfrequency}

```cpp
Status setFrequency(double hz) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `hz` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Oscillator::setWaveform` {#symbol-oscillator-setwaveform}

```cpp
void setWaveform(Waveform value) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `value` | `Waveform` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `Oscillator::reset` {#symbol-oscillator-reset}

```cpp
void reset(double phase = 0) noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `phase` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `Oscillator::next` {#symbol-oscillator-next}

```cpp
T next() noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

Retour : `T` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Oscillator::process` {#symbol-oscillator-process}

```cpp
Status process(AudioBlock<T> output, bool add = false) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `output` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |
| `add` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

Retour : `Status` — Vérifier Status ; ok indique le succès.
