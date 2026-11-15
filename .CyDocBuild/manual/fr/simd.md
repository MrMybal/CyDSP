---
id: simd
title: Architectures CPU et SIMD
description: Choisir les noyaux disponibles à la préparation.
section: Guides
order: 7
sourceRevision: 1
parent: contracts
---

## Dispatch et binaires {#dispatch}

Compilez une archive par ABI cible. La sélection automatique utilise les capacités compilées CPU/OS : AVX-512, AVX2 ou SSE2 sur x64 compatible ; SVE puis NEON sous ARM64 Linux/Android compatible ; sinon la base disponible. Windows/macOS ARM64 utilisent NEON. AVX-512 est réservé à x64 ; SVE exige un matériel ARM64 compatible.

Forcer un backend indisponible retourne `unsupported_backend` en conservant l’ancien plan. VectorOps::create prépare une table immuable ; VectorOps utilise le scalaire par défaut. Consultez `backend()` lorsqu’il est exposé.

## Comportement numérique {#numerics}

Le SIMD traite échantillons, canaux ou voix indépendants lorsque les dépendances le permettent. Filtres récursifs et enveloppes conservent l’ordre du signal. Les réductions de produits scalaires peuvent s’arrondir différemment selon le backend ; l’identité binaire universelle n’est pas supposée.

Les options ISA restent dans les implémentations privées. Le code portable n’utilise ni drapeau global propre au CPU, fast-math, contraction FMA ni mutation de l’état flottant. La sélection automatique suit les capacités, sans mesurer chaque buffer. Profilez les processeurs et découpages réels avant de la remplacer. SVE utilise la longueur vectorielle courante du thread ; son exécution matérielle reste une [limite de cible](doc:limits).

## API publique {#api}

- [cydsp/core/Cpu.h](doc:api-core-cpu)
- [cydsp/math/VectorOps.h](doc:api-math-vectorops)
