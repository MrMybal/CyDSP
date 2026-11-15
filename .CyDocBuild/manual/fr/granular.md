---
id: granular
title: Granulation de samples et du direct
description: Planifier des grains bornés et des nuages reproductibles.
section: Guides
order: 24
sourceRevision: 1
parent: synthesis
---

## Grains sur samples {#sample}

Granulator prépare une sortie mono/stéréo depuis une source SampleData mono/stéréo préparée. maximumGrains vaut 1–4096, 128 par défaut. La source est empruntée : conservez objet et données vivants, stables et inchangés jusqu’à l’arrêt du traitement et au reset/nouvelle préparation du granulateur.

GrainParameters précise positionSeconds, durationMs, pitchRatio signé, gain, pan, enveloppe, interpolation et wrap. Un ratio négatif lit à l’envers. La conversion des taux source/sortie s’applique séparément. Les enveloppes Hann, triangulaire et rectangulaire ont des comportements de clic/spectre différents. Les grains s’ajoutent sans limitation automatique.

trigger accepte un offset en frames de sortie inférieur à maxBlockFrames. Des appels suivants plus courts reportent le délai restant. Une réserve pleine retourne update_queue_full au lieu de voler des voix. activeGrains inclut les grains en attente ; droppedGrains compte les événements automatiques refusés.

## Nuages et historique live {#live}

setCloud choisit densité et dispersions symétriques de position/hauteur/pan ; une densité nulle arrête les nouveaux événements sans arrêter les grains actifs. reset(seed) redémarre une planification reproductible par instance. process(add=false) efface la sortie ; add=true s’ajoute à l’audio présent.

LiveGranulator utilise un historique circulaire d’entrée préparé au lieu d’un sample décodé. Configurez capacités d’historique/grains avant traitement. Freeze conserve l’historique capturé ; chaque grain possède vitesse signée et gains de canaux. Gardez les lectures dans l’historique disponible et réglez explicitement dry/wet. Un historique figé n’est pas un nouveau buffer d’enregistrement infini.

## API publique {#api}

- [cydsp/synthesis/Granulator.h](doc:api-synthesis-granulator)
- [cydsp/synthesis/LiveGranulator.h](doc:api-synthesis-livegranulator)
