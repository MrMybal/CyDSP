---
id: api-processors-polynomialinterpolator
title: PolynomialInterpolator
description: Lecteurs d’échantillons polynomiaux Lagrange/Catmull–Rom.
section: Référence API
order: 1157
sourceRevision: 1
parent: api-processors
symbol: cydsp::PolynomialInterpolator
---

Lecteurs d’échantillons polynomiaux Lagrange/Catmull–Rom.

## Inclusion {#include}

```cpp
#include <cydsp/processors/PolynomialInterpolator.h>
```

[Utilisation et contrats](doc:delay) · [Règles communes de traitement](doc:contracts)

## `PolynomialInterpolation` {#symbol-polynomialinterpolation}

```cpp
enum class PolynomialInterpolation { lagrange, catmullRom };
```

## `PolynomialInterpolator` {#symbol-polynomialinterpolator}

```cpp
template <Sample T> class PolynomialInterpolator;
```

### `PolynomialInterpolator::prepare` {#symbol-polynomialinterpolator-prepare}

```cpp
Status prepare(const ProcessSpec&, double inputRate, PolynomialInterpolation = PolynomialInterpolation::lagrange, unsigned order = 3, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `inputRate` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `PolynomialInterpolation` | `PolynomialInterpolation::lagrange` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `order` | `unsigned int` | `3` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `PolynomialInterpolator::process` {#symbol-polynomialinterpolator-process}

```cpp
Status process(AudioBlock<const T> source, double firstFrame, AudioBlock<T> output) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `source` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `firstFrame` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `output` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.
