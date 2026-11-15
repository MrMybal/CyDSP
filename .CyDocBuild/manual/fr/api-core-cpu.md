---
id: api-core-cpu
title: Cpu
description: Détection des capacités CPU/OS, backends compilés et noms des backends.
section: Référence API
order: 1114
sourceRevision: 1
parent: api-core
symbol: cydsp::Architecture
---

Détection des capacités CPU/OS, backends compilés et noms des backends.

## Inclusion {#include}

```cpp
#include <cydsp/core/Cpu.h>
```

[Utilisation et contrats](doc:simd) · [Règles communes de traitement](doc:contracts)

## `Architecture` {#symbol-architecture}

```cpp
enum class Architecture { unknown, x86, x64, arm32, arm64 };
```

## `CpuBackend` {#symbol-cpubackend}

```cpp
enum class CpuBackend { automatic, scalar, sse2, avx2, neon, avx512, sve };
```

## `CpuFeatures` {#symbol-cpufeatures}

```cpp
struct CpuFeatures;
```

| Champ | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `architecture` | `Architecture` | `Architecture::unknown` | Valeur/configuration du type déclaré ; appliquer le contrat du module lié. |
| `sse2` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `avx2` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `neon` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `avx512` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |
| `sve` | `bool` | `false` | Option ou prédicat vrai/faux explicite. |

## `detectCpuFeatures` {#symbol-detectcpufeatures}

```cpp
CpuFeatures detectCpuFeatures() noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

Retour : `CpuFeatures` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.

## `backendName` {#symbol-backendname}

```cpp
const char* backendName(CpuBackend) noexcept;
```

Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.

| Paramètre | Type | Défaut | Sens |
| --- | --- | --- | --- |
| `argument 1` | `CpuBackend` | — | Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation. |

Retour : `const char *` — Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.
