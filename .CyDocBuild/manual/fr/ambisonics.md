---
id: ambisonics
title: Ambisonics et binaural HOA
description: Encoder, tourner et décoder les ordres zéro à sept ACN/SN3D.
section: Guides
order: 38
sourceRevision: 1
parent: spatial
---

## Format et rotations {#format}

ambisonicCoefficients, ambisonicRotation et AmbisonicRenderer acceptent les ordres 0–7, soit `(order+1)^2` canaux : 1–64. L’ordre est ACN et la normalisation SN3D. Au premier ordre : W,Y,Z,X. Convertissez explicitement FuMa/N3D hors de cette API.

Les rotations sont actives : encoder la direction d puis tourner égale encoder R*d. Pour l’orientation d’auditeur, fournissez la rotation inverse. Ne combinez pas une transformation monde-vers-auditeur avec une deuxième rotation de champ équivalente.

## Décoder vers enceintes ou oreilles {#decode}

Le décodage d’enceintes utilise des moindres carrés régularisés pour la disposition fournie. Choisissez la régularisation adaptée aux matrices mal conditionnées/sous-déterminées. Des canaux mathématiques supplémentaires ne recréent pas la résolution absente d’enceintes clairsemées ; ce n’est ni un décodeur AllRAD ni une correction de salle.

HoaBinaural projette les HRIR mesurées dans le champ harmonique, incorpore les retards acoustiques fractionnaires aux FIR et prépare un MatrixConvolver à deux oreilles et entrées HOA. La rotation du champ peut précéder la convolution. Préparez les filtres hors traitement, conservez les HRTF partagées et déclarez latences/queues de conversion/convolution dans le graphe.

```diagram
{"type":"flow","title":"Chaîne de signal HOA","nodes":[{"id":"sources","label":"Sources","x":0,"y":0},{"id":"encode","label":"ACN/SN3D","x":190,"y":0},{"id":"rotate","label":"Rotation","x":390,"y":0},{"id":"ears","label":"Binaural HOA","x":590,"y":0}],"edges":[{"from":"sources","to":"encode"},{"from":"encode","to":"rotate"},{"from":"rotate","to":"ears"}]}
```
