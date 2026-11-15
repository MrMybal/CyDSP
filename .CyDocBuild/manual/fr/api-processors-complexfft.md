---
id: api-processors-complexfft
title: ComplexFft
description: Transformations complexes avec sens direct/inverse explicite et sélection du backend.
section: Référence API
order: 1134
sourceRevision: 1
parent: api-processors
symbol: cydsp::ComplexFft
---

Transformations complexes avec sens direct/inverse explicite et sélection du backend.

## Inclusion {#include}

```cpp
#include <cydsp/processors/ComplexFft.h>
```

[Utilisation et contrats](doc:fft) · [Règles communes de traitement](doc:contracts)

## `FftDirection` {#symbol-fftdirection}

```cpp
enum class FftDirection { forward, inverse };
```

## `ComplexFft` {#symbol-complexfft}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class ComplexFft;
```

### `ComplexFft::ComplexFft<T>` {#symbol-complexfft-complexfft-t}

```cpp
ComplexFft() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `ComplexFft::ComplexFft<T>` {#symbol-complexfft-complexfft-t-2}

```cpp
ComplexFft(const ComplexFft&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ComplexFft<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `ComplexFft::operator=` {#symbol-complexfft-operator}

```cpp
ComplexFft& operator=(const ComplexFft&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ComplexFft<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `ComplexFft<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ComplexFft::ComplexFft<T>` {#symbol-complexfft-complexfft-t-3}

```cpp
ComplexFft(ComplexFft&& other) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `ComplexFft<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `ComplexFft::operator=` {#symbol-complexfft-operator-2}

```cpp
ComplexFft& operator=(ComplexFft&& other) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `ComplexFft<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `ComplexFft<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ComplexFft::prepare` {#symbol-complexfft-prepare}

```cpp
Status prepare(std::size_t size, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `size` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ComplexFft::reset` {#symbol-complexfft-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `ComplexFft::process` {#symbol-complexfft-process}

```cpp
Status process(std::span<const std::complex<T>>, std::span<std::complex<T>>, FftDirection) const noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const std::complex<T>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `std::span<std::complex<T>>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 3` | `FftDirection` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ComplexFft::size` {#symbol-complexfft-size}

```cpp
std::size_t size() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ComplexFft::backend` {#symbol-complexfft-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
