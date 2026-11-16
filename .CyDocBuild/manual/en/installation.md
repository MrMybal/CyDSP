---
id: installation
title: Build and link CyDSP
description: Build the static libraries and use their CMake targets.
section: Guides
order: 1
revision: 1
parent: overview
---

## Requirements {#requirements}

Use CMake 3.24 or later and a C++20 compiler. Obtain an authorized checkout of the private repository. Python is needed only for repository helper scripts. Five pinned numerical adaptations and their notices are included; configuration does not download them.

The main target is `CyDSP::CyDSP`. `CyDSP::Stretch` adds optional time/pitch/formant processing and is built by default. Set `CYDSP_BUILD_STRETCH=OFF` when unnecessary.

## Build Windows x64 {#build}

From the checkout with Visual Studio 2022 C++ tools:

```sh
cmake -S . -B build/release -G "Visual Studio 17 2022" -A x64
cmake --build build/release --config Release
cmake --install build/release --config Release --prefix build/install
```

Production builds create static libraries; native tests and measurements are opt-in. Select a separate compiler/toolchain and archive for each target ABI.

## Link an application {#link}

Point `CMAKE_PREFIX_PATH` at the absolute install prefix. After defining the application's target:

```cmake
find_package(CyDSP 0.4.0 CONFIG REQUIRED)
target_link_libraries(MyApplication PRIVATE CyDSP::CyDSP)
# target_link_libraries(MyApplication PRIVATE CyDSP::Stretch)
```

Include an individual `cydsp/...` header or `cydsp/CyDSP.h`. Match headers and archive; rebuild consumers after object-layout changes. Package compatibility is limited to one minor version. Compile-time constants/macros are in `cydsp/core/Version.h`. See [CPU selection](doc:simd) and [target limits](doc:limits).

## Public API {#api}

- [cydsp/core/Version.h](doc:api-core-version)
