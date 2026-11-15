---
id: api-core-version
title: Version
description: Compile-time version constants and macros for matching headers to the library.
section: API reference
order: 1118
revision: 1
parent: api-core
symbol: cydsp::versionMajor
---

Compile-time version constants and macros for matching headers to the library.

## Include {#include}

```cpp
#include <cydsp/core/Version.h>
```

[Usage and contracts](doc:installation) · [Common processing rules](doc:contracts)

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


## Version macros {#macros}

```cpp
#define CYDSP_VERSION_MAJOR 0
#define CYDSP_VERSION_MINOR 3
#define CYDSP_VERSION_PATCH 2
```
