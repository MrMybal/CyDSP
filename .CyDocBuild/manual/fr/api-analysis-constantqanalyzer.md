---
id: api-analysis-constantqanalyzer
title: ConstantQAnalyzer
description: Projections complexes directes de longueurs variables, avec budgets de taille des noyaux explicites.
section: Référence API
order: 1100
sourceRevision: 1
parent: api-analysis
symbol: cydsp::ConstantQAnalyzer
---

Projections complexes directes de longueurs variables, avec budgets de taille des noyaux explicites.

## Inclusion {#include}

```cpp
#include <cydsp/analysis/ConstantQAnalyzer.h>
```

[Utilisation et contrats](doc:analysis) · [Règles communes de traitement](doc:contracts)

## `ConstantQConfig` {#symbol-constantqconfig}

```cpp
struct ConstantQConfig;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `minimumHz` | `double` | `55` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `binsPerOctave` | `unsigned int` | `12` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `bins` | `std::size_t` | `96` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `hopFrames` | `std::size_t` | `512` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `maximumKernelFrames` | `std::size_t` | `131072` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

## `ConstantQAnalyzer` {#symbol-constantqanalyzer}

```cpp
template <Sample T> class ConstantQAnalyzer;
```

### `ConstantQAnalyzer::ConstantQAnalyzer<T>` {#symbol-constantqanalyzer-constantqanalyzer-t}

```cpp
ConstantQAnalyzer();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `ConstantQAnalyzer::~ConstantQAnalyzer<T>` {#symbol-constantqanalyzer-constantqanalyzer-t-2}

```cpp
~ConstantQAnalyzer();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `ConstantQAnalyzer::ConstantQAnalyzer<T>` {#symbol-constantqanalyzer-constantqanalyzer-t-3}

```cpp
ConstantQAnalyzer(ConstantQAnalyzer&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `ConstantQAnalyzer<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `ConstantQAnalyzer::operator=` {#symbol-constantqanalyzer-operator}

```cpp
ConstantQAnalyzer& operator=(ConstantQAnalyzer&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `ConstantQAnalyzer<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `ConstantQAnalyzer<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ConstantQAnalyzer::prepare` {#symbol-constantqanalyzer-prepare}

```cpp
Status prepare(const ProcessSpec&, const ConstantQConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `const ConstantQConfig &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ConstantQAnalyzer::process` {#symbol-constantqanalyzer-process}

```cpp
Status process(AudioBlock<const T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ConstantQAnalyzer::reset` {#symbol-constantqanalyzer-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `ConstantQAnalyzer::hasFrame` {#symbol-constantqanalyzer-hasframe}

```cpp
bool hasFrame() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ConstantQAnalyzer::frameCount` {#symbol-constantqanalyzer-framecount}

```cpp
std::uint64_t frameCount() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::uint64_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ConstantQAnalyzer::centerFrequencies` {#symbol-constantqanalyzer-centerfrequencies}

```cpp
std::span<const double> centerFrequencies() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::span<const double>` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ConstantQAnalyzer::magnitudes` {#symbol-constantqanalyzer-magnitudes}

```cpp
std::span<const double> magnitudes(std::size_t channel) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `channel` | `std::size_t` | — | Indice/compte de canaux dans la disposition planaire déclarée. |

Retour : `std::span<const double>` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ConstantQAnalyzer::windowFrames` {#symbol-constantqanalyzer-windowframes}

```cpp
std::size_t windowFrames(std::size_t bin) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `bin` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ConstantQAnalyzer::backend` {#symbol-constantqanalyzer-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
