---
id: api-processors-hilbert
title: Hilbert
description: Signaux en quadrature FIR à retard commun pour le traitement du signal analytique.
section: Référence API
order: 1146
sourceRevision: 1
parent: api-processors
symbol: cydsp::HilbertTransform
---

Signaux en quadrature FIR à retard commun pour le traitement du signal analytique.

## Inclusion {#include}

```cpp
#include <cydsp/processors/Hilbert.h>
```

[Utilisation et contrats](doc:spectral) · [Règles communes de traitement](doc:contracts)

## `HilbertTransform` {#symbol-hilberttransform}

```cpp
template <Sample T> class HilbertTransform;
```

### `HilbertTransform::HilbertTransform<T>` {#symbol-hilberttransform-hilberttransform-t}

```cpp
HilbertTransform();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `HilbertTransform::~HilbertTransform<T>` {#symbol-hilberttransform-hilberttransform-t-2}

```cpp
~HilbertTransform();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `HilbertTransform::HilbertTransform<T>` {#symbol-hilberttransform-hilberttransform-t-3}

```cpp
HilbertTransform(HilbertTransform&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `HilbertTransform<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `HilbertTransform::operator=` {#symbol-hilberttransform-operator}

```cpp
HilbertTransform& operator=(HilbertTransform&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `HilbertTransform<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `HilbertTransform<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `HilbertTransform::prepare` {#symbol-hilberttransform-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t taps = 129, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `taps` | `std::size_t` | `129` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `HilbertTransform::process` {#symbol-hilberttransform-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T> inPhase, AudioBlock<T> quadrature) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `inPhase` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |
| `quadrature` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `HilbertTransform::reset` {#symbol-hilberttransform-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `HilbertTransform::latencyFrames` {#symbol-hilberttransform-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
