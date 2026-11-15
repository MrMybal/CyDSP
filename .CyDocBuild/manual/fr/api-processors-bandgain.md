---
id: api-processors-bandgain
title: BandGain
description: Gains de propagation trois bandes préparés sur les fondations de filtrage existantes.
section: Référence API
order: 1129
sourceRevision: 1
parent: api-processors
symbol: cydsp::BandGain
---

Gains de propagation trois bandes préparés sur les fondations de filtrage existantes.

## Inclusion {#include}

```cpp
#include <cydsp/processors/BandGain.h>
```

[Utilisation et contrats](doc:spatial) · [Règles communes de traitement](doc:contracts)

## `BandGain` {#symbol-bandgain}

```cpp
template <Sample T> class BandGain;
```

### `BandGain::BandGain<T>` {#symbol-bandgain-bandgain-t}

```cpp
BandGain();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `BandGain::~BandGain<T>` {#symbol-bandgain-bandgain-t-2}

```cpp
~BandGain();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `BandGain::BandGain<T>` {#symbol-bandgain-bandgain-t-3}

```cpp
BandGain(BandGain&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `BandGain<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `BandGain::operator=` {#symbol-bandgain-operator}

```cpp
BandGain& operator=(BandGain&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `BandGain<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `BandGain<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `BandGain::prepare` {#symbol-bandgain-prepare}

```cpp
Status prepare(const ProcessSpec&, double lowCrossoverHz = 400, double highCrossoverHz = 4000) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `lowCrossoverHz` | `double` | `400` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `highCrossoverHz` | `double` | `4000` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `BandGain::setGains` {#symbol-bandgain-setgains}

```cpp
Status setGains(const std::array<double, 3>&, std::size_t rampFrames = 0) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const std::array<double, 3> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `rampFrames` | `std::size_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `BandGain::process` {#symbol-bandgain-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `BandGain::reset` {#symbol-bandgain-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.
