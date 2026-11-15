---
id: api-processors-advancedfilterdesign
title: AdvancedFilterDesign
description: Conceptions de bandes d’ordre élevé, égalisation FIR et conversion FIR en phase minimale.
section: Référence API
order: 1126
sourceRevision: 1
parent: api-processors
symbol: cydsp::IirBandDesign
---

Conceptions de bandes d’ordre élevé, égalisation FIR et conversion FIR en phase minimale.

## Inclusion {#include}

```cpp
#include <cydsp/processors/AdvancedFilterDesign.h>
```

[Utilisation et contrats](doc:filter-design) · [Règles communes de traitement](doc:contracts)

## `IirBandDesign` {#symbol-iirbanddesign}

```cpp
struct IirBandDesign;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `prototype` | `IirDesign` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `lowHz` | `double` | `500` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `highHz` | `double` | `2000` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `bandStop` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

## `designBandIir` {#symbol-designbandiir}

```cpp
Status designBandIir(const IirBandDesign&, std::span<BiquadCoefficients>, std::size_t& count) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const IirBandDesign &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `std::span<BiquadCoefficients>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `count` | `std::size_t &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `FirEqPoint` {#symbol-fireqpoint}

```cpp
struct FirEqPoint;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `frequencyHz` | `double` | `0` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `gainDb` | `double` | `0` | Niveau logarithmique selon la référence dB indiquée. |

## `FirPhase` {#symbol-firphase}

```cpp
enum class FirPhase { linear, minimum };
```

## `designFirEqualizer` {#symbol-designfirequalizer}

```cpp
Status designFirEqualizer(double sampleRate, std::span<const FirEqPoint>, std::span<double> taps, FirPhase = FirPhase::linear, CpuBackend = CpuBackend::automatic) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `sampleRate` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `argument 2` | `std::span<const FirEqPoint>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `taps` | `std::span<double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 4` | `FirPhase` | `FirPhase::linear` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `minimumPhaseFir` {#symbol-minimumphasefir}

```cpp
Status minimumPhaseFir(std::span<const double> input, std::span<double> output, double floorDb = -180, CpuBackend = CpuBackend::automatic) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `input` | `std::span<const double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `output` | `std::span<double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `floorDb` | `double` | `-180` | Niveau logarithmique selon la référence dB indiquée. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.
