---
id: api-processors-realfft
title: RealFft
description: Transformations réelles exposant les bins de DC à Nyquist, avec inverse normalisée.
section: Référence API
order: 1158
sourceRevision: 1
parent: api-processors
symbol: cydsp::RealFft
---

Transformations réelles exposant les bins de DC à Nyquist, avec inverse normalisée.

## Inclusion {#include}

```cpp
#include <cydsp/processors/RealFft.h>
```

[Utilisation et contrats](doc:fft) · [Règles communes de traitement](doc:contracts)

## `RealFft` {#symbol-realfft}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class RealFft;
```

### `RealFft::RealFft<T>` {#symbol-realfft-realfft-t}

```cpp
RealFft() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `RealFft::RealFft<T>` {#symbol-realfft-realfft-t-2}

```cpp
RealFft(const RealFft&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const RealFft<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `RealFft::operator=` {#symbol-realfft-operator}

```cpp
RealFft& operator=(const RealFft&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const RealFft<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `RealFft<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `RealFft::RealFft<T>` {#symbol-realfft-realfft-t-3}

```cpp
RealFft(RealFft&& other) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `RealFft<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `RealFft::operator=` {#symbol-realfft-operator-2}

```cpp
RealFft& operator=(RealFft&& other) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `RealFft<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `RealFft<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `RealFft::prepare` {#symbol-realfft-prepare}

```cpp
Status prepare(std::size_t size, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `size` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `RealFft::reset` {#symbol-realfft-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `RealFft::forward` {#symbol-realfft-forward}

```cpp
Status forward(std::span<const T>, std::span<std::complex<T>>) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<std::complex<T>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `RealFft::inverse` {#symbol-realfft-inverse}

```cpp
Status inverse(std::span<const std::complex<T>>, std::span<T>) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const std::complex<T>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `RealFft::size` {#symbol-realfft-size}

```cpp
std::size_t size() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `RealFft::bins` {#symbol-realfft-bins}

```cpp
std::size_t bins() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `RealFft::backend` {#symbol-realfft-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
