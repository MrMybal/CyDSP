---
id: sofa
title: Import SOFA et banques HRTF
description: Importer les mesures en option ; préparer la qualité et les caches portables.
section: Guides
order: 43
sourceRevision: 1
parent: hrtf
---

## Cibles optionnelles {#targets}

| Option de compilation | Cible liée | Dépendances |
| --- | --- | --- |
| Cœur par défaut | CyDSP::CyDSP | Aucune dépendance SOFA/format de fichier |
| CYDSP_BUILD_HRTF_ASSETS=ON | CyDSP::HrtfAssets | Cœur uniquement |
| CYDSP_BUILD_SOFA=ON | CyDSP::Sofa | Lecteur compact privé libmysofa BSD-3-Clause + miniz MIT |
| Aussi CYDSP_SOFA_COMPLETE=ON | CyDSP::Sofa | SDK externes HDF5 ≥1.14.6 et zlib |

Toutes les cibles de production sont statiques. SOFA et assets valent OFF par défaut ; le lecteur compact vaut ON dans un importeur activé. Automatic choisit le lecteur complet s’il est compilé, sinon le compact. CMake ne télécharge aucun SDK. Le SDK complet doit être présent à l’édition de liens finale ; déclarez C et C++ et utilisez CMAKE_PREFIX_PATH pour le localiser.

~~~cmake
# Depuis les sources :
set(CYDSP_BUILD_SOFA ON CACHE BOOL "")
add_subdirectory(CyDSP)
target_link_libraries(editor PRIVATE CyDSP::Sofa)
# Depuis un paquet installé :
find_package(CyDSP 0.4.0 CONFIG REQUIRED COMPONENTS Sofa)
~~~

Une compilation distincte pour le jeu peut activer seulement HrtfAssets et charger les caches décodés sans aucun lecteur de format.

## Import mémoire et erreurs {#import}

Incluez cydsp/spatial/Sofa.h. L’hôte fournit les octets ; loadSofa ne lit aucun fichier applicatif. Import, préparation, EQ et lecture du cache allouent : utilisez un worker/éditeur, jamais le callback audio. Un échec préserve la sortie précédente ; SofaLoadReport donne une erreur précise.

~~~cpp
cydsp::SofaHrtf asset;
cydsp::SofaLoadOptions options;
cydsp::SofaLoadReport diagnostic;
const auto status = cydsp::loadSofa(fileBytes, asset, options, &diagnostic);
~~~

SimpleFreeFieldHRIR exige des IR M×2×N. GeneralFIR exige allowGeneralFir=true et une géométrie de récepteurs à deux oreilles. Les autres conventions échouent explicitement. Les sources sphériques en degrés/radians ou cartésiennes en mètres/cm/mm deviennent des directions unitaires relatives à ListenerPosition/View/Up (+X avant, +Y gauche, +Z haut). Les vecteurs d’écoute absents prennent les défauts de convention. ReceiverPosition identifie les oreilles par Y local. EmitterPosition non nul et les données multi-émetteurs ne sont pas pris en charge.

Data.Delay est lu en frames au taux source depuis 1×2 ou M×2, puis étendu par point. Il reste distinct de la détection d’onset des FIR. SamplingRate doit être constant (8–768 kHz). Distances et métadonnées/licence d’origine sont conservées.

Le lecteur complet gère les superblocs 0/2/3, objets 1/2, liens/attributs compacts ou denses, données contiguës/en blocs avec index modernes single/fixed/extensible, float32/64 et chaînes fixes/variables. Deflate, shuffle et Fletcher32 sont autorisés. Liens/stockages externes, datasets virtuels et filtres inconnus sont rejetés avant lecture ; CyDSP n’a pas de chargement de filtre personnalisé.

Le lecteur compact couvre les objets v2, disposition v3, index de blocs B-tree v1 avec shuffle puis deflate (les autres pipelines échouent), tableaux contigus et métadonnées compactes/denses prises en charge. Utilisez le complet pour les anciens objets v1 ou disposition/index v4. Le compact convertit les tableaux en float32. SofaHrtf64 exige donc le complet ou allowPrecisionLoss=true explicitement. Les rapports exposent la perte de précision ; une sortie float réduit volontairement une entrée float64. Un callback inflate noexcept peut utiliser zlib de l’hôte ; il doit retourner ok uniquement pour une longueur exacte et un flux zlib dont le checksum a été vérifié.

Limites par défaut : fichier 128 Mio, buffers décodés 512 Mio, métadonnées 64 Kio, 100000 points, 32768 taps. Échantillons et tableaux de coordonnées/délais partagent un budget. Le parseur compact borne aussi allocations, itérations de lecture et récursion. Ce sont des bornes de buffers CyDSP, pas de toute la mémoire du processus, des caches SDK, copies d’image de fichier et temporaires FFT. Utilisez les limites globales du worker/sandbox de l’hôte pour un import non fiable.

## Préparation et qualité {#prepare}

~~~cpp
cydsp::SofaPreparation settings;
const auto quality = cydsp::hrtfPreparation(
    cydsp::HrtfQuality::realtimeHigh, 48000, settings.hrtf);
if (quality != cydsp::Status::ok) return quality;
settings.diffuseFieldEqualization = true; // correction commune optionnelle
settings.normalization = cydsp::HrtfLevelNormalization::frontalEnergy;
cydsp::HrtfSet<float> bank;
cydsp::SofaPreparationReport report;
const auto prepared = cydsp::prepareSofaHrtf(asset, bank, settings, &report);
~~~

La préparation sélectionne la distance mesurée la plus proche de distanceMetres (1 m par défaut), puis les points à distanceToleranceMetres de cette couche. Elle ne mélange pas les directions identiques mesurées à plusieurs rayons et ne crée pas de mesures manquantes.

L’EQ optionnelle en champ diffus moyenne la puissance des deux oreilles, lisse la puissance logarithmique au tiers d’octave, borne le gain inverse (±12 dB par défaut), puis crée une FIR commune à phase minimale avec les primitives FFT/filtres CyDSP. Fournissez les poids d’angle solide par point d’origine pour une grille irrégulière ; une liste vide signifie des poids égaux, signalés dans le rapport. C’est une correction approchée, pas une calibration de casque ni une intégration exacte d’une grille irrégulière.

FrontalEnergy applique un gain commun pour que l’énergie impulsionnelle moyenne gauche/droite du point le plus frontal vaille un, en préservant les différences de niveau entre oreilles. Cela n’aplatit pas le spectre frontal. L’EQ allonge la FIR ; vérifiez distance, gain et troncature dans le rapport.

| HrtfQuality | FIR maximale | Interpolation de direction | Délai fractionnaire / demi-longueur sinc de conversion |
| --- | --- | --- | --- |
| economy | 256 taps | 4 voisins | Linéaire / 16 |
| realtimeHigh | 2048 taps | 8 voisins | Sinc à 17 taps / 64 |
| reference | 65536 taps | Tous les points | Sinc à 65 taps / 128 |

reference rejette la troncature, accepte jusqu’à 100000 points et conserve la double précision avec SofaHrtf64/HrtfSet<double>. Les bornes se règlent dans les limites de l’en-tête et du budget de coefficients. HrtfPreparation par défaut conserve le comportement global/linéaire existant ; les profils sont explicites. QualityProfile associe ses trois niveaux à ces profils. Plusieurs profils peuvent coexister dans une application.

Les modes à voisins utilisent un index spatial équilibré préparé et des poids de Shepard normalisés. Un ensemble fini peut changer aux frontières ; fondez les changements de direction. La pondération globale conserve la continuité sur toute la banque. HrirBinaural incorpore le noyau de délai fractionnaire pendant setDirection ; L’ancienne surcharge HoaBinaural l’applique en préparation. La conception spectrale évalue plutôt les délais fractionnaires par la phase des bins et déclare sa propre latence ajoutée. La latence commune ajoutée vaut la moitié entière de la longueur du noyau et est exposée par chaque renderer.

HrirBinaural utilise une convolution FIR directe : les longues FIR de référence et les mises à jour globales visent l’éditeur/offline. Un profil de qualité ne promet ni un calcul plus rapide ni une meilleure localisation perçue pour toute mesure. HoaBinaural utilise la convolution matricielle existante. Process/reset/setDirection préparés n’allouent pas ; sérialisez paramètres et traitement d’une même instance. Les coefficients HrtfSet partagés sont immuables ; historiques et fondus restent propres aux voix.

## Cache portable et droits {#cache}

encodeHrtfAsset stocke les mesures décodées et leurs métadonnées dans un cache versionné little-endian float32/64 avec CRC32. decodeHrtfAsset vérifie tailles, checksum et données avant de remplacer la sortie, avec seulement CyDSP::HrtfAssets. Réduire un cache double exige allowPrecisionLoss. Ce sont des mesures décodées, pas un renderer ni un résultat d’EQ sérialisé ; appelez prepareSofaHrtf après chargement avec la qualité/EQ choisies. Le CRC détecte une corruption accidentelle, pas un remplacement malveillant.

Aucun jeu HRTF mesuré n’est distribué. Les tests utilisent des signaux synthétiques originaux MIT. Le code CyDSP original reste MIT ; lecteurs/SDK optionnels conservent leurs licences permissives. Les droits des mesures restent distincts : vérifiez les termes de chaque banque distribuée et conservez les [notices des composants](doc:licensing).

## API publique {#api}

- [cydsp/spatial/Sofa.h](doc:api-spatial-sofa)
