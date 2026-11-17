---
id: api-core-version
title: Version
description: Constantes et macros de version pour faire correspondre les en-têtes et la bibliothèque.
section: Référence API
order: 1118
sourceRevision: 1
parent: api-core
symbol: cydsp::versionMajor
---

Constantes et macros de version pour faire correspondre les en-têtes et la bibliothèque.

## Inclusion {#include}

```cpp
#include <cydsp/core/Version.h>
```

[Utilisation et contrats](doc:installation) · [Règles communes de traitement](doc:contracts)

## `versionMajor` {#symbol-versionmajor}

```cpp
inline constexpr int versionMajor = CYDSP_VERSION_MAJOR;
```

## `versionMinor` {#symbol-versionminor}

```cpp
inline constexpr int versionMinor = CYDSP_VERSION_MINOR;
```

## `versionPatch` {#symbol-versionpatch}

```cpp
inline constexpr int versionPatch = CYDSP_VERSION_PATCH;
```


## Macros de version {#macros}

```cpp
#define CYDSP_VERSION_MAJOR 0
#define CYDSP_VERSION_MINOR 4
#define CYDSP_VERSION_PATCH 5
```
