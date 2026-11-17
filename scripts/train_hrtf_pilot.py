# Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
# Original experimental training tool. No external HRTF model implementation.
"""Fit a small directional field of stable peaking biquads plus interaural delay.

Explicit CUDA training only; outputs stay in ignored build/hrtf-runs.
The objective approximates magnitude and low-band ITD, not full HRIR phase.
"""
import argparse
import csv
from datetime import datetime, timezone
import hashlib
import io
import json
import math
from pathlib import Path
import platform
import subprocess
import time
import uuid

import h5py
import numpy as np
import torch
from torch import nn

ROOT = Path(__file__).resolve().parents[1]
CORPUS = ROOT / "build/hrtf-training/sadie-ii-v2-2"
SEED = 20261116


def sha256(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def replace_file(temporary, path):
    # Windows readers/scanners can hold a short-lived handle without delete sharing.
    # Keep the old complete checkpoint until replacement succeeds.
    for attempt in range(10):
        try:
            temporary.replace(path)
            return
        except PermissionError:
            if attempt == 9:
                raise
            time.sleep(min(.01 * 2 ** attempt, .32))


def write_json(path, value):
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(value, indent=2, allow_nan=False) + "\n",
                         encoding="utf-8", newline="\n")
    replace_file(temporary, path)


def split_directions(directions, seed):
    # Identical directions and both ears always stay in the same partition.
    _, groups = np.unique(np.round(directions, 7), axis=0, return_inverse=True)
    unique = np.unique(groups)
    rng = np.random.default_rng(seed)
    rng.shuffle(unique)
    n = len(unique)
    assignments = np.empty(n, dtype=np.int8)
    assignments[unique[:int(.8 * n)]] = 0
    assignments[unique[int(.8 * n):int(.9 * n)]] = 1
    assignments[unique[int(.9 * n):]] = 2
    return [np.flatnonzero(assignments[groups] == i) for i in range(3)]


def load_subject(subject, frequencies, evaluation_frequencies, sample_rate=96000):
    indexes = sorted(CORPUS.glob(f"index-{sample_rate}*.json"))
    index, item = None, None
    for candidate in indexes:
        current = json.loads(candidate.read_text(encoding="utf-8"))
        found = next((f for f in current["files"] if f["subject"] == subject), None)
        if found is not None:
            if item is not None and found["sha256"] != item["sha256"]:
                raise ValueError("Conflicting subject indexes")
            index, item = current, found
    if item is None:
        raise ValueError("Subject missing; run scripts/fetch_hrtf_training.py first")
    path = (CORPUS / item["file"]).resolve()
    if not path.is_relative_to(CORPUS.resolve()) or sha256(path) != item["sha256"]:
        raise ValueError("Invalid training SOFA provenance")
    with h5py.File(path, "r") as sofa:
        # Deliberately bounded to the known, byte-verified SADIE II files.
        rate = float(np.asarray(sofa["Data.SamplingRate"]).ravel()[0])
        if rate != sample_rate or sofa["Data.IR"].shape[1:] != (2, 512 if sample_rate == 96000 else 256):
            raise ValueError("Expected original two-ear SADIE data at the selected sample rate")
        if "Apache License, Version 2.0" not in str(sofa.attrs["License"]):
            raise ValueError("Unexpected data rights")
        for name, expected in (("ListenerView", [1, 0, 0]), ("ListenerUp", [0, 0, 1]),
                               ("ListenerPosition", [0, 0, 0])):
            if not np.allclose(np.asarray(sofa[name]).reshape(-1, 3), expected):
                raise ValueError("Pilot supports only the default SADIE listener frame")
        position = np.asarray(sofa["SourcePosition"], dtype=np.float64)
        kind = str(sofa["SourcePosition"].attrs["Type"]).lower()
        units = str(sofa["SourcePosition"].attrs["Units"]).lower()
        if "spherical" not in kind or "degree" not in units:
            raise ValueError("Pilot expects SADIE spherical degree source coordinates")
        azimuth, elevation = np.deg2rad(position[:, 0]), np.deg2rad(position[:, 1])
        directions = np.column_stack((np.cos(elevation) * np.cos(azimuth),
                                      np.cos(elevation) * np.sin(azimuth),
                                      np.sin(elevation))).astype(np.float32)
        receivers = np.asarray(sofa["ReceiverPosition"]).reshape(2, 3)
        ears = np.argsort(-receivers[:, 1])
        if receivers[ears[0], 1] <= receivers[ears[1], 1]:
            raise ValueError("Cannot identify left and right")
        impulse = np.asarray(sofa["Data.IR"], dtype=np.float64)[:, ears, :]
        delay = np.asarray(sofa["Data.Delay"], dtype=np.float64).reshape(-1, 2)[:, ears]
        delay = np.broadcast_to(delay, (len(directions), 2))
    if not np.isfinite(impulse).all():
        raise ValueError("Nonfinite training impulse")
    fft_size = 4096
    fft_hz = np.fft.rfftfreq(fft_size, 1 / rate)
    spectrum = np.empty((len(directions), 2, len(frequencies)), np.float32)
    evaluation = np.empty((len(directions), 2, len(evaluation_frequencies)), np.float32)
    itd = np.empty(len(directions), np.float32)
    for start in range(0, len(directions), 128):
        end = min(start + 128, len(directions))
        transformed = np.fft.rfft(impulse[start:end], n=fft_size, axis=-1)
        db = 20 * np.log10(np.maximum(np.abs(transformed), 1e-7))
        for row in range(end - start):
            for ear in range(2):
                spectrum[start + row, ear] = np.interp(frequencies, fft_hz, db[row, ear])
                evaluation[start + row, ear] = np.interp(evaluation_frequencies, fft_hz, db[row, ear])
        itd[start:end] = estimate_itd_us(transformed, fft_hz, delay[start:end], rate)
    if not np.isfinite(itd).all() or np.max(np.abs(itd)) >= 1200:
        raise ValueError("ITD outside the explicit +/-1200 us pilot bound")
    provenance = {**item, "dataset": index["dataset"], "version": index["version"],
                  "sourceDataLicense": index["license"], "directions": len(directions),
                  "coordinateFrame": "+X front, +Y left, +Z up",
                  "itdDefinition": "right minus left, low-band phase slope, microseconds",
                  "itdBandHz": [200, 1500], "fftSize": fft_size,
                  "originalFilesModified": False}
    return directions, spectrum, evaluation, itd, provenance


def estimate_itd_us(transformed, fft_hz, delay, rate):
    # phase(H_left * conj(H_right)) slopes upward when right arrives later.
    low = (fft_hz >= 200) & (fft_hz <= 1500)
    omega = 2 * np.pi * fft_hz[low]
    centered = omega - omega.mean()
    phase = np.unwrap(np.angle(transformed[:, 0, low] *
                               np.conj(transformed[:, 1, low])), axis=-1)
    return (phase @ centered / np.dot(centered, centered) +
            (delay[:, 1] - delay[:, 0]) / rate) * 1e6


class HrtfField(nn.Module):
    def __init__(self, width=64, filters=12, sample_rate=96000,
                 center_grid="log", feature_bands=3):
        super().__init__()
        if center_grid not in ("log", "hybrid") or not 1 <= feature_bands <= 6:
            raise ValueError("Unsupported center grid or feature encoding")
        self.filters = filters
        self.sample_rate = sample_rate
        self.feature_bands = feature_bands
        self.network = nn.Sequential(nn.Linear(3 + 6 * feature_bands, width), nn.Tanh(),
                                     nn.Linear(width, width), nn.Tanh(),
                                     nn.Linear(width, 2 * (filters * 3 + 1) + 1))
        nn.init.normal_(self.network[-1].weight, std=.001)
        nn.init.zeros_(self.network[-1].bias)
        self.log_spacing = math.log(20000 / 100) / (filters - 1)
        if center_grid == "log":
            centers = torch.logspace(math.log10(100), math.log10(20000), filters)
            shift = torch.full_like(centers, .45 * self.log_spacing)
        else:
            centers = torch.as_tensor(np.concatenate((np.geomspace(150, 1500, 3),
                                                       np.linspace(2500, 20000, filters - 3))),
                                      dtype=torch.float32)
            distances = centers.log().diff()
            shift = .45 * torch.cat((distances[:1], torch.minimum(distances[:-1], distances[1:]),
                                     distances[-1:]))
        self.register_buffer("centers", centers)
        # Derived constant; omitted so legacy log-grid checkpoint state stays loadable.
        self.register_buffer("center_shift", shift, persistent=False)

    def features(self, directions):
        phase = directions.unsqueeze(-1) * directions.new_tensor(
            [math.pi * 2 ** i for i in range(self.feature_bands)])
        return torch.cat((directions, phase.sin().flatten(1), phase.cos().flatten(1)), dim=-1)

    def forward(self, directions):
        return self.decode_parameters(self.network(self.features(directions)))

    def decode_parameters(self, raw):
        parameters = raw[:, :-1].reshape(-1, 2, self.filters * 3 + 1)
        shape = parameters[:, :, :self.filters * 3].reshape(-1, 2, self.filters, 3)
        centers = self.centers * torch.exp(self.center_shift * shape[..., 0].tanh())
        q = .3 + 11.7 * shape[..., 1].sigmoid()
        gain = 24 * shape[..., 2].tanh()
        level = 30 * parameters[:, :, -1].tanh()
        itd = 1200 * raw[:, -1].tanh()
        return centers, q, gain, level, itd

    def response(self, prediction, frequencies):
        centers, q, gain, level, itd = prediction
        return peaking_db(centers, q, gain, level, frequencies, self.sample_rate), itd


def peaking_db(centers, q, gain, level, frequencies, rate):
    # Magnitude of the same RBJ peaking section used by CyDSP. The sine-product
    # avoids catastrophic cancellation in cos(w)-cos(w0) at low frequencies.
    w = 2 * math.pi * frequencies[None, None, None, :] / rate
    w0 = 2 * math.pi * centers[..., None] / rate
    delta = -2 * ((w + w0) * .5).sin() * ((w - w0) * .5).sin()
    alpha_sin = w0.sin() / (2 * q[..., None]) * w.sin()
    amplitude = torch.exp(gain[..., None] * (math.log(10) / 40))
    numerator = delta.square() + (alpha_sin * amplitude).square()
    denominator = delta.square() + (alpha_sin / amplitude).square()
    sections = (10 / math.log(10)) * (numerator.clamp_min(1e-30).log() -
                                                   denominator.clamp_min(1e-30).log())
    return sections.sum(dim=-2) + level[..., None]


def self_test():
    torch.manual_seed(SEED)
    dtype = torch.float64
    centers = torch.logspace(2, 4.3, 12, dtype=dtype)[None, None, :]
    q = torch.full_like(centers, 2)
    gain = torch.linspace(-18, 18, 12, dtype=dtype)[None, None, :]
    frequencies = torch.logspace(2.3, 4.3, 99, dtype=dtype)
    level = torch.zeros(1, 1, dtype=dtype)
    actual = peaking_db(centers, q, gain, level, frequencies, 96000)
    w0 = 2 * math.pi * centers / 96000
    alpha = w0.sin() / (2 * q)
    a = torch.exp(gain * (math.log(10) / 40))
    z = torch.exp(-2j * math.pi * frequencies / 96000)
    numerator = ((1 + alpha * a)[..., None] - 2 * w0.cos()[..., None] * z +
                 (1 - alpha * a)[..., None] * z.square())
    denominator = ((1 + alpha / a)[..., None] - 2 * w0.cos()[..., None] * z +
                   (1 - alpha / a)[..., None] * z.square())
    oracle = (20 * (numerator / denominator).abs().log10()).sum(dim=-2)
    if not torch.allclose(actual, oracle, atol=1e-7, rtol=1e-7):
        raise AssertionError("Differentiable response disagrees with complex RBJ oracle")
    zero = peaking_db(centers, q, torch.zeros_like(gain), level, frequencies, 96000)
    if zero.abs().max() > 1e-12:
        raise AssertionError("Zero-gain section must be identity")
    gain.requires_grad_()
    if not torch.autograd.gradcheck(lambda g: peaking_db(centers, q, g, level, frequencies, 96000),
                                    (gain,), eps=1e-5, atol=1e-5):
        raise AssertionError("Gain gradient check failed")
    # The parameter bounds must produce stable normalized sections.
    a0 = 1 + alpha / a
    a1 = -2 * w0.cos() / a0
    a2 = (1 - alpha / a) / a0
    for first, second in zip(a1.flatten().numpy(), a2.flatten().numpy()):
        if np.max(np.abs(np.roots([1, first, second]))) >= 1:
            raise AssertionError("Unstable peaking pole")
    directions = np.array([[1., 0, 0], [1., 0, 0], [0., 1, 0], [0., 0, 1],
                           [-1., 0, 0], [0., -1, 0], [0., 0, -1]])
    split = split_directions(directions, SEED)
    if not any(0 in group and 1 in group for group in split):
        raise AssertionError("Duplicate directions leaked across split")
    impulse = np.zeros((1, 2, 512))
    impulse[0, 0, 8] = 1
    impulse[0, 1, 56] = 1
    transformed = np.fft.rfft(impulse, n=4096, axis=-1)
    known = estimate_itd_us(transformed, np.fft.rfftfreq(4096, 1/96000),
                            np.zeros((1, 2)), 96000)
    if not np.allclose(known, 500, atol=1e-8):
        raise AssertionError("Known right-minus-left ITD sign/scale")
    checkpoint = io.BytesIO()
    torch.save({"state_dict": {"example": torch.ones(1)},
                "metadata": {"torch": str(torch.__version__)}}, checkpoint)
    checkpoint.seek(0)
    if torch.load(checkpoint, weights_only=True)["metadata"]["torch"] != str(torch.__version__):
        raise AssertionError("Safe checkpoint round trip")
    pure = (centers.detach().repeat(2, 2, 1), q.detach().repeat(2, 2, 1),
            torch.zeros_like(centers).repeat(2, 2, 1), torch.zeros(2, 2, dtype=dtype),
            torch.tensor([500., -300.], dtype=dtype))
    left_delay, right_delay, intrinsic = compensated_delays_us(pure)
    if intrinsic.abs().max() > 1e-9 or not torch.allclose(right_delay-left_delay, pure[-1]):
        raise AssertionError("Causal ITD compensation of identity filters")
    pure[2][0, 0, 0] = 12
    pure[2][1, 1, 1] = -9
    left_delay, right_delay, intrinsic = compensated_delays_us(pure)
    frequencies = torch.fft.rfftfreq(4096, 1/96000, dtype=dtype)
    frequencies = frequencies[(frequencies >= 200) & (frequencies <= 1500)]
    c, qualities, gains = pure[:3]
    omega = 2*math.pi*c/96000
    alpha = omega.sin()/(2*qualities)
    amplitude = torch.exp(gains*(math.log(10)/40))
    z = torch.exp(-2j*math.pi*frequencies/96000)
    numerator = (1+alpha*amplitude)[...,None]-2*omega.cos()[...,None]*z+(1-alpha*amplitude)[...,None]*z.square()
    denominator = (1+alpha/amplitude)[...,None]-2*omega.cos()[...,None]*z+(1-alpha/amplitude)[...,None]*z.square()
    phase = torch.angle(numerator/denominator).sum(-2)
    centered = 2*math.pi*(frequencies-frequencies.mean())
    oracle_itd = ((phase[:,0]-phase[:,1])*centered).sum(-1)/centered.square().sum()*1e6
    if not torch.allclose(oracle_itd, intrinsic, atol=1e-7, rtol=1e-7):
        raise AssertionError("Asymmetric cascade phase oracle")
    if not torch.allclose(oracle_itd+right_delay-left_delay, pure[-1], atol=1e-7):
        raise AssertionError("Cascade plus compensated delays must equal desired total ITD")
    print("PASS RBJ oracle, gradients, poles, known ITD, causal compensation, split and checkpoint", flush=True)


@torch.no_grad()
def metrics(model, directions, targets, itd, frequencies, indices, batch_size):
    sums = torch.zeros(3, device=directions.device, dtype=torch.float64)
    for chunk in indices.split(batch_size):
        db, delay = model.response(model(directions[chunk]), frequencies)
        error = delay - itd[chunk]
        sums += torch.stack(((db - targets[chunk]).square().sum(),
                             error.square().sum(), error.abs().sum())).double()
    count = len(indices)
    values = sums.cpu().tolist()
    return {"magnitudeRmseDb": math.sqrt(values[0] / (count * 2 * targets.shape[-1])),
            "itdRmseUs": math.sqrt(values[1] / count), "itdMaeUs": values[2] / count}




def compensated_delays_us(prediction, sample_rate=96000):
    # The network head predicts desired total ITD, not an external delay.
    # Subtract the cascade's own phase-slope ITD before assigning causal delays.
    centers, q, gain, _, desired_itd = prediction
    hz = torch.fft.rfftfreq(4096, 1/sample_rate, device=centers.device, dtype=centers.dtype)
    hz = hz[(hz >= 200) & (hz <= 1500)]
    w = 2 * math.pi * hz[None, None, None, :] / sample_rate
    w0 = 2 * math.pi * centers[..., None] / sample_rate
    delta = -2 * ((w+w0)*.5).sin() * ((w-w0)*.5).sin()
    alpha_sin = w0.sin() / (2*q[..., None]) * w.sin()
    amplitude = torch.exp(gain[..., None] * (math.log(10)/40))
    phase = (torch.atan2(alpha_sin*amplitude, delta) -
             torch.atan2(alpha_sin/amplitude, delta)).sum(dim=-2)
    omega = 2 * math.pi * hz
    centered = omega-omega.mean()
    intrinsic = ((phase[:, 0]-phase[:, 1])*centered).sum(-1) / centered.square().sum() * 1e6
    residual = desired_itd-intrinsic
    return (-residual).clamp_min(0), residual.clamp_min(0), intrinsic

def validation_summary(model, x, eval_y, delay, log_hz, linear_hz, indices, batch_size, grid):
    log = metrics(model, x, eval_y[..., :256], delay, log_hz, indices, batch_size)
    linear = metrics(model, x, eval_y[..., 256:], delay, linear_hz, indices, batch_size)
    squared = (log["magnitudeRmseDb"] ** 2 if grid == "log" else
               linear["magnitudeRmseDb"] ** 2 if grid == "linear" else
               .5 * (log["magnitudeRmseDb"] ** 2 + linear["magnitudeRmseDb"] ** 2))
    return {"magnitudeRmseDb": math.sqrt(squared), "itdRmseUs": log["itdRmseUs"],
            "itdMaeUs": log["itdMaeUs"], "validationLogRmseDb": log["magnitudeRmseDb"],
            "validationLinearRmseDb": linear["magnitudeRmseDb"],
            "selectionScore": squared / 100 + (log["itdRmseUs"] / 200) ** 2}


def model_from_metadata(metadata, device):
    settings = metadata["arguments"]
    return HrtfField(settings["width"], settings["filters"],
                     sample_rate=settings.get("sample_rate", 96000), center_grid=settings.get("center_grid", "log"),
                     feature_bands=settings.get("feature_bands", 3)).to(device)


@torch.no_grad()
def evaluate_checkpoint(output, device, batch_size=512):
    output = output.resolve()
    if not output.is_relative_to((ROOT / "build/hrtf-runs").resolve()):
        raise ValueError("Evaluation outputs must stay in ignored build/hrtf-runs")
    checkpoint = torch.load(output / "best.pt", map_location=device, weights_only=True)
    meta = checkpoint["metadata"]
    log_grid, linear_grid = np.geomspace(200, 20000, 256), np.linspace(200, 20000, 257)
    xyz, log_target, linear_target, target_itd, source = load_subject(meta["arguments"]["subject"],
                                                                    log_grid, linear_grid, meta["arguments"].get("sample_rate", 96000))
    if source["sha256"] != meta["source"]["sha256"]:
        raise ValueError("Checkpoint source differs from the evaluation source")
    split = split_directions(xyz, meta["seed"])
    stored_split = np.load(output / "split.npz")
    for key, expected in zip(("train", "validation", "test"), split):
        if not np.array_equal(stored_split[key], expected):
            raise ValueError("Direction split changed since training")
    offset = meta["magnitudeOffsetDb"]
    x, y, linear_y, delay = [torch.as_tensor(a, device=device) for a in
                            (xyz, log_target - offset, linear_target - offset, target_itd)]
    hz, ehz = [torch.as_tensor(a, device=device, dtype=torch.float32) for a in (log_grid, linear_grid)]
    train, val, test = [torch.as_tensor(a, device=device) for a in split]
    model = model_from_metadata(meta, device)
    model.load_state_dict(checkpoint["state_dict"])
    log_error = metrics(model, x, y, delay, hz, test, batch_size)
    linear_error = metrics(model, x, linear_y, delay, ehz, test, batch_size)
    nearest = torch.cat([(x[part] @ x[train].T).argmax(-1) for part in test.split(256)])
    neighbors = train[nearest]
    squared = []
    max_radius = 0.
    for part in torch.arange(len(x), device=device).split(batch_size):
        prediction = model(x[part])
        c, q, gain, _, _ = [a.double() for a in prediction]
        omega = 2 * math.pi * c / model.sample_rate
        alpha = omega.sin() / (2 * q)
        amplitude = torch.exp(gain * (math.log(10) / 40))
        norm = 1 + alpha / amplitude
        a1, a2 = -2 * omega.cos() / norm, (1 - alpha / amplitude) / norm
        root = (a1.square() - 4 * a2).to(torch.complex128).sqrt()
        radius = torch.maximum(((-a1 + root) * .5).abs(), ((-a1 - root) * .5).abs())
        max_radius = max(max_radius, float(radius.max()))
    if max_radius >= 1 or not math.isfinite(max_radius):
        raise RuntimeError("Nonfinite or unstable predicted section")
    for part in test.split(batch_size):
        db, _ = model.response(model(x[part]), ehz)
        squared.append((db - linear_y[part]).square())
    squared = torch.cat(squared)
    bands = {}
    for name, low, high in (("200-2000", 200, 2000), ("2000-8000", 2000, 8000),
                            ("8000-20000", 8000, 20000.1)):
        mask = (linear_grid >= low) & (linear_grid < high)
        bands[name] = float(squared[..., mask].mean().sqrt())
    result = {"checkpointSha256": sha256(output / "best.pt"), "sourceSha256": source["sha256"],
              "testDirections": len(test), "heldOutTestLogFrequency": log_error,
              "heldOutTestLinearFrequency": linear_error,
              "testMeanTrainingSpectrumRmseDb": float((y[train].mean(0) - y[test]).square().mean().sqrt()),
              "testNearestTrainingDirectionRmseDb": float((y[neighbors] - y[test]).square().mean().sqrt()),
              "testNearestTrainingDirectionLinearRmseDb": float((linear_y[neighbors] - linear_y[test]).square().mean().sqrt()),
              "testLinearBandsRmseDb": bands, "allDirectionPoleRadiusMax": max_radius,
              "testHeldOutUntilEnd": True,
              "meaning": "Within-listener magnitude/phase-slope ITD approximation; no listening or audio CPU speed claim"}
    write_json(output / "evaluation.json", result)
    return result



def atomic_torch_save(path, value):
    temporary = path.with_suffix(path.suffix + ".tmp")
    torch.save(value, temporary)
    replace_file(temporary, path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--subject", default="D2")
    parser.add_argument("--sample-rate", type=int, choices=(48000, 96000), default=96000)
    parser.add_argument("--transfer-sample-rate", action="store_true", help="Explicit same-listener transfer from another source rate; fresh optimizer")
    parser.add_argument("--steps", type=int, default=3000)
    parser.add_argument("--batch-size", type=int, default=512)
    parser.add_argument("--learning-rate", type=float, default=.002, help="Initial Adam/cosine learning rate")
    parser.add_argument("--minimum-learning-rate", type=float, default=.0002, help="Final cosine learning rate")
    parser.add_argument("--validation-every", type=int, default=250)
    parser.add_argument("--width", type=int, default=64)
    parser.add_argument("--filters", type=int, default=12)
    parser.add_argument("--feature-bands", type=int, default=3)
    parser.add_argument("--center-grid", choices=("log", "hybrid"), default="log")
    parser.add_argument("--frequency-grid", choices=("log", "mixed"), default="log")
    parser.add_argument("--selection-grid", choices=("log", "mixed", "linear"), default="log")
    parser.add_argument("--validation-only", action="store_true")
    parser.add_argument("--initial-weights", type=Path, help="Fine-tune weights; optimizer/RNG start fresh")
    parser.add_argument("--output-dir", type=Path, help="New empty directory inside build/hrtf-runs")
    parser.add_argument("--resume-run", type=Path, help="Restore resume.pt, optimizer, RNG and original schedule")
    parser.add_argument("--stop-after", type=int, help="Pause at this absolute step, without changing the schedule")
    parser.add_argument("--evaluate-run", type=Path, help="Explicit held-out evaluation after model selection")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    self_test()
    if args.self_test:
        return
    if not torch.cuda.is_available():
        raise RuntimeError("CUDA is required; no CPU fallback")
    torch.set_num_threads(4)
    torch.manual_seed(SEED)
    torch.cuda.manual_seed_all(SEED)
    torch.backends.cuda.matmul.allow_tf32 = False
    torch.backends.cudnn.allow_tf32 = False
    device = torch.device("cuda:0")
    if args.evaluate_run:
        result = evaluate_checkpoint(args.evaluate_run, device, args.batch_size)
        print(json.dumps({"event": "evaluated", "directory": args.evaluate_run.as_posix(), **result}), flush=True)
        return
    resume = None
    if args.resume_run:
        output = args.resume_run.resolve()
        if not output.is_relative_to((ROOT / "build/hrtf-runs").resolve()):
            raise ValueError("Resume must stay in ignored build/hrtf-runs")
        resume = torch.load(output / "resume.pt", map_location="cpu", weights_only=True)
        if resume.get("resumeSchema") != 1:
            raise ValueError("Unsupported resumable checkpoint")
        metadata = resume["metadata"]
        if metadata["sourceCodeSha256"] != sha256(Path(__file__)):
            raise ValueError("Resume requires the same training source; use --initial-weights for a changed trainer")
        if metadata["torch"] != str(torch.__version__):
            raise ValueError("Resume requires the recorded PyTorch version")
        stop_after = args.stop_after
        args = argparse.Namespace(**metadata["arguments"])
        args.resume_run, args.output_dir, args.stop_after = output, output, stop_after
    if not 100 <= args.steps <= 1000000 or not 1 <= args.batch_size <= 4096:
        parser.error("Steps: 100..1000000, batch size: 1..4096")
    if not 16 <= args.width <= 256 or not 4 <= args.filters <= 32 or args.validation_every < 1 or not 1 <= args.feature_bands <= 6:
        parser.error("Width: 16..256, filters: 4..32, feature bands: 1..6, positive validation interval")
    if not (math.isfinite(args.learning_rate) and math.isfinite(args.minimum_learning_rate)
            and 0 < args.minimum_learning_rate <= args.learning_rate <= .1):
        parser.error("Learning rates must be finite, 0 < minimum <= initial <= 0.1")
    if args.stop_after is not None and not 1 <= args.stop_after <= args.steps:
        parser.error("--stop-after must be inside the original step schedule")
    wall_start = time.perf_counter()
    log_grid, linear_grid = np.geomspace(200, 20000, 256), np.linspace(200, 20000, 257)
    train_grid = (log_grid if args.frequency_grid == "log" else
                  np.concatenate((np.geomspace(200, 20000, 128), np.linspace(200, 20000, 128))))
    xyz, target, evaluation, itd, provenance = load_subject(args.subject, train_grid, np.concatenate((log_grid, linear_grid)), args.sample_rate)
    split = split_directions(xyz, SEED)
    offset = float(target[split[0]].mean())
    initial_checkpoint, initialization = None, None
    if resume:
        if metadata["source"]["sha256"] != provenance["sha256"]:
            raise ValueError("Resumed source data changed")
        for key, expected in zip(("train", "validation", "test"), split):
            if not np.array_equal(resume["split"][key].numpy(), expected):
                raise ValueError("Resumed partition changed")
        offset = metadata["magnitudeOffsetDb"]
        initialization = metadata["initialWeights"]
    elif args.initial_weights:
        path = args.initial_weights.resolve()
        if not path.is_relative_to((ROOT / "build/hrtf-runs").resolve()):
            raise ValueError("Use a local training checkpoint")
        initial_checkpoint = torch.load(path, map_location=device, weights_only=True)
        old = initial_checkpoint["metadata"]
        for key, default in (("width", 64), ("filters", 12), ("feature_bands", 3), ("center_grid", "log")):
            if old["arguments"].get(key, default) != getattr(args, key):
                raise ValueError("Initial checkpoint architecture mismatch: " + key)
        if old["source"]["sha256"] != provenance["sha256"]:
            if not (args.transfer_sample_rate and old["arguments"].get("sample_rate", 96000) != args.sample_rate
                    and old["arguments"]["subject"] == args.subject
                    and all(old["source"][k] == provenance[k] for k in ("dataset", "version", "coordinateFrame"))):
                raise ValueError("Initial checkpoint source mismatch; rate transfer must be explicit and same listener/dataset")
        offset = old["magnitudeOffsetDb"]
        initialization = {"checkpoint": path.relative_to(ROOT).as_posix(), "sha256": sha256(path),
                          "bestStep": initial_checkpoint["step"], "exactOptimizerResume": False,
                          "sourceSampleRate": old["arguments"].get("sample_rate", 96000),
                          "targetSampleRate": args.sample_rate}
    target -= offset
    evaluation -= offset
    if not resume:
        stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        output = (args.output_dir or ROOT / "build/hrtf-runs" / (stamp + "-" + args.subject + "-" + uuid.uuid4().hex[:8])).resolve()
        runs_root = (ROOT / "build/hrtf-runs").resolve()
        if output == runs_root or not output.is_relative_to(runs_root):
            raise ValueError("New outputs must stay under ignored build/hrtf-runs")
        output.mkdir(parents=True, exist_ok=False)
        arguments = {k: (v.as_posix() if isinstance(v, Path) else v) for k, v in vars(args).items()}
        metadata = {"schemaVersion": 3, "utc": stamp, "arguments": arguments, "seed": SEED,
                    "python": platform.python_version(), "platform": platform.platform(),
                    "torch": str(torch.__version__), "cuda": torch.version.cuda,
                    "gpu": torch.cuda.get_device_name(0), "gpuTotalBytes": torch.cuda.get_device_properties(0).total_memory,
                    "tf32": False, "trainingPrecision": "float32", "warmupSteps": 50,
                    "sourceCodeSha256": sha256(Path(__file__)), "source": provenance,
                    "trainingBandHz": [200, 20000], "trainingFrequencyGrid": args.frequency_grid,
                    "trainingFrequencyBins": len(train_grid), "selectionFrequencyGrid": args.selection_grid,
                    "independentLogEvaluationBins": 256, "independentLinearEvaluationBins": 257,
                    "magnitudeOffsetDb": offset, "targetFloorDb": -140, "itdLimitUs": 1200,
                    "initialWeights": initialization,
                    "split": {k: len(v) for k, v in zip(("train", "validation", "test"), split)},
                    "splitMeaning": "Direction interpolation within one known listener, not unseen-listener generalization",
                    "architecture": {"type": f"MLP predicting {args.filters} bounded RBJ peaking sections/ear and ITD",
                                     "hiddenLayers": 2, "width": args.width, "featureBands": args.feature_bands,
                                     "filtersPerEar": args.filters, "centerGrid": args.center_grid,
                                     "filterGainLimitDb": 24, "overallGainLimitDb": 30, "qRange": [.3, 12]},
                    "limitations": ["Magnitude and low-band ITD only; full phase is not modeled",
                                    "Experimental per-listener checkpoint; explicit native export/validation required",
                                    "No human listening/localization study; native performance measured separately",
                                    "Dataset Apache-2.0 rights and attribution remain separate from MIT code"]}
    output.mkdir(parents=True, exist_ok=True)
    (output / "LICENSE-data.txt").write_bytes((ROOT / "data/hrtf/sadie-ii-v2-2/LICENSE.txt").read_bytes())
    attribution = (ROOT / "data/hrtf/sadie-ii-v2-2/NOTICE.txt").read_text(encoding="utf-8").split("CyDSP selection:")[0]
    (output / "NOTICE-data.txt").write_text(
        attribution + f"\nTraining source: SADIE II {args.subject}, original {args.sample_rate} Hz HRIRs.\n"
        "Source SHA-256: " + provenance["sha256"] +
        "\nOriginal SOFA unchanged; derived targets and learned field are experimental approximations.\n",
        encoding="utf-8", newline="\n")
    (output / "training-source.py").write_bytes(Path(__file__).read_bytes())
    np.savez(output / "split.npz", **dict(zip(("train", "validation", "test"), split)))
    x, y, eval_y, delay = [torch.as_tensor(a, device=device) for a in (xyz, target, evaluation, itd)]
    train_idx, val_idx, _ = [torch.as_tensor(a, device=device) for a in split]
    hz, log_hz, linear_hz = [torch.as_tensor(a, device=device, dtype=torch.float32)
                            for a in (train_grid, log_grid, linear_grid)]
    model = HrtfField(args.width, args.filters, sample_rate=args.sample_rate, center_grid=args.center_grid,
                      feature_bands=args.feature_bands).to(device)
    if initial_checkpoint:
        model.load_state_dict(initial_checkpoint["state_dict"])
    metadata["parameters"] = sum(p.numel() for p in model.parameters())
    metadata["float32WeightBytes"] = metadata["parameters"] * 4
    write_json(output / "metadata.json", metadata)
    optimizer = torch.optim.Adam(model.parameters(), lr=args.learning_rate)
    generator = torch.Generator(device=device).manual_seed(SEED)
    torch.cuda.reset_peak_memory_stats()

    def measure():
        return validation_summary(model, x, eval_y, delay, log_hz, linear_hz, val_idx,
                                  args.batch_size, args.selection_grid)

    def update():
        indices = train_idx[torch.randint(len(train_idx), (args.batch_size,), generator=generator, device=device)]
        optimizer.zero_grad(set_to_none=True)
        db, prediction_itd = model.response(model(x[indices]), hz)
        loss = ((db - y[indices]) / 10).square().mean() + ((prediction_itd - delay[indices]) / 200).square().mean()
        loss.backward()
        torch.nn.utils.clip_grad_norm_(model.parameters(), 10)
        optimizer.step()
        return loss.detach()

    history = []
    if resume:
        model.load_state_dict(resume["state_dict"])
        optimizer.load_state_dict(resume["optimizer"])
        generator.set_state(resume["generatorState"].cpu())
        torch.set_rng_state(resume["torchRngState"].cpu())
        torch.cuda.set_rng_state(resume["cudaRngState"].cpu(), device)
        atomic_torch_save(output / "best.pt", resume["bestCheckpoint"])
        initial = resume["initialValidation"]
        best, best_step = resume["bestScore"], resume["bestStep"]
        records, training_seconds, start_step = resume["records"], resume["trainingSeconds"], resume["step"]
        prior_wall = resume.get("wallSeconds", 0.)
        history = resume.get("sessions", [])
        if args.stop_after is not None and args.stop_after <= start_step:
            raise ValueError("--stop-after must be later than the resumed step")
    else:
        initial = measure()
        atomic_torch_save(output / "best.pt", {"state_dict": model.state_dict(), "metadata": metadata,
                                             "step": 0, "validation": initial})
        best, best_step, records, training_seconds, start_step, prior_wall = initial["selectionScore"], 0, [], 0., 0, 0.
        for _ in range(50):
            update()
    history.append({"utc": datetime.now(timezone.utc).isoformat(), "gpu": torch.cuda.get_device_name(0),
                    "torch": str(torch.__version__), "cuda": torch.version.cuda,
                    "python": platform.python_version(), "fromStep": start_step})
    write_json(output / "sessions.json", history)
    torch.cuda.synchronize()

    def save_resume(step):
        # Only call at fully completed optimizer steps; a mid-step interrupt keeps the previous checkpoint.
        torch.cuda.synchronize()
        atomic_torch_save(output / "resume.pt", {
            "resumeSchema": 1, "state_dict": model.state_dict(), "optimizer": optimizer.state_dict(),
            "generatorState": generator.get_state().cpu(), "torchRngState": torch.get_rng_state(),
            "cudaRngState": torch.cuda.get_rng_state(device).cpu(), "metadata": metadata,
            "step": step, "bestScore": best, "bestStep": best_step,
            "bestCheckpoint": torch.load(output / "best.pt", map_location="cpu", weights_only=True),
            "initialValidation": initial, "records": records, "trainingSeconds": training_seconds,
            "wallSeconds": prior_wall + time.perf_counter() - wall_start, "sessions": history,
            "split": {k: torch.as_tensor(v) for k, v in zip(("train", "validation", "test"), split)}})
        write_json(output / "resume-info.json", {"step": step, "sha256": sha256(output / "resume.pt"),
                                               "plannedSteps": args.steps, "bestStep": best_step})

    save_resume(start_step)
    print(json.dumps({"event": "resumed" if resume else "started", "directory": output.relative_to(ROOT).as_posix(),
                      "gpu": torch.cuda.get_device_name(0), "parameters": metadata["parameters"],
                      "fromStep": start_step, "split": metadata["split"], "initialValidation": initial,
                      "validationOnly": args.validation_only}), flush=True)
    log_stream = (output / "progress.csv").open("w", encoding="utf-8", newline="")
    fields = ("step", "trainingSeconds", "millisecondsPerStep", "loss", "magnitudeRmseDb", "itdRmseUs",
              "itdMaeUs", "validationLogRmseDb", "validationLinearRmseDb", "selectionScore")
    logger = csv.DictWriter(log_stream, fieldnames=fields)
    logger.writeheader()
    logger.writerows(records)
    log_stream.flush()
    block_start, block_step = time.perf_counter(), start_step
    last_saved_step = start_step
    try:
        for step in range(start_step + 1, args.steps + 1):
            for group in optimizer.param_groups:
                group["lr"] = args.minimum_learning_rate + (args.learning_rate - args.minimum_learning_rate) * .5 * (1 + math.cos(math.pi * step / args.steps))
            loss = update()
            pause = args.stop_after is not None and step >= args.stop_after and step < args.steps
            if step % args.validation_every and step != args.steps and not pause:
                continue
            torch.cuda.synchronize()
            elapsed = time.perf_counter() - block_start
            training_seconds += elapsed
            value, measured = float(loss), measure()
            if not all(math.isfinite(v) for v in (value, *measured.values())):
                raise RuntimeError("Nonfinite training/validation")
            row = {"step": step, "trainingSeconds": training_seconds,
                   "millisecondsPerStep": 1000 * elapsed / (step - block_step), "loss": value, **measured}
            records.append(row)
            logger.writerow(row)
            log_stream.flush()
            print(json.dumps({"event": "progress", **row}), flush=True)
            if measured["selectionScore"] < best:
                best, best_step = measured["selectionScore"], step
                atomic_torch_save(output / "best.pt", {"state_dict": model.state_dict(), "metadata": metadata,
                                                      "step": step, "validation": measured})
            save_resume(step)
            last_saved_step = step
            write_json(output / "status.json", {"status": "paused" if pause else "running",
                                               "lastStep": step, "bestStep": best_step})
            if pause:
                print(json.dumps({"event": "paused", "directory": output.relative_to(ROOT).as_posix(),
                                  "lastStep": step, "plannedSteps": args.steps}), flush=True)
                return
            torch.cuda.synchronize()
            block_step, block_start = step, time.perf_counter()
        checkpoint = torch.load(output / "best.pt", map_location=device, weights_only=True)
        model.load_state_dict(checkpoint["state_dict"])
        peak_bytes = torch.cuda.max_memory_allocated()
        evaluation_result = {} if args.validation_only else evaluate_checkpoint(output, device, args.batch_size)
        report = {"status": "completed", "gpu": torch.cuda.get_device_name(0), "subject": args.subject,
                  "parameters": metadata["parameters"], "steps": args.steps, "warmupSteps": 50,
                  "batchSize": args.batch_size, "bestStep": best_step, "trainingSeconds": training_seconds,
                  "millisecondsPerStep": 1000 * training_seconds / args.steps, "peakAllocatedGpuBytes": peak_bytes,
                  "initialValidation": initial, "bestValidation": checkpoint["validation"],
                  "validationOnly": args.validation_only, "heldOutTestEvaluated": not args.validation_only,
                  "initialWeights": initialization, "sessions": history,
                  "estimatedSameConfigurationSeconds": {str(n): training_seconds / args.steps * n
                                                        for n in (10000, 30000, 100000)},
                  "estimateMeaning": "Fixed configuration compute estimate, not convergence or another GPU",
                  "source": provenance, "limitations": metadata["limitations"], **evaluation_result}
        write_json(output / "weights.json", {"metadata": metadata, "step": best_step,
                    "state": {key: value.detach().cpu().tolist() for key, value in model.state_dict().items()}})
        packages = subprocess.check_output([__import__("sys").executable, "-m", "pip", "freeze"], text=True)
        (output / "environment.txt").write_text(packages, encoding="utf-8")
        report["wallSeconds"] = prior_wall + time.perf_counter() - wall_start
        report["blockMillisecondsPerStepRange"] = [min(row["millisecondsPerStep"] for row in records),
                                                   max(row["millisecondsPerStep"] for row in records)]
        write_json(output / "report.json", report)
        write_json(output / "status.json", {"status": "completed", "report": "report.json", "lastStep": args.steps})
        print(json.dumps({"event": "completed", "directory": output.relative_to(ROOT).as_posix(), **report}), flush=True)
    except BaseException as error:
        write_json(output / "status.json", {"status": "interrupted", "error": str(error),
                                            "lastSavedStep": last_saved_step, "lastSavedBestStep": best_step})
        raise
    finally:
        log_stream.close()


if __name__ == "__main__":
    main()
