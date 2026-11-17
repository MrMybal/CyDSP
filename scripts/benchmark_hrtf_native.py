# Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
"""Export trained weights, validate native C++ inference and benchmark CyDSP SIMD."""
import argparse
from datetime import datetime, timezone
import json
import math
import os
from pathlib import Path
import platform
import shutil
import struct
import subprocess
import sys
import uuid

import numpy as np
import torch
from train_hrtf_pilot import (ROOT, CORPUS, model_from_metadata, load_subject, peaking_db,
                              sha256, write_json)
from benchmark_hrtf_inference import cpu_name

DEFAULT_RUN = ROOT / "build/hrtf-runs/colab-a100-hrtf-001-D2-f24-10a7ea96"
CPP = ROOT / "scripts/native/HrtfInference.cpp"
DEFAULT_CMAKE = Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe")


def f32bytes(value):
    if isinstance(value, torch.Tensor):
        value = value.detach().cpu().numpy()
    result = np.asarray(value, dtype="<f4")
    if not np.isfinite(result).all():
        raise ValueError("Nonfinite export")
    return result.tobytes(order="C")


def pack_parameters(prediction):
    center, q, gain, level, itd = prediction
    shape = torch.stack((center, q, gain), dim=-1).flatten(2)
    ears = torch.cat((shape, level.unsqueeze(-1)), dim=-1).flatten(1)
    return torch.cat((ears, itd.unsqueeze(-1)), dim=-1)


def unpack_parameters(values, filters):
    p = values[:, :-1].reshape(-1, 2, filters * 3 + 1)
    shape = p[:, :, :filters * 3].reshape(-1, 2, filters, 3)
    return shape[..., 0], shape[..., 1], shape[..., 2], p[:, :, -1], values[:, -1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", type=Path, default=DEFAULT_RUN)
    parser.add_argument("--library", type=Path, default=ROOT / "build/windows-x64/Release/CyDSP.lib",
                        help="Existing Release static archive; build it explicitly before running")
    parser.add_argument("--cmake", type=Path,
                        default=Path(shutil.which("cmake") or str(DEFAULT_CMAKE)))
    args = parser.parse_args()
    run = args.run.resolve()
    if not run.is_relative_to((ROOT / "build/hrtf-runs").resolve()):
        parser.error("Use an ignored local HRTF checkpoint")
    library = args.library.resolve()
    if not library.is_file() or not args.cmake.is_file():
        parser.error("Build the CyDSP static archive and provide a working CMake executable")
    checkpoint = torch.load(run / "best.pt", map_location="cpu", weights_only=True)
    meta = checkpoint["metadata"]
    if sha256(run / "training-source.py") != meta["sourceCodeSha256"]:
        raise ValueError("Training snapshot provenance mismatch")
    torch.set_num_threads(1)
    torch.set_num_interop_threads(1)
    model = model_from_metadata(meta, "cpu").eval()
    model.load_state_dict(checkpoint["state_dict"])
    width, filters, bands = [meta["arguments"][name] for name in ("width", "filters", "feature_bands")]
    inputs, outputs = 3 + 6 * bands, 2 * (3 * filters + 1) + 1
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    out = ROOT / "build/hrtf-native-inference" / (stamp + "-" + uuid.uuid4().hex[:8])
    out.mkdir(parents=True)
    data = bytearray(b"CYHRTF01" + struct.pack("<5I", inputs, width, outputs, filters, bands))
    for index in (0, 2, 4):
        for name in ("weight", "bias"):
            data.extend(f32bytes(checkpoint["state_dict"][f"network.{index}.{name}"]))
    phases = torch.tensor([math.pi * 2 ** i for i in range(bands)], dtype=torch.float32)
    for tensor in (model.centers, model.center_shift, phases):
        data.extend(f32bytes(tensor))
    (out / "weights.bin").write_bytes(data)
    xyz, _, _, _, source = load_subject(meta["arguments"]["subject"], np.array([1000.]), np.array([1000.]), model.sample_rate)
    if source["sha256"] != meta["source"]["sha256"]:
        raise ValueError("SOFA source provenance mismatch")
    generator = torch.Generator().manual_seed(20261116)
    random = torch.randn(512, 3, generator=generator)
    random /= random.norm(dim=-1, keepdim=True)
    axes = torch.cat((torch.eye(3), -torch.eye(3)), dim=0)
    directions = torch.cat((torch.as_tensor(xyz), random, axes), dim=0)
    with torch.inference_mode():
        encoded = model.features(directions)
        raw = model.network(encoded)
        prediction = pack_parameters(model.decode_parameters(raw))
    reference = bytearray(b"CYHRREF1" + struct.pack("<3I", len(directions), inputs, outputs))
    for tensor in (directions, encoded, raw, prediction):
        reference.extend(f32bytes(tensor))
    (out / "reference.bin").write_bytes(reference)
    configure_dir = out / "cmake-source"
    configure_dir.mkdir()
    archive = library.as_posix()
    cmake_source = f"""cmake_minimum_required(VERSION 3.24)
project(CyDSPHrtfNativeBenchmark LANGUAGES CXX)
add_executable(hrtf-native "{CPP.as_posix()}")
target_compile_features(hrtf-native PRIVATE cxx_std_20)
target_include_directories(hrtf-native PRIVATE "{(ROOT / 'include').as_posix()}")
target_link_libraries(hrtf-native PRIVATE "{archive}")
if(MSVC)
  target_compile_options(hrtf-native PRIVATE /O2 /fp:strict /utf-8 /W4)
else()
  target_compile_options(hrtf-native PRIVATE -O3 -ffp-contract=off -fno-fast-math -Wall -Wextra -Wpedantic)
endif()
"""
    (configure_dir / "CMakeLists.txt").write_text(cmake_source, encoding="utf-8", newline="\n")
    build = out / "cmake-build"
    command = [str(args.cmake), "-S", str(configure_dir), "-B", str(build), "-DCMAKE_BUILD_TYPE=Release"]
    if os.name == "nt":
        command += ["-G", "Visual Studio 17 2022", "-A", "x64"]
    subprocess.run(command, check=True, cwd=root_directory())
    subprocess.run([str(args.cmake), "--build", str(build), "--config", "Release",
                    "--target", "hrtf-native", "--parallel", "4"], check=True)
    executable = build / ("Release/hrtf-native.exe" if os.name == "nt" else "hrtf-native")
    # The measured process loads only the binary assets and the CyDSP static archive.
    process = subprocess.Popen([str(executable), str(out / "weights.bin"), str(out / "reference.bin"),
                                str(out)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               text=True, encoding="utf-8", bufsize=1)
    events, log = [], []
    for line in process.stdout:
        log.append(line)
        print(line, end="", flush=True)
        if line.startswith("{"):
            events.append(json.loads(line))
    if process.wait():
        raise RuntimeError("Native validation/benchmark failed")
    (out / "native-output.jsonl").write_text("".join(log), encoding="utf-8", newline="\n")
    quality = {}
    hz = torch.linspace(200, 20000, 257)
    with torch.inference_mode():
        for event in events:
            if event["event"] != "validation":
                continue
            backend = event["backend"]
            values = np.fromfile(out / (backend + "-predictions.bin"), dtype="<f4").reshape(len(directions), outputs)
            maximum, square_sum, count = 0., 0., 0
            for start in range(0, len(directions), 64):
                actual = torch.from_numpy(values[start:start+64])
                expected = prediction[start:start+64]
                a = peaking_db(*unpack_parameters(actual, filters)[:4], hz, model.sample_rate)
                b = peaking_db(*unpack_parameters(expected, filters)[:4], hz, model.sample_rate)
                error = (a-b).double()
                maximum = max(maximum, float(error.abs().max()))
                square_sum += float(error.square().sum())
                count += error.numel()
            if maximum > .005:
                raise ValueError("Native spectrum disagrees with the trained predictor")
            quality[backend] = {"maximumMagnitudeDifferenceDb": maximum,
                                "magnitudeDifferenceRmseDb": math.sqrt(square_sum/count)}
            print(json.dumps({"event": "spectral-equivalence", "backend": backend, **quality[backend]}), flush=True)
    dependencies = None
    if os.name == "nt":
        dumpbin = Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\bin\Hostx64\x64\dumpbin.exe")
        if dumpbin.is_file():
            dependencies = subprocess.check_output([str(dumpbin), "/DEPENDENTS", str(executable)],
                                                   text=True, errors="replace")
            (out / "dependencies.txt").write_text(dependencies, encoding="utf-8")
            if any(name in dependencies.lower() for name in ("torch", "c10.dll", "python", "onnx", "cuda")):
                raise ValueError("Unexpected native runtime dependency")
    result = {"utc": stamp, "cpu": cpu_name(), "platform": platform.platform(), "threads": 1,
              "runtime": "Native C++20 + static CyDSP VectorOps + standard runtime; no PyTorch/Python",
              "referenceExporter": "PyTorch is used offline for export and numerical validation only",
              "checkpointSha256": sha256(run / "best.pt"), "weightsBinarySha256": sha256(out / "weights.bin"),
              "architecture": [inputs,width,width,outputs], "parameters": sum(p.numel() for p in model.parameters()),
              "referenceDirections": {"measured":len(xyz), "additionalRandom":512, "axes":6,
                                      "total":len(directions)},
              "precision": "float32, standard sin/cos/tanh/exp; precise compilation, no global ISA/fast-math",
              "preparedBuffers": True, "batchMethod": "Sequential direction predictions, no batched GEMM",
              "sourceSha256": sha256(CPP), "staticLibrarySha256": sha256(library),
              "executableSha256": sha256(executable), "runtimeDependenciesInspected": dependencies is not None,
              "events": events, "spectralEquivalence": quality,
              "limits": ["Warm empirical timings, no hard real-time worst-case guarantee",
                         "No audio filtering, RBJ coefficient generation or ITD phase compensation in timing",
                         "Prototype benchmark evaluator; no public neural C++ renderer API added"]}
    write_json(out / "report.json", result)
    (out / "native-source.cpp").write_bytes(CPP.read_bytes())
    (out / "exporter-source.py").write_bytes(Path(__file__).read_bytes())
    (out / "LICENSE-code.txt").write_bytes((ROOT / "LICENSE").read_bytes())
    for filename in ("LICENSE-data.txt","NOTICE-data.txt"):
        (out / filename).write_bytes((run / filename).read_bytes())
    lines=["# Inférence HRTF native C++","",
           "Le processus chronométré est un exécutable C++20 lié à la bibliothèque statique CyDSP.",
           "Il ne charge ni Python, ni PyTorch, ni ONNX, ni CUDA. PyTorch sert seulement à exporter les poids et à vérifier les résultats hors mesure.",
           "", "CPU : "+result["cpu"]+", un thread.",
           "Architecture : "+" → ".join(map(str,result["architecture"]))+".",
           "", "Les fonctions SIMD publiques VectorOps::dot de CyDSP calculent les couches denses.",
           "Les activations et conversions utilisent les fonctions mathématiques standard précises.",
           "Poids et buffers sont préparés avant l’inférence, sans allocation ni I/O dans la prédiction.",
           "",
           "| Backend CyDSP | Calcul | Directions par appel | Médiane par appel (µs) | P95 (µs) |",
           "| --- | --- | ---: | ---: | ---: |"]
    for event in events:
        if event["event"]=="timing":
            name="Réseau seul, entrée encodée" if event["operation"]=="networkOnly" else "Direction → réseau → paramètres bornés"
            lines.append(f"| {event['backend']} | {name} | {event['batchDirections']} | "
                         f"{event['medianUs']:.3f} | {event['p95Us']:.3f} |")
    lines += ["","Les lots natifs exécutent les directions successivement ; ils ne constituent pas un GEMM batched.",
              "Les percentiles sont empiriques en session Windows ordinaire, sans changement de priorité, affinité, alimentation ou antivirus.",
              "",
              "## Équivalence numérique","",
              f"Les {len(directions)} directions vérifiées comprennent {len(xyz)} mesures originales, 512 directions aléatoires et les six axes.",
              "Chaque backend est comparé aux sorties du prédicteur entraîné, puis sur 257 fréquences de 200 Hz à 20 kHz.",
              "",
              "| Backend | Écart spectral maximal | Écart spectral RMS |","| --- | ---: | ---: |"]
    for backend, value in quality.items():
        lines.append(f"| {backend} | {value['maximumMagnitudeDifferenceDb']:.9f} dB | {value['magnitudeDifferenceRmseDb']:.9f} dB |")
    lines += ["","Ce prototype conserve les équations et la précision du modèle ; aucune approximation rapide de tanh ou exp n’est ajoutée.",
              "Les différences résiduelles viennent des réductions float32 et des implémentations mathématiques.",
              "Le chronométrage exclut construction des coefficients RBJ, compensation de phase/ITD, traitement des échantillons audio et chargement.",
              "Il mesure le prédicteur natif expérimental, sans ajouter de renderer neuronal à l’API publique.",
              "", "Le contrôle des dépendances du binaire est enregistré dans dependencies.txt quand disponible.", ""]
    (out / "Rapport.fr.md").write_text("\n".join(lines), encoding="utf-8", newline="\n")
    print(json.dumps({"event":"completed","report":str(out / "Rapport.fr.md")}),flush=True)


def root_directory():
    return str(ROOT)


if __name__ == "__main__":
    main()
