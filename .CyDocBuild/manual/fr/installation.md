---
id: installation
title: Compiler et lier CyDSP
description: Compiler les bibliothèques statiques et utiliser leurs cibles CMake.
section: Guides
order: 1
sourceRevision: 1
parent: overview
---

## Prérequis {#requirements}

Utilisez CMake 3.24 ou ultérieur et un compilateur C++20. Obtenez une copie autorisée du dépôt privé. Python sert uniquement aux scripts utilitaires du dépôt. Cinq adaptations numériques figées et leurs notices sont incluses ; la configuration ne les télécharge pas.

La cible principale est `CyDSP::CyDSP`. `CyDSP::Stretch` ajoute le traitement optionnel de durée/hauteur/formants et se compile par défaut. Définissez `CYDSP_BUILD_STRETCH=OFF` s’il est inutile.

## Compiler Windows x64 {#build}

Depuis le dépôt avec les outils C++ Visual Studio 2022 :

```sh
cmake -S . -B build/release -G "Visual Studio 17 2022" -A x64
cmake --build build/release --config Release
cmake --install build/release --config Release --prefix build/install
```

Les builds de production créent des bibliothèques statiques ; tests natifs et mesures sont activés explicitement. Choisissez un compilateur/toolchain et une archive distincts pour chaque ABI cible.

## Lier une application {#link}

Faites pointer `CMAKE_PREFIX_PATH` vers le préfixe d’installation absolu. Après définition de la cible de l’application :

```cmake
find_package(CyDSP 0.4.4 CONFIG REQUIRED)
target_link_libraries(MyApplication PRIVATE CyDSP::CyDSP)
# target_link_libraries(MyApplication PRIVATE CyDSP::Stretch)
```

Incluez un en-tête individuel `cydsp/...` ou `cydsp/CyDSP.h`. Faites correspondre en-têtes et archive ; recompilez les consommateurs après changement de disposition des objets. La compatibilité du package est limitée à une version mineure. Les constantes/macros de compilation figurent dans `cydsp/core/Version.h`. Consultez la [sélection CPU](doc:simd) et les [limites des cibles](doc:limits).

## API publique {#api}

- [cydsp/core/Version.h](doc:api-core-version)
