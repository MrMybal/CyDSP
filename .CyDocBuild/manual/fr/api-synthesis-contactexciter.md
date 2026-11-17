---
id: api-synthesis-contactexciter
title: ContactExciter
description: Excitation de contact en force/vitesse/impulsion pour impacts, glissement, roulement et archet.
section: Référence API
order: 1183
sourceRevision: 1
parent: api-synthesis
symbol: cydsp::ContactExciter
---

Excitation de contact en force/vitesse/impulsion pour impacts, glissement, roulement et archet.

## Inclusion {#include}

```cpp
#include <cydsp/synthesis/ContactExciter.h>
```

[Utilisation et contrats](doc:physical) · [Règles communes de traitement](doc:contracts)

## `ContactKind` {#symbol-contactkind}

```cpp
enum class ContactKind { impact, sliding, rolling, bowed };
```

## `ContactParameters` {#symbol-contactparameters}

```cpp
struct ContactParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `kind` | `ContactKind` | `ContactKind::sliding` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `normalForceNewtons` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `velocityMetresPerSecond` | `double` | `.1` | Vitesse en mètres par seconde. |
| `friction` | `double` | `.5` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `roughness` | `double` | `.2` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `roughnessSpacingMetres` | `double` | `.001` | Longueur en mètres. |
| `massKg` | `double` | `.01` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `stiffnessNewtonsPerMetre` | `double` | `10000` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `gain` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |

## `ContactExciter` {#symbol-contactexciter}

```cpp
template <Sample T> class ContactExciter;
```

### `ContactExciter::ContactExciter<T>` {#symbol-contactexciter-contactexciter-t}

```cpp
ContactExciter();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `ContactExciter::~ContactExciter<T>` {#symbol-contactexciter-contactexciter-t-2}

```cpp
~ContactExciter();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `ContactExciter::ContactExciter<T>` {#symbol-contactexciter-contactexciter-t-3}

```cpp
ContactExciter(ContactExciter&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `ContactExciter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `ContactExciter::operator=` {#symbol-contactexciter-operator}

```cpp
ContactExciter& operator=(ContactExciter&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `ContactExciter<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `ContactExciter<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ContactExciter::prepare` {#symbol-contactexciter-prepare}

```cpp
Status prepare(const ProcessSpec& mono) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `mono` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ContactExciter::setParameters` {#symbol-contactexciter-setparameters}

```cpp
Status setParameters(const ContactParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ContactParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ContactExciter::strike` {#symbol-contactexciter-strike}

```cpp
Status strike(double impulseNewtonSeconds) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `impulseNewtonSeconds` | `double` | — | Durée ou position en secondes. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ContactExciter::process` {#symbol-contactexciter-process}

```cpp
Status process(AudioBlock<const T> surfaceVelocity, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `surfaceVelocity` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ContactExciter::render` {#symbol-contactexciter-render}

```cpp
Status render(AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ContactExciter::reset` {#symbol-contactexciter-reset}

```cpp
void reset(std::uint64_t seed = 0x4359445350ULL) noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `seed` | `std::uint64_t` | `0x4359445350ULL` | Graine aléatoire déterministe par instance. |
