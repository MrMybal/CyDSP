---
id: api-processors-biquadcascade
title: BiquadCascade
description: Cascade de sections biquad validées avec historiques indépendants par canal.
section: Référence API
order: 1132
sourceRevision: 1
parent: api-processors
symbol: cydsp::BiquadCascade
---

Cascade de sections biquad validées avec historiques indépendants par canal.

## Inclusion {#include}

```cpp
#include <cydsp/processors/BiquadCascade.h>
```

[Utilisation et contrats](doc:filters) · [Règles communes de traitement](doc:contracts)

## `BiquadCascade` {#symbol-biquadcascade}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class BiquadCascade;
```

### `BiquadCascade::BiquadCascade<T>` {#symbol-biquadcascade-biquadcascade-t}

```cpp
BiquadCascade() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `BiquadCascade::BiquadCascade<T>` {#symbol-biquadcascade-biquadcascade-t-2}

```cpp
BiquadCascade(const BiquadCascade&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const BiquadCascade<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `BiquadCascade::operator=` {#symbol-biquadcascade-operator}

```cpp
BiquadCascade& operator=(const BiquadCascade&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const BiquadCascade<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `BiquadCascade<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `BiquadCascade::BiquadCascade<T>` {#symbol-biquadcascade-biquadcascade-t-3}

```cpp
BiquadCascade(BiquadCascade&& other) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `BiquadCascade<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `BiquadCascade::operator=` {#symbol-biquadcascade-operator-2}

```cpp
BiquadCascade& operator=(BiquadCascade&& other) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `BiquadCascade<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `BiquadCascade<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `BiquadCascade::prepare` {#symbol-biquadcascade-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const BiquadCoefficients>) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `std::span<const BiquadCoefficients>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `BiquadCascade::setSection` {#symbol-biquadcascade-setsection}

```cpp
Status setSection(std::size_t, const BiquadCoefficients&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `const BiquadCoefficients &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `BiquadCascade::reset` {#symbol-biquadcascade-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `BiquadCascade::process` {#symbol-biquadcascade-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `BiquadCascade::sections` {#symbol-biquadcascade-sections}

```cpp
std::size_t sections() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
