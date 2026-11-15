---
id: api-spectral-stft
title: Stft
description: Analyse/synthèse causale par recouvrement-addition pondéré et callback spectral sans allocation.
section: Référence API
order: 1179
sourceRevision: 1
parent: api-spectral
symbol: cydsp::Stft
---

Analyse/synthèse causale par recouvrement-addition pondéré et callback spectral sans allocation.

## Inclusion {#include}

```cpp
#include <cydsp/spectral/Stft.h>
```

[Utilisation et contrats](doc:spectral) · [Règles communes de traitement](doc:contracts)

## `StftConfig` {#symbol-stftconfig}

```cpp
struct StftConfig;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `fftFrames` | `std::size_t` | `1024` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `hopFrames` | `std::size_t` | `256` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `window` | `WindowType` | `WindowType::hann` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `Stft` {#symbol-stft}

```cpp
template <Sample T> class Stft;
```

### `Stft::Processor` {#symbol-stft-processor}

```cpp
using Processor = void (*)(void*, std::size_t channel, std::span<std::complex<T>> bins) noexcept;
```

### `Stft::Stft<T>` {#symbol-stft-stft-t}

```cpp
Stft();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `Stft::~Stft<T>` {#symbol-stft-stft-t-2}

```cpp
~Stft();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `Stft::Stft<T>` {#symbol-stft-stft-t-3}

```cpp
Stft(Stft&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Stft<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `Stft::operator=` {#symbol-stft-operator}

```cpp
Stft& operator=(Stft&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Stft<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Stft<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Stft::prepare` {#symbol-stft-prepare}

```cpp
Status prepare(const ProcessSpec&, const StftConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `const StftConfig &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Stft::setProcessor` {#symbol-stft-setprocessor}

```cpp
void setProcessor(Processor, void* context = nullptr) noexcept;
```

Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `Processor` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `context` | `void *` | `nullptr` | Contexte de callback emprunté ; l’application possède durée de vie et synchronisation. |

### `Stft::process` {#symbol-stft-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Stft::flush` {#symbol-stft-flush}

```cpp
Status flush(AudioBlock<T>) noexcept;
```

Vider la queue documentée et vérifier la fin ; réinitialiser avant un nouveau flux.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `Stft::reset` {#symbol-stft-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `Stft::latencyFrames` {#symbol-stft-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Stft::tailFrames` {#symbol-stft-tailframes}

```cpp
std::size_t tailFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Stft::bins` {#symbol-stft-bins}

```cpp
std::size_t bins() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Stft::frameCount` {#symbol-stft-framecount}

```cpp
std::uint64_t frameCount() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::uint64_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `Stft::backend` {#symbol-stft-backend}

```cpp
CpuBackend backend() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuBackend` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
