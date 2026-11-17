# Colab L4 sans connexion Google Drive

Si le montage Drive échoue, utiliser [CyDSP_HRTF_L4_Direct.ipynb](CyDSP_HRTF_L4_Direct.ipynb).

1. Importer ce notebook dans Colab et sélectionner **GPU → L4**.
2. Lancer **Tout exécuter**. Quand le sélecteur apparaît, choisir sur ton PC le
   fichier **build/colab/CyDSP-HRTF-L4-Direct.zip**, sans le décompresser.
3. À la fin, conserver sur ton PC le fichier **CyDSP-HRTF-L4-Results.zip**
   téléchargé par le notebook. Ce ZIP contient poids, reprise et rapports.

Ce mode n'appelle ni le montage Drive ni l'authentification Drive. Il effectue le
même affinage D2 48 kHz / 24 filtres / 140000 étapes, environ 30 minutes de calcul
L4 estimées. Les dépendances et les SOFA sont téléchargés sur le stockage Colab.
Les checkpoints restent dans cette session : télécharger le ZIP avant de la
fermer/supprimer. La dernière cellule peut récupérer un checkpoint partiel après
interruption. Pour reprendre sur une nouvelle machine Colab depuis un ZIP conservé,
mettre RESTORE_RESULT=True et le fournir à la cellule de reprise. Dans la même
machine, les checkpoints locaux sont réutilisés.

Reconstruction : **python scripts/make_hrtf_direct_colab.py**.

---

# Affiner la qualité pendant l'intégration : Colab L4

Utiliser le nouveau [CyDSP_HRTF_L4_Quality.ipynb](CyDSP_HRTF_L4_Quality.ipynb).

1. Importer ce fichier .ipynb dans Google Colab.
2. Copier le fichier **CyDSP-HRTF-L4-Quality.zip**, disponible localement sous
   **build/colab**, dans le dossier **Mon Drive → CyDSP-HRTF**, sans le décompresser.
3. Dans Colab, choisir **GPU → L4**, puis **Exécution → Tout exécuter**.
   Autoriser le montage de Drive lorsque Google le demande.

Cette nouvelle expérience repart du meilleur modèle **48 kHz / D2 / 24 filtres**,
compatible avec le rendu natif CyDSP. Elle conserve l'architecture, batch 512,
et utilise un taux d'apprentissage plus faible : **0,0003 → 0,00001**.
**140 000 mises à jour** correspondent à environ **29,4 minutes de calcul**
si les 12,6 ms/étape observés sur la L4 précédente se confirment. Installation,
validation et sauvegardes s'ajoutent ; l'amélioration n'est pas garantie.
Le meilleur modèle initial reste éligible si aucun nouveau checkpoint ne le dépasse.

Les sorties vont dans **Mon Drive → CyDSP-HRTF → experiments →
l4-hrtf-48k-quality-001**. Les poids actuellement utilisés par l'intégration
ne sont pas remplacés automatiquement. Après une coupure, reprendre avec le
même notebook, le même ZIP, le même nom et les mêmes réglages.
L'ancienne expérience a100-hrtf-001 et son couple notebook/ZIP restent distincts.

Reconstruction : **python scripts/make_hrtf_colab.py --profile l4-quality**.
Les poids 48 kHz locaux sont nécessaires pour inclure cette référence ; une copie
sans poids peut générer un départ de zéro avec --without-reference-weights.

---

# Entraînement HRTF sur Google Colab

1. Ouvrir [CyDSP_HRTF_A100.ipynb](CyDSP_HRTF_A100.ipynb) dans Google Colab
   avec **Fichier → Importer un notebook**.
2. Déposer **build/colab/CyDSP-HRTF-Colab.zip** dans le dossier
   **CyDSP-HRTF** de Google Drive, sans le décompresser.
3. Choisir un runtime GPU **L4** ou **A100**, puis exécuter les cellules dans l’ordre.

Le notebook et le ZIP doivent provenir de la même génération : leur SHA-256
est vérifié avant extraction. Le ZIP fourni localement fait moins de 1 Mo ;
il contient scripts et références expérimentales, sans les SOFA ni CUDA.
Les dépendances et données sont téléchargées automatiquement.

Par défaut : D2/KEMAR, 24 filtres par oreille, batch 512, affinage de référence
sur 100000 mises à jour. La L4 et les A100 utilisent la même recette float32.
Le test de batches mesure le débit réel de la machine allouée. La disponibilité
du GPU et les limites de session sont gérées par Colab.

La reprise conserve poids, optimiseur Adam, états aléatoires et calendrier.
Garder le même nom d’expérience et le même plan pour reprendre depuis Drive.
Les sauvegardes antérieures restent disponibles. Une coupure brutale peut
perdre le travail depuis la dernière sauvegarde réussie.

Les résultats restent expérimentaux : un auditeur par modèle, amplitude et
ITD basse fréquence à 96 kHz. Les références D2 ont déjà été testées ; leur
évaluation finale ne constitue pas un nouveau test indépendant. Aucun
renderer neuronal C++ n’est ajouté par ce notebook.

Sources de reconstruction et détails :
[guide d’entraînement](../scripts/README.hrtf-training.md).
Sur une copie sans les poids de référence locaux, reconstruire avec
**python scripts/make_hrtf_colab.py --without-reference-weights**.
Cela produit un notebook configuré pour partir de zéro.

Code original MIT ; données SADIE II Apache-2.0 avec leurs notices.

Le nom A100 du fichier est conservé ; il accepte désormais la L4 par défaut.
Dans un notebook déjà importé, mettre REQUIRE_A100 = False puis relancer les
cellules de configuration et d’installation. Le ZIP déjà déposé sur Drive
reste inchangé et utilisable, sans nouvel envoi.
