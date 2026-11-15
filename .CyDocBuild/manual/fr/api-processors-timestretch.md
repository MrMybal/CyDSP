---
id: api-processors-timestretch
title: TimeStretch
description: Étirement de durée, hauteur et formants optionnel dans la cible CyDSP::Stretch.
section: Référence API
order: 1166
sourceRevision: 1
parent: api-processors
symbol: cydsp::TimeStretch
---

Étirement de durée, hauteur et formants optionnel dans la cible CyDSP::Stretch.

## Inclusion {#include}

```cpp
#include <cydsp/processors/TimeStretch.h>
```

[Utilisation et contrats](doc:pitch) · [Règles communes de traitement](doc:contracts)

## `TimeStretch` {#symbol-timestretch}

```cpp
template <Sample T> class TimeStretch;
```

### `TimeStretch::TimeStretch<T>` {#symbol-timestretch-timestretch-t}

```cpp
TimeStretch();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `TimeStretch::~TimeStretch<T>` {#symbol-timestretch-timestretch-t-2}

```cpp
~TimeStretch();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `TimeStretch::TimeStretch<T>` {#symbol-timestretch-timestretch-t-3}

```cpp
TimeStretch(TimeStretch&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `TimeStretch<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `TimeStretch::operator=` {#symbol-timestretch-operator}

```cpp
TimeStretch& operator=(TimeStretch&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `TimeStretch<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `TimeStretch<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `TimeStretch::prepare` {#symbol-timestretch-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t maximumOutputFrames, bool cheaper = false, bool splitComputation = true, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `maximumOutputFrames` | `std::size_t` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `cheaper` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `splitComputation` | `bool` | `true` | Option ou prédicat vrai/faux explicite. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `TimeStretch::setPitchSemitones` {#symbol-timestretch-setpitchsemitones}

```cpp
Status setPitchSemitones(double semitones, double tonalityLimitHz = 0) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `semitones` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `tonalityLimitHz` | `double` | `0` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `TimeStretch::setFormant` {#symbol-timestretch-setformant}

```cpp
Status setFormant(double multiplier, bool compensatePitch = false, double fundamentalHz = 0) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `multiplier` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `compensatePitch` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `fundamentalHz` | `double` | `0` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `TimeStretch::process` {#symbol-timestretch-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `TimeStretch::prime` {#symbol-timestretch-prime}

```cpp
Status prime(AudioBlock<const T> history) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `history` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `TimeStretch::flush` {#symbol-timestretch-flush}

```cpp
Status flush(AudioBlock<T>) noexcept;
```

Vider la queue documentée et vérifier la fin ; réinitialiser avant un nouveau flux.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `TimeStretch::reset` {#symbol-timestretch-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `TimeStretch::backend` {#symbol-timestretch-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `TimeStretch::inputLatency` {#symbol-timestretch-inputlatency}

```cpp
std::size_t inputLatency() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `TimeStretch::outputLatency` {#symbol-timestretch-outputlatency}

```cpp
std::size_t outputLatency() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
