---
id: api-spectral-harmonicpercussive
title: HarmonicPercussive
description: Séparation causale par médianes temps/fréquence avec sorties harmonique/percussive complémentaires.
section: Référence API
order: 1178
sourceRevision: 1
parent: api-spectral
symbol: cydsp::HarmonicPercussive
---

Séparation causale par médianes temps/fréquence avec sorties harmonique/percussive complémentaires.

## Inclusion {#include}

```cpp
#include <cydsp/spectral/HarmonicPercussive.h>
```

[Utilisation et contrats](doc:spectral) · [Règles communes de traitement](doc:contracts)

## `HpssConfig` {#symbol-hpssconfig}

```cpp
struct HpssConfig;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `stft` | `StftConfig` | `{1024, 256, WindowType::hann}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `temporalFrames` | `std::size_t` | `17` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `frequencyBins` | `std::size_t` | `17` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `HarmonicPercussive` {#symbol-harmonicpercussive}

```cpp
template <Sample T> class HarmonicPercussive;
```

### `HarmonicPercussive::HarmonicPercussive<T>` {#symbol-harmonicpercussive-harmonicpercussive-t}

```cpp
HarmonicPercussive();
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `HarmonicPercussive::~HarmonicPercussive<T>` {#symbol-harmonicpercussive-harmonicpercussive-t-2}

```cpp
~HarmonicPercussive();
```

Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.

### `HarmonicPercussive::HarmonicPercussive<T>` {#symbol-harmonicpercussive-harmonicpercussive-t-3}

```cpp
HarmonicPercussive(HarmonicPercussive&&) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `HarmonicPercussive<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `HarmonicPercussive::operator=` {#symbol-harmonicpercussive-operator}

```cpp
HarmonicPercussive& operator=(HarmonicPercussive&&) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `HarmonicPercussive<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `HarmonicPercussive<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `HarmonicPercussive::prepare` {#symbol-harmonicpercussive-prepare}

```cpp
Status prepare(const ProcessSpec&, const HpssConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `const HpssConfig &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 3` | `CpuBackend` | `CpuBackend::automatic` | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `HarmonicPercussive::process` {#symbol-harmonicpercussive-process}

```cpp
Status process(AudioBlock<const T>, AudioBlock<T> harmonic, AudioBlock<T> percussive) noexcept;
```

Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `harmonic` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |
| `percussive` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `HarmonicPercussive::reset` {#symbol-harmonicpercussive-reset}

```cpp
void reset() noexcept;
```

Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.

### `HarmonicPercussive::latencyFrames` {#symbol-harmonicpercussive-latencyframes}

```cpp
std::size_t latencyFrames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
