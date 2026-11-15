---
id: delay
title: Délai, historique et interpolation
description: Lire l’historique fractionnaire et choisir les transitions de délai.
section: Guides
order: 14
sourceRevision: 1
parent: overview
---

## Historique partagé {#history}

CircularHistory écrit chaque échantillon source une fois et accepte des lectures indépendantes. L’âge zéro désigne le dernier échantillon écrit ; passé indisponible/futur donnent zéro. Les fenêtres entières sont rassemblées en double. Les choix comprennent nearest, linéaire, Lagrange cubique et Catmull–Rom ; les lecteurs sinc offrent une alternative filtrée préparée.

Qualité d’interpolation et plage de lecture autorisée sont distinctes. Gardez les positions dans l’historique préparé et respectez l’ordre causal. ThiranDelay est un passe-tout séquentiel du premier ordre pour un retard fractionnaire dans (0,1], pas un historique à accès aléatoire.

## Taps et modulation {#taps}

MultiTapDelay route jusqu’à 4096 taps entrée/sortie dans les capacités de canaux/retards préparées. Le retard est en frames. setTaps fond des configurations complètes ; une édition pendant la transition retourne transition_in_progress. Le fondu entre lecteurs évite le glissement de hauteur d’une tête déplacée en continu.

DelayEffect module volontairement la position pour flanger, vibrato et wow/flutter ; réglez délai/profondeur/vitesse/mix sans considérer le mode comme un preset caché. Consultez les [effets](doc:effects). Un retard de propagation physique variable produit déjà du Doppler : évitez d’appliquer deux fois la même variation de hauteur.

## API publique {#api}

- [cydsp/core/CircularHistory.h](doc:api-core-circularhistory)
- [cydsp/processors/DelayLine.h](doc:api-processors-delayline)
- [cydsp/processors/MultiTapDelay.h](doc:api-processors-multitapdelay)
- [cydsp/processors/PolynomialInterpolator.h](doc:api-processors-polynomialinterpolator)
- [cydsp/processors/SincInterpolator.h](doc:api-processors-sincinterpolator)
