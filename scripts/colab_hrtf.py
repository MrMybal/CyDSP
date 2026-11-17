# Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
"""Drive checkpoint transport and a small source bundle; standard library only."""
import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import queue
import shutil
import signal
import stat
import subprocess
import threading
import time
import uuid
import zipfile

ROOT = Path(__file__).resolve().parents[1]
MAX_FILE = 16 * 1024 * 1024
MAX_TOTAL = 64 * 1024 * 1024
ARTIFACTS = ("best.pt", "metadata.json", "split.npz", "training-source.py", "LICENSE-data.txt",
             "NOTICE-data.txt", "progress.csv", "status.json", "report.json", "weights.json",
             "environment.txt", "sessions.json", "evaluation.json", "checkpoint-validation.json",
             "itd-compensation.json", "validator-source.py", "delay-helper-source.py",
             "frozen-selection.json")


def digest(value):
    return hashlib.sha256(value).hexdigest()


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


def atomic_bytes(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".part-" + uuid.uuid4().hex)
    try:
        temporary.write_bytes(value)
        for attempt in range(10):
            try:
                temporary.replace(path)
                break
            except PermissionError:
                if attempt == 9:
                    raise
                time.sleep(min(.01 * 2 ** attempt, .32))
    finally:
        temporary.unlink(missing_ok=True)


def atomic_json(path, value):
    atomic_bytes(path, (json.dumps(value, indent=2, allow_nan=False) + "\n").encode("utf-8"))


def safe_name(name):
    path = PurePosixPath(name)
    if (not name or "\\" in name or ":" in name or path.is_absolute()
            or any(p in (".", "..") for p in name.split("/"))
            or path.as_posix() != name):
        raise ValueError("Unsafe archive path: " + name)
    return path


def zip_payload(payload, kind, info=None):
    if len(payload) > 120 or sum(map(len, payload.values())) > MAX_TOTAL:
        raise ValueError("Archive payload limit")
    manifest = {"schema": 1, "kind": kind, "info": info or {}, "files": {}}
    for name, value in sorted(payload.items()):
        safe_name(name)
        if len(value) > MAX_FILE:
            raise ValueError("Archive member limit: " + name)
        manifest["files"][name] = {"bytes": len(value), "sha256": digest(value)}
    output = io.BytesIO()
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        members = {**payload, "manifest.json": (json.dumps(manifest, indent=2) + "\n").encode()}
        for name, value in sorted(members.items()):
            # Fixed metadata makes an unchanged source bundle byte-reproducible.
            member = zipfile.ZipInfo(name, date_time=(2026, 1, 1, 0, 0, 0))
            member.compress_type = zipfile.ZIP_DEFLATED
            member.external_attr = 0o100644 << 16
            archive.writestr(member, value, compresslevel=6)
    return output.getvalue()


def checked_payload(value, kind):
    """Verify the complete archive before creating any output file."""
    if len(value) > MAX_TOTAL:
        raise ValueError("Compressed archive limit")
    with zipfile.ZipFile(io.BytesIO(value)) as archive:
        members = archive.infolist()
        names = [m.filename for m in members]
        if len(names) > 121 or len(set(names)) != len(names) or "manifest.json" not in names:
            raise ValueError("Duplicate, missing or excessive members")
        if sum(m.file_size for m in members) > MAX_TOTAL:
            raise ValueError("Uncompressed archive limit")
        for member in members:
            safe_name(member.filename)
            if member.is_dir() or member.file_size > MAX_FILE or stat.S_ISLNK(member.external_attr >> 16):
                raise ValueError("Unsupported archive member")
        manifest = json.loads(archive.read("manifest.json"))
        if manifest.get("schema") != 1 or manifest.get("kind") != kind:
            raise ValueError("Archive format mismatch")
        if set(names) != set(manifest["files"]) | {"manifest.json"}:
            raise ValueError("Archive does not match its manifest")
        result = {}
        for name, record in manifest["files"].items():
            data = archive.read(name)
            if len(data) != record["bytes"] or digest(data) != record["sha256"]:
                raise ValueError("Archive checksum mismatch: " + name)
            result[name] = data
        return result, manifest["info"]


def unpack_bundle(bundle, destination, expected_sha256):
    value = bundle.read_bytes()
    if digest(value) != expected_sha256:
        raise ValueError("Source bundle SHA-256 mismatch")
    payload, _ = checked_payload(value, "cydsp-hrtf-colab-source")
    destination = destination.resolve()
    for name, data in payload.items():
        output = destination.joinpath(*safe_name(name).parts)
        if not output.resolve().is_relative_to(destination):
            raise ValueError("Extraction escaped destination")
        # Existing runs are preserved; only byte-identical source files can be reused.
        if output.exists() and output.read_bytes() != data:
            raise ValueError("Source destination already differs: " + name)
    for name, data in payload.items():
        atomic_bytes(destination.joinpath(*safe_name(name).parts), data)
    return destination


def snapshot_run(run, destination, force=False, label="Drive"):
    """resume.pt is a single self-contained, atomic optimizer/sampler checkpoint."""
    descriptor = run / "resume-info.json"
    if not descriptor.is_file():
        return False
    before = read_json(descriptor)
    value = (run / "resume.pt").read_bytes()
    after = read_json(descriptor)
    if before != after or digest(value) != before["sha256"]:
        return False  # Trainer committed a newer generation while we read.
    pointer = destination / "latest.json"
    old = read_json(pointer) if pointer.is_file() else {}
    if not force and old.get("checkpointSha256") == before["sha256"]:
        return False
    payload = {"resume.pt": value, "resume-info.json": (json.dumps(before) + "\n").encode()}
    for name in ARTIFACTS:
        if (run / name).is_file():
            payload[name] = (run / name).read_bytes()
    packed = zip_payload(payload, "cydsp-hrtf-colab-resume", before)
    archive_name = "step-" + str(before["step"]).zfill(8) + "-" + uuid.uuid4().hex[:8] + ".zip"
    target = destination / archive_name
    atomic_bytes(target, packed)
    if digest(target.read_bytes()) != digest(packed):
        raise OSError("Drive backup read-back failed")
    # The pointer is committed last. Prior immutable generations stay available.
    atomic_json(pointer, {"archive": archive_name, "archiveSha256": digest(packed),
                          "checkpointSha256": before["sha256"], "step": before["step"],
                          "plannedSteps": before["plannedSteps"], "utcSeconds": time.time()})
    print(label + " checkpoint:", before["step"], "/", before["plannedSteps"], flush=True)
    return True


def restore_run(destination, run):
    pointer = destination / "latest.json"
    if not pointer.is_file():
        return False
    record = read_json(pointer)
    name = safe_name(record["archive"])
    if len(name.parts) != 1:
        raise ValueError("Invalid checkpoint archive name")
    value = (destination / str(name)).read_bytes()
    if digest(value) != record["archiveSha256"]:
        raise ValueError("Drive snapshot checksum mismatch; prior generations are retained")
    payload, info = checked_payload(value, "cydsp-hrtf-colab-resume")
    if digest(payload["resume.pt"]) != record["checkpointSha256"] or info["step"] != record["step"]:
        raise ValueError("Checkpoint pointer mismatch")
    if set(payload) - set(ARTIFACTS) - {"resume.pt", "resume-info.json"}:
        raise ValueError("Unexpected run artifact")
    run.mkdir(parents=True, exist_ok=True)
    if (run / "resume.pt").exists():
        if digest((run / "resume.pt").read_bytes()) != record["checkpointSha256"]:
            raise ValueError("Local run differs from Drive; preserve both and choose explicitly")
        return False
    for name, data in payload.items():
        atomic_bytes(run / name, data)
    return True


def run_with_backup(command, root, run, destination, seconds=60, label="Drive"):
    """Show progress and mirror completed checkpoints while this child runs."""
    process = subprocess.Popen(command, cwd=root, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True, bufsize=1)
    messages = queue.Queue()

    def read_output():
        try:
            for line in process.stdout:
                messages.put(line)
        finally:
            messages.put(None)

    reader = threading.Thread(target=read_output, daemon=True)
    reader.start()
    last_sync, output_done = time.monotonic(), False
    interrupted = False
    backup_error = None
    try:
        while not output_done or process.poll() is None:
            try:
                line = messages.get(timeout=1)
                if line is None:
                    output_done = True
                else:
                    try:
                        event = json.loads(line)
                    except (ValueError, TypeError):
                        print(line, end="", flush=True)
                    else:
                        if event.get("event") == "progress":
                            print(f"Step {event['step']}: validation log "
                                  f"{event['validationLogRmseDb']:.3f} dB, linear "
                                  f"{event['validationLinearRmseDb']:.3f} dB, "
                                  f"ITD {event['itdRmseUs']:.3f} us, "
                                  f"{event['millisecondsPerStep']:.2f} ms/step", flush=True)
                        elif event.get("event") in ("started", "resumed", "completed", "paused"):
                            print(event["event"], event.get("directory", ""), flush=True)
                        else:
                            print(line, end="", flush=True)
            except queue.Empty:
                pass
            if time.monotonic() - last_sync >= seconds:
                snapshot_run(run, destination, label=label)
                last_sync = time.monotonic()
        result = process.wait()
    except BaseException:
        interrupted = True
        raise
    finally:
        if process.poll() is None:
            process.send_signal(signal.SIGINT if hasattr(signal, "SIGINT") else signal.SIGTERM)
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.terminate()
                process.wait(timeout=10)
        reader.join(timeout=2)
        process.stdout.close()
        try:
            snapshot_run(run, destination, force=True, label=label)
        except Exception as error:
            backup_error = error
            print("Final " + label + " backup failed:", error, flush=True)
        if backup_error is not None and not interrupted:
            raise backup_error
    if result:
        raise subprocess.CalledProcessError(result, command)


def export_direct_result(persistent, destination):
    """Create a bounded self-contained download from verified latest checkpoints."""
    persistent = persistent.resolve()
    payload = {}
    for name in ("plan.json", "frozen-selection.json"):
        if (persistent / name).is_file():
            payload[name] = (persistent / name).read_bytes()
    count = 0
    for pointer in sorted((persistent / "runs").glob("*/latest.json")):
        record = read_json(pointer)
        archive = safe_name(record["archive"])
        if len(archive.parts) != 1:
            raise ValueError("Invalid direct checkpoint name")
        packed = (pointer.parent / str(archive)).read_bytes()
        if digest(packed) != record["archiveSha256"]:
            raise ValueError("Direct snapshot SHA-256 mismatch")
        snapshot, info = checked_payload(packed, "cydsp-hrtf-colab-resume")
        if digest(snapshot["resume.pt"]) != record["checkpointSha256"] or info["step"] != record["step"]:
            raise ValueError("Direct snapshot pointer mismatch")
        prefix = "runs/" + pointer.parent.name + "/"
        safe_name(prefix + "latest.json")
        payload[prefix + "latest.json"] = pointer.read_bytes()
        payload[prefix + str(archive)] = packed
        count += 1
    if not count or "plan.json" not in payload:
        raise ValueError("No complete checkpoint available to download yet")
    value = zip_payload(payload, "cydsp-hrtf-colab-direct-result",
                        {"experiment": persistent.name, "runs": count})
    atomic_bytes(destination, value)
    if digest(destination.read_bytes()) != digest(value):
        raise OSError("Direct result read-back failed")
    return destination


def restore_direct_result(archive, persistent):
    """Restore only a complete verified package without overwriting different files."""
    payload, info = checked_payload(archive.read_bytes(), "cydsp-hrtf-colab-direct-result")
    if persistent.name != info.get("experiment") or "plan.json" not in payload:
        raise ValueError("Result belongs to a different experiment")
    plan = json.loads(payload["plan.json"])
    if plan.get("storage") != "colab-local-download":
        raise ValueError("Expected a direct-upload experiment")
    pointers = [name for name in payload if name.startswith("runs/") and name.endswith("/latest.json")]
    if len(pointers) != info.get("runs") or not pointers:
        raise ValueError("Missing run pointers")
    allowed = {"plan.json", "frozen-selection.json"}
    for name in pointers:
        path = safe_name(name)
        if len(path.parts) != 3:
            raise ValueError("Invalid result run path")
        record = json.loads(payload[name])
        snapshot_name = safe_name(record["archive"])
        if len(snapshot_name.parts) != 1:
            raise ValueError("Invalid snapshot path")
        key = str(path.parent / snapshot_name)
        packed = payload.get(key, b"")
        if digest(packed) != record["archiveSha256"]:
            raise ValueError("Result checkpoint hash mismatch")
        snapshot, state = checked_payload(packed, "cydsp-hrtf-colab-resume")
        if digest(snapshot["resume.pt"]) != record["checkpointSha256"] or state["step"] != record["step"]:
            raise ValueError("Result pointer mismatch")
        allowed.update((name, key))
    if set(payload) - allowed:
        raise ValueError("Unexpected direct result files")
    persistent = persistent.resolve()
    for name, value in payload.items():
        output = persistent.joinpath(*safe_name(name).parts)
        if not output.resolve().is_relative_to(persistent):
            raise ValueError("Direct restore path escaped experiment")
        if output.exists() and output.read_bytes() != value:
            raise ValueError("Preserve existing result: choose an empty session")
    for name, value in payload.items():
        atomic_bytes(persistent.joinpath(*safe_name(name).parts), value)
    return persistent

def make_bundle(include_reference_weights=True, profile="baseline"):
    if profile not in ("baseline", "l4-quality", "l4-direct"):
        raise ValueError("Unknown Colab profile")
    names = ["LICENSE", "scripts/README.hrtf-training.md",
             "scripts/requirements-hrtf-training.txt", "scripts/fetch_hrtf_training.py",
             "scripts/train_hrtf_pilot.py", "scripts/compare_hrtf_quality.py",
             "scripts/check_hrtf_field.py", "scripts/diagnose_hrtf_field.py",
             "scripts/benchmark_hrtf_training.py", "scripts/colab_hrtf.py",
             "data/hrtf/training-sources.json", "data/hrtf/sadie-ii-v2-2/LICENSE.txt",
             "data/hrtf/sadie-ii-v2-2/NOTICE.txt"]
    payload = {name: (ROOT / name).read_bytes() for name in names}
    references = {}
    if include_reference_weights:
        selected = {12: "20261116T184102Z-D2-b40809f4", 24: "20261116T184219Z-D2-5a3037c9"}
        expected = {12: "6bf94d075dc03bd1dbcd404960ae550a3f56e0cb66e6407d1c0b019b19a6969b",
                    24: "0a9d8e7e1aa7c469cb680f531ec2b6b15cc77ae650ae1325e49499d801d9065e"}
        if profile in ("l4-quality", "l4-direct"):
            selected = {24: "native-48000-D2-f24-v1"}
            expected = {24: "176ac7d21141ebc2a2db13fdd3907cd0dd94c9e317fdb2e31a907fe661586b32"}
        for filters, name in selected.items():
            original = ROOT / "build/hrtf-runs" / name
            if not original.is_dir():
                raise FileNotFoundError("Reference unavailable; rebuild with --without-reference-weights")
            if digest((original / "best.pt").read_bytes()) != expected[filters]:
                raise ValueError("Frozen reference weights changed")
            relative = "build/hrtf-runs/reference-D2-" + str(filters)
            for filename in ("best.pt", "metadata.json", "training-source.py", "split.npz",
                             "evaluation.json", "LICENSE-data.txt", "NOTICE-data.txt"):
                payload[relative + "/" + filename] = (original / filename).read_bytes()
            references[str(filters)] = {"subject": "D2", "checkpoint": relative + "/best.pt",
                                        "sha256": expected[filters], "previouslyEvaluated": True,
                                        "sampleRate": 48000 if profile != "baseline" else 96000}
    payload["colab-references.json"] = (json.dumps(references, indent=2) + "\n").encode()
    value = zip_payload(payload, "cydsp-hrtf-colab-source",
                        {"codeLicense": "MIT", "dataLicense": "Apache-2.0",
                         "includesMeasuredSofa": False, "includesCudaLibraries": False})
    target = ROOT / "build/colab" / {"baseline": "CyDSP-HRTF-Colab.zip", "l4-quality": "CyDSP-HRTF-L4-Quality.zip", "l4-direct": "CyDSP-HRTF-L4-Direct.zip"}[profile]
    atomic_bytes(target, value)
    atomic_bytes(target.with_suffix(".zip.sha256"), (digest(value) + "  " + target.name + "\n").encode())
    return target, digest(value)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--without-reference-weights", action="store_true")
    parser.add_argument("--profile", choices=("baseline", "l4-quality", "l4-direct"), default="baseline")
    args = parser.parse_args()
    path, checksum = make_bundle(not args.without_reference_weights, args.profile)
    print(json.dumps({"bundle": str(path), "bytes": path.stat().st_size, "sha256": checksum}))


if __name__ == "__main__":
    main()
