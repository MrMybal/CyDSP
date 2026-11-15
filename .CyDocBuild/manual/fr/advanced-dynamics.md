---
id: advanced-dynamics
title: EQ dynamique, transitoires et effets non linéaires
description: Choisir le transfert, l’antialiasing et le contrat du détecteur.
section: Guides
order: 13
sourceRevision: 1
parent: dynamics
---

## Saturation et antialiasing {#saturation}

AdaaSaturator fournit tanh, atan, cubic, hardClip, wavefolder et hysteresis. Drive, bias et outputGain sont explicites. Avec antialias actif, les formes sans mémoire utilisent l’antialiasing par primitive du premier ordre, avec limite au point milieu pour les échantillons proches. Il réduit le repliement sans l’éliminer et produit un lissage dépendant de la fréquence.

L’hystérésis est un opérateur de jeu borné suivi de tanh, pas un solveur physique d’aimantation. MathAccuracy fast et antialias sont indépendants. processMixed fusionne un étage d’entrée double optionnel, la forme et le mixage linéaire, tout en arrondissant le signal wet à T avant mélange. Conservez cet ordre pour comparer des chaînes séparées.

## Égaliseur dynamique et transitoires {#detectors}

DynamicEqualizer utilise des corrections passe-bande à crête unitaire, en cascade par bande, et des détecteurs indépendants alimentés par le sidechain original. Il accepte jusqu’à 16 bandes ; rangeDb peut atténuer ou amplifier. Il n’est pas interchangeable avec toute topologie d’égaliseur dynamique en cloche. DeEsser réutilise une bande. MultibandDynamics emploie une séparation FIR complémentaire, jusqu’à 16 bandes, avec la latence du FIR et sans anticipation du compresseur.

TransientShaper compare des enveloppes rapides/lentes et applique les contrôles attaque/sustain avec un gain maximal. processGain émet les gains linéaires correspondants et avance les mêmes enveloppes que process ; n’appelez pas les deux pour le même événement si un double avancement n’est pas voulu. Le mode fast approxime l’exponentielle du gain ; le [choix de précision](doc:quality#accuracy) reste local.

## Limitation true peak {#limiting}

TruePeakLimiter utilise un suréchantillonnage 4x/8x, une anticipation liée et un filtrage FIR de retour. La marge par défaut est 0,5 dB. La latence est un double fractionnaire. Le plafond dépend du modèle de reconstruction/filtrage : évaluez les crêtes interéchantillons sur les contenus réels. Il ne promet ni certification broadcast ni borne universelle de crête analogique.

## API publique {#api}

- [cydsp/processors/AdvancedDynamics.h](doc:api-processors-advanceddynamics)
