# CyDSP 0.1.0 — notes de version

[English](VERSION.md) · [README](README.fr.md) · [Suivi des problèmes](https://github.com/MrMybal/CyDSP/issues)

## Version actuelle et disponibilité

- **Composant** : bibliothèque statique de traitement audio en C++20 CyDSP.
- **Version des sources** : `0.1.0`, déclarée par `project(CyDSP VERSION 0.1.0)` dans [CMakeLists.txt](CMakeLists.txt).
- **Statut** : en développement. Il désigne l'implémentation actuelle des sources, pas une version binaire stable.
- **Disponibilité** : dépôt privé des sources ; aucune release publiée, aucun tag ni binaire téléchargeable de l'application ou de la bibliothèque.
- **Licence** : [MIT](LICENSE), copyright © 2026 MrMybal CyberAlien. Les notices tierces conservent leur attribution d'origine.

## Contenu des sources actuelles

- Tampons planaires communs, conversion de disposition, matrices de canaux et conventions de préparation/statut pour `float` et `double`.
- Rampes de gain, filtres FIR/IIR, cascades, banques de voix et transitions d'égalisation.
- FFT complexes et réelles, noyaux vectoriels, fenêtres et délais entiers ou fractionnaires.
- Convolution uniforme, hybride, non uniforme et progressive, MIMO à FFT partagées et publication préparée ou asynchrone des réponses impulsionnelles.
- Rééchantillonnage rationnel, rampes de rapport préparées, vidage de fin de flux et graphes de routage préparés.
- Implémentations privées scalaire/SSE2/AVX2/AVX-512/NEON/SVE, sélectionnées selon la cible compilée et les capacités du processeur/système.
- Cible CMake statique, installation du paquet et licence MIT de CyDSP livrée avec les notices amont conservées.

Les [contrats des modules](docs/Modules.md) précisent le comportement pris en charge. Ces notes décrivent les sources actuelles, sans leur attribuer de date de release ni promettre des fonctions futures.

## Limites documentées

- x64 et ARM64 sont prioritaires. x86 et ARM32 restent des cibles de compatibilité secondaires.
- Les vérifications natives ont été exécutées sur Windows x64/x86 et Quest 3 Android ARM64/ARM32. Linux de bureau, macOS et l'exécution d'applications Windows ARM64 demandent encore leurs environnements cibles.
- AVX-512 a été exécuté sur le processeur x64 compatible disponible. SVE se compile et se lie pour Android ARM64, mais doit encore être exécuté sur du matériel SVE, avec plusieurs longueurs vectorielles.
- ARM64EC est explicitement refusé. Sur ARM32, les traitements SIMD en double précision reviennent à une implémentation scalaire.
- Les modules optionnels ajoutent les périphériques audio/MIDI, les fichiers audio et la hauteur/durée ; l'adaptateur Unreal reste à réaliser. Les vérifications numériques n'établissent pas la qualité d'écoute ni la latence audio physique.
- La convolution progressive répartit le travail tardif et peut augmenter le coût CPU moyen. Les performances dépendent de la charge, de la configuration et de la cible ; aucune accélération universelle n'est annoncée.
- Les instances de processeurs modifiables suivent les règles de propriété et de sérialisation du [guide d'architecture](docs/Architecture.md) ; la publication asynchrone des réponses impulsionnelles possède son propre contrat producteur/consommateur.

## Problèmes connus et retours

Aucune liste de bugs propre à une release n'est publiée. Cela ne signifie pas que la bibliothèque est exempte de bugs. Consulter ou signaler les problèmes dans le [suivi privé](https://github.com/MrMybal/CyDSP/issues), avec l'ABI cible, le compilateur, la configuration du traitement et un exemple minimal reproductible. Une autorisation d'accès au dépôt est nécessaire.

Les limites ci-dessus décrivent la couverture et les frontières d'intégration ; elles ne constituent pas des tickets de bugs confirmés. Les vérifications exécutées et les mesures détaillées restent séparées dans [Validation](docs/Validation.md) et [Measurements](docs/Measurements.md), actuellement en anglais.

Les nouveaux DSP, modules optionnels, fonctions de synthèse/MPE/lecture et limites restantes des formats et plateformes figurent dans la [couverture détaillée](docs/Coverage.md). La parité complète avec un framework audio n’est pas annoncée.

## Extension audio intégrée

La source actuelle ajoute la synthèse polyphonique, un sampler, les voix personnalisées,
MPE/RPN et les messages UMP/MIDI 2, un transport avec boucles et une carte de tempo.
Le module optionnel `CyDSP::Sources` fournit lecture anticipée, seeks rééchantillonnés
et enregistrement sur worker. Les algorithmes MIT de Stretch sont adaptés dans le
moteur privé et utilisent les FFT, vecteurs et conventions CyDSP.

Le sampler interpole linéairement ; il ne garantit pas une conversion sans repliement.
Les transports UMP natifs, MIDI-CI et la configuration de flux sont implémentés.
L’interopérabilité avec du matériel MIDI 2 et la compilation/exécution Apple restent
à valider. Le service fournit les ressources de propriétés de base ; les ressources
spécialisées et les comportements des profils viennent de l’application ; voir [MIDI 2](docs/Midi2.md) et la
[couverture fonctionnelle](docs/Coverage.md) pour le périmètre exact.

Le service MIDI-CI fournit ResourceList/DeviceInfo/ChannelList/JSONSchema, les
instances de ressources, GET et SET complet/partiel, la pagination et les
abonnements notify/full/partial. Son client ajoute un cache à durée de validité,
le rafraîchissement regroupé et la protection contre les anciennes réponses GET.
Les [contrats du service](docs/CiProperty.md) précisent les politiques applicatives
et les validations matérielles restantes. Les tests macOS/Linux sont reportés
à la demande du propriétaire.

Ajouts DAW du 12 novembre : [contrats et périmètre](docs/DawIntegration.md), avec DSP, synthèse/presets, lecture, périphériques/MIDI et fichiers/codecs. La validation macOS/Linux reste différée.

Corrections du rapport d’intégration : métadonnées WAV publiques, capacités/identités/horloges audio, MIDI multiport/SysEx, proxy PCM et politiques de sources. Le module [Plugins](docs/Plugins.md) fournit des hôtes CLAP/VST3/LV2/JSFX, des éditeurs natifs et un worker séparé. Les formats supplémentaires et les modes spécifiques utilisent des fournisseurs explicites ; la migration de CyDAW et la validation physique restent distinctes.

Hébergement LV2/JSFX : adaptateurs CyDSP originaux, automation/MIDI/état, bus et worker LV2, éditeurs et canvas natifs Windows. Les dépendances conservent leurs notices ; voir les [contrats Plugins](docs/Plugins.md) et les [licences](docs/Licensing.md).
