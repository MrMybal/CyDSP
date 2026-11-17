# Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
"""Benchmark training-only CUDA updates, without evaluating held-out directions."""
import argparse
import json
import math
from pathlib import Path
import time
import numpy as np
import torch
from train_hrtf_pilot import HrtfField, SEED, ROOT, load_subject, split_directions, write_json


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--subject", default="D2")
    parser.add_argument("--sample-rate", type=int, choices=(48000, 96000), default=96000)
    parser.add_argument("--filters", type=int, default=24, choices=(12, 24))
    parser.add_argument("--batches", nargs="+", type=int, default=[512, 1024, 2048])
    parser.add_argument("--iterations", type=int, default=80)
    parser.add_argument("--output", type=Path, default=ROOT / "build/colab/batch-benchmark.json")
    args = parser.parse_args()
    if not 10 <= args.iterations <= 1000 or any(not 1 <= n <= 4096 for n in args.batches):
        parser.error("Iterations: 10..1000; batches: 1..4096")
    output = args.output.resolve()
    if not output.is_relative_to((ROOT / "build").resolve()):
        parser.error("Benchmark output must stay under build")
    if not torch.cuda.is_available():
        raise RuntimeError("CUDA required")
    torch.set_num_threads(4)
    torch.backends.cuda.matmul.allow_tf32 = False
    torch.backends.cudnn.allow_tf32 = False
    hz = np.concatenate((np.geomspace(200, 20000, 128), np.linspace(200, 20000, 128)))
    xyz, target, _, itd, _ = load_subject(args.subject, hz, np.array([1000.]), args.sample_rate)
    train = split_directions(xyz, SEED)[0]
    target -= target[train].mean()
    x, y, delay, indices, frequency = [torch.as_tensor(a, device="cuda") for a in
                                      (xyz, target, itd, train, hz.astype(np.float32))]
    rows = []
    for batch in args.batches:
        torch.manual_seed(SEED)
        torch.cuda.manual_seed_all(SEED)
        model = HrtfField(96, args.filters, sample_rate=args.sample_rate, center_grid="hybrid", feature_bands=4).cuda()
        optimizer = torch.optim.Adam(model.parameters(), lr=.002)
        generator = torch.Generator(device="cuda").manual_seed(SEED)
        torch.cuda.reset_peak_memory_stats()

        def update():
            chosen = indices[torch.randint(len(indices), (batch,), device="cuda", generator=generator)]
            optimizer.zero_grad(set_to_none=True)
            db, predicted_delay = model.response(model(x[chosen]), frequency)
            loss = ((db - y[chosen]) / 10).square().mean() + ((predicted_delay - delay[chosen]) / 200).square().mean()
            loss.backward()
            torch.nn.utils.clip_grad_norm_(model.parameters(), 10)
            optimizer.step()
            return loss

        try:
            for _ in range(20):
                update()
            torch.cuda.synchronize()
            started = time.perf_counter()
            for _ in range(args.iterations):
                loss = update()
            torch.cuda.synchronize()
            elapsed = time.perf_counter() - started
            if not math.isfinite(float(loss.detach())):
                raise RuntimeError("Nonfinite benchmark")
            row = {"batch": batch, "millisecondsPerStep": elapsed * 1000 / args.iterations,
                   "samplesPerSecond": batch * args.iterations / elapsed,
                   "peakAllocatedGpuBytes": torch.cuda.max_memory_allocated(),
                   "computeSecondsFor100000Steps": elapsed / args.iterations * 100000}
        except torch.cuda.OutOfMemoryError:
            row = {"batch": batch, "outOfMemory": True}
        rows.append(row)
        print(json.dumps(row), flush=True)
        del optimizer, model
        torch.cuda.empty_cache()
    output.parent.mkdir(parents=True, exist_ok=True)
    write_json(output, {"gpu": torch.cuda.get_device_name(0), "torch": str(torch.__version__),
                        "cuda": torch.version.cuda, "precision": "float32, TF32 disabled", "rows": rows,
                        "meaning": "Training compute only; batch changes convergence; no automatic batch selection or audio CPU speed claim",
                        "heldOutDirectionsEvaluated": False})


if __name__ == "__main__":
    main()
