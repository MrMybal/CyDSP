---
id: api-spatial-hybridreverb
title: HybridReverb
description: Réponse précoce par convolution préparée combinée à une queue FDN contrôlée indépendamment.
section: Référence API
order: 1173
sourceRevision: 1
parent: api-spatial
symbol: cydsp::HybridReverb
---

Réponse précoce par convolution préparée combinée à une queue FDN contrôlée indépendamment.

## Inclusion {#include}

```cpp
#include <cydsp/spatial/HybridReverb.h>
```

[Utilisation et contrats](doc:reverb) · [Règles communes de traitement](doc:contracts)

## `HybridReverb` {#symbol-hybridreverb}

```cpp
template <Sample T> class HybridReverb;
```

### `HybridReverb::HybridReverb<T>` {#symbol-hybridreverb-hybridreverb-t}

```cpp
HybridReverb();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `HybridReverb::~HybridReverb<T>` {#symbol-hybridreverb-hybridreverb-t-2}

```cpp
~HybridReverb();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `HybridReverb::HybridReverb<T>` {#symbol-hybridreverb-hybridreverb-t-3}

```cpp
HybridReverb(HybridReverb&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `HybridReverb<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `HybridReverb::operator=` {#symbol-hybridreverb-operator}

```cpp
HybridReverb& operator=(HybridReverb&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `HybridReverb<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `HybridReverb<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `HybridReverb::prepare` {#symbol-hybridreverb-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const T> impulse, const FdnConfig& = {}, const ConvolutionConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `impulse` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 3` | `const FdnConfig &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 4` | `const ConvolutionConfig &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `HybridReverb::setMix` {#symbol-hybridreverb-setmix}

```cpp
Status setMix(double impulseGain, double lateGain, std::size_t rampFrames = 256) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `impulseGain` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `lateGain` | `double` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `rampFrames` | `std::size_t` | `256` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `HybridReverb::setLateParameters` {#symbol-hybridreverb-setlateparameters}

```cpp
Status setLateParameters(const FdnParameters&, std::size_t rampFrames = 256) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const FdnParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `rampFrames` | `std::size_t` | `256` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `HybridReverb::stageImpulse` {#symbol-hybridreverb-stageimpulse}

```cpp
Status stageImpulse(std::span<const T>) noexcept;
```

Préparer le stage hors traitement temps réel ; respecter la durée de vie des données/contextes de transition.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const T>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `HybridReverb::startImpulseTransition` {#symbol-hybridreverb-startimpulsetransition}

```cpp
Status startImpulseTransition(std::size_t frames = 256) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `frames` | `std::size_t` | `256` | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `HybridReverb::process` {#symbol-hybridreverb-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `HybridReverb::reset` {#symbol-hybridreverb-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `HybridReverb::nominalTailFrames` {#symbol-hybridreverb-nominaltailframes}

```cpp
std::size_t nominalTailFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
