# Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
"""Validate frozen local HRTF weights without executing archived Python source."""
import argparse
import json
from pathlib import Path
import h5py
import numpy as np
import torch
from train_hrtf_pilot import ROOT, CORPUS, model_from_metadata, sha256, write_json, compensated_delays_us, estimate_itd_us


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", type=Path, required=True)
    args = parser.parse_args()
    run = args.run.resolve()
    if not run.is_relative_to((ROOT / "build/hrtf-runs").resolve()):
        parser.error("Use an ignored local HRTF run")
    torch.set_num_threads(4)
    checkpoint = torch.load(run / "best.pt", map_location="cpu", weights_only=True)
    meta = checkpoint["metadata"]
    rate = meta["arguments"].get("sample_rate", 96000)
    source = run / "training-source.py"
    if sha256(source) != meta["sourceCodeSha256"]:
        raise ValueError("Training source snapshot mismatch")
    original = (CORPUS / meta["source"]["file"]).resolve()
    if not original.is_relative_to(CORPUS.resolve()) or sha256(original) != meta["source"]["sha256"]:
        raise ValueError("SOFA provenance mismatch")
    with h5py.File(original, "r") as sofa:
        position = np.asarray(sofa["SourcePosition"])
        all_test = np.load(run / "split.npz")["test"]
        ears = np.argsort(-np.asarray(sofa["ReceiverPosition"]).reshape(2, 3)[:, 1])
        test_impulses = np.asarray(sofa["Data.IR"][all_test], dtype=np.float64)[:, ears]
        source_delay = np.asarray(sofa["Data.Delay"], dtype=np.float64).reshape(-1, 2)[:, ears]
        source_delay = np.broadcast_to(source_delay, (len(position), 2))[all_test]
    az, el = np.deg2rad(position[:, 0]), np.deg2rad(position[:, 1])
    xyz = np.column_stack((np.cos(el) * np.cos(az), np.cos(el) * np.sin(az), np.sin(el))).astype("float32")
    cpu = model_from_metadata(meta, "cpu")
    cpu.load_state_dict(checkpoint["state_dict"])
    gpu = model_from_metadata(meta, "cuda")
    gpu.load_state_dict(checkpoint["state_dict"])
    hz = np.linspace(200, 20000, 257)
    selection = np.load(run / "split.npz")["test"][:128]
    max_radius = 0.
    with torch.no_grad():
        for indices in np.array_split(np.arange(len(xyz)), int(np.ceil(len(xyz) / 512))):
            prediction = gpu(torch.as_tensor(xyz[indices], device="cuda"))
            c, q, gain, _, _ = [t.cpu().numpy().astype("float64") for t in prediction]
            omega = 2 * np.pi * c / rate
            alpha = np.sin(omega) / (2 * q)
            amplitude = np.exp(gain * np.log(10) / 40)
            norm = 1 + alpha / amplitude
            a1, a2 = -2 * np.cos(omega) / norm, (1 - alpha / amplitude) / norm
            root = np.sqrt((a1 * a1 - 4 * a2).astype("complex128"))
            max_radius = max(max_radius, float(np.maximum(abs((-a1+root)/2), abs((-a1-root)/2)).max()))
        if max_radius >= 1 or not np.isfinite(max_radius):
            raise AssertionError("Unstable predicted poles")
        prediction = gpu(torch.as_tensor(xyz[selection], device="cuda"))
        c, q, gain, level, _ = [t.cpu().numpy().astype("float64") for t in prediction]
        omega = 2 * np.pi * c / rate
        alpha, amplitude = np.sin(omega) / (2 * q), np.exp(gain * np.log(10) / 40)
        z = np.exp(-2j * np.pi * hz / rate)[None, None, None, :]
        numerator = (1+alpha*amplitude)[..., None] - 2*np.cos(omega)[..., None]*z + (1-alpha*amplitude)[..., None]*z*z
        denominator = (1+alpha/amplitude)[..., None] - 2*np.cos(omega)[..., None]*z + (1-alpha/amplitude)[..., None]*z*z
        oracle = (20*np.log10(abs(numerator/denominator))).sum(axis=-2)+level[..., None]
        actual, itd = gpu.response(prediction, torch.as_tensor(hz, device="cuda", dtype=torch.float32))
        cpu_db, cpu_itd = cpu.response(cpu(torch.as_tensor(xyz[selection])), torch.as_tensor(hz, dtype=torch.float32))
        oracle_error = float(abs(actual.cpu().numpy()-oracle).max())
        cpu_error = float(abs(actual.cpu().numpy()-cpu_db.numpy()).max())
        itd_error = float(abs(itd.cpu().numpy()-cpu_itd.numpy()).max())
    if oracle_error >= .005 or cpu_error >= .005 or itd_error >= .1:
        raise AssertionError("CPU/GPU or complex128 oracle disagreement")
    export = json.loads((run / "weights.json").read_text(encoding="utf-8"))
    for key, value in cpu.state_dict().items():
        if not np.array_equal(np.asarray(export["state"][key], dtype="float32"), value.numpy()):
            raise AssertionError("Plain parameter export changed")
    transformed = np.fft.rfft(test_impulses, n=4096, axis=-1)
    reference_itd = estimate_itd_us(transformed, np.fft.rfftfreq(4096, 1/rate), source_delay, rate)
    with torch.no_grad():
        prediction = gpu(torch.as_tensor(xyz[all_test], device="cuda"))
        ldelay, rdelay, intrinsic = compensated_delays_us(prediction, rate)
        c, q, gain, _, desired = [t.cpu().numpy().astype("float64") for t in prediction]
        low_hz = np.fft.rfftfreq(4096, 1/rate)
        low_hz = low_hz[(low_hz >= 200) & (low_hz <= 1500)]
        w0 = 2*np.pi*c/rate
        alpha = np.sin(w0)/(2*q)
        amplitude = np.exp(gain*np.log(10)/40)
        z = np.exp(-2j*np.pi*low_hz/rate)[None, None, None, :]
        b = (1+alpha*amplitude)[...,None]-2*np.cos(w0)[...,None]*z+(1-alpha*amplitude)[...,None]*z*z
        a = (1+alpha/amplitude)[...,None]-2*np.cos(w0)[...,None]*z+(1-alpha/amplitude)[...,None]*z*z
        phase = np.angle(b/a).sum(axis=-2)
        centered = 2*np.pi*(low_hz-low_hz.mean())
        oracle_intrinsic = ((phase[:,0]-phase[:,1])*centered).sum(-1)/np.square(centered).sum()*1e6
        phase_error = float(abs(intrinsic.cpu().numpy()-oracle_intrinsic).max())
        implied_naive = desired+oracle_intrinsic
        implied_corrected = oracle_intrinsic+rdelay.cpu().numpy()-ldelay.cpu().numpy()
        restoration_error = float(abs(implied_corrected-desired).max())
        if phase_error >= .05 or restoration_error >= .05:
            raise AssertionError(f"Cascade ITD compensation failed: phase={phase_error}, restoration={restoration_error}")
        compensation = {"definition": "desired head ITD minus cascade intrinsic ITD; causal positive ear delays",
                        "directions": len(all_test),
                        "maximumDelayScope": "evaluated test directions, not a global allocation bound",
                        "phaseOracleMaxErrorUs": phase_error, "restorationMaxErrorUs": restoration_error,
                        "uncompensatedImpliedItdRmseUs": float(np.sqrt(np.square(implied_naive-reference_itd).mean())),
                        "compensatedImpliedItdRmseUs": float(np.sqrt(np.square(implied_corrected-reference_itd).mean())),
                        "maximumAdditionalEarDelayUs": float(torch.maximum(ldelay, rdelay).max()),
                        "warning": "Analytic low-band phase prediction only; no streamed audio renderer or full-phase validation"}
    write_json(run / "itd-compensation.json", compensation)
    report = {"status": "passed", "checkpointSha256": sha256(run / "best.pt"),
              "allDirections": len(xyz), "maximumPoleRadius": max_radius,
              "oracleMaxErrorDb": oracle_error, "cpuGpuMaxErrorDb": cpu_error,
              "cpuGpuMaxItdErrorUs": itd_error, "exactPlainWeightExport": True,
              "archivedSourceHashVerified": True, "archivedSourceExecuted": False,
              "itdCompensation": compensation,
              "meaning": "Numerical checks, not perceptual or CPU audio performance certification"}
    write_json(run / "checkpoint-validation.json", report)
    print(json.dumps(report), flush=True)


if __name__ == "__main__":
    main()
