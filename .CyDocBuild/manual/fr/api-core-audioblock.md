---
id: api-core-audioblock
title: AudioBlock
description: Vues planaires empruntées, sous-blocs et validation des formes et recouvrements.
section: Référence API
order: 1110
sourceRevision: 1
parent: api-core
symbol: cydsp::AudioBlock
---

Vues planaires empruntées, sous-blocs et validation des formes et recouvrements.

## Inclusion {#include}

```cpp
#include <cydsp/core/AudioBlock.h>
```

[Utilisation et contrats](doc:buffers) · [Règles communes de traitement](doc:contracts)

## `AudioBlock` {#symbol-audioblock}

```cpp
template <Sample T> class AudioBlock;
```

### `AudioBlock::AudioBlock<T>` {#symbol-audioblock-audioblock-t}

```cpp
constexpr AudioBlock() noexcept = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `AudioBlock::AudioBlock<T>` {#symbol-audioblock-audioblock-t-2}

```cpp
constexpr AudioBlock(T* const* data, std::size_t channels, std::size_t frames) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `data` | `T *const *` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `channels` | `std::size_t` | — | Indice/compte de canaux dans la disposition planaire déclarée. |
| `frames` | `std::size_t` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |

### `AudioBlock::AudioBlock<T>` {#symbol-audioblock-audioblock-t-3}

```cpp
template <Sample U> requires(std::is_const_v<T> && std::same_as<std::remove_const_t<T>, U>) constexpr AudioBlock(const AudioBlock<U>& other) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `const AudioBlock<U> &` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

### `AudioBlock::channels` {#symbol-audioblock-channels}

```cpp
constexpr std::size_t channels() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AudioBlock::frames` {#symbol-audioblock-frames}

```cpp
constexpr std::size_t frames() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AudioBlock::frameOffset` {#symbol-audioblock-frameoffset}

```cpp
constexpr std::size_t frameOffset() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `std::size_t` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AudioBlock::subBlock` {#symbol-audioblock-subblock}

```cpp
constexpr AudioBlock subBlock(std::size_t offset, std::size_t frames) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `offset` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `frames` | `std::size_t` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `AudioBlock<T>` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AudioBlock::channelData` {#symbol-audioblock-channeldata}

```cpp
constexpr T* const* channelData() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `T *const *` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AudioBlock::isValid` {#symbol-audioblock-isvalid}

```cpp
constexpr bool isValid() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AudioBlock::channel` {#symbol-audioblock-channel}

```cpp
constexpr std::span<T> channel(std::size_t index) const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `index` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `std::span<T>` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `validateProcessBlocks` {#symbol-validateprocessblocks}

```cpp
template <Sample T> requires(!std::is_const_v<T>) [[nodiscard]] Status validateProcessBlocks(const ProcessSpec&, AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Taux, bloc maximal et nombre de canaux préparés. |
| `argument 2` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 3` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `validateDisjointBlocks` {#symbol-validatedisjointblocks}

```cpp
template <Sample T> [[nodiscard]] Status validateDisjointBlocks(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs. |
| `argument 2` | `AudioBlock<T>` | — | Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture. |

Retour : `Status` — Vérifier Status ; ok indique le succès.
