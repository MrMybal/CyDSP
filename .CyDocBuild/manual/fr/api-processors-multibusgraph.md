---
id: api-processors-multibusgraph
title: MultiBusGraph
description: Bus explicites, sidechains, alignement de latence et transitions entre DAG préparés.
section: Référence API
order: 1150
sourceRevision: 1
parent: api-processors
symbol: cydsp::MultiBusGraph
---

Bus explicites, sidechains, alignement de latence et transitions entre DAG préparés.

## Inclusion {#include}

```cpp
#include <cydsp/processors/MultiBusGraph.h>
```

[Utilisation et contrats](doc:graphs) · [Règles communes de traitement](doc:contracts)

## `graphInputNode` {#symbol-graphinputnode}

```cpp
inline constexpr std::uint32_t graphInputNode = 0;
```

## `graphOutputNode` {#symbol-graphoutputnode}

```cpp
inline constexpr std::uint32_t graphOutputNode = 0xffffffffu;
```

## `DspBus` {#symbol-dspbus}

```cpp
struct DspBus;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `id` | `std::uint32_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `channels` | `std::size_t` | `2` | Indice/compte de canaux dans la disposition planaire déclarée. |

## `DspConnection` {#symbol-dspconnection}

```cpp
struct DspConnection;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `sourceNode` | `std::uint32_t` | `graphInputNode` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `sourceBus` | `std::uint32_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `destinationNode` | `std::uint32_t` | `graphOutputNode` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `destinationBus` | `std::uint32_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `gain` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |

## `DspNode` {#symbol-dspnode}

```cpp
template <Sample T> struct DspNode;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `id` | `std::uint32_t` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `context` | `void *` | `nullptr` | Contexte de callback emprunté ; l’application possède durée de vie et synchronisation. |
| `process` | `Process` | `nullptr` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `inputs` | `std::span<const DspBus>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `outputs` | `std::span<const DspBus>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `timing` | `DspTiming` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `reset` | `void (*)(void *) noexcept` | `nullptr` | Contexte de callback emprunté ; l’application possède durée de vie et synchronisation. |

### `DspNode::Process` {#symbol-dspnode-process}

```cpp
using Process = Status (*)(void*, std::span<const AudioBlock<const T>> inputs, std::span<AudioBlock<T>> outputs) noexcept;
```

## `MultiBusGraph` {#symbol-multibusgraph}

```cpp
template <Sample T> class MultiBusGraph;
```

### `MultiBusGraph::MultiBusGraph<T>` {#symbol-multibusgraph-multibusgraph-t}

```cpp
MultiBusGraph();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `MultiBusGraph::~MultiBusGraph<T>` {#symbol-multibusgraph-multibusgraph-t-2}

```cpp
~MultiBusGraph();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `MultiBusGraph::MultiBusGraph<T>` {#symbol-multibusgraph-multibusgraph-t-3}

```cpp
MultiBusGraph(MultiBusGraph&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `MultiBusGraph<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `MultiBusGraph::operator=` {#symbol-multibusgraph-operator}

```cpp
MultiBusGraph& operator=(MultiBusGraph&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `MultiBusGraph<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `MultiBusGraph<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MultiBusGraph::prepare` {#symbol-multibusgraph-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const DspBus> inputs, std::span<const DspBus> outputs, std::span<const DspNode<T>>, std::span<const DspConnection>, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `inputs` | `std::span<const DspBus>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `outputs` | `std::span<const DspBus>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 4` | `std::span<const DspNode<T>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 5` | `std::span<const DspConnection>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 6` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `MultiBusGraph::process` {#symbol-multibusgraph-process}

```cpp
Status process(std::span<const AudioBlock<const T>> inputs, std::span<AudioBlock<T>> outputs) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `inputs` | `std::span<const AudioBlock<const T>>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `outputs` | `std::span<AudioBlock<T>>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `MultiBusGraph::setBypassed` {#symbol-multibusgraph-setbypassed}

```cpp
Status setBypassed(std::uint32_t node, bool bypassed, std::size_t fadeFrames = 64) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `node` | `std::uint32_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `bypassed` | `bool` | — | Option ou prédicat vrai/faux explicite. |
| `fadeFrames` | `std::size_t` | `64` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `MultiBusGraph::reset` {#symbol-multibusgraph-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `MultiBusGraph::timing` {#symbol-multibusgraph-timing}

```cpp
DspTiming timing() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `DspTiming` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MultiBusGraph::inputBuses` {#symbol-multibusgraph-inputbuses}

```cpp
std::span<const DspBus> inputBuses() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::span<const DspBus>` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MultiBusGraph::outputBuses` {#symbol-multibusgraph-outputbuses}

```cpp
std::span<const DspBus> outputBuses() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::span<const DspBus>` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MultiBusGraph::specification` {#symbol-multibusgraph-specification}

```cpp
ProcessSpec specification() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `ProcessSpec` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MultiBusGraph::prepared` {#symbol-multibusgraph-prepared}

```cpp
bool prepared() const noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `GraphTransition` {#symbol-graphtransition}

```cpp
template <Sample T> class GraphTransition;
```

### `GraphTransition::GraphTransition<T>` {#symbol-graphtransition-graphtransition-t}

```cpp
GraphTransition();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `GraphTransition::~GraphTransition<T>` {#symbol-graphtransition-graphtransition-t-2}

```cpp
~GraphTransition();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `GraphTransition::GraphTransition<T>` {#symbol-graphtransition-graphtransition-t-3}

```cpp
GraphTransition(GraphTransition&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `GraphTransition<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `GraphTransition::operator=` {#symbol-graphtransition-operator}

```cpp
GraphTransition& operator=(GraphTransition&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `GraphTransition<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `GraphTransition<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `GraphTransition::prepare` {#symbol-graphtransition-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const DspBus> inputs, std::span<const DspBus> outputs, std::size_t latencyBudgetFrames, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `inputs` | `std::span<const DspBus>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `outputs` | `std::span<const DspBus>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `latencyBudgetFrames` | `std::size_t` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `GraphTransition::setInitial` {#symbol-graphtransition-setinitial}

```cpp
Status setInitial(MultiBusGraph<T>&&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `MultiBusGraph<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `GraphTransition::stage` {#symbol-graphtransition-stage}

```cpp
Status stage(MultiBusGraph<T>&&) noexcept;
```

Préparer le stage hors traitement temps réel ; respecter la durée de vie des données/contextes de transition.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `MultiBusGraph<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `GraphTransition::startTransition` {#symbol-graphtransition-starttransition}

```cpp
Status startTransition(std::size_t fadeFrames, std::size_t extraWarmupFrames = 0) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `fadeFrames` | `std::size_t` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `extraWarmupFrames` | `std::size_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `GraphTransition::process` {#symbol-graphtransition-process}

```cpp
Status process(std::span<const AudioBlock<const T>>, std::span<AudioBlock<T>>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const AudioBlock<const T>>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `std::span<AudioBlock<T>>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `GraphTransition::reset` {#symbol-graphtransition-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `GraphTransition::latencyFrames` {#symbol-graphtransition-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `GraphTransition::transitionRemainingFrames` {#symbol-graphtransition-transitionremainingframes}

```cpp
std::size_t transitionRemainingFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `GraphTransition::warmupRemainingFrames` {#symbol-graphtransition-warmupremainingframes}

```cpp
std::size_t warmupRemainingFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
