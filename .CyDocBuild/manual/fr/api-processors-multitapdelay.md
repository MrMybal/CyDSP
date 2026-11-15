---
id: api-processors-multitapdelay
title: MultiTapDelay
description: Taps entrée/sortie bornés, fondu de configurations et délai fractionnaire Thiran séquentiel.
section: Référence API
order: 1152
sourceRevision: 1
parent: api-processors
symbol: cydsp::MultiTapDelay
---

Taps entrée/sortie bornés, fondu de configurations et délai fractionnaire Thiran séquentiel.

## Inclusion {#include}

```cpp
#include <cydsp/processors/MultiTapDelay.h>
```

[Utilisation et contrats](doc:delay) · [Règles communes de traitement](doc:contracts)

## `DelayTap` {#symbol-delaytap}

```cpp
struct DelayTap;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `inputChannel` | `std::size_t` | `0` | Indice/compte de canaux dans la disposition planaire déclarée. |
| `outputChannel` | `std::size_t` | `0` | Indice/compte de canaux dans la disposition planaire déclarée. |
| `delayFrames` | `double` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `gain` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `interpolation` | `HistoryInterpolation` | `HistoryInterpolation::linear` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `MultiTapDelay` {#symbol-multitapdelay}

```cpp
template <Sample T> class MultiTapDelay;
```

### `MultiTapDelay::MultiTapDelay<T>` {#symbol-multitapdelay-multitapdelay-t}

```cpp
MultiTapDelay();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `MultiTapDelay::~MultiTapDelay<T>` {#symbol-multitapdelay-multitapdelay-t-2}

```cpp
~MultiTapDelay();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `MultiTapDelay::MultiTapDelay<T>` {#symbol-multitapdelay-multitapdelay-t-3}

```cpp
MultiTapDelay(MultiTapDelay&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `MultiTapDelay<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `MultiTapDelay::operator=` {#symbol-multitapdelay-operator}

```cpp
MultiTapDelay& operator=(MultiTapDelay&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `MultiTapDelay<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `MultiTapDelay<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MultiTapDelay::prepare` {#symbol-multitapdelay-prepare}

```cpp
Status prepare(const ProcessSpec& inputSpec, std::size_t outputChannels, std::size_t maximumDelayFrames, std::size_t maximumTaps = 32, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `inputSpec` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `outputChannels` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `maximumDelayFrames` | `std::size_t` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `maximumTaps` | `std::size_t` | `32` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `MultiTapDelay::setTaps` {#symbol-multitapdelay-settaps}

```cpp
Status setTaps(std::span<const DelayTap>, std::size_t transitionFrames = 0) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const DelayTap>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `transitionFrames` | `std::size_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `MultiTapDelay::process` {#symbol-multitapdelay-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `MultiTapDelay::reset` {#symbol-multitapdelay-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `MultiTapDelay::transitionRemainingFrames` {#symbol-multitapdelay-transitionremainingframes}

```cpp
std::size_t transitionRemainingFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MultiTapDelay::tailFrames` {#symbol-multitapdelay-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `ThiranDelay` {#symbol-thirandelay}

```cpp
template <Sample T> class ThiranDelay;
```

### `ThiranDelay::setDelay` {#symbol-thirandelay-setdelay}

```cpp
Status setDelay(double fractionalFrames) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `fractionalFrames` | `double` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ThiranDelay::processSample` {#symbol-thirandelay-processsample}

```cpp
T processSample(T input) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `input` | `T` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `T` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ThiranDelay::reset` {#symbol-thirandelay-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.
