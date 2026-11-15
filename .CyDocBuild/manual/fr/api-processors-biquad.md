---
id: api-processors-biquad
title: Biquad
description: Coefficients normalisés stables, conceptions paramétrique/shelving et biquad multicanal préparé.
section: Référence API
order: 1130
sourceRevision: 1
parent: api-processors
symbol: cydsp::Biquad
---

Coefficients normalisés stables, conceptions paramétrique/shelving et biquad multicanal préparé.

## Inclusion {#include}

```cpp
#include <cydsp/processors/Biquad.h>
```

[Utilisation et contrats](doc:filters) · [Règles communes de traitement](doc:contracts)

## `BiquadType` {#symbol-biquadtype}

```cpp
enum class BiquadType { lowPass, highPass, bandPass, notch, allPass };
```

## `BiquadCoefficients` {#symbol-biquadcoefficients}

```cpp
struct BiquadCoefficients;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `b0` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `b1` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `b2` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `a1` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `a2` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `BiquadCoefficients::isStable` {#symbol-biquadcoefficients-isstable}

```cpp
constexpr bool isStable() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `designBiquad` {#symbol-designbiquad}

```cpp
Status designBiquad(BiquadType, double sampleRate, double frequency, double q, BiquadCoefficients& result) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `BiquadType` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `sampleRate` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `frequency` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `q` | `double` | — | Facteur de qualité du filtre positif et fini. |
| `result` | `BiquadCoefficients &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `designPeakingBiquad` {#symbol-designpeakingbiquad}

```cpp
Status designPeakingBiquad(double sampleRate, double frequency, double q, double gainDb, BiquadCoefficients&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `sampleRate` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `frequency` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `q` | `double` | — | Facteur de qualité du filtre positif et fini. |
| `gainDb` | `double` | — | Niveau logarithmique selon la référence dB indiquée. |
| `argument 5` | `BiquadCoefficients &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `designShelfBiquad` {#symbol-designshelfbiquad}

```cpp
Status designShelfBiquad(bool highShelf, double sampleRate, double frequency, double slope, double gainDb, BiquadCoefficients&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `highShelf` | `bool` | — | Option ou prédicat vrai/faux explicite. |
| `sampleRate` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `frequency` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `slope` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `gainDb` | `double` | — | Niveau logarithmique selon la référence dB indiquée. |
| `argument 6` | `BiquadCoefficients &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `PeakingBiquadDesigner` {#symbol-peakingbiquaddesigner}

```cpp
class PeakingBiquadDesigner;
```

### `PeakingBiquadDesigner::prepare` {#symbol-peakingbiquaddesigner-prepare}

```cpp
Status prepare(double sampleRate, double frequencyHz, double q) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `sampleRate` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `frequencyHz` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `q` | `double` | — | Facteur de qualité du filtre positif et fini. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `PeakingBiquadDesigner::design` {#symbol-peakingbiquaddesigner-design}

```cpp
Status design(double gainDb, BiquadCoefficients&) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `gainDb` | `double` | — | Niveau logarithmique selon la référence dB indiquée. |
| `argument 2` | `BiquadCoefficients &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `PeakingBiquadDesigner::designFor` {#symbol-peakingbiquaddesigner-designfor}

```cpp
template <Sample T> [[nodiscard]] Status designFor(double gainDb, BiquadCoefficients& output) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `gainDb` | `double` | — | Niveau logarithmique selon la référence dB indiquée. |
| `output` | `BiquadCoefficients &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `Biquad` {#symbol-biquad}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class Biquad;
```

### `Biquad::Biquad<T>` {#symbol-biquad-biquad-t}

```cpp
Biquad() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `Biquad::Biquad<T>` {#symbol-biquad-biquad-t-2}

```cpp
Biquad(const Biquad&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const Biquad<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `Biquad::operator=` {#symbol-biquad-operator}

```cpp
Biquad& operator=(const Biquad&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const Biquad<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Biquad<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Biquad::Biquad<T>` {#symbol-biquad-biquad-t-3}

```cpp
Biquad(Biquad&& other) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `Biquad<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `Biquad::operator=` {#symbol-biquad-operator-2}

```cpp
Biquad& operator=(Biquad&& other) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `Biquad<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Biquad<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Biquad::prepare` {#symbol-biquad-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Biquad::reset` {#symbol-biquad-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `Biquad::setCoefficients` {#symbol-biquad-setcoefficients}

```cpp
Status setCoefficients(const BiquadCoefficients&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const BiquadCoefficients &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Biquad::process` {#symbol-biquad-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.
