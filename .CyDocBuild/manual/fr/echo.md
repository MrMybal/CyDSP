---
id: echo
title: Annulation d’écho acoustique
description: Aligner référence haut-parleur et microphones avant adaptation NLMS.
section: Guides
order: 32
sourceRevision: 1
parent: restoration
---

## Référence et canaux {#reference}

EchoCanceller reçoit une référence mono représentant l’audio envoyé aux haut-parleurs et 1–8 microphones. Chaque micro possède son chemin FIR NLMS indépendant de prédiction/résidu, jusqu’à 4096 taps. Préparez le retard maximal ; l’hôte fournit l’alignement jusqu’à 0,5 s et doit gérer horloges séparées et dérive.

Utilisez le véritable flux haut-parleur après les traitements de sortie pertinents, pas un prémix sans rapport. Surveillez activité de référence, adaptation, puissance résiduelle, ERLE et compteurs via les métriques sérialisées par le propriétaire.

## Double parole et limites {#adaptation}

Après l’apprentissage initial, puissance résiduelle et cohérence prédiction/microphone contrôlent le gel lié de l’adaptation avec maintien configurable. Référence silencieuse ou adaptation désactivée gèlent aussi l’apprentissage. forceAdaptation demande un réapprentissage volontaire après changement de chemin.

Réglez les seuils sur la parole et les changements de pièce réels. Le FIR linéaire ne résout ni non-linéarités du haut-parleur, ni dérive d’horloge, ni une pile complète de communication. Alignez les blocs micro/référence et conservez le modèle de retard lors des découpages.

## API publique {#api}

- [cydsp/processors/EchoCanceller.h](doc:api-processors-echocanceller)
