---
id: api-processors-filterdesign
title: FilterDesign
description: Conception passe-bas/haut Butterworth, Chebyshev I/II, elliptique et Bessel.
section: Référence API
order: 1142
sourceRevision: 1
parent: api-processors
symbol: cydsp::IirFamily
---

Conception passe-bas/haut Butterworth, Chebyshev I/II, elliptique et Bessel.

## Inclusion {#include}

```cpp
#include <cydsp/processors/FilterDesign.h>
```

[Utilisation et contrats](doc:filter-design) · [Règles communes de traitement](doc:contracts)

## `IirFamily` {#symbol-iirfamily}

```cpp
enum class IirFamily { butterworth, chebyshevI, chebyshevII, elliptic, bessel };
```

## `IirDesign` {#symbol-iirdesign}

```cpp
struct IirDesign;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `family` | `IirFamily` | `IirFamily::butterworth` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `response` | `BiquadType` | `BiquadType::lowPass` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `order` | `unsigned int` | `4` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `sampleRate` | `double` | `48000` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `cutoff` | `double` | `1000` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `rippleDb` | `double` | `1` | Niveau logarithmique selon la référence dB indiquée. |
| `stopBandDb` | `double` | `80` | Niveau logarithmique selon la référence dB indiquée. |
| `ellipticRolloff` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `designIir` {#symbol-designiir}

```cpp
Status designIir(const IirDesign&, std::span<BiquadCoefficients>, std::size_t& count) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const IirDesign &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `std::span<BiquadCoefficients>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `count` | `std::size_t &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.
