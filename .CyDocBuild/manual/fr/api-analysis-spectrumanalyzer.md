---
id: api-analysis-spectrumanalyzer
title: SpectrumAnalyzer
description: Spectres d’amplitude et PSD par FFT réelle, caractéristiques et bandes Mel/logarithmiques.
section: Référence API
order: 1106
sourceRevision: 1
parent: api-analysis
symbol: cydsp::SpectrumAnalyzer
---

Spectres d’amplitude et PSD par FFT réelle, caractéristiques et bandes Mel/logarithmiques.

## Inclusion {#include}

```cpp
#include <cydsp/analysis/SpectrumAnalyzer.h>
```

[Utilisation et contrats](doc:analysis) · [Règles communes de traitement](doc:contracts)

## `SpectrumConfig` {#symbol-spectrumconfig}

```cpp
struct SpectrumConfig;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `fftFrames` | `std::size_t` | `2048` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `hopFrames` | `std::size_t` | `512` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `window` | `WindowType` | `WindowType::hann` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `SpectralFeatures` {#symbol-spectralfeatures}

```cpp
struct SpectralFeatures;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `rms` | `double` | `0` | Durée en millisecondes. |
| `centroidHz` | `double` | `0` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `rolloffHz` | `double` | `0` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `flatness` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `positiveFlux` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `dominantHz` | `double` | `0` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |

## `SpectrumAnalyzer` {#symbol-spectrumanalyzer}

```cpp
template <Sample T> class SpectrumAnalyzer;
```

### `SpectrumAnalyzer::SpectrumAnalyzer<T>` {#symbol-spectrumanalyzer-spectrumanalyzer-t}

```cpp
SpectrumAnalyzer();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `SpectrumAnalyzer::~SpectrumAnalyzer<T>` {#symbol-spectrumanalyzer-spectrumanalyzer-t-2}

```cpp
~SpectrumAnalyzer();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `SpectrumAnalyzer::SpectrumAnalyzer<T>` {#symbol-spectrumanalyzer-spectrumanalyzer-t-3}

```cpp
SpectrumAnalyzer(SpectrumAnalyzer&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `SpectrumAnalyzer<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `SpectrumAnalyzer::operator=` {#symbol-spectrumanalyzer-operator}

```cpp
SpectrumAnalyzer& operator=(SpectrumAnalyzer&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `SpectrumAnalyzer<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `SpectrumAnalyzer<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SpectrumAnalyzer::prepare` {#symbol-spectrumanalyzer-prepare}

```cpp
Status prepare(const ProcessSpec&, const SpectrumConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `const SpectrumConfig &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SpectrumAnalyzer::process` {#symbol-spectrumanalyzer-process}

```cpp
Status process(AudioBlock<const T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SpectrumAnalyzer::reset` {#symbol-spectrumanalyzer-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `SpectrumAnalyzer::hasFrame` {#symbol-spectrumanalyzer-hasframe}

```cpp
bool hasFrame() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SpectrumAnalyzer::frameCount` {#symbol-spectrumanalyzer-framecount}

```cpp
std::uint64_t frameCount() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::uint64_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SpectrumAnalyzer::bins` {#symbol-spectrumanalyzer-bins}

```cpp
std::size_t bins() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SpectrumAnalyzer::binFrequency` {#symbol-spectrumanalyzer-binfrequency}

```cpp
double binFrequency(std::size_t bin) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `bin` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SpectrumAnalyzer::magnitudes` {#symbol-spectrumanalyzer-magnitudes}

```cpp
std::span<const double> magnitudes(std::size_t channel) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Indice/compte de canaux dans la disposition planaire déclarée. |

Retour : `std::span<const double>` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SpectrumAnalyzer::powerDensity` {#symbol-spectrumanalyzer-powerdensity}

```cpp
std::span<const double> powerDensity(std::size_t channel) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Indice/compte de canaux dans la disposition planaire déclarée. |

Retour : `std::span<const double>` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SpectrumAnalyzer::complexSpectrum` {#symbol-spectrumanalyzer-complexspectrum}

```cpp
std::span<const std::complex<double>> complexSpectrum(std::size_t channel) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Indice/compte de canaux dans la disposition planaire déclarée. |

Retour : `std::span<const std::complex<double>>` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SpectrumAnalyzer::features` {#symbol-spectrumanalyzer-features}

```cpp
SpectralFeatures features(std::size_t channel) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Indice/compte de canaux dans la disposition planaire déclarée. |

Retour : `SpectralFeatures` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SpectrumAnalyzer::backend` {#symbol-spectrumanalyzer-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `SpectralBandScale` {#symbol-spectralbandscale}

```cpp
enum class SpectralBandScale { mel, logarithmic };
```

## `SpectralBandConfig` {#symbol-spectralbandconfig}

```cpp
struct SpectralBandConfig;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `bands` | `std::size_t` | `40` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `minimumHz` | `double` | `20` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `maximumHz` | `double` | `0` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `scale` | `SpectralBandScale` | `SpectralBandScale::mel` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `SpectralBands` {#symbol-spectralbands}

```cpp
class SpectralBands;
```

### `SpectralBands::SpectralBands` {#symbol-spectralbands-spectralbands}

```cpp
SpectralBands();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `SpectralBands::~SpectralBands` {#symbol-spectralbands-spectralbands-2}

```cpp
~SpectralBands();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `SpectralBands::SpectralBands` {#symbol-spectralbands-spectralbands-3}

```cpp
SpectralBands(SpectralBands&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `SpectralBands &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `SpectralBands::operator=` {#symbol-spectralbands-operator}

```cpp
SpectralBands& operator=(SpectralBands&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `SpectralBands &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `SpectralBands &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SpectralBands::prepare` {#symbol-spectralbands-prepare}

```cpp
Status prepare(double sampleRate, std::size_t fftFrames, const SpectralBandConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `sampleRate` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `fftFrames` | `std::size_t` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `argument 3` | `const SpectralBandConfig &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SpectralBands::process` {#symbol-spectralbands-process}

```cpp
Status process(std::span<const double> bins, std::span<double> bands) const noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `bins` | `std::span<const double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `bands` | `std::span<double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SpectralBands::centerFrequencies` {#symbol-spectralbands-centerfrequencies}

```cpp
std::span<const double> centerFrequencies() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::span<const double>` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SpectralBands::backend` {#symbol-spectralbands-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
