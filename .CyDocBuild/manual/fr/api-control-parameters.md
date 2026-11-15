---
id: api-control-parameters
title: Parameters
description: ID de paramètres stables, conversions normalisées et événements à l’échantillon rendus en blocs de contrôle.
section: Référence API
order: 1108
sourceRevision: 1
parent: api-control
symbol: cydsp::ParameterUnit
---

ID de paramètres stables, conversions normalisées et événements à l’échantillon rendus en blocs de contrôle.

## Inclusion {#include}

```cpp
#include <cydsp/control/Parameters.h>
```

[Utilisation et contrats](doc:parameters) · [Règles communes de traitement](doc:contracts)

## `ParameterUnit` {#symbol-parameterunit}

```cpp
enum class ParameterUnit { none, hertz, seconds, milliseconds, decibels, linearGain, semitones, metres, degrees };
```

## `ParameterScale` {#symbol-parameterscale}

```cpp
enum class ParameterScale { linear, logarithmic };
```

## `ParameterDescriptor` {#symbol-parameterdescriptor}

```cpp
struct ParameterDescriptor;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `id` | `std::uint32_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `name` | `std::string_view` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `unit` | `ParameterUnit` | `ParameterUnit::none` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `minimum` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `maximum` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `initial` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `scale` | `ParameterScale` | `ParameterScale::linear` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `modulatable` | `bool` | `true` | Option ou prédicat vrai/faux explicite. |

## `ParameterEvent` {#symbol-parameterevent}

```cpp
struct ParameterEvent;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `id` | `std::uint32_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `frameOffset` | `std::size_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `value` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `rampFrames` | `std::size_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

## `parameterToNormalized` {#symbol-parametertonormalized}

```cpp
double parameterToNormalized(const ParameterDescriptor&, double value) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ParameterDescriptor &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `value` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `parameterFromNormalized` {#symbol-parameterfromnormalized}

```cpp
double parameterFromNormalized(const ParameterDescriptor&, double normalized) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ParameterDescriptor &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `normalized` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `DspTiming` {#symbol-dsptiming}

```cpp
struct DspTiming;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `latencyFrames` | `std::size_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `tailFrames` | `std::size_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

## `ParameterBank` {#symbol-parameterbank}

```cpp
class ParameterBank;
```

### `ParameterBank::ParameterBank` {#symbol-parameterbank-parameterbank}

```cpp
ParameterBank();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `ParameterBank::~ParameterBank` {#symbol-parameterbank-parameterbank-2}

```cpp
~ParameterBank();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `ParameterBank::ParameterBank` {#symbol-parameterbank-parameterbank-3}

```cpp
ParameterBank(ParameterBank&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `ParameterBank &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `ParameterBank::operator=` {#symbol-parameterbank-operator}

```cpp
ParameterBank& operator=(ParameterBank&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `ParameterBank &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `ParameterBank &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ParameterBank::prepare` {#symbol-parameterbank-prepare}

```cpp
Status prepare(std::span<const ParameterDescriptor>, std::size_t maximumBlockFrames) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const ParameterDescriptor>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `maximumBlockFrames` | `std::size_t` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ParameterBank::render` {#symbol-parameterbank-render}

```cpp
Status render(std::span<const ParameterEvent>, AudioBlock<double>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const ParameterEvent>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `AudioBlock<double>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ParameterBank::reset` {#symbol-parameterbank-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `ParameterBank::value` {#symbol-parameterbank-value}

```cpp
double value(std::uint32_t id) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `id` | `std::uint32_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ParameterBank::size` {#symbol-parameterbank-size}

```cpp
std::size_t size() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
