---
id: spatial
title: Coordonnées 3D et propagation
description: Utiliser mètres, radians, poses source/auditeur et milieux acoustiques.
section: Guides
order: 34
sourceRevision: 1
parent: overview
---

## Une convention de coordonnées {#coordinates}

CyDSP utilise un repère direct : **+X devant, +Y à gauche, +Z vers le haut**. Positions en mètres, vitesses en m/s, angles en radians et quaternions `(w,x,y,z)`. Convertissez explicitement axes et échelle du jeu à la frontière. Normalisez les orientations et utilisez la validation au lieu de fournir directement les unités propres au moteur.

relativeSource donne distance, directions locale/monde, azimut/élévation et vitesses radiales. relativeSources écrit les lignes auditeur-major à `listener * sourceCount + source`. interpolatePose interpole les quaternions par l’arc court.

## Distance, directivité et milieux {#propagation}

DistanceAttenuation choisit une atténuation inverse, exponentielle ou linéaire avec référence/distance maximale, rolloff et politique de clamp. Directivity utilise un motif, des pertes arrière par bandes, un plancher et une conservation optionnelle de polarité. AcousticMedium contient vitesse du son, densité et absorption trois bandes ; les helpers atmosphériques utilisent température, humidité et pression. waterMedium est un préréglage de référence, pas une physique automatique de l’eau.

acousticTravel cumule retards/pertes des segments. DopplerPlan met en cache vitesse/intensité/bornes de hauteur pour les mises à jour répétées ; DopplerResampler applique le ratio entrée/sortie obtenu avec comptes explicites. N’appliquez pas le même Doppler par rééchantillonnage puis par retard de propagation variable. L’hôte possède géométrie, détection des occlusions, mises à jour et FIFO audio.

## API publique {#api}

- [cydsp/processors/BandGain.h](doc:api-processors-bandgain)
- [cydsp/spatial/Doppler.h](doc:api-spatial-doppler)
- [cydsp/spatial/SpatialMath.h](doc:api-spatial-spatialmath)
