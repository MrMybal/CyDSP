---
id: api-processors-filterbank
title: FilterBank
description: Bandes FIR complémentaires dont la somme neutre reconstruit l’entrée retardée.
section: Référence API
order: 1141
sourceRevision: 1
parent: api-processors
symbol: cydsp::FirFilterBank
---

Bandes FIR complémentaires dont la somme neutre reconstruit l’entrée retardée.

## Inclusion {#include}

```cpp
#include <cydsp/processors/FilterBank.h>
```

[Utilisation et contrats](doc:filter-design) · [Règles communes de traitement](doc:contracts)

## `FirFilterBank` {#symbol-firfilterbank}

```cpp
template <Sample T> class FirFilterBank;
```

### `FirFilterBank::FirFilterBank<T>` {#symbol-firfilterbank-firfilterbank-t}

```cpp
FirFilterBank();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `FirFilterBank::~FirFilterBank<T>` {#symbol-firfilterbank-firfilterbank-t-2}

```cpp
~FirFilterBank();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `FirFilterBank::FirFilterBank<T>` {#symbol-firfilterbank-firfilterbank-t-3}

```cpp
FirFilterBank(FirFilterBank&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `FirFilterBank<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `FirFilterBank::operator=` {#symbol-firfilterbank-operator}

```cpp
FirFilterBank& operator=(FirFilterBank&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `FirFilterBank<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `FirFilterBank<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `FirFilterBank::prepare` {#symbol-firfilterbank-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const double> crossoversHz, std::size_t taps = 129, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `crossoversHz` | `std::span<const double>` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `taps` | `std::size_t` | `129` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `FirFilterBank::process` {#symbol-firfilterbank-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T> bands) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `bands` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `FirFilterBank::sumBands` {#symbol-firfilterbank-sumbands}

```cpp
Status sumBands(AudioBlock<const T> bands, AudioBlock<T> output) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `bands` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `output` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `FirFilterBank::reset` {#symbol-firfilterbank-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `FirFilterBank::bands` {#symbol-firfilterbank-bands}

```cpp
std::size_t bands() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `FirFilterBank::latencyFrames` {#symbol-firfilterbank-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `FirFilterBank::backend` {#symbol-firfilterbank-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
