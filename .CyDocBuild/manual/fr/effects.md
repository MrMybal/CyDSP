---
id: effects
title: Effets de délai et modulation
description: Configurer chorus, phaser, échos, flanger et modulation d’amplitude.
section: Guides
order: 15
sourceRevision: 1
parent: delay
---

## Choisir le comportement et la capacité {#configure}

Chorus et Phaser exposent leurs jeux de paramètres. DelayEffect combine un historique circulaire, jusqu’à 16 taps directs et une lecture de feedback filtrée/saturée. Préparez d’abord maximumDelayMs ; tous les retards et excursions de modulation doivent tenir dans cette capacité.

Ses modes echo, flanger, vibrato et wowFlutter interprètent delayMs, depthMs, rateHz, feedback, lowPassHz, feedbackDrive, duckDb, wet et dry. Un changement de mode ne remplace pas ces valeurs par des presets cachés. Le routage ping-pong est explicite. Filtrage/saturation du feedback modifient décroissance et timbre ; gardez de la réserve de niveau et un feedback dans les bornes acceptées.

## Amplitude et gestion des queues {#amplitude}

AmplitudeModulator utilise vitesse/profondeur et le choix autopan pour trémolo/panoramique. Le panoramique ne reproduit pas les indices d’élévation HRTF. Reset redémarre la phase selon le contrat du processeur.

Continuez à fournir du silence pendant la décroissance des échos/réverbérations ; cesser d’appeler process fige le temps interne. Pour des événements précis à l’échantillon, découpez les blocs ou utilisez une entrée de modulation autorisée. Gardez un mouvement continu du délai si la modulation de hauteur est voulue, et des fondus de lecteurs sinon.

## API publique {#api}

- [cydsp/processors/AdvancedEffects.h](doc:api-processors-advancedeffects)
- [cydsp/processors/Effects.h](doc:api-processors-effects)
