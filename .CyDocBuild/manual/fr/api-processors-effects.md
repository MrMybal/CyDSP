---
id: api-processors-effects
title: Effects
description: Chorus, phaser et réverbération de type Schroeder utilisant l’API préparée commune.
section: Référence API
order: 1140
sourceRevision: 1
parent: api-processors
symbol: cydsp::ChorusParameters
---

Chorus, phaser et réverbération de type Schroeder utilisant l’API préparée commune.

## Inclusion {#include}

```cpp
#include <cydsp/processors/Effects.h>
```

[Utilisation et contrats](doc:effects) · [Règles communes de traitement](doc:contracts)

## `ChorusParameters` {#symbol-chorusparameters}

```cpp
struct ChorusParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `rateHz` | `double` | `.5` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `depth` | `double` | `.5` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `centreDelayMs` | `double` | `7` | Durée en millisecondes. |
| `feedback` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `wet` | `double` | `.5` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |

## `PhaserParameters` {#symbol-phaserparameters}

```cpp
struct PhaserParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `rateHz` | `double` | `.5` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `depth` | `double` | `.5` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `centreHz` | `double` | `1000` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `feedback` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `wet` | `double` | `.5` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |

## `ReverbParameters` {#symbol-reverbparameters}

```cpp
struct ReverbParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `decaySeconds` | `double` | `1.5` | Durée ou position en secondes. |
| `damping` | `double` | `.3` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `wet` | `double` | `.25` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `width` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `Chorus` {#symbol-chorus}

```cpp
template <Sample T> class Chorus;
```

### `Chorus::Chorus<T>` {#symbol-chorus-chorus-t}

```cpp
Chorus();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `Chorus::~Chorus<T>` {#symbol-chorus-chorus-t-2}

```cpp
~Chorus();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `Chorus::Chorus<T>` {#symbol-chorus-chorus-t-3}

```cpp
Chorus(Chorus&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Chorus<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `Chorus::operator=` {#symbol-chorus-operator}

```cpp
Chorus& operator=(Chorus&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Chorus<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Chorus<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Chorus::prepare` {#symbol-chorus-prepare}

```cpp
Status prepare(const ProcessSpec&, double maximumDelayMs = 100) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `maximumDelayMs` | `double` | `100` | Durée en millisecondes. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Chorus::setParameters` {#symbol-chorus-setparameters}

```cpp
Status setParameters(const ChorusParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ChorusParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Chorus::process` {#symbol-chorus-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Chorus::reset` {#symbol-chorus-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

## `Phaser` {#symbol-phaser}

```cpp
template <Sample T> class Phaser;
```

### `Phaser::Phaser<T>` {#symbol-phaser-phaser-t}

```cpp
Phaser();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `Phaser::~Phaser<T>` {#symbol-phaser-phaser-t-2}

```cpp
~Phaser();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `Phaser::Phaser<T>` {#symbol-phaser-phaser-t-3}

```cpp
Phaser(Phaser&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Phaser<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `Phaser::operator=` {#symbol-phaser-operator}

```cpp
Phaser& operator=(Phaser&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Phaser<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Phaser<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Phaser::prepare` {#symbol-phaser-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Phaser::setParameters` {#symbol-phaser-setparameters}

```cpp
Status setParameters(const PhaserParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const PhaserParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Phaser::process` {#symbol-phaser-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Phaser::reset` {#symbol-phaser-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

## `Reverb` {#symbol-reverb}

```cpp
template <Sample T> class Reverb;
```

### `Reverb::Reverb<T>` {#symbol-reverb-reverb-t}

```cpp
Reverb();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `Reverb::~Reverb<T>` {#symbol-reverb-reverb-t-2}

```cpp
~Reverb();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `Reverb::Reverb<T>` {#symbol-reverb-reverb-t-3}

```cpp
Reverb(Reverb&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Reverb<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `Reverb::operator=` {#symbol-reverb-operator}

```cpp
Reverb& operator=(Reverb&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Reverb<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Reverb<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Reverb::prepare` {#symbol-reverb-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Reverb::setParameters` {#symbol-reverb-setparameters}

```cpp
Status setParameters(const ReverbParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ReverbParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Reverb::process` {#symbol-reverb-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Reverb::reset` {#symbol-reverb-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.
