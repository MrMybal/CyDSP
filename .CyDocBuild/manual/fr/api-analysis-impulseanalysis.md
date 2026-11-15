---
id: api-analysis-impulseanalysis
title: ImpulseAnalysis
description: Onset, premières réflexions, décroissance/RT60 et correspondance nominale FDN depuis une IR en mémoire.
section: Référence API
order: 1102
sourceRevision: 1
parent: api-analysis
symbol: cydsp::DecayFit
---

Onset, premières réflexions, décroissance/RT60 et correspondance nominale FDN depuis une IR en mémoire.

## Inclusion {#include}

```cpp
#include <cydsp/analysis/ImpulseAnalysis.h>
```

[Utilisation et contrats](doc:diagnostics) · [Règles communes de traitement](doc:contracts)

## `DecayFit` {#symbol-decayfit}

```cpp
struct DecayFit;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `seconds` | `double` | `0` | Durée ou position en secondes. |
| `rSquared` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `valid` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

## `ImpulseBandAnalysis` {#symbol-impulsebandanalysis}

```cpp
struct ImpulseBandAnalysis;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `centerHz` | `double` | `0` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `energy` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `edt` | `DecayFit` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `t20` | `DecayFit` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `t30` | `DecayFit` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `EarlyReflection` {#symbol-earlyreflection}

```cpp
struct EarlyReflection;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `seconds` | `double` | `0` | Durée ou position en secondes. |
| `amplitude` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `ImpulseAnalysisConfig` {#symbol-impulseanalysisconfig}

```cpp
struct ImpulseAnalysisConfig;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `onsetThresholdDb` | `double` | `-40` | Niveau logarithmique selon la référence dB indiquée. |
| `directWindowMs` | `double` | `2.5` | Durée en millisecondes. |
| `reflectionThresholdDb` | `double` | `-20` | Niveau logarithmique selon la référence dB indiquée. |
| `subtractTailNoise` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

## `ImpulseReport` {#symbol-impulsereport}

```cpp
struct ImpulseReport;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `silent` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `noiseCompensated` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `onsetSeconds` | `double` | `0` | Durée ou position en secondes. |
| `peakSeconds` | `double` | `0` | Durée ou position en secondes. |
| `energy` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `centerTimeSeconds` | `double` | `0` | Durée ou position en secondes. |
| `clarity50Db` | `double` | `0` | Niveau logarithmique selon la référence dB indiquée. |
| `clarity80Db` | `double` | `0` | Niveau logarithmique selon la référence dB indiquée. |
| `definition50` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `directToReverberantDb` | `double` | `0` | Niveau logarithmique selon la référence dB indiquée. |
| `broadband` | `ImpulseBandAnalysis` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `octaveBands` | `std::array<ImpulseBandAnalysis, 9>` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `reflections` | `std::array<EarlyReflection, 32>` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `reflectionCount` | `std::size_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `analyzeImpulse` {#symbol-analyzeimpulse}

```cpp
Status analyzeImpulse(std::span<const float>, double sampleRate, ImpulseReport&, const ImpulseAnalysisConfig& = {}) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const float>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `sampleRate` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `argument 3` | `ImpulseReport &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |
| `argument 4` | `const ImpulseAnalysisConfig &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `analyzeImpulse` {#symbol-analyzeimpulse-2}

```cpp
Status analyzeImpulse(std::span<const double>, double sampleRate, ImpulseReport&, const ImpulseAnalysisConfig& = {}) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `sampleRate` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `argument 3` | `ImpulseReport &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |
| `argument 4` | `const ImpulseAnalysisConfig &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `matchFdnToImpulse` {#symbol-matchfdntoimpulse}

```cpp
Status matchFdnToImpulse(const ImpulseReport&, FdnParameters&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ImpulseReport &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `FdnParameters &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.
