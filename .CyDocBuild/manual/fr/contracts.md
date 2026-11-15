---
id: contracts
title: Contrats de traitement
description: Durées de vie, états, unités, latence et queues.
section: Guides
order: 3
sourceRevision: 1
parent: overview
---

## Cycle de vie et propriété {#lifecycle}

`prepare` valide et peut allouer ; un échec conserve la configuration précédente utilisable. `process` et `reset` conservent le stockage préparé sans allocation/libération, verrou ni entrée/sortie. Nouvelle préparation, destruction, déplacements et staging explicite comme `stageImpulse` se font hors traitement temps réel.

Sérialisez tous les accès à une instance mutable, setters et getters compris. Les vues ne possèdent ni échantillons ni tableaux de pointeurs. Données partagées et contextes ont la durée de vie précisée par leur API. Un `AudioBuffer::resize` réussi invalide les anciennes vues.

## Unités et erreurs {#status}

Une frame contient un échantillon par canal. Les fréquences sont en Hz ; les durées suivent le suffixe (`Seconds`, `Ms`, `Frames`) ; les gains sont linéaires sauf mention dB. Les formats sont float/double ; les comptes utilisent généralement `std::size_t`.

| État | Action de l’appelant |
| --- | --- |
| `ok` | Utiliser la sortie ; respecter les comptes des API à taux variable. |
| `not_prepared` | Préparer d’abord. |
| `invalid_spec`, `invalid_argument` | Corriger dimensions, plages ou valeurs non finies. |
| `channel_mismatch`, `block_too_large` | Respecter les canaux et le bloc maximal préparés. |
| `partial_overlap` | Utiliser un alias exact autorisé ou du stockage disjoint. |
| `unsupported_backend` | Choisir un backend compilé et disponible pour le CPU/OS. |
| `transition_in_progress`, `update_queue_full`, `would_block` | Conserver/réessayer selon l’API ; ne pas attendre en boucle dans le callback. |
| `end_of_stream` | Vidange terminée ; réinitialiser avant un nouveau flux. |

Les erreurs ordinaires de validation préservent sortie et état. Une erreur de callback de graphe ne peut annuler les nœuds exécutés : les sorties externes sont mises au silence. Des gains ou sommes extrêmes peuvent déborder malgré des entrées finies ; CyDSP ne normalise ni n’écrête globalement l’audio.

## Latence et queues {#timing}

La latence retarde la sortie ; une queue continue après la fin d’entrée. Consultez `latencyFrames()`/`tailFrames()` et conservez les fractions retournées en double. Un convolver sans latence de bloc ajoutée peut utiliser une IR contenant un prédélai acoustique. Fournissez des zéros ou appelez `flush`/`finish` jusqu’à vidange. Un freeze peut créer une queue infinie. Reset efface l’historique au lieu de le rendre.

## API publique {#api}

- [cydsp/core/Types.h](doc:api-core-types)
