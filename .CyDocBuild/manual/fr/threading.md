---
id: threading
title: Propriété temps réel
description: Sérialiser les accès et publier des contrôles bornés.
section: Guides
order: 5
sourceRevision: 1
parent: contracts
---

## Un propriétaire par processeur mutable {#owner}

CyDSP ne crée pas de thread de traitement. Confiez chaque instance mutable à un propriétaire audio et sérialisez setters, reset, getters, déplacements et destruction. Copiez les métriques d’affichage dans des instantanés possédés par l’application au lieu de lire simultanément l’état d’un analyseur.

Samples, contextes et tableaux empruntés restent à des adresses stables jusqu’à l’arrêt des utilisateurs. Les états HRTF préparés partagés ont la durée de vie particulière décrite dans la [préparation HRTF](doc:hrtf).

## Communication bornée {#queues}

`SpscQueue` exige exactement un producteur et un consommateur. Préparez sa capacité hors traitement ; vérifiez push/pop et définissez une politique de saturation. N’introduisez pas de valeurs exigeant une destruction temps réel sans protocole de propriété.

`AsyncConvolver` accepte explicitement un producteur préparant une IR en parallèle d’un consommateur audio. Trois emplacements conservés évitent attente et destruction de plans retirés dans le callback. Préparation, reset et destruction exigent l’arrêt des deux côtés. Traitez la pression de file comme un résultat normal.

Sans allocation ne signifie pas peu coûteux : grandes requêtes de sonie, interpolation HRTF, rotations HOA et conception de coefficients se planifient. `noexcept` ne constitue pas une borne de temps CPU.

## API publique {#api}

- [cydsp/core/SpscQueue.h](doc:api-core-spscqueue)
- [cydsp/processors/AsyncConvolver.h](doc:api-processors-asyncconvolver)
