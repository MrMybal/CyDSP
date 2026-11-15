---
id: api-math-window
title: Window
description: Génération de fenêtres float/double avec symétrie et normalisation explicites.
section: Référence API
order: 1122
sourceRevision: 1
parent: api-math
symbol: cydsp::WindowType
---

Génération de fenêtres float/double avec symétrie et normalisation explicites.

## Inclusion {#include}

```cpp
#include <cydsp/math/Window.h>
```

[Utilisation et contrats](doc:fft) · [Règles communes de traitement](doc:contracts)

## `WindowType` {#symbol-windowtype}

```cpp
enum class WindowType { rectangular, hann, hamming, blackman, blackmanHarris, nuttall, flatTop, bartlett, triangular, cosine, kaiser };
```

## `WindowSymmetry` {#symbol-windowsymmetry}

```cpp
enum class WindowSymmetry { symmetric, periodic };
```

## `WindowNormalization` {#symbol-windownormalization}

```cpp
enum class WindowNormalization { none, peak, coherentGain, energy };
```

## `WindowParameters` {#symbol-windowparameters}

```cpp
struct WindowParameters;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `type` | `WindowType` | `WindowType::hann` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `symmetry` | `WindowSymmetry` | `WindowSymmetry::symmetric` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `normalization` | `WindowNormalization` | `WindowNormalization::none` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `beta` | `double` | `8.6` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

## `makeWindow` {#symbol-makewindow}

```cpp
Status makeWindow(const WindowParameters&, std::span<float>) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const WindowParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `std::span<float>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `makeWindow` {#symbol-makewindow-2}

```cpp
Status makeWindow(const WindowParameters&, std::span<double>) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `const WindowParameters &` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `std::span<double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `makeWindow` {#symbol-makewindow-3}

```cpp
Status makeWindow(WindowType, std::span<float>, WindowSymmetry = WindowSymmetry::symmetric) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `WindowType` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `std::span<float>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 3` | `WindowSymmetry` | `WindowSymmetry::symmetric` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.

## `makeWindow` {#symbol-makewindow-4}

```cpp
Status makeWindow(WindowType, std::span<double>, WindowSymmetry = WindowSymmetry::symmetric) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `WindowType` | — | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `argument 2` | `std::span<double>` | — | Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie. |
| `argument 3` | `WindowSymmetry` | `WindowSymmetry::symmetric` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |

Retour : `Status` — Vérifier Status ; ok indique le succès.
