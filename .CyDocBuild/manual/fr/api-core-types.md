---
id: api-core-types
title: Types
description: Types d’échantillons, dimensions de traitement, états et politiques de calcul.
section: Référence API
order: 1117
sourceRevision: 1
parent: api-core
symbol: cydsp::Sample
---

Types d’échantillons, dimensions de traitement, états et politiques de calcul.

## Inclusion {#include}

```cpp
#include <cydsp/core/Types.h>
```

[Utilisation et contrats](doc:contracts) · [Règles communes de traitement](doc:contracts)

## `Sample` {#symbol-sample}

```cpp
template <class T> concept Sample = std::same_as<std::remove_const_t<T>, float> || std::same_as<std::remove_const_t<T>, double>;
```

## `isFinite` {#symbol-isfinite}

```cpp
template <Sample T> constexpr bool isFinite(T value) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `value` | `T` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `MathAccuracy` {#symbol-mathaccuracy}

```cpp
enum class MathAccuracy { precise, fast };
```

## `Status` {#symbol-status}

```cpp
enum class Status { ok, invalid_spec, invalid_argument, not_prepared, channel_mismatch, block_too_large, unsupported_backend, partial_overlap, allocation_failed, transition_in_progress, end_of_stream, update_queue_full, io_error, unsupported_format, device_unavailable, would_block, timed_out, ambiguous, incompatible_version, disconnected };
```

## `ProcessSpec` {#symbol-processspec}

```cpp
struct ProcessSpec;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `sampleRate` | `double` | `48000.0` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `maxBlockFrames` | `std::size_t` | `512` | Compte ou offset de frames ; une frame contient un échantillon par canal. |
| `channels` | `std::size_t` | `2` | Indice/compte de canaux dans la disposition planaire déclarée. |

### `ProcessSpec::isValid` {#symbol-processspec-isvalid}

```cpp
constexpr bool isValid() const noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `isPowerOfTwo` {#symbol-ispoweroftwo}

```cpp
constexpr bool isPowerOfTwo(std::size_t value) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `value` | `std::size_t` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
