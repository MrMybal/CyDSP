---
id: api-synthesis-physicalmodels
title: PhysicalModels
description: Cordes et tubes Karplus–Strong/guides d’ondes, banques modales et modes de membranes/plaques.
section: Référence API
order: 1184
sourceRevision: 1
parent: api-synthesis
symbol: cydsp::ResonantMode
---

Cordes et tubes Karplus–Strong/guides d’ondes, banques modales et modes de membranes/plaques.

## Inclusion {#include}

```cpp
#include <cydsp/synthesis/PhysicalModels.h>
```

[Utilisation et contrats](doc:physical) · [Règles communes de traitement](doc:contracts)

## `ResonantMode` {#symbol-resonantmode}

```cpp
struct ResonantMode;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `frequencyHz` | `double` | `440` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `decaySeconds` | `double` | `2` | Durée ou position en secondes. |
| `gain` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `pan` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `ModalSurface` {#symbol-modalsurface}

```cpp
enum class ModalSurface { rectangularMembrane, rectangularPlate };
```

## `ModalResonator` {#symbol-modalresonator}

```cpp
template <Sample T> class ModalResonator;
```

### `ModalResonator::ModalResonator<T>` {#symbol-modalresonator-modalresonator-t}

```cpp
ModalResonator();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `ModalResonator::~ModalResonator<T>` {#symbol-modalresonator-modalresonator-t-2}

```cpp
~ModalResonator();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `ModalResonator::ModalResonator<T>` {#symbol-modalresonator-modalresonator-t-3}

```cpp
ModalResonator(ModalResonator&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `ModalResonator<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `ModalResonator::operator=` {#symbol-modalresonator-operator}

```cpp
ModalResonator& operator=(ModalResonator&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `ModalResonator<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `ModalResonator<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ModalResonator::prepare` {#symbol-modalresonator-prepare}

```cpp
Status prepare(const ProcessSpec& mono, std::size_t outputChannels = 2, std::size_t maximumModes = 128) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `mono` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `outputChannels` | `std::size_t` | `2` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `maximumModes` | `std::size_t` | `128` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ModalResonator::setModes` {#symbol-modalresonator-setmodes}

```cpp
Status setModes(std::span<const ResonantMode>) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const ResonantMode>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ModalResonator::setSurface` {#symbol-modalresonator-setsurface}

```cpp
Status setSurface(ModalSurface, double fundamentalHz, double aspect = 1.3, double decaySeconds = 2, double strikeX = .37, double strikeY = .43) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `ModalSurface` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `fundamentalHz` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `aspect` | `double` | `1.3` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `decaySeconds` | `double` | `2` | Durée ou position en secondes. |
| `strikeX` | `double` | `.37` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `strikeY` | `double` | `.43` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ModalResonator::process` {#symbol-modalresonator-process}

```cpp
Status process(AudioBlock<const T> excitation, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `excitation` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ModalResonator::reset` {#symbol-modalresonator-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

## `WaveguideModel` {#symbol-waveguidemodel}

```cpp
enum class WaveguideModel { karplusStrong, string, tube };
```

## `WaveguideParameters` {#symbol-waveguideparameters}

```cpp
struct WaveguideParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `model` | `WaveguideModel` | `WaveguideModel::karplusStrong` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `frequencyHz` | `double` | `220` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `decaySeconds` | `double` | `2` | Durée ou position en secondes. |
| `brightness` | `double` | `.5` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `dispersion` | `double` | `.2` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `closedTube` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

## `Waveguide` {#symbol-waveguide}

```cpp
template <Sample T> class Waveguide;
```

### `Waveguide::Waveguide<T>` {#symbol-waveguide-waveguide-t}

```cpp
Waveguide();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `Waveguide::~Waveguide<T>` {#symbol-waveguide-waveguide-t-2}

```cpp
~Waveguide();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `Waveguide::Waveguide<T>` {#symbol-waveguide-waveguide-t-3}

```cpp
Waveguide(Waveguide&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Waveguide<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `Waveguide::operator=` {#symbol-waveguide-operator}

```cpp
Waveguide& operator=(Waveguide&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Waveguide<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Waveguide<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Waveguide::prepare` {#symbol-waveguide-prepare}

```cpp
Status prepare(const ProcessSpec&, double minimumFrequencyHz = 20) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `minimumFrequencyHz` | `double` | `20` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Waveguide::setParameters` {#symbol-waveguide-setparameters}

```cpp
Status setParameters(const WaveguideParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const WaveguideParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Waveguide::pluck` {#symbol-waveguide-pluck}

```cpp
void pluck(double gain = 1, std::uint64_t seed = 1) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `gain` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `seed` | `std::uint64_t` | `1` | Graine aléatoire déterministe par instance. |

### `Waveguide::process` {#symbol-waveguide-process}

```cpp
Status process(AudioBlock<const T> excitation, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `excitation` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Waveguide::render` {#symbol-waveguide-render}

```cpp
Status render(AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Waveguide::reset` {#symbol-waveguide-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.
