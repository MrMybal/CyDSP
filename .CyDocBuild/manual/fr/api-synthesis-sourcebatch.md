---
id: api-synthesis-sourcebatch
title: SourceBatch
description: Emplacements de sources fixes, banques de samples/tables partagées, voix sinus SIMD et gains multicanaux.
section: Référence API
order: 1187
sourceRevision: 1
parent: api-synthesis
symbol: cydsp::SourceBatch
---

Emplacements de sources fixes, banques de samples/tables partagées, voix sinus SIMD et gains multicanaux.

## Inclusion {#include}

```cpp
#include <cydsp/synthesis/SourceBatch.h>
```

[Utilisation et contrats](doc:sources) · [Règles communes de traitement](doc:contracts)

## `BatchSourceKind` {#symbol-batchsourcekind}

```cpp
enum class BatchSourceKind { sine, wavetable, grain };
```

## `BatchSource` {#symbol-batchsource}

```cpp
struct BatchSource;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `kind` | `BatchSourceKind` | `BatchSourceKind::sine` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `asset` | `std::size_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `frequencyHz` | `double` | `440` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `gain` | `double` | `1` | Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur. |
| `phase` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `samplePosition` | `double` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `playbackRatio` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `durationFrames` | `std::size_t` | `4800` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `startOffsetFrames` | `std::size_t` | `0` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `channelGains` | `std::array<double, 64>` | `{1}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `SourceBatch` {#symbol-sourcebatch}

```cpp
template <Sample T> class SourceBatch;
```

### `SourceBatch::SourceBatch<T>` {#symbol-sourcebatch-sourcebatch-t}

```cpp
SourceBatch();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `SourceBatch::~SourceBatch<T>` {#symbol-sourcebatch-sourcebatch-t-2}

```cpp
~SourceBatch();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `SourceBatch::SourceBatch<T>` {#symbol-sourcebatch-sourcebatch-t-3}

```cpp
SourceBatch(SourceBatch&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `SourceBatch<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `SourceBatch::operator=` {#symbol-sourcebatch-operator}

```cpp
SourceBatch& operator=(SourceBatch&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `SourceBatch<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `SourceBatch<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SourceBatch::prepare` {#symbol-sourcebatch-prepare}

```cpp
Status prepare(const ProcessSpec& output, std::size_t maximumVoices, std::span<const Wavetable<T>* const> tables = {}, std::span<const SampleData<T>* const> samples = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `output` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `maximumVoices` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `tables` | `std::span<const Wavetable<T> *const>` | `{}` | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `samples` | `std::span<const SampleData<T> *const>` | `{}` | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 5` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SourceBatch::start` {#symbol-sourcebatch-start}

```cpp
Status start(std::size_t slot, const BatchSource&) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `slot` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `const BatchSource &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SourceBatch::stop` {#symbol-sourcebatch-stop}

```cpp
Status stop(std::size_t slot) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `slot` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SourceBatch::setPosition` {#symbol-sourcebatch-setposition}

```cpp
Status setPosition(std::size_t slot, Vec3, int ambisonicOrder = -1) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `slot` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `Vec3` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `ambisonicOrder` | `int` | `-1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SourceBatch::process` {#symbol-sourcebatch-process}

```cpp
Status process(AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `SourceBatch::reset` {#symbol-sourcebatch-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `SourceBatch::activeVoices` {#symbol-sourcebatch-activevoices}

```cpp
std::size_t activeVoices() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `SourceBatch::backend` {#symbol-sourcebatch-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
