---
id: api-synthesis-advancedsynthesis
title: AdvancedSynthesis
description: FM/PM, synchronisation, unisson, enveloppes multisegments, LFO sample-and-hold et sources procédurales.
section: Référence API
order: 1181
sourceRevision: 1
parent: api-synthesis
symbol: cydsp::EnvelopeSegment
---

FM/PM, synchronisation, unisson, enveloppes multisegments, LFO sample-and-hold et sources procédurales.

## Inclusion {#include}

```cpp
#include <cydsp/synthesis/AdvancedSynthesis.h>
```

[Utilisation et contrats](doc:synthesis) · [Règles communes de traitement](doc:contracts)

## `EnvelopeSegment` {#symbol-envelopesegment}

```cpp
struct EnvelopeSegment;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `seconds` | `double` | `.1` | Durée ou position en secondes. |
| `target` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `curve` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `MultiSegmentEnvelope` {#symbol-multisegmentenvelope}

```cpp
class MultiSegmentEnvelope;
```

### `MultiSegmentEnvelope::MultiSegmentEnvelope` {#symbol-multisegmentenvelope-multisegmentenvelope}

```cpp
MultiSegmentEnvelope() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `MultiSegmentEnvelope::MultiSegmentEnvelope` {#symbol-multisegmentenvelope-multisegmentenvelope-2}

```cpp
MultiSegmentEnvelope(const MultiSegmentEnvelope&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const MultiSegmentEnvelope &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `MultiSegmentEnvelope::operator=` {#symbol-multisegmentenvelope-operator}

```cpp
MultiSegmentEnvelope& operator=(const MultiSegmentEnvelope&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const MultiSegmentEnvelope &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `MultiSegmentEnvelope &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MultiSegmentEnvelope::MultiSegmentEnvelope` {#symbol-multisegmentenvelope-multisegmentenvelope-3}

```cpp
MultiSegmentEnvelope(MultiSegmentEnvelope&& other) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `MultiSegmentEnvelope &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `MultiSegmentEnvelope::operator=` {#symbol-multisegmentenvelope-operator-2}

```cpp
MultiSegmentEnvelope& operator=(MultiSegmentEnvelope&& other) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `MultiSegmentEnvelope &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `MultiSegmentEnvelope &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MultiSegmentEnvelope::prepare` {#symbol-multisegmentenvelope-prepare}

```cpp
Status prepare(double sampleRate, std::size_t maximumSegments = 32) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `sampleRate` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `maximumSegments` | `std::size_t` | `32` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `MultiSegmentEnvelope::setSegments` {#symbol-multisegmentenvelope-setsegments}

```cpp
Status setSegments(std::span<const EnvelopeSegment>, std::size_t releaseSegment, std::size_t loopBegin = 0, std::size_t loopEnd = 0) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const EnvelopeSegment>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `releaseSegment` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `loopBegin` | `std::size_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `loopEnd` | `std::size_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `MultiSegmentEnvelope::trigger` {#symbol-multisegmentenvelope-trigger}

```cpp
void trigger() noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

### `MultiSegmentEnvelope::release` {#symbol-multisegmentenvelope-release}

```cpp
void release() noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

### `MultiSegmentEnvelope::reset` {#symbol-multisegmentenvelope-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `MultiSegmentEnvelope::next` {#symbol-multisegmentenvelope-next}

```cpp
double next() noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `MultiSegmentEnvelope::active` {#symbol-multisegmentenvelope-active}

```cpp
bool active() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `LfoShape` {#symbol-lfoshape}

```cpp
enum class LfoShape { sine, triangle, sampleAndHold, smoothRandom };
```

## `ModulationLfo` {#symbol-modulationlfo}

```cpp
class ModulationLfo;
```

### `ModulationLfo::prepare` {#symbol-modulationlfo-prepare}

```cpp
Status prepare(double sampleRate) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `sampleRate` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ModulationLfo::setParameters` {#symbol-modulationlfo-setparameters}

```cpp
Status setParameters(double frequencyHz, LfoShape) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `frequencyHz` | `double` | — | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `argument 2` | `LfoShape` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ModulationLfo::reset` {#symbol-modulationlfo-reset}

```cpp
void reset(std::uint64_t seed = 1) noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `seed` | `std::uint64_t` | `1` | Graine aléatoire déterministe par instance. |

### `ModulationLfo::next` {#symbol-modulationlfo-next}

```cpp
double next() noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

Retour : `double` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `ModulatedOscillatorParameters` {#symbol-modulatedoscillatorparameters}

```cpp
struct ModulatedOscillatorParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `waveform` | `Waveform` | `Waveform::sine` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `frequencyHz` | `double` | `440` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `fmDepth` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `pmDepthCycles` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `syncHz` | `double` | `0` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `morph` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `detuneCents` | `double` | `10` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `spread` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `gain` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `unison` | `std::size_t` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `ModulatedOscillator` {#symbol-modulatedoscillator}

```cpp
template <Sample T> class ModulatedOscillator;
```

### `ModulatedOscillator::ModulatedOscillator<T>` {#symbol-modulatedoscillator-modulatedoscillator-t}

```cpp
ModulatedOscillator();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `ModulatedOscillator::~ModulatedOscillator<T>` {#symbol-modulatedoscillator-modulatedoscillator-t-2}

```cpp
~ModulatedOscillator();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `ModulatedOscillator::ModulatedOscillator<T>` {#symbol-modulatedoscillator-modulatedoscillator-t-3}

```cpp
ModulatedOscillator(ModulatedOscillator&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `ModulatedOscillator<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `ModulatedOscillator::operator=` {#symbol-modulatedoscillator-operator}

```cpp
ModulatedOscillator& operator=(ModulatedOscillator&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `ModulatedOscillator<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `ModulatedOscillator<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `ModulatedOscillator::prepare` {#symbol-modulatedoscillator-prepare}

```cpp
Status prepare(const ProcessSpec&, std::span<const Wavetable<T>* const> tables = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `tables` | `std::span<const Wavetable<T> *const>` | `{}` | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ModulatedOscillator::setParameters` {#symbol-modulatedoscillator-setparameters}

```cpp
Status setParameters(const ModulatedOscillatorParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ModulatedOscillatorParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ModulatedOscillator::process` {#symbol-modulatedoscillator-process}

```cpp
Status process(AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ModulatedOscillator::process` {#symbol-modulatedoscillator-process-2}

```cpp
Status process(AudioBlock<const T> fm, AudioBlock<const T> pm, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `fm` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `pm` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 3` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ModulatedOscillator::reset` {#symbol-modulatedoscillator-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `ModulatedOscillator::latencyFrames` {#symbol-modulatedoscillator-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `ProceduralKind` {#symbol-proceduralkind}

```cpp
enum class ProceduralKind { wind, engine, impact };
```

## `ProceduralParameters` {#symbol-proceduralparameters}

```cpp
struct ProceduralParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `kind` | `ProceduralKind` | `ProceduralKind::wind` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `frequencyHz` | `double` | `80` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `intensity` | `double` | `.5` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `roughness` | `double` | `.5` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `decaySeconds` | `double` | `.2` | Durée ou position en secondes. |

## `ProceduralSource` {#symbol-proceduralsource}

```cpp
template <Sample T> class ProceduralSource;
```

### `ProceduralSource::prepare` {#symbol-proceduralsource-prepare}

```cpp
Status prepare(const ProcessSpec&) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ProceduralSource::setParameters` {#symbol-proceduralsource-setparameters}

```cpp
Status setParameters(const ProceduralParameters&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProceduralParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ProceduralSource::trigger` {#symbol-proceduralsource-trigger}

```cpp
void trigger(double strength = 1) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `strength` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `ProceduralSource::process` {#symbol-proceduralsource-process}

```cpp
Status process(AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `ProceduralSource::reset` {#symbol-proceduralsource-reset}

```cpp
void reset(std::uint64_t seed = 1) noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `seed` | `std::uint64_t` | `1` | Graine aléatoire déterministe par instance. |
