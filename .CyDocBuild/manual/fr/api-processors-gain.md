---
id: api-processors-gain
title: Gain
description: Gain linéaire avec rampe avançant une fois par frame pour tous les canaux.
section: Référence API
order: 1144
sourceRevision: 1
parent: api-processors
symbol: cydsp::Gain
---

Gain linéaire avec rampe avançant une fois par frame pour tous les canaux.

## Inclusion {#include}

```cpp
#include <cydsp/processors/Gain.h>
```

[Utilisation et contrats](doc:mixing) · [Règles communes de traitement](doc:contracts)

## `Gain` {#symbol-gain}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class Gain;
```

### `Gain::prepare` {#symbol-gain-prepare}

```cpp
Status prepare(const ProcessSpec&, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Gain::reset` {#symbol-gain-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `Gain::setGain` {#symbol-gain-setgain}

```cpp
Status setGain(T linearGain, std::size_t rampFrames = 0) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `linearGain` | `T` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `rampFrames` | `std::size_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Gain::process` {#symbol-gain-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Gain::backend` {#symbol-gain-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Gain::currentGain` {#symbol-gain-currentgain}

```cpp
T currentGain() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `T` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
