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
find_package(CyDSP 0.4.6 CONFIG REQUIRED COMPONENTS Sofa)
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

HrirBinaural choisit une convolution directe ou FFT overlap-save en double précision. Le mode automatique utilise la FFT à partir de 96 taps et 32 frames par appel si le plan respecte sa limite et le rapport délai/taps. Les appels courts et plans trop grands gardent le traitement direct. Les longues FIR de référence et mises à jour globales peuvent encore nécessiter un budget éditeur/offline. Un profil de qualité ne promet ni un calcul plus rapide ni une meilleure localisation perçue pour toute mesure. HoaBinaural utilise la convolution matricielle existante. Process/reset/setDirection préparés n’allouent pas ; sérialisez paramètres et traitement d’une même instance. Les coefficients HrtfSet partagés sont immuables ; historiques et fondus restent propres aux voix.

## Cache portable et droits {#cache}

encodeHrtfAsset stocke les mesures décodées et leurs métadonnées dans un cache versionné little-endian float32/64 avec CRC32. decodeHrtfAsset vérifie tailles, checksum et données avant de remplacer la sortie, avec seulement CyDSP::HrtfAssets. Réduire un cache double exige allowPrecisionLoss. Ce sont des mesures décodées, pas un renderer ni un résultat d’EQ sérialisé ; appelez prepareSofaHrtf après chargement avec la qualité/EQ choisies. Le CRC détecte une corruption accidentelle, pas un remplacement malveillant.

La sélection SADIE II optionnelle est décrite ci-dessous. Les fixtures synthétiques restent des signaux originaux MIT. Le code CyDSP original reste MIT ; les lecteurs/SDK optionnels conservent leurs licences permissives. Les droits des mesures sont distincts : conservez les [notices des composants et données](doc:licensing).

## Banque mesurée optionnelle sous 100 Mo {#bundled}

Le dépôt inclut trois fichiers inchangés de [SADIE II version 2-2](https://zenodo.org/records/12092466). Choisissez un profil d’auditeur parmi ces alternatives ; les fichiers ne sont pas combinés automatiquement.

| Profil | Sujet | Directions | Octets SOFA |
| --- | --- | --- | --- |
| Mannequin Neumann KU100 | D1 | 8802 | 36310650 |
| Mannequin KEMAR | D2 | 8802 | 36310650 |
| Auditeur humain | H10 | 2114 | 8753949 |

Tous les fichiers choisis contiennent les mesures originales à 48 kHz, 256 taps et deux oreilles en float64 (SimpleFreeFieldHRIR). Les signaux totalisent 81375249 octets, soit 81,38 Mo décimaux. Le dossier complet, métadonnées et notices comprises, doit rester strictement sous 100000000 octets. Aucun recadrage, rééchantillonnage, réencodage, EQ ou normalisation n’a été appliqué. Le nom d’origine mentionne des enregistrements sur 24 bits ; les tableaux Data.IR stockés sont en float64.

Le dossier est data/hrtf/sadie-ii-v2-2. Son installation vaut OFF par défaut et n’intègre pas les données dans la bibliothèque statique :

~~~sh
cmake -S . -B build/release -DCYDSP_INSTALL_HRTF_DATA=ON
cmake --build build/release --config Release
cmake --install build/release --config Release --prefix build/install
# Ou installer uniquement le composant de mesures :
cmake --install build/release --component HrtfData --prefix build/install
~~~

La destination par défaut est share/CyDSP/hrtf/sadie-ii-v2-2. Le paquet installé définit CyDSP_HRTF_DATA_DIR lorsque l’option est activée. L’hôte choisit le fichier, lit ses octets et les passe à loadSofa ; le rendu n’effectue aucune lecture disque implicite et ne choisit aucun profil par défaut. Les trois profils dépassent les 2048 directions de HrtfPreparation par défaut : choisissez realtimeHigh (jusqu’à 16384) ou fixez maximumPoints avant prepareSofaHrtf. Le poids du fichier diffère de la mémoire décodée, des temporaires de préparation et du coût de rendu par voix. realtimeHigh à 48 kHz conserve ici les 256 taps ; EQ ou conversion peuvent les allonger. Le lecteur complet préserve float64 pour l’éditeur/référence ; le compact convertit en float32 pour le rendu courant. Partagez le HrtfSet préparé immuable entre voix et faites des fondus aux changements de direction.

manifest.json fige archive/membre d’origine, taille et SHA-256 de chaque fichier choisi. LICENSE.txt reproduit le texte Apache-2.0 original ; NOTICE.txt conserve attribution University of York et provenance. Joignez-les aux mesures distribuées ; signalez les modifications ultérieures. Le code C++ reste MIT. Ces profils constituent un choix initial, sans garantie d’adaptation individuelle ou de meilleure localisation pour tous les auditeurs.

## Corpus d’entraînement séparé {#training}

Davantage de mesures peuvent être utilisées sans agrandir la sélection distribuée. Le script explicite, fondé sur la bibliothèque standard Python, utilise un catalogue figé des 20 sujets SADIE II (D1, D2 et H3–H20) :

~~~sh
python scripts/fetch_hrtf_training.py --list
python scripts/fetch_hrtf_training.py
# Sélection explicite plus petite, au besoin :
python scripts/fetch_hrtf_training.py --subjects H3 H4 H5 --rate 48000
~~~

Le défaut est 96 kHz, 512 taps par oreille : les 20 SOFA totalisent 498669509 octets (498,67 Mo). Les ZIP d’origine vérifiés totalisent 267332358 octets ; conserver archives et fichiers extraits prend environ 766 Mo, hors petits index/notices. Les sorties restent sous build/hrtf-training/sadie-ii-v2-2 ignoré et sont exclues de l’installation des données. Le script contrôle taille/checksum des archives, extrait seulement le membre original choisi et enregistre le SHA-256 de chaque SOFA avec ses droits.

Le téléchargement n’entraîne aucun modèle. Le pilote CUDA expérimental séparé ajuste un auditeur connu avec des filtres peaking bornés et un ITD basse fréquence. Son environnement isolé PyTorch/NumPy/h5py est optionnel ; ces outils ne sont pas des dépendances du runtime C++. Lancez-le explicitement avec python scripts/train_hrtf_pilot.py --subject D2 --steps 30000 --batch-size 512. Ses contrôles vérifient équations de réponse, gradients, pôles, ITD connu et sérialisation. Poids, sources du pilote, temps et métriques réservées restent sous build/hrtf-runs ignoré ; l’API native expérimentale est décrite dans [HRTF neuronale native](doc:neural-hrtf), avec des poids séparés. Le pilote mesure l’interpolation directionnelle pour un auditeur connu, sans valider phase complète, écoute ou vitesse CPU.

L’évaluation de plusieurs auditeurs doit les séparer, en gardant toutes les directions, fréquences et copies augmentées d’une personne dans la même partition. Le catalogue propose H3–H16 pour l’entraînement, H17–H18 pour la validation, H19–H20 pour le test réservé ; D1/D2 sont des références mannequins. C’est une préparation du découpage, pas un résultat de modèle mesuré. Un renderer neuronal plus rapide demanderait encore des comparaisons CPU, spectre/ITD et écoute face au rendu HRTF existant.

## Comparer les champs neuronaux expérimentaux {#neural-quality}

L’outil de comparaison optionnel enregistre cas et critère de sélection avant l’entraînement. Il compare centres logarithmiques/hybrides, capacité du prédicteur et 12/24 filtres peaking par oreille. La grille hybride place davantage de filtres dans les aigus ; elle ne prétend pas être optimale perceptuellement. L’objectif mixte équilibre les fréquences échantillonnées logarithmiquement et uniformément.

~~~sh
python scripts/compare_hrtf_quality.py --subject D2 --screen-steps 8000 --refine-steps 20000
~~~

Présélection et affinage utilisent seulement la validation. Un modèle par budget de filtres est retenu et les deux hashes des poids sont figés avant l’évaluation du test réservé. Pour les essais manuels, --validation-only diffère le test ; --evaluate-run évalue explicitement une passe locale figée. --initial-weights affine des poids compatibles avec optimiseur/état aléatoire réinitialisés, conserve la normalisation d’origine et garde le modèle initial s’il reste meilleur.

Les expériences restent sous build/hrtf-quality et build/hrtf-runs ignorés. L’outil de diagnostic ajuste des coefficients libres sur des exemples d’entraînement pour examiner les limites de représentation ; ces ajustements ne sont ni un champ déployable ni un optimum prouvé. check_hrtf_field.py vérifie hashes sources/données, pôles prédits, oracle de réponse complexe float64, accord CPU/GPU et export des poids sans exécuter de source archivée.

La sortie de retard du réseau prédit l’ITD total souhaité. Les filtres ajoutent leur propre phase interaurale : l’outil expérimental compensated_delays_us retire cet ITD de pente de phase avant d’affecter des retards externes positifs aux oreilles. Utiliser ces retards résiduels au lieu d’ajouter aussi la sortie brute du réseau. Cet outil d’analyse/préparation peut allouer ; ce n’est pas une API de callback audio. Il aligne une pente basse fréquence, pas la phase HRIR complète. Rétablir magnitudeOffsetDb pour le niveau absolu. Les modèles actuels visent 96 kHz ; un autre taux exige une conception adaptée et validée.

## Entraîner sur Colab / L4 et A100 avec Drive {#colab}

Le notebook optionnel notebooks/CyDSP_HRTF_A100.ipynb accepte un GPU CUDA, dont L4 ou A100 40/80 Go, et Google Drive. Déposer son CyDSP-HRTF-Colab.zip associé, produit sous build/colab, dans MyDrive/CyDSP-HRTF, choisir un runtime L4 ou A100 et exécuter les cellules. Le nom historique du fichier est conservé ; REQUIRE_A100=False accepte la L4 sans changer la recette ni le paquet source déjà déposé. Aucun jeton GitHub privé n’est requis. PyTorch 2.9.1/CUDA 12.8 et NumPy/h5py viennent des index officiels ; les SOFA SADIE II épinglés viennent de Zenodo. Seuls les outils Python optionnels utilisent ces dépendances.

Le défaut affine le champ D2 hybride-24 retenu, largeur 96 et quatre bandes directionnelles, pour 100000 mises à jour supplémentaires. Ses poids de référence ont déjà été testés : les mesures finales suivent ce même test, sans constituer un nouveau test indépendant. La mesure des batches concerne seulement le calcul d’entraînement. La quantité de mémoire GPU ne garantit pas d’accélération ; float32 avec TF32/AMP désactivés est conservé. Chaque modèle vise un auditeur connu à 96 kHz.

Le notebook fige le plan d’expérience, travaille sur stockage local et envoie des checkpoints immuables vérifiés vers Drive environ toutes les 60 secondes et en sortie. resume.pt restaure Adam, états aléatoires, poids, meilleur modèle et calendrier initial ; --initial-weights lance au contraire un nouvel optimiseur. La reprise garde le même paquet source et la même version PyTorch. Changer de GPU peut changer les arrondis. Une coupure brutale peut perdre le travail depuis la dernière sauvegarde réussie. Les sauvegardes précédentes restent disponibles. La disponibilité du GPU et la durée de session dépendent de Colab.

Si l’autorisation Drive échoue, notebooks/CyDSP_HRTF_L4_Direct.ipynb reçoit directement le ZIP source puis télécharge un ZIP de résultats vérifié avec poids et checkpoints de reprise. Il ne monte pas Drive. Conserver le fichier téléchargé avant de libérer la machine Colab ; les checkpoints non exportés restent temporaires. Reconstruire avec python scripts/make_hrtf_direct_colab.py.

Le profil séparé notebooks/CyDSP_HRTF_L4_Quality.ipynb affine le champ D2 48 kHz validé sur 140000 étapes supplémentaires, avec un taux réduit (0,0003 à 0,00001). Le rythme L4 antérieur donne environ 30 minutes de calcul estimées, hors installation/validation/Drive. Reconstruire son notebook et son ZIP distincts avec python scripts/make_hrtf_colab.py --profile l4-quality. Il conserve l’expérience précédente et les poids d’intégration actuels ; la fidélité améliorée reste à mesurer puis écouter.

python scripts/make_hrtf_colab.py reconstruit le couple notebook/paquet source. Sans poids expérimentaux de référence locaux, ajouter --without-reference-weights. Le petit paquet contient scripts/notices et références figées optionnelles, sans SOFA bruts ni binaires CUDA. Le paquet d’entraînement n’ajoute aucune dépendance au runtime C++ ; l’API native séparée reste expérimentale.

Ce sont des budgets de filtres expérimentaux, pas des modes de qualité publics du renderer C++ ni des ratios de vitesse audio CPU mesurés. Présenter les erreurs spectrales sur grilles logarithmique et linéaire, les bandes graves/aiguës et l’ITD. Une meilleure moyenne ne garantit pas une amélioration à chaque fréquence. Le renderer SOFA mesuré reste disponible ; cette procédure ne revendique pas de qualité d’écoute, de phase complète ou d’auditeur inconnu.

## API publique {#api}

- [cydsp/spatial/Sofa.h](doc:api-spatial-sofa)
