---
id: integration
title: Intégrer un DAW, plugin ou jeu
description: Raccorder callbacks, contrôles et scènes possédés par l’application.
section: Guides
order: 39
sourceRevision: 1
parent: overview
---

## Frontière du callback audio {#callback}

Faites correspondre ProcessSpec au taux réel du flux, au nombre de canaux et au plus grand bloc de traitement. Si un callback peut dépasser ce maximum, découpez-le en sous-blocs bornés et conservez les offsets d’événements. Gardez les tableaux de pointeurs planaires vivants pendant chaque appel. Convertissez les entrées/sorties entrelacées via des buffers préparés de l’application si nécessaire.

Les services périphériques/MIDI/fichiers/formats de plugins appartiennent au moteur consommateur. Un processeur CyDSP n’enregistre pas de plugin, n’ouvre pas de périphérique et ne crée pas de fenêtre. CyAudioCore est un consommateur ; l’utilisation directe respecte les mêmes contrats publics.

## Chaînes pratiques {#recipes}

| Usage | Construction et responsabilité |
| --- | --- |
| Insert de DAW | Filtre préparé → dynamique → gain de sortie ; déclarer anticipation et latence FIR à l’hôte. |
| Réverbération convolution | Décoder/adapter le taux de l’IR hors thread → préparer MatrixConvolver → send/mix → vider la queue. |
| Instrument | Planifier les notes → posséder l’allocation des voix → oscillateurs/samples → enveloppe/filtre → réserve/limitation. |
| Émetteur de jeu | Convertir pose/unités → propagation/matériaux/chemins → source indépendante → binaural/HOA → retour de pièce. |
| Affichage de mesure | Traiter l’analyse → copier l’instantané du propriétaire → publier à l’UI par une file de l’application. |

## Reconfiguration {#reconfigure}

Préparez processeurs de remplacement et données immuables hors audio. Publiez une commande à propriété sûre ou préparez une transition autorisée ; ne remplacez/détruisez pas des contextes vivants depuis un autre thread. Lors d’un changement de taux/disposition, arrêtez ou faites une transition explicite puis repréparez tous les composants concernés. Évitez pertes de distance et Doppler doublés, ainsi que latence dry/wet non compensée.
