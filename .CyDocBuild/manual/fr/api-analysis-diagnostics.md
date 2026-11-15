---
id: api-analysis-diagnostics
title: Diagnostics
description: Réponse fréquence/phase, transfert/cohérence, distorsion et sweeps de mesure.
section: Référence API
order: 1101
sourceRevision: 1
parent: api-analysis
symbol: cydsp::FilterResponse
---

Réponse fréquence/phase, transfert/cohérence, distorsion et sweeps de mesure.

## Inclusion {#include}

```cpp
#include <cydsp/analysis/Diagnostics.h>
```

[Utilisation et contrats](doc:diagnostics) · [Règles communes de traitement](doc:contracts)

## `FilterResponse` {#symbol-filterresponse}

```cpp
struct FilterResponse;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `gain` | `std::complex<double>` | `{1, 0}` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `phaseRadians` | `double` | `0` | Angle en radians. |
| `groupDelayFrames` | `double` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

## `firResponse` {#symbol-firresponse}

```cpp
Status firResponse(std::span<const double>, double sampleRate, double frequencyHz, FilterResponse&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `sampleRate` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `frequencyHz` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `argument 4` | `FilterResponse &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `iirResponse` {#symbol-iirresponse}

```cpp
Status iirResponse(std::span<const BiquadCoefficients>, double sampleRate, double frequencyHz, FilterResponse&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const BiquadCoefficients>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `sampleRate` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `frequencyHz` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `argument 4` | `FilterResponse &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `TransferBin` {#symbol-transferbin}

```cpp
struct TransferBin;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `gain` | `std::complex<double>` | `{}` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `phaseRadians` | `double` | `0` | Angle en radians. |
| `groupDelayFrames` | `double` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `coherence` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `TransferAnalyzer` {#symbol-transferanalyzer}

```cpp
template <Sample T> class TransferAnalyzer;
```

### `TransferAnalyzer::TransferAnalyzer<T>` {#symbol-transferanalyzer-transferanalyzer-t}

```cpp
TransferAnalyzer();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `TransferAnalyzer::~TransferAnalyzer<T>` {#symbol-transferanalyzer-transferanalyzer-t-2}

```cpp
~TransferAnalyzer();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `TransferAnalyzer::TransferAnalyzer<T>` {#symbol-transferanalyzer-transferanalyzer-t-3}

```cpp
TransferAnalyzer(TransferAnalyzer&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `TransferAnalyzer<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `TransferAnalyzer::operator=` {#symbol-transferanalyzer-operator}

```cpp
TransferAnalyzer& operator=(TransferAnalyzer&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `TransferAnalyzer<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `TransferAnalyzer<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `TransferAnalyzer::prepare` {#symbol-transferanalyzer-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t fftFrames = 2048, std::size_t hopFrames = 512, std::size_t averages = 32, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `fftFrames` | `std::size_t` | `2048` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `hopFrames` | `std::size_t` | `512` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `averages` | `std::size_t` | `32` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `TransferAnalyzer::process` {#symbol-transferanalyzer-process}

```cpp
Status process(AudioBlock<const T> reference, AudioBlock<const T> measured) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `reference` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `measured` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `TransferAnalyzer::bin` {#symbol-transferanalyzer-bin}

```cpp
TransferBin bin(std::size_t channel, std::size_t index) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Indice/compte de canaux dans la disposition planaire déclarée. |
| `index` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `TransferBin` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `TransferAnalyzer::bins` {#symbol-transferanalyzer-bins}

```cpp
std::size_t bins() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `TransferAnalyzer::analyzedFrames` {#symbol-transferanalyzer-analyzedframes}

```cpp
std::uint64_t analyzedFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::uint64_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `TransferAnalyzer::reset` {#symbol-transferanalyzer-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

## `DistortionReport` {#symbol-distortionreport}

```cpp
struct DistortionReport;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `fundamentalRms` | `double` | `0` | Durée en millisecondes. |
| `harmonicsRms` | `double` | `0` | Durée en millisecondes. |
| `noiseRms` | `double` | `0` | Durée en millisecondes. |
| `dc` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `thd` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `thdPlusNoise` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `measuredHarmonics` | `unsigned int` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `measureDistortion` {#symbol-measuredistortion}

```cpp
template <Sample T> [[nodiscard]] Status measureDistortion(std::span<const T>, double sampleRate, double fundamentalHz, DistortionReport&, unsigned harmonics = 10) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `sampleRate` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `fundamentalHz` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `argument 4` | `DistortionReport &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |
| `harmonics` | `unsigned int` | `10` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `MeasurementSignal` {#symbol-measurementsignal}

```cpp
enum class MeasurementSignal { sine, whiteNoise, impulse, linearSweep, logSweep };
```

## `MeasurementSignalParameters` {#symbol-measurementsignalparameters}

```cpp
struct MeasurementSignalParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `kind` | `MeasurementSignal` | `MeasurementSignal::logSweep` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `startHz` | `double` | `20` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `endHz` | `double` | `20000` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `durationSeconds` | `double` | `5` | Durée ou position en secondes. |
| `amplitude` | `double` | `.5` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `fadeSeconds` | `double` | `.01` | Durée ou position en secondes. |

## `MeasurementGenerator` {#symbol-measurementgenerator}

```cpp
template <Sample T> class MeasurementGenerator;
```

### `MeasurementGenerator::prepare` {#symbol-measurementgenerator-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `MeasurementGenerator::setParameters` {#symbol-measurementgenerator-setparameters}

```cpp
Status setParameters(const MeasurementSignalParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const MeasurementSignalParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `MeasurementGenerator::process` {#symbol-measurementgenerator-process}

```cpp
Status process(AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `MeasurementGenerator::reset` {#symbol-measurementgenerator-reset}

```cpp
void reset(std::uint64_t seed = 1) noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `seed` | `std::uint64_t` | `1` | Graine aléatoire déterministe par instance. |

## `inverseLogSweep` {#symbol-inverselogsweep}

```cpp
template <Sample T> [[nodiscard]] Status inverseLogSweep(std::span<const T> sweep, double startHz, double endHz, std::span<T> inverse) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `sweep` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `startHz` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `endHz` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `inverse` | `std::span<T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `deconvolveSweep` {#symbol-deconvolvesweep}

```cpp
template <Sample T> [[nodiscard]] Status deconvolveSweep(std::span<const T> recording, std::span<const T> excitation, std::span<T> impulse, double regularizationDb = -120, CpuBackend = CpuBackend::automatic) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `recording` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `excitation` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `impulse` | `std::span<T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `regularizationDb` | `double` | `-120` | Niveau logarithmique selon la référence dB indiquée. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.
