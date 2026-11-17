# Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
"""Build a Colab notebook and its matching private-source upload bundle."""
import argparse
import ast
import json
import shutil
from colab_hrtf import ROOT, make_bundle, atomic_bytes

CELLS = [
("markdown", "introduction", r"""
# CyDSP — entraînement HRTF sur Colab / L4 ou A100

Ce notebook entraîne le prototype HRTF CyDSP en **float32** avec une NVIDIA L4
ou une A100 40/80 Go. Les dépendances CUDA viennent de PyTorch et les SOFA SADIE II de
Zenodo. Le calcul utilise le disque local ; Drive conserve les sauvegardes.

1. Sur Drive, créer **CyDSP-HRTF** et y déposer **CyDSP-HRTF-Colab.zip** fourni
   avec ce notebook. Il contient nos scripts, licences et poids de référence.
2. Dans Colab : **Exécution → Modifier le type d'exécution → GPU → L4** (ou A100),
   puis exécuter les cellules dans l'ordre.
3. Autoriser le montage de Drive. Aucun jeton GitHub n'est nécessaire.

Par défaut : KEMAR/D2, 24 filtres par oreille, affinage des meilleurs poids RTX
3090, 100 000 mises à jour supplémentaires, batch 512. Les poids de référence
ont déjà été évalués : l'évaluation finale suit le même test réservé, elle ne
constitue pas un nouveau test indépendant.

La reprise restaure poids, Adam, état aléatoire, meilleur modèle et calendrier
du taux d'apprentissage. Pour reprendre après une coupure, garder ce notebook,
son paquet et les mêmes paramètres. Les mises à jour postérieures à la dernière
sauvegarde Drive réussie peuvent être perdues.

**Code MIT ; données SADIE II Apache-2.0**, notices conservées. Un modèle par
auditeur connu, amplitude + ITD basse fréquence. Prototype expérimental, pas
encore un renderer C++, ni une validation d'écoute ou de vitesse CPU.
"""),
("code", "configuration", r'''
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

EXPERIMENT = "a100-hrtf-001"       # Nouveau nom pour un plan différent.
SUBJECTS = ["D2"]                 # D1, D2 ou H3..H20 ; un modèle par sujet.
FILTER_BUDGETS = [24]             # [12], [24] ou [12, 24].
STEPS = 100_000                   # Calendrier complet, inchangé à la reprise.
BATCH_SIZE = 512                  # Mesurer avant de modifier ce réglage.
VALIDATION_EVERY = 1_000
WARM_START_D2 = True              # False : repartir de zéro.
RUN_BATCH_BENCHMARK = True
RUN_FINAL_EVALUATION = True
DOWNLOAD_ALL_SUBJECTS = False     # Télécharger 20 sujets ne crée pas un modèle multi-auditeur.
CACHE_ARCHIVES_ON_DRIVE = True
SAVE_EVERY_SECONDS = 60
REQUIRE_A100 = False              # L4 acceptée ; True exige spécifiquement une A100.
DRIVE_ROOT = Path("/content/drive/MyDrive/CyDSP-HRTF")
BUNDLE_PATH = DRIVE_ROOT / "CyDSP-HRTF-Colab.zip"
EXPECTED_BUNDLE_SHA256 = "__BUNDLE_SHA256__"

if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_-]{0,63}", EXPERIMENT):
    raise ValueError("Nom d'expérience invalide")
if not SUBJECTS or len(set(SUBJECTS)) != len(SUBJECTS):
    raise ValueError("Choisir des sujets distincts")
if not FILTER_BUDGETS or len(set(FILTER_BUDGETS)) != len(FILTER_BUDGETS) or set(FILTER_BUDGETS) - {12, 24}:
    raise ValueError("Budgets disponibles : 12 et 24")
if not 100 <= STEPS <= 1_000_000 or not 1 <= BATCH_SIZE <= 4096:
    raise ValueError("STEPS : 100..1000000 ; BATCH_SIZE : 1..4096")
if VALIDATION_EVERY < 1 or SAVE_EVERY_SECONDS < 10:
    raise ValueError("Intervalles de sauvegarde invalides")
if not (3, 11) <= sys.version_info[:2] <= (3, 13):
    raise RuntimeError("Environnement prévu pour Python 3.11 à 3.13")
print("Plan :", SUBJECTS, FILTER_BUDGETS, STEPS, "étapes par modèle")
'''),
("markdown", "drive-help", """
## 1. Connecter Drive
Seul le dossier CyDSP-HRTF est utilisé. Si le ZIP porte un autre nom, ajuster
BUNDLE_PATH. L'autorisation Google est gérée par le montage standard Colab.
"""),
("code", "mount-drive", r'''
from google.colab import drive
drive.mount("/content/drive")
if not BUNDLE_PATH.is_file():
    raise FileNotFoundError(f"Déposer CyDSP-HRTF-Colab.zip dans {DRIVE_ROOT}.")
print("Paquet trouvé :", BUNDLE_PATH)
'''),
("markdown", "source-help", """
## 2. Vérifier et préparer les sources
Le SHA-256 associe ce notebook à son paquet. Chemins, tailles et empreintes
sont vérifiés avant extraction. Aucun clonage privé ou transfert du projet
complet n'est nécessaire.
"""),
("code", "unpack-source", r'''
import io
import stat
import zipfile
from pathlib import PurePosixPath

blob = BUNDLE_PATH.read_bytes()
if len(blob) > 64 * 1024 * 1024 or hashlib.sha256(blob).hexdigest() != EXPECTED_BUNDLE_SHA256:
    raise ValueError("Le paquet ne correspond pas à ce notebook (SHA-256)")
ROOT = Path("/content") / ("CyDSP-HRTF-" + EXPECTED_BUNDLE_SHA256[:12])
payload = {}
with zipfile.ZipFile(io.BytesIO(blob)) as archive:
    entries = archive.infolist()
    names = [e.filename for e in entries]
    if len(names) > 121 or len(set(names)) != len(names) or "manifest.json" not in names:
        raise ValueError("Membres du paquet invalides")
    if sum(e.file_size for e in entries) > 64 * 1024 * 1024:
        raise ValueError("Paquet décompressé trop grand")
    for entry in entries:
        name = entry.filename
        path = PurePosixPath(name)
        if (entry.is_dir() or entry.file_size > 16 * 1024 * 1024
                or stat.S_ISLNK(entry.external_attr >> 16)
                or "\\" in name or ":" in name or path.is_absolute()
                or any(p in ("", ".", "..") for p in name.split("/"))
                or path.as_posix() != name):
            raise ValueError("Chemin ou taille de membre invalide")
    manifest = json.loads(archive.read("manifest.json"))
    if manifest.get("schema") != 1 or manifest.get("kind") != "cydsp-hrtf-colab-source":
        raise ValueError("Type de paquet incorrect")
    if set(names) != set(manifest["files"]) | {"manifest.json"}:
        raise ValueError("Contenu différent du manifeste")
    for name, record in manifest["files"].items():
        value = archive.read(name)
        if len(value) != record["bytes"] or hashlib.sha256(value).hexdigest() != record["sha256"]:
            raise ValueError("Empreinte incorrecte : " + name)
        target = ROOT.joinpath(*PurePosixPath(name).parts)
        if not target.resolve().is_relative_to(ROOT.resolve()):
            raise ValueError("Chemin en dehors du projet")
        if target.exists() and target.read_bytes() != value:
            raise ValueError("Source locale modifiée : " + name)
        payload[name] = value
for name, value in payload.items():
    target = ROOT.joinpath(*PurePosixPath(name).parts)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(value)
sys.path.insert(0, str(ROOT / "scripts"))
from colab_hrtf import (
    atomic_bytes, atomic_json, digest, read_json, restore_run,
    run_with_backup, snapshot_run,
)
REFERENCES = read_json(ROOT / "colab-references.json")
catalog = read_json(ROOT / "data/hrtf/training-sources.json")
if set(SUBJECTS) - {s["subject"] for s in catalog["subjects"]}:
    raise ValueError("Sujet absent du catalogue SADIE II")
print("Sources prêtes :", ROOT)
'''),
("markdown", "environment-help", """
## 3. Installer l'environnement et vérifier le GPU
Python isolé : PyTorch **2.9.1 / CUDA 12.8**, NumPy **2.3.5**, h5py **3.15.1**.
Les bibliothèques CUDA sont téléchargées depuis l'index officiel PyTorch.
Le pilote est fourni par Colab : aucune installation de pilote système.
Installation initiale : plusieurs Go, sans copie dans Drive.

La L4 et les A100 utilisent la même recette float32. TF32 et AMP restent
désactivés pour conserver la précision de référence. Le GPU réel, sa mémoire
et le pilote sont affichés. REQUIRE_A100=False accepte la L4 et les autres GPU CUDA.
"""),
("code", "install-environment", r'''
subprocess.run(["nvidia-smi"], check=True)
VENV = ROOT / "build/colab-venv"
PYTHON = VENV / "bin/python"
if not PYTHON.is_file():
    subprocess.run([sys.executable, "-m", "venv", "--without-pip", str(VENV)], check=True)
# pip officiel --python gère aussi un environnement créé sans pip.
pip_command = [sys.executable, "-m", "pip", "--python", str(PYTHON)]
subprocess.run([*pip_command, "install", "--index-url", "https://pypi.org/simple",
                "pip==25.3", "-r", str(ROOT / "scripts/requirements-hrtf-training.txt")], check=True)
subprocess.run([*pip_command, "install", "torch==2.9.1",
                "--index-url", "https://download.pytorch.org/whl/cu128"], check=True)
probe = r"""
import json, torch
if not torch.cuda.is_available():
    raise RuntimeError("CUDA inaccessible : vérifier le runtime GPU Colab et son pilote")
p = torch.cuda.get_device_properties(0)
x = torch.ones(8, device='cuda')
assert float((x*x).sum()) == 8
torch.cuda.synchronize()
print(json.dumps({'gpu': p.name, 'memoryGiB': p.total_memory / 2**30,
                  'torch': str(torch.__version__), 'cuda': torch.version.cuda,
                  'capability': list(torch.cuda.get_device_capability(0))}))
"""
GPU = json.loads(subprocess.check_output([str(PYTHON), "-c", probe], text=True))
print(json.dumps(GPU, indent=2))
if REQUIRE_A100 and "A100" not in GPU["gpu"].upper():
    raise RuntimeError("Session sans A100. Choisir une A100 ou autoriser un autre GPU.")
subprocess.run([str(PYTHON), str(ROOT / "scripts/train_hrtf_pilot.py"), "--self-test"], cwd=ROOT, check=True)
'''),
("markdown", "dataset-help", """
## 4. Télécharger les mesures d'entraînement
SADIE II v2-2 officiel : tailles et SHA-256/MD5 d'archives contrôlés, extraction
avec CRC ZIP et SHA-256 SOFA enregistré. IR inchangées : 96 kHz / 512 taps par
oreille. D2 : environ **38 Mo** de ZIP + **72,5 Mo** de SOFA.
Les 20 sujets : environ **267 Mo** de ZIP + **499 Mo** de SOFA.

Le cache Drive facultatif conserve seulement les ZIP ; les calculs utilisent
les copies locales. Télécharger les 20 sujets ne transforme pas le réseau en
modèle personnalisé multi-auditeur.
"""),
("code", "download-data", r'''
selected_sources = [s for s in catalog["subjects"]
                    if DOWNLOAD_ALL_SUBJECTS or s["subject"] in SUBJECTS]
CORPUS = ROOT / "build/hrtf-training/sadie-ii-v2-2"
archive_cache = DRIVE_ROOT / "cache/sadie-ii-v2-2"
for item in selected_sources:
    name = item["subject"] + "_HRIR_SOFA.zip"
    local, cached = CORPUS / "archives" / name, archive_cache / name
    if CACHE_ARCHIVES_ON_DRIVE and not local.exists() and cached.is_file():
        atomic_bytes(local, cached.read_bytes())
command = [str(PYTHON), str(ROOT / "scripts/fetch_hrtf_training.py"), "--rate", "96000"]
if not DOWNLOAD_ALL_SUBJECTS:
    command += ["--subjects", *SUBJECTS]
subprocess.run(command, cwd=ROOT, check=True)
if CACHE_ARCHIVES_ON_DRIVE:
    for item in selected_sources:
        name = item["subject"] + "_HRIR_SOFA.zip"
        local, cached = CORPUS / "archives" / name, archive_cache / name
        value = local.read_bytes()
        if len(value) != item["archiveBytes"] or digest(value) != item["archiveSha256"]:
            raise ValueError("Archive locale invalide")
        if not cached.exists() or digest(cached.read_bytes()) != item["archiveSha256"]:
            atomic_bytes(cached, value)
            if digest(cached.read_bytes()) != item["archiveSha256"]:
                raise OSError("Échec de vérification du cache Drive")
print("Corpus local prêt pour", SUBJECTS)
'''),
("markdown", "benchmark-help", """
## 5. Mesurer les batches sur ce GPU
Essai court sur les seules directions d'entraînement : **millisecondes par
mise à jour**, **exemples par seconde** et mémoire GPU. Il ne modifie pas
automatiquement BATCH_SIZE : un autre batch modifie aussi la convergence.
La projection pour 100 000 étapes est du calcul seul, hors installation,
données, validation et sauvegardes.

Le petit modèle peut rester limité par les lancements de kernels. Ces mesures
permettent d'évaluer le gain réel sans le déduire de la quantité de VRAM.
"""),
("code", "measure-batches", r'''
if RUN_BATCH_BENCHMARK:
    benchmark_path = ROOT / "build/colab/batch-benchmark.json"
    subprocess.run([str(PYTHON), str(ROOT / "scripts/benchmark_hrtf_training.py"),
                    "--subject", SUBJECTS[0], "--filters", str(max(FILTER_BUDGETS)),
                    "--iterations", "80", "--output", str(benchmark_path)], cwd=ROOT, check=True)
    benchmark = read_json(benchmark_path)
    atomic_json(DRIVE_ROOT / "benchmarks" / (EXPERIMENT + ".json"), benchmark)
    for row in benchmark["rows"]:
        if "millisecondsPerStep" in row:
            print(f"Batch {row['batch']}: {row['millisecondsPerStep']:.2f} ms/étape ; "
                  f"{row['samplesPerSecond']:,.0f} exemples/s ; "
                  f"{row['peakAllocatedGpuBytes']/2**30:.2f} Go GPU ; "
                  f"100k étapes ≈ {row['computeSecondsFor100000Steps']/60:.1f} min de calcul")
else:
    print("Mesure de batches ignorée.")
'''),
("markdown", "training-help", """
## 6. Entraîner / reprendre
Réseau 96 × 96, quatre bandes de caractéristiques directionnelles, centres
hybrides et objectif spectral mixte. Un modèle par sujet et budget de filtres.
Le meilleur modèle est sélectionné uniquement sur la validation.

D2 est initialisé avec les poids de référence, avec un nouvel Adam. Ensuite,
une reprise après coupure restaure **tout l'état** depuis resume.pt, sans refaire
les 50 mises à jour de préchauffage.

Le plan est enregistré dans Drive avant l'entraînement. Pour le changer,
choisir un autre EXPERIMENT. Pour reprendre, garder le même paquet et le même
plan. Les sauvegardes précédentes restent disponibles, sans suppression.

Le checkpoint local est préféré dans la session actuelle. Dans une nouvelle
session, le dernier ZIP Drive vérifié est restauré. Une interruption conserve
le dernier pas complet sauvegardé. La perte potentielle dépend des intervalles
de checkpoint et de copie Drive, plus la durée réelle des écritures.
"""),
("code", "train-or-resume", r'''
PERSISTENT = DRIVE_ROOT / "experiments" / EXPERIMENT
plan = {"schema": 1, "sourceBundleSha256": EXPECTED_BUNDLE_SHA256, "subjects": SUBJECTS,
        "filterBudgets": FILTER_BUDGETS, "steps": STEPS, "batchSize": BATCH_SIZE,
        "validationEvery": VALIDATION_EVERY, "warmStartD2": WARM_START_D2,
        "width": 96, "featureBands": 4, "centerGrid": "hybrid",
        "frequencyGrid": "mixed", "selectionGrid": "mixed",
        "precision": "float32; TF32 off; AMP off", "sampleRate": 96000}
plan_path = PERSISTENT / "plan.json"
if plan_path.exists() and read_json(plan_path) != plan:
    raise ValueError("Le plan Drive diffère. Garder le plan de reprise ou choisir un autre EXPERIMENT.")
atomic_json(plan_path, plan)
RUNS = []
for subject in SUBJECTS:
    for filters in FILTER_BUDGETS:
        name = f"{EXPERIMENT}-{subject}-f{filters}"
        run = ROOT / "build/hrtf-runs" / name
        backup = PERSISTENT / "runs" / name
        if not (run / "resume.pt").is_file():
            restore_run(backup, run)
        info = read_json(run / "resume-info.json") if (run / "resume-info.json").is_file() else {}
        done = ((run / "report.json").is_file()
                and read_json(run / "report.json").get("status") == "completed"
                and info.get("step") == STEPS)
        if done:
            print(name, ": déjà terminé.")
        else:
            trainer = [str(PYTHON), str(ROOT / "scripts/train_hrtf_pilot.py")]
            if (run / "resume.pt").is_file():
                command = [*trainer, "--resume-run", str(run)]
            else:
                command = [*trainer, "--subject", subject, "--filters", str(filters), "--width", "96",
                           "--feature-bands", "4", "--center-grid", "hybrid",
                           "--frequency-grid", "mixed", "--selection-grid", "mixed",
                           "--steps", str(STEPS), "--batch-size", str(BATCH_SIZE),
                           "--validation-every", str(VALIDATION_EVERY),
                           "--validation-only", "--output-dir", str(run)]
                reference = REFERENCES.get(str(filters))
                if WARM_START_D2 and subject == "D2":
                    if reference is None:
                        raise ValueError("Poids D2 absents du paquet ; désactiver WARM_START_D2.")
                    command += ["--initial-weights", str(ROOT / reference["checkpoint"])]
            run_with_backup(command, ROOT, run, backup, SAVE_EVERY_SECONDS)
        RUNS.append({"subject": subject, "filters": filters, "run": run, "backup": backup})
print("Entraînements terminés :", PERSISTENT)
'''),
("markdown", "evaluation-help", """
## 7. Figer les modèles puis évaluer
Les hashes de tous les meilleurs poids sont figés avant l'évaluation finale.
Rapports : erreurs spectrales en dB sur grilles log et linéaire, erreur ITD en
microsecondes, voisin mesuré, pôles stables, oracle RBJ complexe float64,
accord CPU/GPU et compensation du retard propre aux filtres.
Une erreur plus basse est préférable.

Découpage 80/10/10 des directions d'un auditeur connu. D2 avec poids de référence
déjà évalués : suivi du même test, pas un nouveau test indépendant.
Ne pas sélectionner des réglages sur ces mêmes chiffres de test. Le renderer
mesuré reste la référence ; entraîner sur GPU ne démontre pas un gain de vitesse
de rendu audio CPU.
"""),
("code", "freeze-and-evaluate", r'''
selection = {"plan": plan, "checkpoints": [
    {"subject": item["subject"], "filters": item["filters"],
     "run": item["run"].relative_to(ROOT).as_posix(),
     "sha256": digest((item["run"] / "best.pt").read_bytes()),
     "testPreviouslyUsedByReference": WARM_START_D2 and item["subject"] == "D2"}
    for item in RUNS]}
frozen_path = PERSISTENT / "frozen-selection.json"
if frozen_path.exists() and read_json(frozen_path) != selection:
    raise ValueError("Sélection figée modifiée ; conserver les résultats et créer une autre expérience.")
atomic_json(frozen_path, selection)
for item, checkpoint in zip(RUNS, selection["checkpoints"]):
    run, backup = item["run"], item["backup"]
    if digest((run / "best.pt").read_bytes()) != checkpoint["sha256"]:
        raise ValueError("Poids modifiés après gel")
    atomic_json(run / "frozen-selection.json", selection)
    if RUN_FINAL_EVALUATION:
        for script, flag in (("train_hrtf_pilot.py", "--evaluate-run"), ("check_hrtf_field.py", "--run")):
            run_with_backup([str(PYTHON), str(ROOT / "scripts" / script), flag, str(run)],
                            ROOT, run, backup, SAVE_EVERY_SECONDS)
        result = read_json(run / "evaluation.json")
        print(item["subject"], item["filters"], "filtres :",
              f"log {result['heldOutTestLogFrequency']['magnitudeRmseDb']:.3f} dB ;",
              f"linéaire {result['heldOutTestLinearFrequency']['magnitudeRmseDb']:.3f} dB ;",
              f"ITD {result['heldOutTestLinearFrequency']['itdRmseUs']:.3f} µs")
    snapshot_run(run, backup, force=True)
print("Résultats sauvegardés :", PERSISTENT)
'''),
("markdown", "results-help", """
## 8. Lire les courbes et récupérer les sorties
Courbes de validation uniquement. Les ZIP Drive contiennent resume.pt,
best.pt, métadonnées, CSV, licences, sources exactes puis poids JSON et rapports.
Pour me transmettre une passe : le dernier ZIP indiqué par latest.json,
plan.json, frozen-selection.json et le JSON de benchmark suffisent.
Les SOFA se retéléchargent avec les mêmes hashes.

resume.pt reprend l'entraînement ; best.pt est le meilleur modèle de validation.
Leurs rôles sont différents. Le rendu à un autre taux que 96 kHz nécessite
une adaptation et une validation propres.

Sources officielles :
[Colab](https://research.google.com/colaboratory/faq.html),
[PyTorch / CUDA](https://pytorch.org/get-started/previous-versions/),
[pip / venv](https://pip.pypa.io/en/stable/topics/python-option/),
[SADIE II v2-2](https://zenodo.org/records/12092466).
La disponibilité du GPU et la durée de session dépendent de Colab.
"""),
("code", "plot-results", r'''
import csv
import matplotlib.pyplot as plt

fig, axes = plt.subplots(1, 2, figsize=(13, 4))
for item in RUNS:
    with (item["run"] / "progress.csv").open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    steps = [int(row["step"]) for row in rows]
    label = f"{item['subject']} / {item['filters']} filtres"
    axes[0].plot(steps, [float(row["validationLinearRmseDb"]) for row in rows], label=label)
    axes[1].plot(steps, [float(row["itdRmseUs"]) for row in rows], label=label)
for ax in axes:
    ax.set_xlabel("Mises à jour")
    ax.grid(alpha=.25)
    ax.legend()
axes[0].set_ylabel("Erreur spectrale de validation, grille linéaire (dB RMS)")
axes[1].set_ylabel("Erreur ITD de validation (µs RMS)")
fig.tight_layout()
plt.show()
print("Dossier Drive :", PERSISTENT)
for item in RUNS:
    pointer = read_json(item["backup"] / "latest.json")
    print(item["subject"], item["filters"], "filtres →", item["backup"] / pointer["archive"])
'''),
]


def quality_cells():
    """Separate L4 fine-tuning recipe; keep old notebook/bundle pairs resumable."""
    result = []
    for kind, name, original in CELLS:
        source = original.replace("CyDSP-HRTF-Colab.zip", "CyDSP-HRTF-L4-Quality.zip")
        if name == "introduction":
            source = r"""
# CyDSP — affinage qualité HRTF sur Colab L4, environ 30 minutes

Cette nouvelle expérience améliore le modèle **D2/KEMAR à 48 kHz** déjà utilisable
avec l'API native CyDSP. Elle repart des meilleurs poids 48 kHz, avec la même
architecture (24 filtres par oreille, réseau 96 × 96) et un taux d'apprentissage
plus faible. Les poids actuels d'intégration restent disponibles séparément.

1. Importer **CyDSP_HRTF_L4_Quality.ipynb** dans Colab.
2. Copier le **fichier ZIP** **CyDSP-HRTF-L4-Quality.zip**, sans le décompresser,
   dans le **dossier** **Mon Drive → CyDSP-HRTF**.
3. Choisir **Exécution → Modifier le type d'exécution → GPU → L4**, puis
   **Exécution → Tout exécuter** et autoriser Drive.

Par défaut : **140 000 mises à jour supplémentaires**, batch 512,
Adam/cosine **0,0003 → 0,00001**. La passe L4 précédente prenait environ
12,6 ms/étape : cela correspond à **29,4 minutes de calcul estimées**.
Installation, téléchargements, validation et sauvegardes s'ajoutent à cette durée ;
le notebook affiche la vitesse réellement obtenue. Aucune garantie de convergence.

Le meilleur modèle initial reste candidat : si la validation ne s'améliore pas,
il est conservé. Sélection sur la validation seulement ; le test D2 a déjà servi,
les résultats finaux suivent donc le même test, sans nouvelle validation indépendante.

La nouvelle expérience s'appelle **l4-hrtf-48k-quality-001**. Elle ne reprend pas
le calendrier de l'ancienne expérience a100-hrtf-001. Après une coupure de cette
nouvelle expérience, reprendre avec ce même notebook, ce même ZIP et le même nom.
Poids, Adam, état aléatoire et calendrier sont sauvegardés dans Drive.

**Code MIT ; mesures SADIE II Apache-2.0**, notices conservées. Ce modèle approche
l'amplitude et l'ITD basse fréquence d'un auditeur connu ; ni phase HRIR complète
ni personnalisation universelle. L'objectif est la fidélité, avec la même API C++.
"""
        elif name == "configuration":
            source = source.replace('EXPERIMENT = "a100-hrtf-001"', 'EXPERIMENT = "l4-hrtf-48k-quality-001"')
            source = source.replace('SUBJECTS = ["D2"]', 'SAMPLE_RATE = 48_000\nLEARNING_RATE = 0.0003\nMINIMUM_LEARNING_RATE = 0.00001\nREQUIRE_L4 = True\nSUBJECTS = ["D2"]')
            source = source.replace('STEPS = 100_000', 'STEPS = 140_000')
            source = source.replace('# D1, D2 ou H3..H20 ; un modèle par sujet.', '# Ce profil affine D2/KEMAR à 48 kHz.')
            source = source.replace('# [12], [24] ou [12, 24].', '# Référence fournie : 24 filtres à 48 kHz.')
            source = source.replace('RUN_BATCH_BENCHMARK = True', 'RUN_BATCH_BENCHMARK = False')
            source = source.replace('set(FILTER_BUDGETS) - {12, 24}', 'set(FILTER_BUDGETS) - {24}')
            source = source.replace('Budgets disponibles : 12 et 24', 'Ce profil 48 kHz inclut le modèle à 24 filtres')
            source += '\nif SAMPLE_RATE != 48000 or SUBJECTS != ["D2"]:\n    raise ValueError("Ce profil part du modèle D2 48 kHz ; créer une autre expérience pour changer de cible")\n'
            source += 'if not 0 < MINIMUM_LEARNING_RATE <= LEARNING_RATE <= 0.1:\n    raise ValueError("Vérifier les taux d’apprentissage")\n'
            source += 'print("Affinage", SAMPLE_RATE, "Hz ; LR", LEARNING_RATE, "→", MINIMUM_LEARNING_RATE)\n'
        elif name == "environment-help":
            source += '\nCe profil exige une L4 par défaut. Le message GPU confirme le matériel alloué.\n'
        elif name == "install-environment":
            source += '\nif REQUIRE_L4 and "L4" not in GPU["gpu"].upper():\n    raise RuntimeError("Une NVIDIA L4 est requise pour ce profil ; sélectionner la L4 dans Colab.")\n'
        elif name == "dataset-help":
            source = """
## 4. Télécharger les mesures à 48 kHz

SADIE II D2 officiel : environ **38 Mo de ZIP** et **36,3 Mo de SOFA**,
**48 kHz / 256 taps** par oreille. Tailles, SHA-256/MD5 et CRC sont vérifiés.
Les ZIP déjà en cache Drive sont réutilisables. L'entraînement se fait sur le
stockage Colab local ; les archives et sauvegardes seulement sont dans Drive.
"""
        elif name == "download-data":
            source = source.replace('"--rate", "96000"', '"--rate", str(SAMPLE_RATE)')
        elif name == "measure-batches":
            source = source.replace('"--subject", SUBJECTS[0],', '"--subject", SUBJECTS[0], "--sample-rate", str(SAMPLE_RATE),')
        elif name == "training-help":
            source += """

Ce profil affine les meilleurs poids 48 kHz : **0,0003 → 0,00001**, au lieu de
redémarrer à 0,002. Le batch et les filtres restent identiques. L'objectif spectral
mixte et l'ITD sont conservés ; la sélection utilise l'erreur spectrale linéaire
et l'ITD, comme pour le modèle 48 kHz initial. Le meilleur état initial est éligible.
140 000 étapes représentent environ 30 min de calcul L4 selon la passe précédente,
avec une durée réelle variable. Ne pas modifier le calendrier pendant une reprise.
"""
        elif name == "train-or-resume":
            source = source.replace('"selectionGrid": "mixed"', '"selectionGrid": "linear"')
            source = source.replace('"sampleRate": 96000', '"sampleRate": SAMPLE_RATE,\n        "learningRate": LEARNING_RATE, "minimumLearningRate": MINIMUM_LEARNING_RATE')
            source = source.replace('"--selection-grid", "mixed"', '"--selection-grid", "linear",\n                           "--sample-rate", str(SAMPLE_RATE),\n                           "--learning-rate", str(LEARNING_RATE),\n                           "--minimum-learning-rate", str(MINIMUM_LEARNING_RATE)')
            source += '\nprint("Rythme et résultats par modèle :")\nfor item in RUNS:\n    result = read_json(item["run"] / "report.json")\n    initial, best = result["initialValidation"], result["bestValidation"]\n    print(item["subject"], "validation linéaire :", initial["validationLinearRmseDb"], "→", best["validationLinearRmseDb"], "dB RMS ; meilleur pas", result["bestStep"])\n'
        elif name == "results-help":
            source = source.replace('un autre taux que 96 kHz', 'un autre taux que 48 kHz')
            source += '\nLes nouveaux poids restent dans la nouvelle expérience Drive. Ils ne remplacent pas automatiquement les fichiers utilisés par ton intégration. Comparer ensuite le rendu natif et l’écoute avant de les adopter.\n'
        result.append((kind, name, source))
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--without-reference-weights", action="store_true")
    parser.add_argument("--profile", choices=("baseline", "l4-quality"), default="baseline")
    args = parser.parse_args()
    bundle, checksum = make_bundle(not args.without_reference_weights, args.profile)
    notebook_name = "CyDSP_HRTF_L4_Quality.ipynb" if args.profile == "l4-quality" else "CyDSP_HRTF_A100.ipynb"
    cells = []
    for kind, name, source in (quality_cells() if args.profile == "l4-quality" else CELLS):
        source = source.strip().replace("__BUNDLE_SHA256__", checksum) + "\n"
        if args.without_reference_weights:
            if name == "configuration":
                source = source.replace("WARM_START_D2 = True", "WARM_START_D2 = False")
            elif name == "introduction":
                source += "\n**Paquet sans poids de référence : départ de zéro, WARM_START_D2=False.**\n"
        if kind == "code":
            ast.parse(source, filename=name)
        cell = {"cell_type": kind, "id": name, "metadata": {"id": name},
                "source": source.splitlines(keepends=True)}
        if kind == "code":
            cell.update(execution_count=None, outputs=[])
        cells.append(cell)
    notebook = {"nbformat": 4, "nbformat_minor": 5, "metadata": {
        "colab": {"name": notebook_name, "provenance": []},
        "accelerator": "GPU",
        "kernelspec": {"display_name": "Python 3", "language": "python", "name": "python3"},
        "language_info": {"name": "python"},
        "cydsp": {"sourceBundleSha256": checksum, "codeLicense": "MIT", "dataLicense": "Apache-2.0"}},
        "cells": cells}
    path = ROOT / "notebooks" / notebook_name
    atomic_bytes(path, (json.dumps(notebook, ensure_ascii=False, indent=1) + "\n").encode("utf-8"))
    shutil.copyfile(path, bundle.parent / path.name)
    print(json.dumps({"notebook": str(path), "bundle": str(bundle), "bundleBytes": bundle.stat().st_size,
                      "sha256": checksum, "cells": len(cells)}, ensure_ascii=False))


if __name__ == "__main__":
    main()
