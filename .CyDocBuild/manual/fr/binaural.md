---
id: binaural
title: Rendu binaural et sur enceintes
description: Choisir binaural analytique, HRIR mesurées ou dispositions VBAP.
section: Guides
order: 36
sourceRevision: 1
parent: spatial
---

## Rendu au casque {#headphones}

AnalyticBinaural modélise ITD de tête sphérique, ombrage, coloration avant/arrière et proximité d’oreille pour jusqu’à 32 entrées placées. Le rayon de source réduit le détail directionnel. Deux entrées peuvent représenter un émetteur stéréo étendu. C’est un modèle paramétrique, pas une HRTF mesurée ou personnalisée.

HrirBinaural rend les réponses mesurées. Chaque HrirPoint brut fournit des FIR gauche/droite sans onset et des retards d’oreilles séparés au taux préparé. Garder l’onset acoustique dans les FIR et appliquer les retards séparés peut produire filtrage en peigne ou retard doublé. Préférez un [HrtfSet préparé](doc:hrtf) pour convertir/partager les mesures.

setDirection interpole filtres alignés et retards, puis fond les lecteurs. Une demande pendant le fondu retourne transition_in_progress. Conservez la dernière direction acceptée jusqu’à acceptation de la nouvelle ; ne bloquez pas le thread audio en attente.

## Dispositions d’enceintes {#speakers}

VbapLayout prépare paires 2D ou triplets d’enveloppe convexe 3D pour 2–64 enceintes et retourne des gains à énergie unitaire. Hors couverture, choisissez erreur ou repli vers l’enceinte proche à la préparation. Une disposition clairsemée/mal répartie ne reproduit pas toutes les directions également.

Choisissez les [Ambisonics](doc:ambisonics) pour partager un champ intermédiaire entre sources. Panoramique et indices binauraux sont des rendus différents ; appliquer les deux involontairement peut changer niveau et localisation.
## Niveau diffus et calage des réflexions précoces {#diffuse-level}

Les deux surcharges de préparation HoaBinaural remplissent diffuseReferenceEnergy[2], diffuseDecoderEnergy[2], diffuseGain et diffuseMetricsAvailable dans designReport(). Ce sont les énergies impulsionnelles moyennes d’ondes planes décorrélées de bruit blanc unitaire sur les directions de conception. Le chemin temporel historique utilise des poids uniformes et son modèle causal de délai linéaire/sinc. Le chemin spectral utilise les spatialWeights normalisés et son modèle de délai par bins, y compris l’extrémité Nyquist réelle. La puissance décodée inclut les interférences entre canaux harmoniques. Les mesures historiques utilisent les FIR float/double effectives ; les spectrales précèdent la quantification float des coefficients.

Le gain d’amplitude conseillé vaut sqrt(somme énergie référence / somme énergie décodée). Il est commun aux oreilles, n’est pas appliqué automatiquement et restaure l’énergie large bande intégrée sur la grille de conception. Il ne corrige pas la coloration selon fréquence/direction et ne certifie pas la sonie d’un programme arbitraire. Les conceptions silencieuses/dégénérées fournissent diffuseMetricsAvailable=false et conservent gain=1. Ne divisez pas par zéro.

~~~cpp
cydsp::HoaBinaural<float> early;
const auto status = early.prepare({48000, 128, 4}, 1, bank); // LS temporel causal
if (status != cydsp::Status::ok) return status;
const auto report = early.designReport();
const double gain = report.diffuseMetricsAvailable ? report.diffuseGain : 1.0;
// Appliquer le gain fixe aux deux oreilles après rendu.
const auto latency = early.latencyFrames();
~~~

Pour les réflexions précoces calées sur le son direct, cette surcharge LS causale évite la demi-FFT de retard du concepteur spectral. La conversion de taux ou le noyau sinc optionnel peuvent encore ajouter une latence déclarée. Le MagLS spectral explicite garde sa FIR centrée et sa demi-FFT de retard. Cette évolution ne sélectionne aucun MagLS à phase minimale ni raccourci automatique modifiant la phase. Un gain large bande compense le niveau, et non la perte d’amplitude aiguë des LS d’ordre faible. Choisissez selon les contraintes de calage et de spectre.


## API publique {#api}

- [cydsp/spatial/SpatialRendering.h](doc:api-spatial-spatialrendering)
