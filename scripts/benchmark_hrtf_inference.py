# Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
"""Measure warm PyTorch HRTF inference; no training and no audio rendering."""
import argparse
from datetime import datetime, timezone
import gc
import json
import math
import os
from pathlib import Path
import platform
import statistics
import subprocess
import time
import uuid

import numpy as np
import torch
from train_hrtf_pilot import ROOT, model_from_metadata, sha256, write_json

DEFAULT_RUN = ROOT / "build/hrtf-runs/20261116T184219Z-D2-5a3037c9"


def cpu_name():
    if os.name == "nt":
        import winreg
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                            r"HARDWARE\DESCRIPTION\System\CentralProcessor\0") as key:
            return winreg.QueryValueEx(key, "ProcessorNameString")[0].strip()
    if Path("/proc/cpuinfo").is_file():
        for line in Path("/proc/cpuinfo").read_text().splitlines():
            if line.startswith("model name"):
                return line.split(":", 1)[1].strip()
    return platform.processor()


def summary(values):
    data = np.asarray(values, dtype=np.float64)
    if not np.isfinite(data).all() or (data <= 0).any():
        raise ValueError("Invalid timing samples")
    return {"medianUs": float(np.median(data)), "p95Us": float(np.quantile(data, .95)),
            "minimumUs": float(np.min(data)), "maximumUs": float(np.max(data))}


def measure(operation, device, samples, trials, warmup):
    for _ in range(warmup):
        output = operation()
    is_cuda = device.type == "cuda"
    if is_cuda:
        torch.cuda.synchronize(device)
        begin, end = torch.cuda.Event(enable_timing=True), torch.cuda.Event(enable_timing=True)
        begin.record()
        end.record()
        end.synchronize()  # Initialize event objects outside measurements.
    wall, stream, medians = [], [], []
    was_enabled = gc.isenabled()
    gc.disable()
    try:
        for _ in range(trials):
            trial_wall = []
            for _ in range(samples):
                # Previous request is complete. No cross-request asynchronous batching.
                start = time.perf_counter_ns()
                if is_cuda:
                    begin.record()
                output = operation()
                if is_cuda:
                    end.record()
                    end.synchronize()
                elapsed = (time.perf_counter_ns() - start) / 1000
                wall.append(elapsed)
                trial_wall.append(elapsed)
                if is_cuda:
                    stream.append(begin.elapsed_time(end) * 1000)
            medians.append(statistics.median(trial_wall))
    finally:
        if was_enabled:
            gc.enable()
    tensors = output if isinstance(output, tuple) else (output,)
    if any(not torch.isfinite(t).all().item() for t in tensors):
        raise ValueError("Nonfinite inference output")
    return {"wall": summary(wall), "stream": summary(stream) if is_cuda else None,
            "trialMedianUs": medians, "wallSamplesUs": wall,
            "streamSamplesUs": stream if is_cuda else None}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", type=Path, default=DEFAULT_RUN)
    parser.add_argument("--batches", nargs="+", type=int, default=[1, 16, 64, 256])
    parser.add_argument("--samples", type=int, default=300)
    parser.add_argument("--trials", type=int, default=5)
    parser.add_argument("--warmup", type=int, default=200)
    parser.add_argument("--devices", nargs="+", choices=("cpu", "cuda"), default=["cpu", "cuda"])
    args = parser.parse_args()
    if not 50 <= args.samples <= 10000 or not 1 <= args.trials <= 20 or not 20 <= args.warmup <= 5000:
        parser.error("Samples 50..10000; trials 1..20; warmup 20..5000")
    if not args.batches or len(set(args.batches)) != len(args.batches) or any(not 1 <= n <= 4096 for n in args.batches):
        parser.error("Distinct batch sizes in 1..4096")
    if len(set(args.devices)) != len(args.devices):
        parser.error("Distinct devices required")
    run = args.run.resolve()
    if not run.is_relative_to((ROOT / "build/hrtf-runs").resolve()):
        parser.error("Use an ignored local training run")
    checkpoint_path = run / "best.pt"
    checkpoint = torch.load(checkpoint_path, map_location="cpu", weights_only=True)
    meta = checkpoint["metadata"]
    if sha256(run / "training-source.py") != meta["sourceCodeSha256"]:
        raise ValueError("Archived training-source hash mismatch")
    torch.set_num_threads(1)
    torch.set_num_interop_threads(1)
    torch.backends.cuda.matmul.allow_tf32 = False
    torch.backends.cudnn.allow_tf32 = False
    cpu = model_from_metadata(meta, "cpu").eval()
    cpu.load_state_dict(checkpoint["state_dict"])
    settings = meta["arguments"]
    width, filters, bands = settings["width"], settings["filters"], settings["feature_bands"]
    inputs, outputs = 3 + 6 * bands, 2 * (filters * 3 + 1) + 1
    parameters = sum(p.numel() for p in cpu.parameters())
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    out = ROOT / "build/hrtf-inference" / (stamp + "-" + uuid.uuid4().hex[:8])
    out.mkdir(parents=True)
    models = {"cpu": cpu}
    if "cuda" in args.devices:
        if not torch.cuda.is_available():
            raise RuntimeError("CUDA was requested but is unavailable; explicitly use --devices cpu")
        gpu = model_from_metadata(meta, "cuda").eval()
        gpu.load_state_dict(checkpoint["state_dict"])
        models["cuda"] = gpu
    generator = torch.Generator().manual_seed(20261116)
    directions = torch.randn(max(max(args.batches), 64), 3, generator=generator)
    directions /= directions.norm(dim=-1, keepdim=True)
    correctness = {}
    with torch.inference_mode():
        reference = cpu(directions[:64])
        if "cuda" in models:
            candidate = models["cuda"](directions[:64].cuda())
            for name, expected, actual in zip(("centerHz", "q", "gainDb", "levelDb", "itdUs"), reference, candidate):
                actual = actual.cpu()
                torch.testing.assert_close(expected, actual, rtol=1e-4, atol=1e-4)
                correctness[name + "CpuGpuMaxAbsDifference"] = float((expected - actual).abs().max())
        rows = []
        samples = {}
        for device_name in args.devices:
            model = models[device_name]
            device = next(model.parameters()).device
            for batch in args.batches:
                xyz = directions[:batch].to(device)
                encoded = model.features(xyz)
                operations = {"networkOnly": lambda: model.network(encoded),
                              "directionToBoundedParameters": lambda: model(xyz)}
                for name, operation in operations.items():
                    measured = measure(operation, device, args.samples, args.trials, args.warmup)
                    row = {"device": device_name, "batchDirections": batch, "operation": name,
                           "wall": measured["wall"], "stream": measured["stream"],
                           "trialMedianUs": measured["trialMedianUs"],
                           "amortizedWallMedianUsPerDirection": measured["wall"]["medianUs"] / batch,
                           "medianDirectionsPerSecond": batch * 1e6 / measured["wall"]["medianUs"]}
                    rows.append(row)
                    samples[device_name + "-" + str(batch) + "-" + name] = {
                        key: measured[key] for key in ("wallSamplesUs", "streamSamplesUs")}
                    print(json.dumps(row), flush=True)
    try:
        driver = subprocess.check_output(["nvidia-smi", "--query-gpu=name,driver_version,utilization.gpu",
                                           "--format=csv,noheader"], text=True).strip()
    except (OSError, subprocess.SubprocessError):
        driver = "unavailable"
    result = {"schema": 1, "utc": stamp, "cpu": cpu_name(), "platform": platform.platform(),
              "python": platform.python_version(), "torch": str(torch.__version__),
              "cuda": torch.version.cuda, "gpu": torch.cuda.get_device_name(0) if "cuda" in models else None,
              "gpuDriverSnapshot": driver, "cpuThreads": torch.get_num_threads(),
              "interopThreads": torch.get_num_interop_threads(), "precision": "float32, TF32/AMP disabled",
              "architecture": [inputs, width, width, outputs], "parameters": parameters,
              "denseMacsPerDirection": inputs * width + width * width + width * outputs,
              "float32WeightBytes": parameters * 4, "trainingRun": run.relative_to(ROOT).as_posix(),
              "checkpointSha256": sha256(checkpoint_path), "benchmarkSourceSha256": sha256(Path(__file__)),
              "modelProviderSha256": sha256(ROOT / "scripts/train_hrtf_pilot.py"),
              "usesLatestColabWeights": (run / "import-provenance.json").is_file(),
              "checkpointScope": ("Verified user-provided Colab export" if
                                  (run / "import-provenance.json").is_file() else
                                  "Local previously selected model; Colab weights are not implied"),
              "method": {"samplesPerTrial": args.samples, "trials": args.trials, "warmupCallsPerCase": args.warmup,
                         "mode": "eval + inference_mode; warm resident inputs and weights",
                         "wallCpu": "Synchronous call, including Python/PyTorch dispatch and output tensor allocation",
                         "wallCuda": "One completed request at a time; events + host end synchronization included",
                         "streamCuda": "Start/end event span; includes idle gaps while host submits kernels",
                         "batchAmortization": "Lot latency divided by directions; not individual request latency",
                         "excludes": ["Initialization/loading/warmup", "Host-device transfers",
                                      "RBJ coefficient construction", "Cascade phase/ITD compensation",
                                      "Audio sample filtering/delays", "Real-time C++ implementation"],
                         "environmentControl": "Ordinary desktop session; no priority, affinity, power or antivirus changes",
                         "percentiles": "Empirical warm-call timings; not a hard real-time worst-case bound"},
              "correctness": correctness, "rows": rows}
    write_json(out / "report.json", result)
    write_json(out / "samples.json", samples)
    (out / "benchmark-source.py").write_bytes(Path(__file__).read_bytes())
    (out / "model-source.py").write_bytes((ROOT / "scripts/train_hrtf_pilot.py").read_bytes())
    (out / "LICENSE-code.txt").write_bytes((ROOT / "LICENSE").read_bytes())
    for name in ("LICENSE-data.txt", "NOTICE-data.txt"):
        if (run / name).is_file():
            (out / name).write_bytes((run / name).read_bytes())
    lines = ["# Temps de prédiction du modèle HRTF", "",
             "Mesure locale PyTorch en float32, sans entraînement et sans traitement d’échantillons audio.",
             "", "CPU : " + result["cpu"] + " (un thread). GPU : " + str(result["gpu"]) + ".",
             "Architecture : " + " → ".join(map(str, result["architecture"])) + ".",
             str(parameters) + " paramètres ; " + str(result["denseMacsPerDirection"]) +
             " multiplications-accumulations pour les couches denses par direction.",
             "", "Les poids sont ceux de la référence locale ; les derniers poids Colab ne sont pas disponibles ici.",
             "", "| Appareil | Directions par appel | Calcul | Médiane par appel (µs) | P95 par appel (µs) | Moyenne amortie par direction (µs) |",
             "| --- | ---: | --- | ---: | ---: | ---: |"]
    for row in rows:
        label = ("Réseau seul, entrée déjà encodée" if row["operation"] == "networkOnly" else
                 "Direction → encodage → réseau → paramètres bornés")
        lines.append(f"| {row['device']} | {row['batchDirections']} | {label} | "
                     f"{row['wall']['medianUs']:.3f} | {row['wall']['p95Us']:.3f} | "
                     f"{row['amortizedWallMedianUsPerDirection']:.3f} |")
    lines += ["", "Les temps GPU attendent la fin de chaque appel ; les entrées et sorties restent en VRAM.",
              "Les temps CPU incluent le dispatch Python/PyTorch et les allocations de tenseurs.",
              "Le réseau seul omet l’encodage directionnel et la conversion des sorties brutes.",
              "Les deux chemins excluent construction des coefficients RBJ, compensation de phase/ITD,",
              "traitement audio, chargement et transferts CPU/GPU. Les percentiles décrivent les",
              "échantillons mesurés, sans garantir une borne temps réel.",
              "", "La moyenne amortie d’un lot n’est pas la latence d’une prédiction isolée.",
              "Un futur renderer C++ doit avoir ses propres mesures. Aucun entraînement ni modification",
              "des poids, du notebook Colab ou de son paquet Drive n’est effectué par cet outil.", ""]
    (out / "Rapport.fr.md").write_text("\n".join(lines), encoding="utf-8", newline="\n")
    print(json.dumps({"event": "completed", "directory": out.relative_to(ROOT).as_posix(),
                      "report": str(out / "Rapport.fr.md")}), flush=True)


if __name__ == "__main__":
    main()
