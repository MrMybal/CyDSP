---
id: troubleshooting
title: Résoudre les problèmes
description: Diagnostiquer formes, silence, latence, flux et spatialisation.
section: Guides
order: 40
sourceRevision: 1
parent: overview
---

## Symptômes courants {#symptoms}

| Symptôme | Vérification |
| --- | --- |
| not_prepared | Tous les membres ont réussi prepare ; un objet source déplacé exige une préparation. |
| channel_mismatch/block_too_large | Les vues respectent canaux et frames maximales, sidechains compris. |
| partial_overlap | Vues exactement en place autorisées ou buffers disjoints ; compter les offsets de sous-blocs. |
| Silence après erreur de graphe | Chercher le premier Status en échec et vérifier la vie des contextes. |
| Clic lors d’une modification | Utiliser rampes, fondus de filtres stables ou transitions d’IR/graphes préparées. |
| Flanging dry/wet | Aligner la latence fixe et distinguer la phase IIR dépendante de la fréquence. |
| Entrée rééchantillonnée perdue | Garder le suffixe après inputConsumed ; publier seulement outputProduced. |
| Queue de convolution absente | Continuer les zéros ou utiliser l’opération de vidange autorisée. |
| Déclenchement de grain refusé | Vérifier réserve, offset et durée de vie de source/position. |
| Analyse initiale absente | Accumuler une fenêtre entière ; respecter hop et indicateurs de disponibilité. |
| LUFS retourne would_block | Historique exact plein ; dimensionner la session ou arrêter/réinitialiser volontairement. |
| Direction HRTF mobile refusée | Fondu précédent actif ; regrouper/réessayer sans bloquer. |
| Direction ou vitesse 3D incorrecte | Convertir axes, mètres, radians, ordre du quaternion et vitesses radiales. |
| Échantillons différents selon précision/backend | Ordre des réductions, approximation, antialiasing et découpage FFT peuvent différer. |

## Examiner une chaîne minimale {#minimal}

Relevez version, ABI, taux, canaux, découpage de blocs et réglages exacts. Réduisez à un processeur et un signal fini ; vérifiez Status avant d’interpréter la sortie. Faites correspondre les contrats numériques lors d’une comparaison : latence, topologie du détecteur et antialiasing. Un algorithme plus rapide au comportement différent constitue un autre choix de qualité.
