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
## Choisir la conception des filtres binauraux {#design}

L’appel existant prepare(spec, order, bank, regularization, backend) conserve la projection temporelle par moindres carrés. Il reste utile pour la compatibilité et une faible latence ajoutée. Aux faibles ordres, projeter la phase des HRIR dépendante de la direction peut perdre de l’énergie dans les aigus ; augmenter le gain ne corrige pas toute la coloration directionnelle.

La surcharge explicite HoaDecoderDesign ajoute les moindres carrés spectraux pondérés et MagLS. L’implémentation est du C++ MIT original fondé sur l’[objectif publié](https://pub.dega-akustik.de/DAGA_2018/data/articles/000301.pdf), avec les FFT et MatrixConvolver CyDSP. Aucun code IEM/SPARTA ni nouvelle dépendance n’est importé.

~~~cpp
#include <cydsp/spatial/SpatialRendering.h>
cydsp::HoaDecoderDesign design;
design.method = cydsp::HoaDecoderMethod::magLS;
design.crossoverHz = 2000;
design.transitionHz = 500;
design.maxRe = false;
cydsp::HoaBinaural<float> renderer;
const auto status = renderer.prepare({48000, 256, 4}, 1, bank, design);
if (status != cydsp::Status::ok) return status;
const auto report = renderer.designReport();
~~~

Sous la transition, l’ajustement utilise la réponse HRTF complexe, avec les délais d’arrivée propres aux directions et oreilles. Dans la transition il mélange erreurs complexe et d’amplitude ; au-dessus, des projections alternées ajustent les amplitudes avec une initialisation de phase continue entre fréquences. Les itérations sont bornées (12 par défaut, 1–256) ; la solution est locale, pas un optimum global garanti. La bande par défaut va de 1750 à 2250 Hz ; choisissez-la selon contenu, ordre et auditeur. Relâcher la phase aiguë conserve l’ajustement LS grave, pas tous les indices perceptifs ni la résolution spatiale exacte.

La conception spectrale utilise tous les points fournis avec leurs poids ; le nombre de voisins du renderer direct et la longueur de son noyau de délai ne pilotent pas cette projection. Les délais fractionnaires sont évalués spectralement aux bins de conception.

Une ITD appartient à une direction. Avec plusieurs directions mélangées, un délai unique par oreille en sortie ne restitue pas toutes les ITD d’origine. La conception conserve les cibles complexes graves et relâche la phase aiguë ; elle ne prétend pas fournir une opération universelle de retrait/réinjection d’ITD.

maxRe atténue en option les degrés harmoniques aigus par les poids de Legendre max-rE. Il élargit la réponse angulaire et peut modifier l’ajustement d’amplitude/niveau. Ce réglage distinct de MagLS vaut OFF par défaut et ne remplace pas l’optimisation d’amplitude aiguë.

Les poids spatiaux sont vides pour une pondération égale, ou contiennent un poids non négatif par point de la banque. Ils sont normalisés en interne. Utilisez des poids d’angle solide adaptés aux grilles mesurées irrégulières. La régularisation agit sur cette matrice de Gram normalisée, indépendamment du nombre de points ; sa valeur n’est pas numériquement identique à la régularisation non normalisée de l’ancienne surcharge.

## Latence, mémoire et diagnostics {#design-limits}

La conception spectrale s’exécute seulement dans prepare, sur worker/éditeur. Elle conserve toute la FIR conçue, sans troncature silencieuse. La taille FFT automatique est la puissance de deux suivante au moins égale au double de la HRIR complète avec délais (minimum 32, maximum 65536). Une taille explicite doit respecter la même borne basse. Le décalage circulaire causal ajoute taille-FFT/2 frames ; latencyFrames inclut cela et la latence de conversion de fréquence de la HRTF. Par exemple, une FFT de 2048 ajoute 1024 frames : 23,22 ms à 44,1 kHz. C’est un compromis de latence de traitement, pas un algorithme à latence nulle. Une banque trop longue pour la borne FFT échoue explicitement ; la projection historique reste un choix séparé.

maximumStorageBytes (512 Mio par défaut) borne l’estimation des matrices, spectres et FIR du concepteur ; il ne plafonne pas tout le renderer, les copies de filtres typés, la convolution ou le processus. Grilles denses, grandes FFT et ordres élevés alourdissent nettement la préparation. Le rendu audio réutilise le convolver MIMO préparé ; process/reset/setRotation n’allouent pas. Sérialisez paramètres et traitement d’une même instance.

designReport fournit FFT, longueur FIR, latence totale déclarée et erreurs RMS relatives pondérées. lowBandComplexError mesure l’erreur complexe sous la transition. highBandMagnitudeError mesure l’erreur d’amplitude au-dessus ; initialHighBandMagnitudeError donne la référence LS spectrale sur les mêmes données. Ce sont des erreurs sans unité sur les directions de conception et bins FFT hors extrémités, avant quantification des filtres float : ni rapports de vitesse, ni notes d’écoute, ni validation indépendante. spectralMetricsAvailable vaut false pour l’ancienne surcharge : ses champs d’erreur initialisés à zéro ne sont pas des mesures. MagLS ne personnalise pas la HRTF et ne garantit pas l’absence de coloration ; évaluez contenus et mouvements d’auditeur.
