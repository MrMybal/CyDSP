---
id: api-synthesis-wavetable
title: Wavetable
description: Tables préparées par bandes harmoniques et banques d’oscillateurs antirepliement sélectionnées par bloc.
section: Référence API
order: 1189
sourceRevision: 1
parent: api-synthesis
symbol: cydsp::Wavetable
---

Tables préparées par bandes harmoniques et banques d’oscillateurs antirepliement sélectionnées par bloc.

## Inclusion {#include}

```cpp
#include <cydsp/synthesis/Wavetable.h>
```

[Utilisation et contrats](doc:synthesis) · [Règles communes de traitement](doc:contracts)

## `Wavetable` {#symbol-wavetable}

```cpp
template <Sample T> class Wavetable;
```

### `Wavetable::Wavetable<T>` {#symbol-wavetable-wavetable-t}

```cpp
Wavetable();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `Wavetable::~Wavetable<T>` {#symbol-wavetable-wavetable-t-2}

```cpp
~Wavetable();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `Wavetable::Wavetable<T>` {#symbol-wavetable-wavetable-t-3}

```cpp
Wavetable(Wavetable&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Wavetable<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `Wavetable::operator=` {#symbol-wavetable-operator}

```cpp
Wavetable& operator=(Wavetable&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Wavetable<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Wavetable<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Wavetable::prepare` {#symbol-wavetable-prepare}

```cpp
Status prepare(std::span<const T> cycle, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `cycle` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 2` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Wavetable::value` {#symbol-wavetable-value}

```cpp
T value(double phase, double frequency, double sampleRate) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `phase` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `frequency` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `sampleRate` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |

Retour : `T` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Wavetable::size` {#symbol-wavetable-size}

```cpp
std::size_t size() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Wavetable::levels` {#symbol-wavetable-levels}

```cpp
std::size_t levels() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `WavetableOscillator` {#symbol-wavetableoscillator}

```cpp
template <Sample T> class WavetableOscillator;
```

### `WavetableOscillator::prepare` {#symbol-wavetableoscillator-prepare}

```cpp
Status prepare(const ProcessSpec&, const Wavetable<T>&) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `const Wavetable<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `WavetableOscillator::setFrequency` {#symbol-wavetableoscillator-setfrequency}

```cpp
Status setFrequency(double) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `WavetableOscillator::reset` {#symbol-wavetableoscillator-reset}

```cpp
void reset(double phase = 0) noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `phase` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `WavetableOscillator::next` {#symbol-wavetableoscillator-next}

```cpp
T next() noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

Retour : `T` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `WavetableOscillator::process` {#symbol-wavetableoscillator-process}

```cpp
Status process(AudioBlock<T>, bool add = false) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |
| `add` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

Retour : `Status` — Vérifier Status ; ok indique le succès.
