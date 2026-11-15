---
id: api-processors-biquadbank
title: BiquadBank
description: Coefficients et historiques indépendants par voix, avec SIMD entre voix indépendantes.
section: Référence API
order: 1131
sourceRevision: 1
parent: api-processors
symbol: cydsp::BiquadBank
---

Coefficients et historiques indépendants par voix, avec SIMD entre voix indépendantes.

## Inclusion {#include}

```cpp
#include <cydsp/processors/BiquadBank.h>
```

[Utilisation et contrats](doc:filters) · [Règles communes de traitement](doc:contracts)

## `BiquadBank` {#symbol-biquadbank}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class BiquadBank;
```

### `BiquadBank::BiquadBank<T>` {#symbol-biquadbank-biquadbank-t}

```cpp
BiquadBank() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `BiquadBank::BiquadBank<T>` {#symbol-biquadbank-biquadbank-t-2}

```cpp
BiquadBank(const BiquadBank&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const BiquadBank<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `BiquadBank::operator=` {#symbol-biquadbank-operator}

```cpp
BiquadBank& operator=(const BiquadBank&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const BiquadBank<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `BiquadBank<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `BiquadBank::BiquadBank<T>` {#symbol-biquadbank-biquadbank-t-3}

```cpp
BiquadBank(BiquadBank&& other) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `BiquadBank<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `BiquadBank::operator=` {#symbol-biquadbank-operator-2}

```cpp
BiquadBank& operator=(BiquadBank&& other) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `BiquadBank<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `BiquadBank<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `BiquadBank::prepare` {#symbol-biquadbank-prepare}

```cpp
Status prepare(const ProcessSpec&, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `BiquadBank::setCoefficients` {#symbol-biquadbank-setcoefficients}

```cpp
Status setCoefficients(std::size_t voice, const BiquadCoefficients&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `voice` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `const BiquadCoefficients &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `BiquadBank::setAllCoefficients` {#symbol-biquadbank-setallcoefficients}

```cpp
Status setAllCoefficients(const BiquadCoefficients&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const BiquadCoefficients &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `BiquadBank::reset` {#symbol-biquadbank-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `BiquadBank::process` {#symbol-biquadbank-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `BiquadBank::voices` {#symbol-biquadbank-voices}

```cpp
std::size_t voices() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `BiquadBank::backend` {#symbol-biquadbank-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
