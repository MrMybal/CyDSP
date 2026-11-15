---
id: restoration
title: Restauration du bruit, ronflement et clics
description: Utiliser atténuation spectrale, encoches, réparation et filtres adaptatifs.
section: Guides
order: 31
sourceRevision: 1
parent: overview
---

## Choisir le modèle de perturbation {#model}

SpectralDenoiser applique une atténuation spectrale préparée du bruit ; Dehum retire les composantes de ronflement/harmoniques configurées ; Declicker détecte et répare des perturbations impulsionnelles bornées. Adaptez les paramètres à la perturbation et protégez les transitoires utiles. Une atténuation forte ou un mauvais modèle de bruit peuvent créer bruit musical, perte d’aigus ou fausses réparations.

AdaptiveFilter implémente l’adaptation LMS/NLMS contre un signal souhaité/de référence. Choisissez pas/fuite/normalisation dans les plages acceptées, alignez les signaux et surveillez la convergence. Beaucoup de taps augmentent stockage et travail par échantillon. Un chemin adaptatif linéaire ne retire pas une distorsion non linéaire quelconque.

## Traitement et évaluation {#workflow}

Préparez avant le callback, budgétez les retards d’analyse/reconstruction et videz les queues spectrales. Commencez avec des seuils conservateurs, comparez au bypass à sonie égale, puis évaluez transitoires utiles, harmoniques et silence. Aucun parcours de fichiers, base enregistrée de bruit ou worker n’est implicite. Pour la fuite des haut-parleurs vers les micros, utilisez le [contrat de référence de l’annulation d’écho](doc:echo).

## API publique {#api}

- [cydsp/processors/AdaptiveFilter.h](doc:api-processors-adaptivefilter)
- [cydsp/processors/Restoration.h](doc:api-processors-restoration)
