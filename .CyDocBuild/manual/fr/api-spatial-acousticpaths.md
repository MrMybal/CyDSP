---
id: api-spatial-acousticpaths
title: AcousticPaths
description: Emplacements stables de chemins acoustiques avec historique partagé, retards et gains trois bandes.
section: Référence API
order: 1168
sourceRevision: 1
parent: api-spatial
symbol: cydsp::AcousticPath
---

Emplacements stables de chemins acoustiques avec historique partagé, retards et gains trois bandes.

## Inclusion {#include}

```cpp
#include <cydsp/spatial/AcousticPaths.h>
```

[Utilisation et contrats](doc:acoustics) · [Règles communes de traitement](doc:contracts)

## `AcousticPath` {#symbol-acousticpath}

```cpp
struct AcousticPath;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `id` | `std::uint64_t` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `arrivalDirection` | `Vec3` | `{1, 0, 0}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `delaySeconds` | `double` | `0` | Durée ou position en secondes. |
| `bandGains` | `std::array<double, 3>` | `{1, 1, 1}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `AcousticPathRenderer` {#symbol-acousticpathrenderer}

```cpp
template <Sample T> class AcousticPathRenderer;
```

### `AcousticPathRenderer::AcousticPathRenderer<T>` {#symbol-acousticpathrenderer-acousticpathrenderer-t}

```cpp
AcousticPathRenderer();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `AcousticPathRenderer::~AcousticPathRenderer<T>` {#symbol-acousticpathrenderer-acousticpathrenderer-t-2}

```cpp
~AcousticPathRenderer();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `AcousticPathRenderer::AcousticPathRenderer<T>` {#symbol-acousticpathrenderer-acousticpathrenderer-t-3}

```cpp
AcousticPathRenderer(AcousticPathRenderer&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AcousticPathRenderer<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `AcousticPathRenderer::operator=` {#symbol-acousticpathrenderer-operator}

```cpp
AcousticPathRenderer& operator=(AcousticPathRenderer&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AcousticPathRenderer<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `AcousticPathRenderer<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AcousticPathRenderer::prepare` {#symbol-acousticpathrenderer-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t maximumPaths = 16, double maximumDelaySeconds = 2) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `maximumPaths` | `std::size_t` | `16` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `maximumDelaySeconds` | `double` | `2` | Durée ou position en secondes. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AcousticPathRenderer::setPaths` {#symbol-acousticpathrenderer-setpaths}

```cpp
Status setPaths(std::span<const AcousticPath>, std::size_t transitionFrames = 128) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const AcousticPath>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `transitionFrames` | `std::size_t` | `128` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AcousticPathRenderer::process` {#symbol-acousticpathrenderer-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T> arrivals) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `arrivals` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AcousticPathRenderer::reset` {#symbol-acousticpathrenderer-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `AcousticPathRenderer::maximumPaths` {#symbol-acousticpathrenderer-maximumpaths}

```cpp
std::size_t maximumPaths() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AcousticPathRenderer::activePaths` {#symbol-acousticpathrenderer-activepaths}

```cpp
std::size_t activePaths() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AcousticPathRenderer::path` {#symbol-acousticpathrenderer-path}

```cpp
AcousticPath path(std::size_t slot) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `slot` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `AcousticPath` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
