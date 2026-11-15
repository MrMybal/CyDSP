---
id: synthesis
title: Oscillateurs et enveloppes
description: Générer formes, FM/PM, morphing de tables et enveloppes.
section: Guides
order: 22
sourceRevision: 1
parent: overview
---

## Oscillateurs et limites de bande {#oscillators}

Oscillator génère sinus, triangle, dent de scie, carré et bruit blanc avec fréquence d’échantillonnage/canaux préparés. `next` avance d’un échantillon et `process` rend un bloc ; n’appelez pas involontairement les deux sur le même intervalle. Wavetable prépare les versions par bandes harmoniques et WavetableOscillator choisit la banque adaptée une fois par bloc à fréquence fixe.

ModulatedOscillator propose FM/PM audio-rate, synchronisation, morphing de tables et unisson. Ces modes suivent leurs contrats de suréchantillonnage/limitation de bande préparés ; forte modulation, discontinuités de sync ou hauteurs élevées peuvent encore replier le spectre. Le choix d’une table à fréquence fixe ne résout pas tous les spectres modulés.

## Enveloppes et LFO {#envelopes}

Les paramètres Adsr utilisent des secondes pour attaque/decay/release et une amplitude linéaire pour sustain. noteOn/noteOff changent d’étage. `process(span)` écrit la même séquence que des appels next successifs. L’application possède la précision des événements de fin ; consultez active/value depuis le propriétaire ou découpez aux événements.

MultiSegmentEnvelope fournit une séquence préparée. ModulationLfo comprend sample-and-hold et comportement à graine ; ProceduralSource génère les textures bornées configurées. Reset utilise phase/aléatoire par instance, sans générateur global. L’hôte gère allocation MIDI, notes, niveaux et limiteur final.

## API publique {#api}

- [cydsp/processors/Generators.h](doc:api-processors-generators)
- [cydsp/synthesis/AdvancedSynthesis.h](doc:api-synthesis-advancedsynthesis)
- [cydsp/synthesis/Wavetable.h](doc:api-synthesis-wavetable)
