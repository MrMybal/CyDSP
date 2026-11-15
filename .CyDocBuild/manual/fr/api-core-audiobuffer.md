---
id: api-core-audiobuffer
title: AudioBuffer
description: Stockage planaire propriétaire, initialisé à zéro et aligné sur 64 octets, préparé hors callback.
section: Référence API
order: 1111
sourceRevision: 1
parent: api-core
symbol: cydsp::AudioBuffer
---

Stockage planaire propriétaire, initialisé à zéro et aligné sur 64 octets, préparé hors callback.

## Inclusion {#include}

```cpp
#include <cydsp/core/AudioBuffer.h>
```

[Utilisation et contrats](doc:buffers) · [Règles communes de traitement](doc:contracts)

## `AudioBuffer` {#symbol-audiobuffer}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class AudioBuffer;
```

### `AudioBuffer::AudioBuffer<T>` {#symbol-audiobuffer-audiobuffer-t}

```cpp
AudioBuffer() = default;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

### `AudioBuffer::AudioBuffer<T>` {#symbol-audiobuffer-audiobuffer-t-2}

```cpp
AudioBuffer(const AudioBuffer&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const AudioBuffer<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

### `AudioBuffer::operator=` {#symbol-audiobuffer-operator}

```cpp
AudioBuffer& operator=(const AudioBuffer&) = delete;
```

Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const AudioBuffer<T> &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `AudioBuffer<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AudioBuffer::AudioBuffer<T>` {#symbol-audiobuffer-audiobuffer-t-3}

```cpp
AudioBuffer(AudioBuffer&& other) noexcept;
```

Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `AudioBuffer<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `AudioBuffer::operator=` {#symbol-audiobuffer-operator-2}

```cpp
AudioBuffer& operator=(AudioBuffer&& other) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `AudioBuffer<T> &&` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `AudioBuffer<T> &` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AudioBuffer::resize` {#symbol-audiobuffer-resize}

```cpp
Status resize(std::size_t channels, std::size_t frames) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `channels` | `std::size_t` | — | Indice/compte de canaux dans la disposition planaire déclarée. |
| `frames` | `std::size_t` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

### `AudioBuffer::swap` {#symbol-audiobuffer-swap}

```cpp
void swap(AudioBuffer& other) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `other` | `AudioBuffer<T> &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

### `AudioBuffer::block` {#symbol-audiobuffer-block}

```cpp
AudioBlock<T> block() noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `AudioBlock<T>` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AudioBuffer::block` {#symbol-audiobuffer-block-2}

```cpp
AudioBlock<const T> block() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `AudioBlock<const T>` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

### `AudioBuffer::block` {#symbol-audiobuffer-block-3}

```cpp
AudioBlock<T> block(std::size_t frames) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `frames` | `std::size_t` | — | Compte ou offset de frames ; une frame contient un échantillon par canal. |

Retour : `AudioBlock<T>` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
