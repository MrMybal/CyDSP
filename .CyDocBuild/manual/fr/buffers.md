---
id: buffers
title: Buffers et dispositions de canaux
description: Posséder le stockage, emprunter des vues et entrelacer les canaux.
section: Guides
order: 4
sourceRevision: 1
parent: contracts
---

## Vues et stockage {#views}

`AudioBlock<T>` emprunte un tableau de pointeurs planaires, des canaux et des frames. `channel(i)` applique l’offset du sous-bloc ; `channelData()` retourne le tableau original. Les appels `subBlock` imbriqués cumulent les offsets. Ne considérez pas ce tableau original comme pointant déjà sur le début du sous-bloc.

`AudioBuffer<T>::resize(channels, frames)` alloue un stockage de canaux initialisé à zéro, aligné sur 64 octets et doté d’un padding privé. Son bloc expose seulement les frames logiques. Allouez hors traitement. Gardez allocations et tableaux de pointeurs vivants et inchangés pendant l’utilisation des vues empruntées.

## Recouvrement et conversion {#overlap}

Les processeurs usuels à taux fixe acceptent les vues exactement en place par canal correspondant ou disjointes ; le recouvrement partiel échoue. ChannelMatrix et MatrixConvolver permettent aussi les alias exacts entre canaux documentés. FFT, rééchantillonneurs et API changeant la forme ont leurs propres règles.

`interleave`/`deinterleave` utilisent exactement `frames * channels` échantillons plats à `frame * channels + channel`. Tout recouvrement planaire/plat est rejeté ; deinterleave refuse aussi les canaux de sortie qui se recouvrent. Aucune conversion de fréquence, format d’échantillon ou ordre d’enceintes n’est implicite. La validation ne prouve ni la durée de vie ni la capacité des allocations.

## API publique {#api}

- [cydsp/core/AudioBlock.h](doc:api-core-audioblock)
- [cydsp/core/AudioBuffer.h](doc:api-core-audiobuffer)
- [cydsp/core/AudioLayout.h](doc:api-core-audiolayout)
- [cydsp/core/Memory.h](doc:api-core-memory)
