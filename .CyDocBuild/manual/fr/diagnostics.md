---
id: diagnostics
title: Réponses et signaux de mesure
description: Mesurer phase, cohérence, distorsion, sweeps et décroissance d’IR.
section: Guides
order: 30
sourceRevision: 1
parent: analysis
---

## Réponse et distorsion {#response}

firResponse/iirResponse évaluent réponse complexe, phase et retard de groupe des filtres fournis. TransferAnalyzer accumule les spectres référence/sortie pour estimer transfert et cohérence selon la fréquence. Alignez la référence et choisissez fenêtre/durée de moyennage adaptées ; une faible cohérence avertit qu’un ratio n’est pas une réponse linéaire fiable.

MeasurementGenerator fournit les signaux déterministes configurés. measureDistortion estime THD/THD+N depuis une fenêtre périodique et une fondamentale choisie. Utilisez fréquences cohérentes ou fenêtres adaptées et distinguez puissance harmonique du résidu/bruit ; la résolution FFT finie change le résultat.

## Sweeps et impulsions {#impulse}

inverseLogSweep/deconvolveSweep servent à reconstruire une réponse par balayage. Enregistrez assez de temps avant/après, alignez lecture/capture et évitez écrêtage ou traitement variable pour mesurer un système linéaire. Les entrées/sorties fichiers/périphériques restent dans l’application.

analyzeImpulse rapporte onset, premières réflexions, énergie, clarté et ajustements de décroissance/RT60 par bandes. Interprétez l’extrapolation selon la plage d’ajustement valide et le bruit de fond. matchFdnToImpulse propose des réglages FDN nominaux depuis ces résumés, pas une reconstruction parfaite de pièce. Ces mesures sont des travaux de préparation/analyse ; ne placez pas une déconvolution de sweep complète ni une grande analyse d’IR dans le callback temps réel.

## API publique {#api}

- [cydsp/analysis/Diagnostics.h](doc:api-analysis-diagnostics)
- [cydsp/analysis/ImpulseAnalysis.h](doc:api-analysis-impulseanalysis)
