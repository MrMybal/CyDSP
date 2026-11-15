---
id: physical
title: Synthèse physique et contacts
description: Exciter banques modales, cordes et guides avec unités explicites.
section: Guides
order: 26
sourceRevision: 1
parent: synthesis
---

## Résonateurs et guides {#models}

ModalResonator accepte des modes fréquence/décroissance/gain et propose des dispositions de membrane/plaque. Waveguide fournit des modèles de corde/tube et de type Karplus–Strong sur des retards/filtres préparés. Choisissez taux et plage de modes/fréquences représentables ; ce sont des modèles réduits, pas des simulations mécaniques générales.

L’hôte fournit excitation, couplage de rétroaction et conversion des paramètres physiques. Un composant stable ne prouve pas la stabilité de l’instrument couplé. Bornez l’énergie d’excitation et évaluez résonances, longues queues et changements de fréquence d’échantillonnage.

## Excitation de contact {#contact}

ContactExciter produit une excitation mono d’impact/glissement/roulement/archet pour un résonateur. Les unités sont explicites : force normale N, vitesse m/s, masse kg, raideur N/m, impulsion N·s et espacement en mètres. Au plus huit impacts se superposent ; le débordement est signalé.

Les demi-sinus d’impact conservent l’impulsion discrète demandée avant le bloqueur DC à 20 Hz. Le glissement utilise un état de bristle borné ; l’archet ajoute une friction de Stribeck et peut recevoir la vitesse de surface du résonateur. Le roulement déclenche des micro-contacts sur les aspérités. reset(seed) rend les textures reproductibles. Le gain convertit les grandeurs physiques en niveau audio. Aucun moteur de collisions ni solveur de solides n’est créé.

## API publique {#api}

- [cydsp/synthesis/ContactExciter.h](doc:api-synthesis-contactexciter)
- [cydsp/synthesis/PhysicalModels.h](doc:api-synthesis-physicalmodels)
