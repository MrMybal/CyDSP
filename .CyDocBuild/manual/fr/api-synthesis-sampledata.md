---
id: api-synthesis-sampledata
title: SampleData
description: Samples copiés en mémoire, mips antirepliement préparés, interpolation et fondus de boucles.
section: Référence API
order: 1186
sourceRevision: 1
parent: api-synthesis
symbol: cydsp::SampleData
---

Samples copiés en mémoire, mips antirepliement préparés, interpolation et fondus de boucles.

## Inclusion {#include}

```cpp
#include <cydsp/synthesis/SampleData.h>
```

[Utilisation et contrats](doc:sampling) · [Règles communes de traitement](doc:contracts)

## `SampleInterpolation` {#symbol-sampleinterpolation}

```cpp
enum class SampleInterpolation { linear, bandLimited };
```

## `SampleData` {#symbol-sampledata}

```cpp
template <Sample T> class SampleData;
```

### `SampleData::prepare` {#symbol-sampledata-prepare}

```cpp
Status prepare(AudioBlock<const T>, double sampleRate, unsigned rootNote = 60, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `sampleRate` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `rootNote` | `unsigned int` | `60` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SampleData::setLoop` {#symbol-sampledata-setloop}

```cpp
Status setLoop(std::size_t first, std::size_t end, std::size_t crossfadeFrames = 0) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `first` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `end` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `crossfadeFrames` | `std::size_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SampleData::samples` {#symbol-sampledata-samples}

```cpp
AudioBlock<const T> samples() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `AudioBlock<const T>` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SampleData::sampleRate` {#symbol-sampledata-samplerate}

```cpp
double sampleRate() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SampleData::rootNote` {#symbol-sampledata-rootnote}

```cpp
unsigned rootNote() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `unsigned int` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SampleData::loopStart` {#symbol-sampledata-loopstart}

```cpp
std::size_t loopStart() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SampleData::loopEnd` {#symbol-sampledata-loopend}

```cpp
std::size_t loopEnd() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SampleData::loopCrossfade` {#symbol-sampledata-loopcrossfade}

```cpp
std::size_t loopCrossfade() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SampleData::mipLevels` {#symbol-sampledata-miplevels}

```cpp
std::size_t mipLevels() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SampleData::value` {#symbol-sampledata-value}

```cpp
T value(std::size_t channel, double position, double ratio = 1, SampleInterpolation = SampleInterpolation::bandLimited) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Indice/compte de canaux dans la disposition planaire déclarée. |
| `position` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `ratio` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 4` | `SampleInterpolation` | `SampleInterpolation::bandLimited` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `T` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SampleData::render` {#symbol-sampledata-render}

```cpp
Status render(double& position, double ratio, SampleInterpolation, AudioBlock<T> output, std::size_t& inside) const noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `position` | `double &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |
| `ratio` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `SampleInterpolation` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `output` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |
| `inside` | `std::size_t &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.
