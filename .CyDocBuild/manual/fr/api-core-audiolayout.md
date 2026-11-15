---
id: api-core-audiolayout
title: AudioLayout
description: Conversion sans allocation entre blocs planaires et spans entrelacés par frame.
section: Référence API
order: 1112
sourceRevision: 1
parent: api-core
symbol: cydsp::interleave
---

Conversion sans allocation entre blocs planaires et spans entrelacés par frame.

## Inclusion {#include}

```cpp
#include <cydsp/core/AudioLayout.h>
```

[Utilisation et contrats](doc:buffers) · [Règles communes de traitement](doc:contracts)

## `interleave` {#symbol-interleave}

```cpp
Status interleave(AudioBlock<const float>, std::span<float>) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const float>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `std::span<float>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `interleave` {#symbol-interleave-2}

```cpp
Status interleave(AudioBlock<const double>, std::span<double>) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const double>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `std::span<double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `deinterleave` {#symbol-deinterleave}

```cpp
Status deinterleave(std::span<const float>, AudioBlock<float>) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const float>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `AudioBlock<float>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `deinterleave` {#symbol-deinterleave-2}

```cpp
Status deinterleave(std::span<const double>, AudioBlock<double>) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `AudioBlock<double>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.
