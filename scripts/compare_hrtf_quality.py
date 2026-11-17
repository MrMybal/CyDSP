# Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
"""Compare registered HRTF variants on validation, then evaluate two frozen profiles."""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import subprocess
import sys
import time
import uuid
from train_hrtf_pilot import ROOT, sha256, write_json

CASES = [
    {"name": "log12-balanced", "filters": 12, "width": 64, "feature_bands": 3, "center_grid": "log"},
    {"name": "hybrid12-balanced", "filters": 12, "width": 64, "feature_bands": 3, "center_grid": "hybrid"},
    {"name": "hybrid12-wide", "filters": 12, "width": 96, "feature_bands": 4, "center_grid": "hybrid"},
    {"name": "hybrid24-wide", "filters": 24, "width": 96, "feature_bands": 4, "center_grid": "hybrid"},
]


def invoke(study, label, command):
    with (study / (label + ".log")).open("w", encoding="utf-8") as log:
        process = subprocess.Popen(command, cwd=ROOT, stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, text=True, encoding="utf-8")
        completed = None
        for line in process.stdout:
            log.write(line); log.flush()
            try:
                message = json.loads(line)
            except ValueError:
                print(line.rstrip(), flush=True)
                continue
            event = message.get("event")
            if event == "progress":
                print(json.dumps({"trial": label, "step": message["step"],
                                  "logDb": message["validationLogRmseDb"],
                                  "linearDb": message["validationLinearRmseDb"],
                                  "itdUs": message["itdRmseUs"]}), flush=True)
            elif event in ("completed", "evaluated"):
                completed = message
                print(json.dumps({"trial": label, "event": event, "directory": message["directory"]}), flush=True)
        if process.wait() or completed is None:
            raise RuntimeError("Trial failed; inspect " + str(study / (label + ".log")))
        return completed


def train(study, case, steps, subject, initial=None):
    experiment = json.loads((study / "experiment.json").read_text(encoding="utf-8"))
    if sha256(ROOT / "scripts/train_hrtf_pilot.py") != experiment["trainerSha256"]:
        raise RuntimeError("Trainer source changed during the registered study")
    write_json(study / "status.json", {"status": "running", "phase": "refine" if initial else "screen",
                                       "case": case["name"]})
    command = [sys.executable, str(ROOT / "scripts/train_hrtf_pilot.py"), "--subject", subject,
               "--steps", str(steps), "--batch-size", "512", "--validation-every", "2000",
               "--frequency-grid", "mixed", "--selection-grid", "mixed", "--validation-only"]
    for key in ("filters", "width", "feature_bands", "center_grid"):
        command += ["--" + key.replace("_", "-"), str(case[key])]
    if initial:
        command += ["--initial-weights", str(initial)]
    return invoke(study, case["name"] + ("-refine" if initial else "-screen"), command)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--subject", default="D2")
    parser.add_argument("--screen-steps", type=int, default=8000)
    parser.add_argument("--refine-steps", type=int, default=20000)
    args = parser.parse_args()
    if args.screen_steps < 100 or args.refine_steps < 100:
        parser.error("At least 100 steps per phase")
    study = ROOT / "build/hrtf-quality" / (datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ") +
                                          "-comparison-" + uuid.uuid4().hex[:8])
    study.mkdir(parents=True)
    experiment = {"schemaVersion": 1, "subject": args.subject, "cases": CASES,
                  "screenSteps": args.screen_steps, "refineSteps": args.refine_steps,
                  "trainingGrid": "128 logarithmic + 128 linear bins, 200..20000 Hz",
                  "selectionRule": "minimum 0.5*(validation_log_RMSE^2+validation_linear_RMSE^2)/100 + (validation_ITD_RMSE/200)^2",
                  "groups": {"compact12": [c["name"] for c in CASES if c["filters"] == 12],
                             "quality24": [c["name"] for c in CASES if c["filters"] == 24]},
                  "trainerSha256": sha256(ROOT / "scripts/train_hrtf_pilot.py"),
                  "comparisonSha256": sha256(Path(__file__)),
                  "testPolicy": "No candidate test evaluation until best screen variant per filter budget is selected, refined and frozen"}
    write_json(study / "experiment.json", experiment)
    (study / "training-source.py").write_bytes((ROOT / "scripts/train_hrtf_pilot.py").read_bytes())
    (study / "comparison-source.py").write_bytes(Path(__file__).read_bytes())
    results = []
    write_json(study / "status.json", {"status": "running", "phase": "screen", "directory": study.relative_to(ROOT).as_posix()})
    print(json.dumps({"event": "registered", "directory": study.relative_to(ROOT).as_posix(), "cases": CASES}), flush=True)
    start = time.perf_counter()
    try:
        for case in CASES:
            result = train(study, case, args.screen_steps, args.subject)
            if result["heldOutTestEvaluated"]:
                raise RuntimeError("Screening illegally evaluated the test")
            results.append({"case": case, "screen": result})
            write_json(study / "screen.json", results)
        selected = {}
        for label, filters in (("compact12", 12), ("quality24", 24)):
            winner = min((r for r in results if r["case"]["filters"] == filters),
                         key=lambda r: r["screen"]["bestValidation"]["selectionScore"])
            checkpoint = ROOT / winner["screen"]["directory"] / "best.pt"
            refined = train(study, winner["case"], args.refine_steps, args.subject, checkpoint)
            if refined["heldOutTestEvaluated"]:
                raise RuntimeError("Refinement illegally evaluated the test")
            selected[label] = {"case": winner["case"], "screen": winner["screen"], "refined": refined,
                               "checkpointSha256": sha256(ROOT / refined["directory"] / "best.pt")}
        # Immutable model choices precede any new test metrics.
        write_json(study / "selection.json", {"selected": selected, "testEvaluated": False,
                                               "selectionRule": experiment["selectionRule"]})
        evaluations = {}
        for label, selection in selected.items():
            write_json(study / "status.json", {"status": "running", "phase": "evaluate", "case": label})
            directory = ROOT / selection["refined"]["directory"]
            if sha256(directory / "best.pt") != selection["checkpointSha256"]:
                raise RuntimeError("Frozen checkpoint changed")
            evaluations[label] = invoke(study, label + "-evaluate",
                                       [sys.executable, str(ROOT / "scripts/train_hrtf_pilot.py"),
                                        "--evaluate-run", str(directory)])
            if evaluations[label]["checkpointSha256"] != selection["checkpointSha256"]:
                raise RuntimeError("Evaluated checkpoint is not the frozen choice")
        summary = {"status": "completed", "experiment": experiment, "selected": selected,
                   "evaluations": evaluations, "screenResults": results,
                   "totalWallSeconds": time.perf_counter() - start,
                   "limitations": ["Single known listener; no cross-person generalization",
                                   "Magnitude and phase-slope ITD; full HRIR phase is not modeled",
                                   "Filter count indicates additional audio work, not a measured CPU speed ratio",
                                   "No C++ production renderer or listening score"]}
        write_json(study / "summary.json", summary)
        write_json(study / "status.json", {"status": "completed", "summary": "summary.json"})
        print(json.dumps({"event": "completed", "directory": study.relative_to(ROOT).as_posix(),
                          "totalWallSeconds": summary["totalWallSeconds"]}), flush=True)
    except BaseException as error:
        write_json(study / "status.json", {"status": "interrupted", "error": str(error)})
        raise


if __name__ == "__main__":
    main()
