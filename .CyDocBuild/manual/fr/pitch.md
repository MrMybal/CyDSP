---
id: pitch
title: Hauteur, durée et formants
description: Choisir le traitement à têtes de délai, spectral ou Stretch optionnel.
section: Guides
order: 27
sourceRevision: 1
parent: overview
---

## Choisir un moteur {#engines}

PitchEffects fournit correction vers une hauteur cible, intervalles et harmonisation multivoix avec gains, pans et délais créatifs. Son prepare ordinaire utilise deux têtes de délai sinc. prepareSpectral raccorde un moteur à vocodeur de phase annonçant N frames de latence, jusqu’à huit voix en mono/stéréo. Comptez séparément les délais créatifs de chaque voix.

SpectralPitchShifter utilise N puissance de deux de 64–16384 et un hop N/4. Il estime les fréquences instantanées, lie un canal guide et conserve les phases relatives entre canaux. Les pics significatifs guident le verrouillage local de phase ; une enveloppe lissée contrôle la compensation de formants. Le ratio 1 donne une reconstruction retardée aux arrondis près.

## Hypothèses sur les sources {#quality}

La largeur de lissage d’enveloppe vaut 150 Hz par défaut. Trop large, elle perd les formants étroits ; trop faible, elle suit les harmoniques. Le flux spectral positif déclenche heuristiquement une remise en phase aux attaques. Grands intervalles, polyphonie et formants graves mal résolus restent difficiles. La correction monophonique utilise la confiance YIN ; détecter une hauteur ne transcrit pas les notes polyphoniques.

TimeStretch appartient à la cible optionnelle CyDSP::Stretch et expose durée/hauteur/formants. Ses comptes de flux et besoins en buffers diffèrent du taux fixe ; respectez son API exacte. Préparez voix/données hors callback, conservez les contextes empruntés et videz les queues. Les voix d’harmonie spectrale inactives ne sont pas préchauffées automatiquement.

## API publique {#api}

- [cydsp/processors/PitchEffects.h](doc:api-processors-pitcheffects)
- [cydsp/processors/TimeStretch.h](doc:api-processors-timestretch)
- [cydsp/spectral/SpectralPitch.h](doc:api-spectral-spectralpitch)
