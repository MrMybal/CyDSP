---
id: api-spatial-diffraction
title: Diffraction
description: Gains de diffraction de bord/ouverture sur écran mince et filtre de bandes préparé.
section: Référence API
order: 1170
sourceRevision: 1
parent: api-spatial
symbol: cydsp::DiffractionPath
---

Gains de diffraction de bord/ouverture sur écran mince et filtre de bandes préparé.

## Inclusion {#include}

```cpp
#include <cydsp/spatial/Diffraction.h>
```

[Utilisation et contrats](doc:acoustics) · [Règles communes de traitement](doc:contracts)

## `DiffractionPath` {#symbol-diffractionpath}

```cpp
struct DiffractionPath;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `sourceToEdgeMetres` | `double` | `1` | Longueur en mètres. |
| `edgeToListenerMetres` | `double` | `1` | Longueur en mètres. |
| `edgeOffsetMetres` | `double` | `0` | Longueur en mètres. |
| `soundSpeed` | `double` | `343` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `openingWidthMetres` | `double` | `0` | Longueur en mètres. |
| `openingAngleRadians` | `double` | `0` | Angle en radians. |

## `diffractionGain` {#symbol-diffractiongain}

```cpp
Status diffractionGain(const DiffractionPath&, double frequencyHz, double& linearGain) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const DiffractionPath &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `frequencyHz` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `linearGain` | `double &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `DiffractionFilter` {#symbol-diffractionfilter}

```cpp
template <Sample T> class DiffractionFilter;
```

### `DiffractionFilter::DiffractionFilter<T>` {#symbol-diffractionfilter-diffractionfilter-t}

```cpp
DiffractionFilter();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `DiffractionFilter::~DiffractionFilter<T>` {#symbol-diffractionfilter-diffractionfilter-t-2}

```cpp
~DiffractionFilter();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `DiffractionFilter::DiffractionFilter<T>` {#symbol-diffractionfilter-diffractionfilter-t-3}

```cpp
DiffractionFilter(DiffractionFilter&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `DiffractionFilter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `DiffractionFilter::operator=` {#symbol-diffractionfilter-operator}

```cpp
DiffractionFilter& operator=(DiffractionFilter&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `DiffractionFilter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `DiffractionFilter<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `DiffractionFilter::prepare` {#symbol-diffractionfilter-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DiffractionFilter::setPath` {#symbol-diffractionfilter-setpath}

```cpp
Status setPath(const DiffractionPath&, std::size_t rampFrames = 256) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const DiffractionPath &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `rampFrames` | `std::size_t` | `256` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DiffractionFilter::process` {#symbol-diffractionfilter-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DiffractionFilter::reset` {#symbol-diffractionfilter-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.
