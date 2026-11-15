---
id: filter-design
title: Conception des filtres et banques multibandes
description: Préparer IIR d’ordre élevé, FIR linéaires/minimales et séparations.
section: Guides
order: 11
sourceRevision: 1
parent: filters
---

## Familles IIR {#iir}

`designIir` produit des sections passe-bas/haut Butterworth, Chebyshev I/II, elliptique ou Bessel. Réglez ordre du prototype, fréquence d’échantillonnage, coupure et paramètres d’ondulation/atténuation avant traitement. `designBandIir` transforme un prototype d’ordre 1–16 en passe-bande/coupe-bande d’ordre double. Gardez les deux limites sous Nyquist et vérifiez Status.

La conception est un travail de préparation et peut allouer. Vérifiez la réponse finale par les [diagnostics](doc:diagnostics), notamment près de Nyquist et pour les bandes étroites/d’ordre élevé. PeakingBiquadDesigner met en cache la partie fréquence/Q pour l’automation du gain et peut valider la stabilité après arrondi au format d’échantillon.

## Phase FIR et reconstruction {#fir}

`designFirEqualizer` conçoit une réponse tronquée/fenêtrée depuis des points fréquence/gain. Un FIR à phase linéaire retarde sa sortie ; `minimumPhaseFir` utilise le cepstre réel pour une approximation dont l’énergie arrive plus tôt. Troncature et creux profonds peuvent modifier la réponse d’amplitude : mesurez le résultat sans supposer l’équivalence exacte.

FirFilterBank utilise des différences de passe-bas FIR pour au plus 32 bandes complémentaires. Leur somme neutre égale l’entrée retardée de `(taps-1)/2`, aux arrondis près. L’ordre des sorties est `band * inputChannels + channel`. Réservez assez de canaux et compensez cette latence avant un mixage dry/wet parallèle. MultibandDynamics réutilise cette base avec sa propre limite de bandes plus faible.

## API publique {#api}

- [cydsp/processors/AdvancedFilterDesign.h](doc:api-processors-advancedfilterdesign)
- [cydsp/processors/FilterBank.h](doc:api-processors-filterbank)
- [cydsp/processors/FilterDesign.h](doc:api-processors-filterdesign)
