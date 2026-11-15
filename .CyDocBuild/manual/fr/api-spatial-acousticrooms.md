---
id: api-spatial-acousticrooms
title: AcousticRooms
description: Transfert pièces/portails, interpolation de grilles de réponses et bus de réverbération conservés.
section: Référence API
order: 1169
sourceRevision: 1
parent: api-spatial
symbol: cydsp::AcousticRoom
---

Transfert pièces/portails, interpolation de grilles de réponses et bus de réverbération conservés.

## Inclusion {#include}

```cpp
#include <cydsp/spatial/AcousticRooms.h>
```

[Utilisation et contrats](doc:acoustics) · [Règles communes de traitement](doc:contracts)

## `AcousticRoom` {#symbol-acousticroom}

```cpp
struct AcousticRoom;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `id` | `std::uint32_t` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `volumeCubicMetres` | `double` | `100` | Longueur en mètres. |
| `surfaceSquareMetres` | `double` | `120` | Longueur en mètres. |
| `meanAbsorption` | `std::array<double, 3>` | `{.2, .3, .4}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `RoomDecayModel` {#symbol-roomdecaymodel}

```cpp
enum class RoomDecayModel { sabine, eyring };
```

## `RoomResponse` {#symbol-roomresponse}

```cpp
struct RoomResponse;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `decaySeconds` | `std::array<double, 3>` | `{1, 1, 1}` | Durée ou position en secondes. |
| `preDelaySeconds` | `double` | `0` | Durée ou position en secondes. |
| `diffusion` | `double` | `.7` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `wet` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |

## `estimateRoomResponse` {#symbol-estimateroomresponse}

```cpp
Status estimateRoomResponse(const AcousticRoom&, const AcousticMedium&, RoomResponse&, RoomDecayModel = RoomDecayModel::eyring) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const AcousticRoom &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `const AcousticMedium &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `RoomResponse &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |
| `argument 4` | `RoomDecayModel` | `RoomDecayModel::eyring` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `AcousticPortal` {#symbol-acousticportal}

```cpp
struct AcousticPortal;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `id` | `std::uint32_t` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `roomA` | `std::uint32_t` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `roomB` | `std::uint32_t` | `2` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `lengthMetres` | `double` | `1` | Longueur en mètres. |
| `opening` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `bandGains` | `std::array<double, 3>` | `{1, 1, 1}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `medium` | `AcousticMedium` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `bidirectional` | `bool` | `true` | Option ou prédicat vrai/faux explicite. |

## `AcousticPortalGraph` {#symbol-acousticportalgraph}

```cpp
class AcousticPortalGraph;
```

### `AcousticPortalGraph::AcousticPortalGraph` {#symbol-acousticportalgraph-acousticportalgraph}

```cpp
AcousticPortalGraph();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `AcousticPortalGraph::~AcousticPortalGraph` {#symbol-acousticportalgraph-acousticportalgraph-2}

```cpp
~AcousticPortalGraph();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `AcousticPortalGraph::AcousticPortalGraph` {#symbol-acousticportalgraph-acousticportalgraph-3}

```cpp
AcousticPortalGraph(AcousticPortalGraph&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AcousticPortalGraph &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `AcousticPortalGraph::operator=` {#symbol-acousticportalgraph-operator}

```cpp
AcousticPortalGraph& operator=(AcousticPortalGraph&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AcousticPortalGraph &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `AcousticPortalGraph &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AcousticPortalGraph::prepare` {#symbol-acousticportalgraph-prepare}

```cpp
Status prepare(std::span<const AcousticRoom>, std::span<const AcousticPortal>, double lossWeightSecondsPerDb = 0) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const AcousticRoom>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<const AcousticPortal>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `lossWeightSecondsPerDb` | `double` | `0` | Niveau logarithmique selon la référence dB indiquée. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AcousticPortalGraph::route` {#symbol-acousticportalgraph-route}

```cpp
Status route(std::uint32_t sourceRoom, std::uint32_t destinationRoom, std::span<std::uint32_t> portalIds, std::size_t& count, AcousticTravel&) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `sourceRoom` | `std::uint32_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `destinationRoom` | `std::uint32_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `portalIds` | `std::span<std::uint32_t>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `count` | `std::size_t &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |
| `argument 5` | `AcousticTravel &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `RoomCellGrid` {#symbol-roomcellgrid}

```cpp
class RoomCellGrid;
```

### `RoomCellGrid::RoomCellGrid` {#symbol-roomcellgrid-roomcellgrid}

```cpp
RoomCellGrid();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `RoomCellGrid::~RoomCellGrid` {#symbol-roomcellgrid-roomcellgrid-2}

```cpp
~RoomCellGrid();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `RoomCellGrid::RoomCellGrid` {#symbol-roomcellgrid-roomcellgrid-3}

```cpp
RoomCellGrid(RoomCellGrid&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `RoomCellGrid &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `RoomCellGrid::operator=` {#symbol-roomcellgrid-operator}

```cpp
RoomCellGrid& operator=(RoomCellGrid&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `RoomCellGrid &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `RoomCellGrid &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `RoomCellGrid::prepare` {#symbol-roomcellgrid-prepare}

```cpp
Status prepare(Vec3 origin, Vec3 spacing, std::array<std::size_t, 3> dimensions, std::span<const RoomResponse>) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `origin` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `spacing` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `dimensions` | `std::array<std::size_t, 3>` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 4` | `std::span<const RoomResponse>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `RoomCellGrid::sample` {#symbol-roomcellgrid-sample}

```cpp
Status sample(Vec3 position, RoomResponse&) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `position` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `RoomResponse &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `RoomReverbBank` {#symbol-roomreverbbank}

```cpp
template <Sample T> class RoomReverbBank;
```

### `RoomReverbBank::RoomReverbBank<T>` {#symbol-roomreverbbank-roomreverbbank-t}

```cpp
RoomReverbBank();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `RoomReverbBank::~RoomReverbBank<T>` {#symbol-roomreverbbank-roomreverbbank-t-2}

```cpp
~RoomReverbBank();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `RoomReverbBank::RoomReverbBank<T>` {#symbol-roomreverbbank-roomreverbbank-t-3}

```cpp
RoomReverbBank(RoomReverbBank&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `RoomReverbBank<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `RoomReverbBank::operator=` {#symbol-roomreverbbank-operator}

```cpp
RoomReverbBank& operator=(RoomReverbBank&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `RoomReverbBank<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `RoomReverbBank<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `RoomReverbBank::prepare` {#symbol-roomreverbbank-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t rooms = 8, const FdnConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `rooms` | `std::size_t` | `8` | Durée en millisecondes. |
| `argument 3` | `const FdnConfig &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `RoomReverbBank::setRoom` {#symbol-roomreverbbank-setroom}

```cpp
Status setRoom(std::size_t, const FdnParameters&, std::size_t rampFrames = 256) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `const FdnParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `rampFrames` | `std::size_t` | `256` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `RoomReverbBank::beginBlock` {#symbol-roomreverbbank-beginblock}

```cpp
Status beginBlock(std::size_t frames) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `frames` | `std::size_t` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `RoomReverbBank::send` {#symbol-roomreverbbank-send}

```cpp
Status send(std::size_t room, AudioBlock<const T>, double gain = 1) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `room` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `gain` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `RoomReverbBank::render` {#symbol-roomreverbbank-render}

```cpp
Status render() noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `RoomReverbBank::mix` {#symbol-roomreverbbank-mix}

```cpp
Status mix(AudioBlock<T>) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `RoomReverbBank::roomOutput` {#symbol-roomreverbbank-roomoutput}

```cpp
AudioBlock<const T> roomOutput(std::size_t room) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `room` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `AudioBlock<const T>` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `RoomReverbBank::reset` {#symbol-roomreverbbank-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.
