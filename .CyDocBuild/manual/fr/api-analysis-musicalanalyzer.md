---
id: api-analysis-musicalanalyzer
title: MusicalAnalyzer
description: Estimations YIN de hauteur/confiance, attaques, tempo/battements, chroma, MFCC et tonalité.
section: Référence API
order: 1105
sourceRevision: 1
parent: api-analysis
symbol: cydsp::MusicalAnalyzer
---

Estimations YIN de hauteur/confiance, attaques, tempo/battements, chroma, MFCC et tonalité.

## Inclusion {#include}

```cpp
#include <cydsp/analysis/MusicalAnalyzer.h>
```

[Utilisation et contrats](doc:analysis) · [Règles communes de traitement](doc:contracts)

## `MusicalAnalysisConfig` {#symbol-musicalanalysisconfig}

```cpp
struct MusicalAnalysisConfig;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `fftFrames` | `std::size_t` | `2048` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `hopFrames` | `std::size_t` | `256` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `melBands` | `std::size_t` | `40` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `mfccCount` | `std::size_t` | `13` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `minimumPitchHz` | `double` | `60` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `maximumPitchHz` | `double` | `2000` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `onsetThreshold` | `double` | `1.5` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `minimumBpm` | `double` | `50` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `maximumBpm` | `double` | `220` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `MusicalFeatures` {#symbol-musicalfeatures}

```cpp
struct MusicalFeatures;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `fundamentalHz` | `double` | `0` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `pitchConfidence` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `onsetStrength` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `tempoBpm` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `beatConfidence` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `beatPhase` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `keyConfidence` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `onset` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `keyMinor` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `keyPitchClass` | `unsigned int` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `chroma` | `std::array<double, 12>` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `mfcc` | `std::array<double, 32>` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `frameEnd` | `std::uint64_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `MusicalAnalyzer` {#symbol-musicalanalyzer}

```cpp
template <Sample T> class MusicalAnalyzer;
```

### `MusicalAnalyzer::Callback` {#symbol-musicalanalyzer-callback}

```cpp
using Callback = void (*)(void*, const MusicalFeatures&) noexcept;
```

### `MusicalAnalyzer::MusicalAnalyzer<T>` {#symbol-musicalanalyzer-musicalanalyzer-t}

```cpp
MusicalAnalyzer();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `MusicalAnalyzer::~MusicalAnalyzer<T>` {#symbol-musicalanalyzer-musicalanalyzer-t-2}

```cpp
~MusicalAnalyzer();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `MusicalAnalyzer::MusicalAnalyzer<T>` {#symbol-musicalanalyzer-musicalanalyzer-t-3}

```cpp
MusicalAnalyzer(MusicalAnalyzer&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `MusicalAnalyzer<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `MusicalAnalyzer::operator=` {#symbol-musicalanalyzer-operator}

```cpp
MusicalAnalyzer& operator=(MusicalAnalyzer&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `MusicalAnalyzer<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `MusicalAnalyzer<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MusicalAnalyzer::prepare` {#symbol-musicalanalyzer-prepare}

```cpp
Status prepare(const ProcessSpec&, const MusicalAnalysisConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `const MusicalAnalysisConfig &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `MusicalAnalyzer::setCallback` {#symbol-musicalanalyzer-setcallback}

```cpp
void setCallback(Callback, void* context = nullptr) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Callback` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `context` | `void *` | `nullptr` | Contexte de callback emprunté ; l’application possède durée de vie et synchronisation. |

### `MusicalAnalyzer::process` {#symbol-musicalanalyzer-process}

```cpp
Status process(AudioBlock<const T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `MusicalAnalyzer::features` {#symbol-musicalanalyzer-features}

```cpp
MusicalFeatures features() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `MusicalFeatures` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MusicalAnalyzer::reset` {#symbol-musicalanalyzer-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.
