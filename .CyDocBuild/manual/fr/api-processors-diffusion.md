---
id: api-processors-diffusion
title: Diffusion
description: Diffusion par passe-tout de Schroeder et mixage orthogonal Hadamard/Householder.
section: Référence API
order: 1137
sourceRevision: 1
parent: api-processors
symbol: cydsp::OrthogonalMix
---

Diffusion par passe-tout de Schroeder et mixage orthogonal Hadamard/Householder.

## Inclusion {#include}

```cpp
#include <cydsp/processors/Diffusion.h>
```

[Utilisation et contrats](doc:reverb) · [Règles communes de traitement](doc:contracts)

## `OrthogonalMix` {#symbol-orthogonalmix}

```cpp
enum class OrthogonalMix { hadamard, householder };
```

## `orthogonalMix` {#symbol-orthogonalmix-2}

```cpp
template <Sample T> [[nodiscard]] Status orthogonalMix(std::span<const T> input, std::span<T> output, OrthogonalMix kind = OrthogonalMix::hadamard) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `input` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `output` | `std::span<T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `kind` | `OrthogonalMix` | `OrthogonalMix::hadamard` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `DiffusionStage` {#symbol-diffusionstage}

```cpp
struct DiffusionStage;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `delayFrames` | `std::size_t` | `1` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `feedback` | `double` | `.5` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `Diffuser` {#symbol-diffuser}

```cpp
template <Sample T> class Diffuser;
```

### `Diffuser::Diffuser<T>` {#symbol-diffuser-diffuser-t}

```cpp
Diffuser();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `Diffuser::~Diffuser<T>` {#symbol-diffuser-diffuser-t-2}

```cpp
~Diffuser();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `Diffuser::Diffuser<T>` {#symbol-diffuser-diffuser-t-3}

```cpp
Diffuser(Diffuser&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Diffuser<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `Diffuser::operator=` {#symbol-diffuser-operator}

```cpp
Diffuser& operator=(Diffuser&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Diffuser<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Diffuser<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Diffuser::prepare` {#symbol-diffuser-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const DiffusionStage>) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `std::span<const DiffusionStage>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Diffuser::process` {#symbol-diffuser-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Diffuser::reset` {#symbol-diffuser-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.
