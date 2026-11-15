---
id: sampling
title: Samples préparés et boucles
description: Lire des blocs avec interpolation et mips harmoniques.
section: Guides
order: 23
sourceRevision: 1
parent: synthesis
---

## Préparer et posséder les samples {#prepare}

SampleData::prepare copie les échantillons planaires, leur taux et la note racine, puis prépare les mips antirepliement. Décodez les fichiers dans l’application avant l’appel. Les objets empruntant SampleData, comme Granulator et SourceBatch, exigent des adresses stables et un contenu immuable pendant l’utilisation.

`value(channel, position, ratio, interpolation)` lit une position fractionnaire d’échantillon. SampleInterpolation propose linear et bandLimited. Une lecture accélérée peut utiliser un mip de bande réduite ; cela réduit le repliement sans promettre la reconstruction parfaite à tout ratio.

## Boucles et lecture séquentielle {#loops}

setLoop utilise `[first,end)` avec fondu optionnel ; 0/0 désactive la boucle. Préparez les boucles pendant l’arrêt des lecteurs et sérialisez les accès. Le fondu change le début effectif après bouclage en loopStart+loopCrossfade selon le contrat.

`render(position, ratio, mode, output, inside)` choisit le mip une fois par bloc, écrit tous les canaux et avance la position possédée par l’appelant. Les sorties supplémentaires répètent le dernier canal source. Les lectures au-delà de la fin deviennent nulles et avancent encore. inside compte le préfixe lu avant la fin. Les requêtes invalides préservent position/sortie. L’application gère fin de sample et ordonnancement des voix d’instrument.

## API publique {#api}

- [cydsp/synthesis/SampleData.h](doc:api-synthesis-sampledata)
