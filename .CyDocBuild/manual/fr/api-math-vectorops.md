---
id: api-math-vectorops
title: VectorOps
description: Copie, gain, mixage, produits scalaires et calcul complexe par noyaux scalaires/SIMD préparés.
section: Référence API
order: 1121
sourceRevision: 1
parent: api-math
symbol: cydsp::VectorOps
---

Copie, gain, mixage, produits scalaires et calcul complexe par noyaux scalaires/SIMD préparés.

## Inclusion {#include}

```cpp
#include <cydsp/math/VectorOps.h>
```

[Utilisation et contrats](doc:simd) · [Règles communes de traitement](doc:contracts)

## `VectorOps` {#symbol-vectorops}

```cpp
class VectorOps;
```

### `VectorOps::VectorOps` {#symbol-vectorops-vectorops}

```cpp
VectorOps() noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `VectorOps::dot` {#symbol-vectorops-dot}

```cpp
Status dot(std::span<const float>, std::span<const float>, float& result) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const float>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<const float>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `result` | `float &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `VectorOps::dot` {#symbol-vectorops-dot-2}

```cpp
Status dot(std::span<const double>, std::span<const double>, double& result) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<const double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `result` | `double &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `VectorOps::dot` {#symbol-vectorops-dot-3}

```cpp
Status dot(std::span<const float>, std::span<const double>, double& result) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const float>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<const double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `result` | `double &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `VectorOps::multiply` {#symbol-vectorops-multiply}

```cpp
Status multiply(std::span<const float>, std::span<const float>, std::span<float>) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const float>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<const float>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 3` | `std::span<float>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `VectorOps::multiply` {#symbol-vectorops-multiply-2}

```cpp
Status multiply(std::span<const double>, std::span<const double>, std::span<double>) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<const double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 3` | `std::span<double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `VectorOps::complexMultiplyAccumulate` {#symbol-vectorops-complexmultiplyaccumulate}

```cpp
Status complexMultiplyAccumulate(std::span<const std::complex<float>>, std::span<const std::complex<float>>, std::span<std::complex<float>>) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const std::complex<float>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<const std::complex<float>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 3` | `std::span<std::complex<float>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `VectorOps::complexMultiplyAccumulate` {#symbol-vectorops-complexmultiplyaccumulate-2}

```cpp
Status complexMultiplyAccumulate(std::span<const std::complex<double>>, std::span<const std::complex<double>>, std::span<std::complex<double>>) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const std::complex<double>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<const std::complex<double>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 3` | `std::span<std::complex<double>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `VectorOps::complexButterfly` {#symbol-vectorops-complexbutterfly}

```cpp
Status complexButterfly(std::span<std::complex<float>>, std::span<std::complex<float>>, std::span<const std::complex<float>>, bool conjugateTwiddles = false) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<std::complex<float>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<std::complex<float>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 3` | `std::span<const std::complex<float>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `conjugateTwiddles` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `VectorOps::complexButterfly` {#symbol-vectorops-complexbutterfly-2}

```cpp
Status complexButterfly(std::span<std::complex<double>>, std::span<std::complex<double>>, std::span<const std::complex<double>>, bool conjugateTwiddles = false) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<std::complex<double>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<std::complex<double>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 3` | `std::span<const std::complex<double>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `conjugateTwiddles` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `VectorOps::create` {#symbol-vectorops-create}

```cpp
static Status create(CpuBackend requested, VectorOps& result) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `requested` | `CpuBackend` | — | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |
| `result` | `VectorOps &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `VectorOps::backend` {#symbol-vectorops-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `VectorOps::copy` {#symbol-vectorops-copy}

```cpp
Status copy(std::span<const float>, std::span<float>) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const float>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<float>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `VectorOps::copy` {#symbol-vectorops-copy-2}

```cpp
Status copy(std::span<const double>, std::span<double>) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `VectorOps::scale` {#symbol-vectorops-scale}

```cpp
Status scale(std::span<const float>, std::span<float>, float) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const float>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<float>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 3` | `float` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `VectorOps::scale` {#symbol-vectorops-scale-2}

```cpp
Status scale(std::span<const double>, std::span<double>, double) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 3` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `VectorOps::addScaled` {#symbol-vectorops-addscaled}

```cpp
Status addScaled(std::span<const float>, std::span<float>, float) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const float>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<float>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 3` | `float` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `VectorOps::addScaled` {#symbol-vectorops-addscaled-2}

```cpp
Status addScaled(std::span<const double>, std::span<double>, double) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 3` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `VectorOps::fill` {#symbol-vectorops-fill}

```cpp
void fill(std::span<float>, float) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<float>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `float` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `VectorOps::fill` {#symbol-vectorops-fill-2}

```cpp
void fill(std::span<double>, double) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
