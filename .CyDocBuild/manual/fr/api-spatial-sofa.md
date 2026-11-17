---
id: api-spatial-sofa
title: Sofa
description: Import SOFA optionnel depuis la mémoire, préparation float64 et cache HRTF portable.
section: Référence API
order: 1175
sourceRevision: 1
parent: api-spatial
symbol: cydsp::SofaBackend
---

Import SOFA optionnel depuis la mémoire, préparation float64 et cache HRTF portable.

## Inclusion {#include}

```cpp
#include <cydsp/spatial/Sofa.h>
```

[Utilisation et contrats](doc:sofa) · [Règles communes de traitement](doc:contracts)

## `SofaBackend` {#symbol-sofabackend}

```cpp
enum class SofaBackend { automatic, compact, complete };
```

## `SofaError` {#symbol-sofaerror}

```cpp
enum class SofaError { none, invalid_file, unsupported_convention, unsupported_storage, invalid_dimensions, invalid_coordinates, invalid_sampling_rate, invalid_delay, invalid_samples, limit_exceeded, precision_loss, backend_unavailable, allocation_failed };
```

## `SofaLoadReport` {#symbol-sofaloadreport}

```cpp
struct SofaLoadReport;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `backend` | `SofaBackend` | `SofaBackend::automatic` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `error` | `SofaError` | `SofaError::none` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `sourcePrecisionBits` | `unsigned int` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `precisionReduced` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `receiversSwapped` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

## `SofaInflate` {#symbol-sofainflate}

```cpp
using SofaInflate = Status (*)(std::span<const std::byte> input, std::span<std::byte> output, void* context) noexcept;
```

## `SofaLoadOptions` {#symbol-sofaloadoptions}

```cpp
struct SofaLoadOptions;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `backend` | `SofaBackend` | `SofaBackend::automatic` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `maximumFileBytes` | `std::size_t` | `128u * 1024u * 1024u` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `maximumDecodedBytes` | `std::size_t` | `512u * 1024u * 1024u` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `maximumPoints` | `std::size_t` | `100000` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `maximumTaps` | `std::size_t` | `32768` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `maximumMetadataBytes` | `std::size_t` | `65536` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `allowGeneralFir` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `allowPrecisionLoss` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `inflate` | `SofaInflate` | `nullptr` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `inflateContext` | `void *` | `nullptr` | Contexte de callback emprunté ; l’application possède durée de vie et synchronisation. |

## `BasicSofaHrtf` {#symbol-basicsofahrtf}

```cpp
template <Sample T> struct BasicSofaHrtf;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `sampleRate` | `double` | `0` | Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée. |
| `directions` | `std::vector<Vec3>` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `distancesMetres` | `std::vector<double>` | — | Longueur en mètres. |
| `left` | `std::vector<T>` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `right` | `std::vector<T>` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `leftDelay` | `std::vector<double>` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `rightDelay` | `std::vector<double>` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `points` | `std::size_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `taps` | `std::size_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `convention` | `std::string` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `title` | `std::string` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `database` | `std::string` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `listener` | `std::string` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `license` | `std::string` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `SofaHrtf` {#symbol-sofahrtf}

```cpp
using SofaHrtf = BasicSofaHrtf<float>;
```

## `SofaHrtf64` {#symbol-sofahrtf64}

```cpp
using SofaHrtf64 = BasicSofaHrtf<double>;
```

## `loadSofa` {#symbol-loadsofa}

```cpp
Status loadSofa(std::span<const std::byte> file, SofaHrtf& out, const SofaLoadOptions& options = {}, SofaLoadReport* report = nullptr) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `file` | `std::span<const std::byte>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `out` | `SofaHrtf &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |
| `options` | `const SofaLoadOptions &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `report` | `SofaLoadReport *` | `nullptr` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `loadSofa` {#symbol-loadsofa-2}

```cpp
Status loadSofa(std::span<const std::byte> file, SofaHrtf64& out, const SofaLoadOptions& options = {}, SofaLoadReport* report = nullptr) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `file` | `std::span<const std::byte>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `out` | `SofaHrtf64 &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |
| `options` | `const SofaLoadOptions &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `report` | `SofaLoadReport *` | `nullptr` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `sofaBackendAvailable` {#symbol-sofabackendavailable}

```cpp
bool sofaBackendAvailable(SofaBackend backend) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `backend` | `SofaBackend` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `bool` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `HrtfLevelNormalization` {#symbol-hrtflevelnormalization}

```cpp
enum class HrtfLevelNormalization { none, frontalEnergy };
```

## `SofaPreparationReport` {#symbol-sofapreparationreport}

```cpp
struct SofaPreparationReport;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `selectedPoints` | `std::size_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `inputTaps` | `std::size_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `preparedTaps` | `std::size_t` | `0` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `distanceMetres` | `double` | `0` | Longueur en mètres. |
| `appliedGain` | `double` | `1` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `truncated` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `diffuseFieldEqualized` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `uniformDiffuseWeights` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

## `SofaPreparation` {#symbol-sofapreparation}

```cpp
struct SofaPreparation;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `hrtf` | `HrtfPreparation` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `distanceMetres` | `double` | `1` | Longueur en mètres. |
| `distanceToleranceMetres` | `double` | `.001` | Longueur en mètres. |
| `diffuseFieldEqualization` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `diffuseFieldWeights` | `std::span<const double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `equalizationTaps` | `std::size_t` | `129` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `maximumEqualizationDb` | `double` | `12` | Niveau logarithmique selon la référence dB indiquée. |
| `normalization` | `HrtfLevelNormalization` | `HrtfLevelNormalization::none` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `prepareSofaHrtf` {#symbol-preparesofahrtf}

```cpp
Status prepareSofaHrtf(const SofaHrtf& asset, HrtfSet<float>& out, const SofaPreparation& options = {}, SofaPreparationReport* report = nullptr) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `asset` | `const SofaHrtf &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `out` | `HrtfSet<float> &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |
| `options` | `const SofaPreparation &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `report` | `SofaPreparationReport *` | `nullptr` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `prepareSofaHrtf` {#symbol-preparesofahrtf-2}

```cpp
Status prepareSofaHrtf(const SofaHrtf64& asset, HrtfSet<double>& out, const SofaPreparation& options = {}, SofaPreparationReport* report = nullptr) noexcept;
```

Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `asset` | `const SofaHrtf64 &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `out` | `HrtfSet<double> &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |
| `options` | `const SofaPreparation &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `report` | `SofaPreparationReport *` | `nullptr` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `encodeHrtfAsset` {#symbol-encodehrtfasset}

```cpp
Status encodeHrtfAsset(const SofaHrtf& asset, std::vector<std::byte>& out) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `asset` | `const SofaHrtf &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `out` | `std::vector<std::byte> &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `encodeHrtfAsset` {#symbol-encodehrtfasset-2}

```cpp
Status encodeHrtfAsset(const SofaHrtf64& asset, std::vector<std::byte>& out) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `asset` | `const SofaHrtf64 &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `out` | `std::vector<std::byte> &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `decodeHrtfAsset` {#symbol-decodehrtfasset}

```cpp
Status decodeHrtfAsset(std::span<const std::byte> file, SofaHrtf& out, const SofaLoadOptions& options = {}) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `file` | `std::span<const std::byte>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `out` | `SofaHrtf &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |
| `options` | `const SofaLoadOptions &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `decodeHrtfAsset` {#symbol-decodehrtfasset-2}

```cpp
Status decodeHrtfAsset(std::span<const std::byte> file, SofaHrtf64& out, const SofaLoadOptions& options = {}) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `file` | `std::span<const std::byte>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `out` | `SofaHrtf64 &` | — | Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état. |
| `options` | `const SofaLoadOptions &` | `{}` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.
