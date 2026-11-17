# Sélection HRTF SADIE II optionnelle de CyDSP

Trois SOFA originaux de SADIE II version 2-2 : KU100 (D1), KEMAR (D2) et un
auditeur humain (H10). Ils sont tous à 48 kHz, en float64, avec 256 échantillons
par oreille. D1/D2 contiennent chacun 8802 directions ; H10 en contient 2114.
Les fichiers de signaux totalisent 81 375 249 octets (81,38 Mo décimaux).
Le dossier entier reste sous 100 000 000 octets, manifeste et notices compris.

Ces mesures sont sous Apache-2.0, séparément du code original MIT de CyDSP.
Conservez LICENSE.txt, NOTICE.txt et la provenance avec les données redistribuées ;
signalez les traitements/modifications ultérieurs. Rien n'est incorporé à
l'archive C++. Ce sont trois profils d'auditeur au choix, pas trois parties
physiques à combiner. Préférence et localisation s'évaluent à l'écoute.

L'installation du dossier s'active explicitement avec CYDSP_INSTALL_HRTF_DATA=ON.
La destination est share/CyDSP/hrtf/sadie-ii-v2-2 (ou le dossier de données
GNUInstallDirs configuré). Une installation limitée aux données utilise
--component HrtfData. Le paquet CMake installé expose alors CyDSP_HRTF_DATA_DIR.

L'hôte lit le SOFA choisi en mémoire, appelle la surcharge loadSofa pour
SofaHrtf ou SofaHrtf64, puis prepareSofaHrtf et HrirBinaural. SofaLoadOptions
sélectionne le lecteur : le lecteur compact stocke en float32, le lecteur complet
préserve float64. Pour ces trois profils, augmentez maximumPoints au-delà de la
valeur par défaut 2048 (le profil realtimeHigh accepte 16384 points).
À 48 kHz les 256 échantillons fournis sont conservés ; les autres traitements
peuvent changer longueur/latence. Lecture et préparation se font hors callback.

Vérification du dossier :
~~~sh
python scripts/verify_hrtf_data.py
~~~
Pour une copie installée, passez --directory avec le chemin absolu du dossier.

Le test natif d'intégration est optionnel : CYDSP_BUILD_TESTS=ON,
CYDSP_BUILD_SOFA=ON et CYDSP_TEST_HRTF_DATA=ON.
Le corpus d'entraînement plus large se récupère explicitement :
~~~sh
python scripts/fetch_hrtf_training.py
~~~
Par défaut il sélectionne les SOFA originaux à 96 kHz/512 échantillons des
20 sujets. Archives et fichiers extraits restent dans build/hrtf-training,
ignoré par Git et séparé de cette sélection et de l'installation.
Cette commande n'entraîne aucun modèle.

Source : https://zenodo.org/records/12092466
Manuel : https://mrmybal.github.io/CyDSP/Documentation.html#/fr/sofa/bundled
