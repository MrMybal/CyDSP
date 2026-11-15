---
id: api-processors-fir
title: Fir
description: Filtrage FIR causal ; le tap zéro multiplie l’échantillon courant.
section: Référence API
order: 1143
sourceRevision: 1
parent: api-processors
symbol: cydsp::Fir
---

Filtrage FIR causal ; le tap zéro multiplie l’échantillon courant.

## Inclusion {#include}

```cpp
#include <cydsp/processors/Fir.h>
```

[Utilisation et contrats](doc:filters) · [Règles communes de traitement](doc:contracts)

## `Fir` {#symbol-fir}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class Fir;
```

### `Fir::Fir<T>` {#symbol-fir-fir-t}

```cpp
Fir() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `Fir::Fir<T>` {#symbol-fir-fir-t-2}

```cpp
Fir(const Fir&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const Fir<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `Fir::operator=` {#symbol-fir-operator}

```cpp
Fir& operator=(const Fir&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const Fir<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Fir<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Fir::Fir<T>` {#symbol-fir-fir-t-3}

```cpp
Fir(Fir&& other) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `Fir<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `Fir::operator=` {#symbol-fir-operator-2}

```cpp
Fir& operator=(Fir&& other) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `Fir<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Fir<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Fir::prepare` {#symbol-fir-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const T> coefficients, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `coefficients` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Fir::reset` {#symbol-fir-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `Fir::process` {#symbol-fir-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Fir::taps` {#symbol-fir-taps}

```cpp
std::size_t taps() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Fir::backend` {#symbol-fir-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
