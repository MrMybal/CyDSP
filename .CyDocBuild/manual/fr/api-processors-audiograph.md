---
id: api-processors-audiograph
title: AudioGraph
description: Traitement de DAG monobus préparé, scratch conservé et callbacks empruntés.
section: Référence API
order: 1128
sourceRevision: 1
parent: api-processors
symbol: cydsp::AudioGraph
---

Traitement de DAG monobus préparé, scratch conservé et callbacks empruntés.

## Inclusion {#include}

```cpp
#include <cydsp/processors/AudioGraph.h>
```

[Utilisation et contrats](doc:graphs) · [Règles communes de traitement](doc:contracts)

## `graphExternalInput` {#symbol-graphexternalinput}

```cpp
inline constexpr std::size_t graphExternalInput = std::numeric_limits<std::size_t>::max();
```

## `GraphInput` {#symbol-graphinput}

```cpp
struct GraphInput;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `source` | `std::size_t` | `graphExternalInput` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `gain` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |

## `GraphNode` {#symbol-graphnode}

```cpp
template <Sample T> struct GraphNode;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `process` | `Process` | `nullptr` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `context` | `void *` | `nullptr` | Contexte de callback emprunté ; l’application possède durée de vie et synchronisation. |
| `inputs` | `std::span<const GraphInput>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

### `GraphNode::Process` {#symbol-graphnode-process}

```cpp
using Process = Status (*)(void*, AudioBlock<const T>, AudioBlock<T>) noexcept;
```

## `AudioGraph` {#symbol-audiograph}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class AudioGraph;
```

### `AudioGraph::AudioGraph<T>` {#symbol-audiograph-audiograph-t}

```cpp
AudioGraph() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `AudioGraph::AudioGraph<T>` {#symbol-audiograph-audiograph-t-2}

```cpp
AudioGraph(const AudioGraph&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const AudioGraph<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `AudioGraph::operator=` {#symbol-audiograph-operator}

```cpp
AudioGraph& operator=(const AudioGraph&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const AudioGraph<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `AudioGraph<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AudioGraph::AudioGraph<T>` {#symbol-audiograph-audiograph-t-3}

```cpp
AudioGraph(AudioGraph&& other) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `AudioGraph<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `AudioGraph::operator=` {#symbol-audiograph-operator-2}

```cpp
AudioGraph& operator=(AudioGraph&& other) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `AudioGraph<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `AudioGraph<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AudioGraph::prepare` {#symbol-audiograph-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const GraphNode<T>>, std::span<const GraphInput> outputs, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `std::span<const GraphNode<T>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `outputs` | `std::span<const GraphInput>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AudioGraph::reset` {#symbol-audiograph-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `AudioGraph::process` {#symbol-audiograph-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AudioGraph::compiledNodeCount` {#symbol-audiograph-compilednodecount}

```cpp
std::size_t compiledNodeCount() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AudioGraph::workBufferCount` {#symbol-audiograph-workbuffercount}

```cpp
std::size_t workBufferCount() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
