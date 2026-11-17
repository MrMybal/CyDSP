# Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
"""Generate the L4 quality notebook with direct upload/download, no Drive mount."""
import argparse
import ast
import json
import shutil
from colab_hrtf import ROOT, atomic_bytes, make_bundle
from make_hrtf_colab import quality_cells

INTRO = r"""
# CyDSP — affinage L4 avec transfert direct des fichiers

Ce notebook fonctionne **sans connexion ni montage Google Drive**.

1. Importer ce fichier **CyDSP_HRTF_L4_Direct.ipynb** dans Colab.
2. Choisir **GPU → L4**, puis **Exécution → Tout exécuter**.
3. Quand le sélecteur de fichiers apparaît, choisir sur ton PC le fichier
   **CyDSP-HRTF-L4-Direct.zip**, sans le décompresser.
4. À la fin, récupérer le fichier **CyDSP-HRTF-L4-Results.zip** téléchargé par
   le notebook. Il contient les meilleurs poids, la reprise et les rapports.

Le modèle repart du meilleur **D2/KEMAR 48 kHz / 24 filtres** compatible avec
CyDSP. Même architecture, batch 512, 140 000 étapes supplémentaires et taux
Adam/cosine réduit de **0,0003 à 0,00001**. Environ **30 minutes de calcul estimées**
sur la L4, hors installation, téléchargements, validation et export.
Le meilleur modèle initial reste candidat si la validation ne s'améliore pas.

Les checkpoints sont enregistrés **dans la session Colab**. Télécharger le ZIP
avant de fermer/supprimer cette session. La dernière cellule peut aussi être
exécutée manuellement pour récupérer un checkpoint partiel après interruption.
Une perte de la machine Colab efface les fichiers qui n'ont pas été téléchargés.

Après une coupure, la même machine peut reprendre ses checkpoints locaux.
Sur une nouvelle machine, mettre RESTORE_RESULT=True puis fournir un ZIP de
résultats déjà téléchargé quand la cellule de reprise le demande.

Code original **MIT**, données SADIE II **Apache-2.0**, notices incluses.
Modèle d'un auditeur connu : amplitude et ITD basse fréquence, sans phase HRIR
complète. Le test directionnel D2 a déjà été évalué ; aucune nouvelle indépendance
statistique ni amélioration de qualité ne sont présumées.
"""
UPLOAD = r'''
from google.colab import files

if not BUNDLE_PATH.is_file():
    print("Choisir sur ton PC : CyDSP-HRTF-L4-Direct.zip (sans le décompresser).")
    uploaded = files.upload()
    if len(uploaded) != 1:
        raise ValueError("Choisir uniquement le fichier ZIP fourni avec ce notebook.")
    blob = next(iter(uploaded.values()))
    if len(blob) > 64 * 1024 * 1024 or hashlib.sha256(blob).hexdigest() != EXPECTED_BUNDLE_SHA256:
        raise ValueError("Mauvais ZIP : utiliser CyDSP-HRTF-L4-Direct.zip fourni avec ce notebook.")
    BUNDLE_PATH.parent.mkdir(parents=True, exist_ok=True)
    BUNDLE_PATH.write_bytes(blob)
print("Paquet présent dans la session Colab :", BUNDLE_PATH)
'''
RESTORE = r'''
if RESTORE_RESULT:
    from google.colab import files
    print("Choisir un CyDSP-HRTF-L4-Results.zip déjà téléchargé pour cette expérience.")
    uploaded = files.upload()
    if len(uploaded) != 1:
        raise ValueError("Choisir un seul ZIP de résultats.")
    value = next(iter(uploaded.values()))
    if len(value) > 64 * 1024 * 1024:
        raise ValueError("Résultats trop volumineux.")
    archive = RESULTS_ROOT / "imported-results.zip"
    atomic_bytes(archive, value)
    restore_direct_result(archive, RESULTS_ROOT / "experiments" / EXPERIMENT)
    print("Checkpoint vérifié et restauré. Les cellules suivantes reprennent le calcul.")
else:
    print("Nouvelle passe ou reprise des checkpoints déjà présents dans cette session.")
'''
DOWNLOAD = r'''
from google.colab import files

persistent = RESULTS_ROOT / "experiments" / EXPERIMENT
# Refresh local complete checkpoints; safe to run after a training interruption.
for pointer in sorted((persistent / "runs").glob("*/latest.json")):
    run = ROOT / "build/hrtf-runs" / pointer.parent.name
    snapshot_run(run, pointer.parent, force=True)
output = RESULTS_ROOT / "CyDSP-HRTF-L4-Results.zip"
export_direct_result(persistent, output)
print("Résultats prêts :", output, "—", output.stat().st_size, "octets")
print("Conserver ce fichier sur ton PC avant de fermer/supprimer la session Colab.")
files.download(str(output))
'''


def cells():
    output = []
    for kind, name, original in quality_cells():
        source = original.replace('CyDSP-HRTF-L4-Quality.zip', 'CyDSP-HRTF-L4-Direct.zip')
        source = source.replace('l4-hrtf-48k-quality-001', 'l4-hrtf-48k-direct-001')
        source = source.replace('DRIVE_ROOT', 'RESULTS_ROOT')
        source = source.replace('CACHE_ARCHIVES_ON_DRIVE', 'CACHE_ARCHIVES_LOCAL')
        source = source.replace('Dossier Drive :', 'Dossier des résultats Colab :')
        if name == 'introduction':
            source = INTRO
        elif name == 'configuration':
            source = source.replace('Path("/content/drive/MyDrive/CyDSP-HRTF")',
                                    'Path("/content/CyDSP-HRTF-results")')
            source = source.replace('BUNDLE_PATH = RESULTS_ROOT / "CyDSP-HRTF-L4-Direct.zip"',
                                    'BUNDLE_PATH = Path("/content/CyDSP-HRTF-L4-Direct.zip")')
            source = source.replace('CACHE_ARCHIVES_LOCAL = True', 'CACHE_ARCHIVES_LOCAL = False')
            source += '\nRESTORE_RESULT = False  # True : fournir un ZIP de résultats téléchargé avant une coupure.\n'
        elif name == 'drive-help':
            name = 'upload-help'
            source = '## 1. Envoyer le fichier ZIP directement dans Colab\n\nChoisir le fichier ZIP sur ton PC. Cette étape ne demande aucune autorisation Drive.'
        elif name == 'mount-drive':
            name, source = 'upload-input', UPLOAD
        elif name == 'unpack-source':
            source += r'''
from colab_hrtf import (
    run_with_backup as _run_with_backup, snapshot_run as _snapshot_run,
    export_direct_result, restore_direct_result,
)
def run_with_backup(command, root, run, destination, seconds=60):
    return _run_with_backup(command, root, run, destination, seconds, label="Colab local")
def snapshot_run(run, destination, force=False):
    return _snapshot_run(run, destination, force, label="Colab local")
'''
        elif name == 'dataset-help':
            source = '## 4. Télécharger les mesures\n\nLes mesures SADIE II D2 48 kHz originales sont téléchargées depuis Zenodo dans la session Colab. Tailles, hashes et CRC vérifiés. Aucun accès Google Drive.'
        elif name == 'training-help':
            source = '## 6. Affiner / reprendre\n\nLes meilleurs poids initiaux 48 kHz restent éligibles. Même réseau, 140000 étapes, batch 512 et taux réduit. Sélection sur la validation. Poids, Adam et états aléatoires sont enregistrés localement dans Colab avec des snapshots périodiques. Après une interruption sur la même machine, relancer cette cellule. Télécharger un ZIP depuis la dernière cellule avant de libérer la machine.'
        elif name == 'train-or-resume':
            source = source.replace('"precision": "float32;',
                                    '"storage": "colab-local-download", "precision": "float32;')
            source = source.replace('Le plan Drive diffère.', 'Le plan de cette expérience diffère.')
        elif name == 'results-help':
            source = '## 8. Lire les courbes\n\nLes métriques sont en dB RMS et µs RMS ; plus bas est préférable. La cellule suivante télécharge les résultats complets. Les checkpoints restent locaux tant que le ZIP n’a pas été téléchargé.'
        output.append((kind, name, source))
        if name == 'unpack-source':
            output += [('markdown', 'restore-help', '## Reprendre un ZIP déjà téléchargé (optionnel)\n\nLaisser RESTORE_RESULT=False pour la première passe.'),
                       ('code', 'restore-results', RESTORE)]
    output += [('markdown', 'download-help', '## 9. Télécharger les résultats\n\nCette cellule se lance à la fin. Elle peut aussi être exécutée après une interruption pour récupérer le dernier checkpoint complet. Le ZIP est à conserver sur ton PC ; il pourra servir à la reprise et à la comparaison du nouveau modèle.'),
               ('code', 'download-results', DOWNLOAD)]
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--without-reference-weights', action='store_true')
    args = parser.parse_args()
    bundle, checksum = make_bundle(not args.without_reference_weights, 'l4-direct')
    prepared = []
    for kind, name, source in cells():
        source = source.strip().replace('__BUNDLE_SHA256__', checksum) + '\n'
        if args.without_reference_weights:
            if name == 'configuration':
                source = source.replace('WARM_START_D2 = True', 'WARM_START_D2 = False')
            elif name == 'introduction':
                source += '\n**Paquet sans poids de référence : départ de zéro, WARM_START_D2=False.**\n'
        if kind == 'code':
            ast.parse(source, filename=name)
        cell = {'cell_type':kind,'id':name,'metadata':{'id':name},'source':source.splitlines(keepends=True)}
        if kind == 'code':cell.update(execution_count=None,outputs=[])
        prepared.append(cell)
    notebook = {'nbformat':4,'nbformat_minor':5,'metadata':{
        'colab':{'name':'CyDSP_HRTF_L4_Direct.ipynb','provenance':[]},'accelerator':'GPU',
        'kernelspec':{'display_name':'Python 3','language':'python','name':'python3'},
        'language_info':{'name':'python'},
        'cydsp':{'sourceBundleSha256':checksum,'codeLicense':'MIT','dataLicense':'Apache-2.0',
                 'storage':'direct-upload-download'}},'cells':prepared}
    path = ROOT/'notebooks/CyDSP_HRTF_L4_Direct.ipynb'
    atomic_bytes(path,(json.dumps(notebook,ensure_ascii=False,indent=1)+'\n').encode('utf-8'))
    shutil.copyfile(path,bundle.parent/path.name)
    print(json.dumps({'notebook':str(path),'bundle':str(bundle),'bundleBytes':bundle.stat().st_size,
                      'sha256':checksum,'cells':len(prepared)},ensure_ascii=False))


if __name__=='__main__':
    main()
