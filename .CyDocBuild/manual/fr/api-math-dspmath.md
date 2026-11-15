---
id: api-math-dspmath
title: DspMath
description: Conversion des décibels, évaluation polynomiale, tables et valeurs de contrôle lissées.
section: Référence API
order: 1120
sourceRevision: 1
parent: api-math
symbol: cydsp::decibelsToGain
---

Conversion des décibels, évaluation polynomiale, tables et valeurs de contrôle lissées.

## Inclusion {#include}

```cpp
#include <cydsp/math/DspMath.h>
```

[Utilisation et contrats](doc:parameters) · [Règles communes de traitement](doc:contracts)

## `decibelsToGain` {#symbol-decibelstogain}

```cpp
template <Sample T> [[nodiscard]] T decibelsToGain(T db, T floorDb = T{-160}) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `db` | `T` | — | Niveau logarithmique selon la référence dB indiquée. |
| `floorDb` | `T` | `T{-160}` | Niveau logarithmique selon la référence dB indiquée. |

Retour : `T` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `gainToDecibels` {#symbol-gaintodecibels}

```cpp
template <Sample T> [[nodiscard]] T gainToDecibels(T gain, T floorDb = T{-160}) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `gain` | `T` | — | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `floorDb` | `T` | `T{-160}` | Niveau logarithmique selon la référence dB indiquée. |

Retour : `T` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `evaluatePolynomial` {#symbol-evaluatepolynomial}

```cpp
template <Sample T> [[nodiscard]] T evaluatePolynomial(std::span<const T> coefficients, T x) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `coefficients` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `x` | `T` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `T` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `SmoothingCurve` {#symbol-smoothingcurve}

```cpp
enum class SmoothingCurve { linear, multiplicative };
```

## `SmoothedValue` {#symbol-smoothedvalue}

```cpp
class SmoothedValue;
```

### `SmoothedValue::reset` {#symbol-smoothedvalue-reset}

```cpp
Status reset(double initial, SmoothingCurve curve = SmoothingCurve::linear) noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `initial` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `curve` | `SmoothingCurve` | `SmoothingCurve::linear` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SmoothedValue::setTarget` {#symbol-smoothedvalue-settarget}

```cpp
Status setTarget(double target, std::size_t frames) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `target` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `frames` | `std::size_t` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SmoothedValue::next` {#symbol-smoothedvalue-next}

```cpp
double next() noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedValue::current` {#symbol-smoothedvalue-current}

```cpp
double current() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedValue::target` {#symbol-smoothedvalue-target}

```cpp
double target() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SmoothedValue::smoothing` {#symbol-smoothedvalue-smoothing}

```cpp
bool smoothing() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `LookupTable` {#symbol-lookuptable}

```cpp
template <Sample T> class LookupTable;
```

### `LookupTable::Function` {#symbol-lookuptable-function}

```cpp
using Function = T (*)(T, void*) noexcept;
```

### `LookupTable::prepare` {#symbol-lookuptable-prepare}

```cpp
Status prepare(T minimum, T maximum, std::size_t points, Function function, void* context = nullptr) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `minimum` | `T` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `maximum` | `T` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `points` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `function` | `Function` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `context` | `void *` | `nullptr` | Contexte de callback emprunté ; l’application possède durée de vie et synchronisation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `LookupTable::lookup` {#symbol-lookuptable-lookup}

```cpp
T lookup(T x) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `x` | `T` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `T` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
