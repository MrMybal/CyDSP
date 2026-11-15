---
id: api-processors-delayline
title: DelayLine
description: Délai fractionnaire préparé avec changements de retard contrôlés.
section: Référence API
order: 1136
sourceRevision: 1
parent: api-processors
symbol: cydsp::DelayLine
---

Délai fractionnaire préparé avec changements de retard contrôlés.

## Inclusion {#include}

```cpp
#include <cydsp/processors/DelayLine.h>
```

[Utilisation et contrats](doc:delay) · [Règles communes de traitement](doc:contracts)

## `DelayLine` {#symbol-delayline}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class DelayLine;
```

### `DelayLine::DelayLine<T>` {#symbol-delayline-delayline-t}

```cpp
DelayLine() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `DelayLine::DelayLine<T>` {#symbol-delayline-delayline-t-2}

```cpp
DelayLine(const DelayLine&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const DelayLine<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `DelayLine::operator=` {#symbol-delayline-operator}

```cpp
DelayLine& operator=(const DelayLine&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const DelayLine<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `DelayLine<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `DelayLine::DelayLine<T>` {#symbol-delayline-delayline-t-3}

```cpp
DelayLine(DelayLine&& other) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `DelayLine<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `DelayLine::operator=` {#symbol-delayline-operator-2}

```cpp
DelayLine& operator=(DelayLine&& other) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `DelayLine<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `DelayLine<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `DelayLine::prepare` {#symbol-delayline-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t maximumDelayFrames) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `maximumDelayFrames` | `std::size_t` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DelayLine::setDelay` {#symbol-delayline-setdelay}

```cpp
Status setDelay(double frames, std::size_t rampFrames = 0) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `frames` | `double` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `rampFrames` | `std::size_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DelayLine::reset` {#symbol-delayline-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `DelayLine::process` {#symbol-delayline-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `DelayLine::targetDelayFrames` {#symbol-delayline-targetdelayframes}

```cpp
double targetDelayFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `DelayLine::delayFrames` {#symbol-delayline-delayframes}

```cpp
double delayFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
