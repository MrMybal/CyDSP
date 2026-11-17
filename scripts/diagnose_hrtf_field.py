# Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
"""Diagnose an HRTF pilot using training/validation only, never the held-out test."""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import time
import uuid
import numpy as np
import torch
from train_hrtf_pilot import HrtfField, ROOT, load_subject, write_json, sha256


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--steps", type=int, default=2000)
    parser.add_argument("--samples", type=int, default=64)
    args = parser.parse_args()
    run = args.run.resolve()
    if not run.is_relative_to((ROOT / "build/hrtf-runs").resolve()):
        parser.error("Use a local ignored pilot run")
    if not 100 <= args.steps <= 10000 or not 16 <= args.samples <= 256:
        parser.error("Steps 100..10000, samples 16..256")
    if not torch.cuda.is_available():
        raise RuntimeError("CUDA required")
    torch.set_num_threads(4)
    torch.manual_seed(20261116)
    torch.backends.cuda.matmul.allow_tf32 = False
    checkpoint = torch.load(run / "best.pt", map_location="cuda", weights_only=True)
    meta = checkpoint["metadata"]
    log_hz, lin_hz = np.geomspace(200, 20000, 256), np.linspace(200, 20000, 257)
    xyz, log_target, lin_target, itd, provenance = load_subject(meta["arguments"]["subject"], log_hz, lin_hz, meta["arguments"].get("sample_rate", 96000))
    split = np.load(run / "split.npz")
    log_target -= meta["magnitudeOffsetDb"]
    lin_target -= meta["magnitudeOffsetDb"]
    x, y, v, hz, ehz = [torch.as_tensor(a, device="cuda", dtype=torch.float32)
                       for a in (xyz, log_target, lin_target, log_hz, lin_hz)]
    model = HrtfField(meta["arguments"]["width"], meta["arguments"]["filters"],
                      sample_rate=meta["arguments"].get("sample_rate", 96000), center_grid=meta["arguments"].get("center_grid", "log"),
                      feature_bands=meta["arguments"].get("feature_bands", 3)).cuda()
    model.load_state_dict(checkpoint["state_dict"])
    def errors(indices):
        sums = {name: 0. for name in ("200-2000", "2000-8000", "8000-20000")}
        total = 0.
        with torch.no_grad():
            for part in np.array_split(indices, max(1, int(np.ceil(len(indices) / 512)))):
                indices_gpu = torch.as_tensor(part, device="cuda")
                db, _ = model.response(model(x[indices_gpu]), ehz)
                error = (db - v[indices_gpu]).square()
                total += float(error.sum())
                for name, low, high in (("200-2000", 200, 2000), ("2000-8000", 2000, 8000),
                                        ("8000-20000", 8000, 20000.1)):
                    mask = (lin_hz >= low) & (lin_hz < high)
                    sums[name] += float(error[..., mask].sum())
        result = {"linearRmseDb": (total / (len(indices) * 2 * len(lin_hz))) ** .5}
        for name, low, high in (("200-2000", 200, 2000), ("2000-8000", 2000, 8000),
                                ("8000-20000", 8000, 20000.1)):
            count = np.count_nonzero((lin_hz >= low) & (lin_hz < high))
            result[name] = (sums[name] / (len(indices) * 2 * count)) ** .5
        return result
    baseline = {name: errors(split[name]) for name in ("train", "validation")}
    output = ROOT / "build/hrtf-quality" / (datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ") +
                                          "-" + uuid.uuid4().hex[:8])
    output.mkdir(parents=True)
    selected = np.random.default_rng(20261116).choice(split["train"], args.samples, replace=False)
    tlog, tlin = y[selected], v[selected]
    records = []
    for grid, count in (("log", 12), ("hybrid", 12), ("hybrid", 24)):
        torch.manual_seed(20261116)
        direct = HrtfField(64, count, sample_rate=model.sample_rate, center_grid=grid).cuda()
        raw = torch.nn.Parameter(torch.zeros(args.samples, 2 * (3 * count + 1) + 1, device="cuda"))
        with torch.no_grad():
            for ear in range(2):
                raw[:, ear * (3 * count + 1) + 1:ear * (3 * count + 1) + 3 * count:3] = -2.
        optimizer = torch.optim.Adam([raw], lr=.02)
        torch.cuda.synchronize()
        start = time.perf_counter()
        for step in range(args.steps):
            optimizer.zero_grad(set_to_none=True)
            prediction = direct.decode_parameters(raw)
            db, _ = direct.response(prediction, hz)
            linear_db, _ = direct.response(prediction, ehz)
            loss = .5 * ((db - tlog) / 10).square().mean() + .5 * ((linear_db - tlin) / 10).square().mean()
            loss.backward()
            optimizer.step()
        with torch.no_grad():
            prediction = direct.decode_parameters(raw)
            db, _ = direct.response(prediction, hz)
            linear_db, _ = direct.response(prediction, ehz)
            record = {"centerGrid": grid, "filtersPerEar": count,
                      "logRmseDb": float((db - tlog).square().mean().sqrt()),
                      "linearRmseDb": float((linear_db - tlin).square().mean().sqrt())}
        torch.cuda.synchronize()
        record["seconds"] = time.perf_counter() - start
        records.append(record)
        print(json.dumps({"event": "direct_fit", **record}), flush=True)
    report = {"sourceRun": run.relative_to(ROOT).as_posix(), "sourceCheckpointSha256": sha256(run / "best.pt"),
              "source": provenance, "baselineBands": baseline, "directFits": records,
              "selectedTrainingDirections": selected.tolist(), "testEvaluated": False,
              "meaning": "Finite per-direction fits on training examples; not a theoretical approximation bound or a deployable field"}
    write_json(output / "diagnostics.json", report)
    (output / "diagnose-source.py").write_bytes(Path(__file__).read_bytes())
    (output / "pilot-source.py").write_bytes((ROOT / "scripts/train_hrtf_pilot.py").read_bytes())
    print(json.dumps({"event": "completed", "directory": output.relative_to(ROOT).as_posix(), **report}), flush=True)


if __name__ == "__main__":
    main()
