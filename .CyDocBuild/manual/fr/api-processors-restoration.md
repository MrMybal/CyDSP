---
id: api-processors-restoration
title: Restoration
description: Atténuation du bruit spectral, suppression du ronflement et détection/réparation bornée des clics.
section: Référence API
order: 1160
sourceRevision: 1
parent: api-processors
symbol: cydsp::DenoiseParameters
---

Atténuation du bruit spectral, suppression du ronflement et détection/réparation bornée des clics.

## Inclusion {#include}

```cpp
#include <cydsp/processors/Restoration.h>
```

[Utilisation et contrats](doc:restoration) · [Règles communes de traitement](doc:contracts)

## `DenoiseParameters` {#symbol-denoiseparameters}

```cpp
struct DenoiseParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `reductionDb` | `double` | `18` | Niveau logarithmique selon la référence dB indiquée. |
| `strength` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `smoothingMs` | `double` | `30` | Durée en millisecondes. |
| `resonanceThresholdDb` | `double` | `18` | Niveau logarithmique selon la référence dB indiquée. |
| `resonanceReductionDb` | `double` | `0` | Niveau logarithmique selon la référence dB indiquée. |
| `persistenceMs` | `double` | `500` | Durée en millisecondes. |
| `learnNoise` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

## `SpectralDenoiser` {#symbol-spectraldenoiser}

```cpp
template <Sample T> class SpectralDenoiser;
```

### `SpectralDenoiser::SpectralDenoiser<T>` {#symbol-spectraldenoiser-spectraldenoiser-t}

```cpp
SpectralDenoiser();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `SpectralDenoiser::~SpectralDenoiser<T>` {#symbol-spectraldenoiser-spectraldenoiser-t-2}

```cpp
~SpectralDenoiser();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `SpectralDenoiser::SpectralDenoiser<T>` {#symbol-spectraldenoiser-spectraldenoiser-t-3}

```cpp
SpectralDenoiser(SpectralDenoiser&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `SpectralDenoiser<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `SpectralDenoiser::operator=` {#symbol-spectraldenoiser-operator}

```cpp
SpectralDenoiser& operator=(SpectralDenoiser&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `SpectralDenoiser<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `SpectralDenoiser<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SpectralDenoiser::prepare` {#symbol-spectraldenoiser-prepare}

```cpp
Status prepare(const ProcessSpec&, const StftConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `const StftConfig &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SpectralDenoiser::setParameters` {#symbol-spectraldenoiser-setparameters}

```cpp
Status setParameters(const DenoiseParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const DenoiseParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SpectralDenoiser::setNoiseProfile` {#symbol-spectraldenoiser-setnoiseprofile}

```cpp
Status setNoiseProfile(std::span<const double> power) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `power` | `std::span<const double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SpectralDenoiser::noiseProfile` {#symbol-spectraldenoiser-noiseprofile}

```cpp
std::span<const double> noiseProfile() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::span<const double>` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SpectralDenoiser::process` {#symbol-spectraldenoiser-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SpectralDenoiser::reset` {#symbol-spectraldenoiser-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `SpectralDenoiser::latencyFrames` {#symbol-spectraldenoiser-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `DehumParameters` {#symbol-dehumparameters}

```cpp
struct DehumParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `fundamentalHz` | `double` | `50` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `q` | `double` | `30` | Facteur de qualité du filtre positif et fini. |
| `reductionDb` | `double` | `60` | Niveau logarithmique selon la référence dB indiquée. |
| `harmonics` | `std::size_t` | `8` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `Dehum` {#symbol-dehum}

```cpp
template <Sample T> class Dehum;
```

### `Dehum::Dehum<T>` {#symbol-dehum-dehum-t}

```cpp
Dehum();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `Dehum::~Dehum<T>` {#symbol-dehum-dehum-t-2}

```cpp
~Dehum();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `Dehum::Dehum<T>` {#symbol-dehum-dehum-t-3}

```cpp
Dehum(Dehum&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Dehum<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `Dehum::operator=` {#symbol-dehum-operator}

```cpp
Dehum& operator=(Dehum&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Dehum<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Dehum<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Dehum::prepare` {#symbol-dehum-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t maximumHarmonics = 32) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `maximumHarmonics` | `std::size_t` | `32` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Dehum::setParameters` {#symbol-dehum-setparameters}

```cpp
Status setParameters(const DehumParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const DehumParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Dehum::process` {#symbol-dehum-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Dehum::reset` {#symbol-dehum-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

## `DeclickParameters` {#symbol-declickparameters}

```cpp
struct DeclickParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `absoluteThreshold` | `double` | `.02` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `deviationRatio` | `double` | `6` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `Declicker` {#symbol-declicker}

```cpp
template <Sample T> class Declicker;
```

### `Declicker::Declicker<T>` {#symbol-declicker-declicker-t}

```cpp
Declicker();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `Declicker::~Declicker<T>` {#symbol-declicker-declicker-t-2}

```cpp
~Declicker();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `Declicker::Declicker<T>` {#symbol-declicker-declicker-t-3}

```cpp
Declicker(Declicker&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Declicker<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `Declicker::operator=` {#symbol-declicker-operator}

```cpp
Declicker& operator=(Declicker&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Declicker<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Declicker<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Declicker::prepare` {#symbol-declicker-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t contextFrames = 8) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `contextFrames` | `std::size_t` | `8` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Declicker::setParameters` {#symbol-declicker-setparameters}

```cpp
Status setParameters(const DeclickParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const DeclickParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Declicker::process` {#symbol-declicker-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Declicker::reset` {#symbol-declicker-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `Declicker::latencyFrames` {#symbol-declicker-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Declicker::repairedSamples` {#symbol-declicker-repairedsamples}

```cpp
std::uint64_t repairedSamples() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::uint64_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
